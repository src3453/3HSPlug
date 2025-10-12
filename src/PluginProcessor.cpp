// S3HS RAM転送用関数宣言
#include "s3hs_core/ram.cpp"
/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PatchBankData.h"
#include "DrumPcmSampleLoader.h"
#include <algorithm> // for std::find
// Drum PCM RAMグローバル実体
uint8_t* g_pcmRam = nullptr;
size_t g_pcmRamSize = 0;

#define DEFAULT_CHIP_COUNT 3


// g_pcmRam → S3HS内蔵RAM転送
void transferPcmRamToS3HS(std::vector<Byte>& s3hsRam) {
    if (!g_pcmRam || g_pcmRamSize == 0) return;
    std::vector<Byte> pcmVec(g_pcmRam, g_pcmRam + g_pcmRamSize);
    ram_poke2array(s3hsRam, 0, pcmVec);
    printf("[DrumPCM] g_pcmRam transferred to S3HS RAM (%zu bytes)\n", g_pcmRamSize);
}

//==============================================================================
void _3HSPlugAudioProcessor::resetGM()
{
    printf("[GM] GM reset\n");
    // すべてのCCを0にリセット
    for (int ch = 0; ch < 16; ++ch) {
        for (int cc = 0; cc < 128; ++cc) {
            channelCC[ch][cc] = 0;
        }
        channelCC[ch][10] = 64;  // パン
        channelCC[ch][7]  = 127; // ボリューム
        channelCC[ch][11] = 127; // エクスプレッション
        channelPitchBend[ch] = 0x0; // ピッチベンドをセンターにリセット
        channelCoarseTune[ch] = 0x0; // コースチューニングをセンターにリセット
        channelFineTune[ch] = 0x0; // ファインチューニングをセンターにリセット
        channelPitchBendRange[ch] = 2; // デフォルトのピッチベンドレンジを2半音に設定
        channelRpnMsb[ch] = 0; // RPN MSBをリセット
        channelRpnLsb[ch] = 0; // RPN LSBをリセット
        currentProgram[ch] = 0; // プログラム番号を0にリセット
        currentBank[ch] = 0; // バンク番号を0にリセット
        channelSustainPedal[ch] = false; // サスティンペダルをリセット
        heldNotes[ch].clear(); // ホールドノートをクリア
        channelSustainPedal[ch] = false; // サスティンペダルをリセット
        gsDrumChannels.clear(); // GSドラムチャンネルをクリア
    }
    resetPatchBanks(); // オーバーライドされたパッチをリセット
}

std::vector<DrumPcmChannelDebugInfo> _3HSPlugAudioProcessor::getDrumPcmChannelInfoAll() const
{
    std::vector<DrumPcmChannelDebugInfo> result;
    int totalChannels = static_cast<int>(drumPcmChannelStates.size());
    
    for (int i = 0; i < totalChannels; ++i) {
        DrumPcmChannelDebugInfo info;
        
        // 実際のドラムチャンネル状態から情報を取得
        if (i < static_cast<int>(drumPcmChannelStates.size())) {
            const auto& state = drumPcmChannelStates[i];
            info.noteNumber = state.noteNumber;
            info.pcmAddr = state.pcmAddr;
            info.sampleRate = state.sampleRate;
            info.active = state.inUse;
            info.midiChannel = state.midiChannel;
            info.velocity = state.velocity;
            info.lastUsedTick = state.lastUsedTick;
        } else {
            // デフォルト値
            info.noteNumber = -1;
            info.pcmAddr = 0;
            info.sampleRate = 0;
            info.active = false;
            info.midiChannel = -1;
            info.velocity = 0;
            info.lastUsedTick = 0;
        }
        
        result.push_back(info);
    }
    return result;
}


_3HSPlugAudioProcessor::_3HSPlugAudioProcessor()
    #if !defined(JucePlugin_PreferredChannelConfigurations)
        : AudioProcessor (BusesProperties()
    #if ! JucePlugin_IsMidiEffect
    #if ! JucePlugin_IsSynth
            .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
    #endif
            .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
    #endif
        ),
        parameters(*this, nullptr)
    #else
        : parameters(*this, nullptr)
    #endif
    {
        // PCM RAM領域を1MB確保
        constexpr size_t PCM_RAM_SIZE = 0x400000; // 4MB (仕様通り)
        if (!g_pcmRam) {
            g_pcmRam = new uint8_t[PCM_RAM_SIZE];
            g_pcmRamSize = PCM_RAM_SIZE;
            std::fill(g_pcmRam, g_pcmRam + g_pcmRamSize, 0);
            printf("[DrumPCM] g_pcmRam allocated: %zu bytes\n", g_pcmRamSize);
        }


        channelPitchBend.fill(0x2000);
        // サウンドチップ数を設定（初期値1、将来拡張可）
        numChips = DEFAULT_CHIP_COUNT; // 例: 2チップ構成
        s3hsSounds.resize(numChips);
        voiceSlots.resize(numChips * numVoices);
        displayBufferL.resize(numChips*12);
        displayBufferR.resize(numChips*12);
        # define DISPLAY_BUFFER_SIZE 1024 
        for (int i = 0; i < numChips*12; ++i) {
            displayBufferL[i].resize(DISPLAY_BUFFER_SIZE);
            displayBufferR[i].resize(DISPLAY_BUFFER_SIZE);
        }
        voiceMutexes.clear();
        for (int i = 0; i < numChips; ++i) {
            voiceMutexes.emplace_back(std::make_unique<std::mutex>());
        }
        
        // ドラムPCMチャンネル状態の初期化（各チップごと4チャンネル）
        drumPcmChannelStates.resize(numChips * 4);
        // ドラムPCMサンプルロード
        loadAllDrumSamples(drumKeymapManager, 0);
        
        // 全チップにPCM RAMを転送
        for (int chip = 0; chip < numChips; ++chip) {
            transferPcmRamToS3HS(s3hsSounds[chip].ram);
            printf("[DrumPCM] PCM RAM transferred to chip %d\n", chip);
        }
        initializePatchBanks(); // パッチバンク初期化
        resetGM(); // GMリセット
        
}

_3HSPlugAudioProcessor::~_3HSPlugAudioProcessor()
{
}

//==============================================================================
const juce::String _3HSPlugAudioProcessor::getName() const
{
    return JucePlugin_Name; // このIntelliSenceエラーは無視する
}

bool _3HSPlugAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool _3HSPlugAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool _3HSPlugAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double _3HSPlugAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int _3HSPlugAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

float _3HSPlugAudioProcessor::getCurrentPitchBendScaled(int channel)
{   
    if (channel < 1 || channel > 16) {
        return 0.0f; // 無効なチャンネルの場合は0を返す
    }
    return (static_cast<float>(channelPitchBend[channel-1]) / 8192.0f) * channelPitchBendRange[channel-1];
}

int _3HSPlugAudioProcessor::getCurrentProgram()
{
    return 0;
}

void _3HSPlugAudioProcessor::setCurrentProgram (int index)
{
}

int _3HSPlugAudioProcessor::getCurrentProgramForChannel(int channel) const
{
    if (channel >= 1 && channel <= 16)
        return currentProgram[channel-1];
    return 0;
}

int _3HSPlugAudioProcessor::getCurrentProgramBankForChannel(int channel) const
{
    if (channel >= 1 && channel <= 16)
        return currentBank[channel-1];
    return 0;
}

const juce::String _3HSPlugAudioProcessor::getProgramName (int index)
{
    return {};
}

void _3HSPlugAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void _3HSPlugAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // S3HS音源エンジン初期化
    for (int chip = 0; chip < numChips; ++chip) {
        s3hsSounds[chip].initSound();
        s3hsSounds[chip].setSampleRate(static_cast<float>(sampleRate));
    }

    // DCオフセット除去フィルタの初期化
    dcHighPassFilters.clear();
    for (int i = 0; i < getTotalNumOutputChannels(); ++i) {
        juce::IIRFilter filter;
        filter.setCoefficients(juce::IIRCoefficients::makeHighPass(sampleRate, 20.0));
        dcHighPassFilters.push_back(filter);
    }
}

void _3HSPlugAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool _3HSPlugAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void _3HSPlugAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // パフォーマンス測定開始
    auto processStartTime = std::chrono::high_resolution_clock::now();
    auto midiStartTime = processStartTime;
    
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // 出力バッファのクリア
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // GMリセット検出（SysEx: F0 7E 7F 09 01 F7 またはCC#121=0）
    bool gmReset = false;
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        // SysEx GM Reset
        // JUCEのgetSysExData()では、SysExの1バイト目(0xF0)と最後の1バイトは取り除かれる
        // 例: F0 7E 7F 09 01 F7 → {7E, 7F, 09, 01}

        if (msg.isSysEx()) {
            const uint8* data = msg.getSysExData();
            printf("[MIDI] SysEx: ");
            for (int i = 0; i < msg.getSysExDataSize(); ++i) {
                printf("%02X ", data[i]);
            }
            printf("\n");
        }

        // 3HSPlug Patch Override (SysEx: F0 7D 33 48 00 <patch#(00~7F)> <relative addr(00~3F)> <data MSB(00~0F)> <data LSB(00~0F)> <checksum> F7)

        if (msg.isSysEx() && msg.getSysExDataSize() == 8 && msg.getSysExData()[0] == 0x7D && 
            msg.getSysExData()[1] == 0x33 && msg.getSysExData()[2] == 0x48 && msg.getSysExData()[3] == 0x00) {
            const uint8* data = msg.getSysExData();
            int patchNumber = data[4];
            int relativeAddr = data[5];
            int valueMSB = data[6];
            int valueLSB = data[7];
            int value = (valueMSB << 4) | valueLSB;
            printf("[Patch Override] Patch %02X, Addr %02X, Value %02X\n", patchNumber, relativeAddr, value);
            // パッチオーバーライド処理
            if (patchNumber >= 0 && patchNumber < 128) {
                setPatchOverride(0, patchNumber, relativeAddr, value); // デフォルトでバンク0を使用
            }
        }

        // GM Reset (SysEx: F0 7E 7F 09 01 F7)
        if (msg.isSysEx() && msg.getSysExDataSize() == 4)
        {
            const uint8* data = msg.getSysExData();
            if (data[0] == 0x7E && data[2] == 0x09 && data[3] == 0x01)
                gmReset = true;
        }

        // GS Reset (SysEx: F0 41 10 42 12 40 00 7F 00 41 F7)
        if (msg.isSysEx() && msg.getSysExDataSize() == 9)
        {
            const uint8* data = msg.getSysExData();
            if (data[0] == 0x41 && data[1] == 0x10 && data[2] == 0x42 &&
                data[3] == 0x12 && data[4] == 0x40 && data[5] == 0x00 &&
                data[6] == 0x7F && data[7] == 0x00 && data[8] == 0x41)
            {
                gmReset = true; // GSリセットもGMリセットとして扱う
                gsDrumChannels.clear(); // GSドラムチャンネルをクリア
                printf("[GS] GS Reset received, Drum channels cleared\n");
            }
        }

        // GS Drum Part SysEx (F0 41 10 42 12 40 1x 15 mm sum F7)
        if (msg.isSysEx() && (msg.getSysExDataSize() == 9))
        {
            const uint8* data = msg.getSysExData();
            // GS Drum Part判定
            if (data[0] == 0x41 && data[1] == 0x10 && data[2] == 0x42 &&
                data[3] == 0x12 && data[4] == 0x40 && data[6] == 0x15) // loosened sysex check
            {
                uint8_t part = data[5]; // 1x
                uint8_t mm = data[7];   // マップ
                // part番号→MIDIチャンネル変換
                uint8_t midiCh = (part == 0x10 ? 10 : 
                    (part >= 0x1A && part <= 0x1F) ? (part - 0x10 + 1) : 
                    (part >= 0x11 && part <= 0x19) ? (part - 0x10) : 0);
                printf("[GS] part %d (MIDI CH%d) Map %d\n", part, midiCh, mm);
                if (mm >= 1) { // GSm 拡張 : 1-2だけではなく 3-15もドラムマップとして扱う
                    gsDrumChannels.insert(midiCh);
                    printf("[GS] MIDI CH%d set to Drum (MAP%d)\n", midiCh, mm);
                }
            }
        }
        // CC#120 (All Sound Off) - GMリセットとは別処理
        // + CC#123 (All Notes Off)
        if (msg.isController() && (msg.getControllerNumber() == 120 || msg.getControllerNumber() == 123) && msg.getControllerValue() == 0) {
            // All Sound Off: 指定チャンネルの音のみを停止
            int targetChannel = msg.getChannel();
            printf("[MIDI] All Sound Off at CH%d\n", targetChannel);
            
            // FM音源ボイス：該当チャンネルのみを音量0ダミーノートで上書き
            for (int flat = 0; flat < numChips * numVoices; ++flat) {
                auto& v = voiceSlots[flat];
                if (v.inUse && v.midiChannel == targetChannel) {
                    int chip = flat / numVoices;
                    int vIdx = flat % numVoices;
                    int baseAddr = 0x400000 + 0x40 * vIdx;
                    
                    // 音量を0に設定（即座に無音化）
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x10, 0);
                    
                    // ダミー周波数設定（0Hz）
                    int dummyFreq = 0;
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x00, (dummyFreq >> 8) & 0xFF);
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x01, dummyFreq & 0xFF);
                    
                    // Gate OFF（音量0なのでリリースにはならず、即座に音が止まる）
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x1E, 0);
                    s3hsSounds[chip].resetGate(vIdx);
                    
                    // ボイススロット状態をダミーノートに更新
                    v.inUse = false; // 使用中フラグをfalseに設定
                    v.noteNumber = -1; // ダミーノート識別用
                    v.midiChannel = 0;
                    v.velocity = 0;
                    v.volume = 0;
                    v.lastUsedTick = currentTick;
                }
            }
            
            // ドラムPCMチャンネル：該当チャンネルのみを音量0で上書き
            for (int i = 0; i < static_cast<int>(drumPcmChannelStates.size()); ++i) {
                auto& drumState = drumPcmChannelStates[i];
                if (drumState.inUse && drumState.midiChannel == targetChannel) {
                    int chip = i / 4;
                    int pcmChannel = i % 4;
                    
                    // 音量を0に設定（即座に無音化）
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x02, 0);
                    
                    // ダミーPCM設定（適当なアドレス）
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x10, 0x00);
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x11, 0x00);
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x12, 0x00);
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x13, 0x00);
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x14, 0x00);
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x15, 0x01);
                    
                    // ダミー周波数設定
                    int dummyPcmFreq = static_cast<int>(std::floor(0)); // 0Hz
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x00, (dummyPcmFreq >> 8) & 0xFF);
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x01, dummyPcmFreq & 0xFF);
                    
                    // PCM再生トリガ（音量0で開始）
                    s3hsSounds[chip].wtSync(pcmChannel);
                    
                    // ドラムPCMチャンネル状態をダミーに更新
                    drumState.inUse = false;
                    drumState.noteNumber = -1; // ダミーノート識別用
                    drumState.midiChannel = 0;
                    drumState.velocity = 0;
                    drumState.lastUsedTick = currentTick;
                    drumState.pcmAddr = 0;
                    drumState.sampleRate = 8000;
                }
            }
            
            // ホールドノート：該当チャンネルのみクリア
            if (targetChannel >= 1 && targetChannel <= 16) {
                heldNotes[targetChannel - 1].clear();
            }
        }
    }
    if (gmReset)
    {
        // GMリセット時も音量0ダミーノート方式で即座に停止
        printf("[MIDI] GM Reset executed (dummy note method)\n");
        
        // FM音源ボイスを音量0ダミーノートで上書き
        for (int flat = 0; flat < numChips * numVoices; ++flat) {
            int chip = flat / numVoices;
            int vIdx = flat % numVoices;
            int baseAddr = 0x400000 + 0x40 * vIdx;
            
            // 音量を0に設定（即座に無音化）
            s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x10, 0);
            
            // ダミー周波数設定（0Hz）
            int dummyFreq = 0;
            s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x00, (dummyFreq >> 8) & 0xFF);
            s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x01, dummyFreq & 0xFF);
            
            // Gate OFF（音量0なのでリリースにはならず、即座に音が止まる）
            s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x1E, 0);
            s3hsSounds[chip].resetGate(vIdx);
            
            // ボイススロット状態をダミーノートに更新
            voiceSlots[flat].inUse = false;
            voiceSlots[flat].noteNumber = -1; // ダミーノート識別用
            voiceSlots[flat].midiChannel = 0;
            voiceSlots[flat].velocity = 0;
            voiceSlots[flat].volume = 0;
            voiceSlots[flat].lastUsedTick = currentTick;
        }
        
        // ドラムPCMチャンネルを音量0で上書き
        for (int i = 0; i < static_cast<int>(drumPcmChannelStates.size()); ++i) {
            int chip = i / 4;
            int pcmChannel = i % 4;
            
            // 音量を0に設定（即座に無音化）
            s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x02, 0);
            
            // PCM再生トリガ（音量0で開始）
            s3hsSounds[chip].wtSync(pcmChannel);
            
            // ドラムPCMチャンネル状態をダミーに更新
            auto& drumState = drumPcmChannelStates[i];
            drumState.inUse = false;
            drumState.noteNumber = -1; // ダミーノート識別用
            drumState.midiChannel = 0;
            drumState.velocity = 0;
            drumState.lastUsedTick = currentTick;
            drumState.pcmAddr = 0;
            drumState.sampleRate = 8000;
        }
        
        // GMリセット時、全チャンネルのボリューム・エクスプレッションを127にリセット
        resetGM();
    }

    // パラメータ値をCH1レジスタに反映（例: 0x400000～）
    // ここでは簡易的にボリューム・ADSR・モード等を反映する例
    // 詳細なレジスタ割り当てはs3hs register map.md参照

    // MIDIノートON/OFFをGate/Freqに反映
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        int ch = msg.getChannel();
        // CC#7: ボリューム, CC#11: エクスプレッション, CC#64: Sustain Pedal
        if (msg.isController()) { // MIDI CCメッセージを解析し、必要に応じてレジスタを更新する
            this->channelCC[ch][msg.getControllerNumber()] = msg.getControllerValue();
            
            // CC#0 (Bank Select MSB) 処理 - バンク変更 (例: 008:080: Sine Wave)
            if (msg.getControllerNumber() == 0) {
                int bankMSB = msg.getControllerValue();
                currentBank[ch - 1] = bankMSB;
                printf("[GS] Bank Select MSB CH%d: %d (Bank: %d)\n", ch, bankMSB, currentBank[ch - 1]);
            }
            
            // CC#32 (Bank Select LSB) 処理 - バンクには使用しない
            /*if (msg.getControllerNumber() == 32) {
                int bankLSB = msg.getControllerValue();
                currentBank[ch - 1] = bankLSB;
                printf("[GS] Bank Select LSB CH%d: %d (Bank: %d)\n", ch, bankLSB, currentBank[ch - 1]);
            }*/
            
            // CC#64 (Sustain Pedal) 処理
            if (msg.getControllerNumber() == 64) {
                bool newSustainState = msg.getControllerValue() >= 64;
                bool oldSustainState = channelSustainPedal[ch - 1];
                channelSustainPedal[ch - 1] = newSustainState;
                
                printf("[MIDI] Sustain Pedal CH%d: %s\n", ch, newSustainState ? "ON" : "OFF");
                
                // ペダルが離された場合、ホールド中のノートを停止
                if (oldSustainState && !newSustainState) {
                    auto& heldList = heldNotes[ch - 1];
                    for (int heldNote : heldList) {
                        // ホールド中のノートを実際に停止
                        for (int flat = 0; flat < numChips * numVoices; ++flat) {
                            auto& v = voiceSlots[flat];
                            if (v.inUse && v.noteNumber == heldNote && v.midiChannel == ch) {
                                int chip = flat / numVoices;
                                std::lock_guard<std::mutex> lock(*voiceMutexes[chip]);
                                int vIdx = flat % numVoices;
                                int baseAddr = 0x400000 + 0x40 * vIdx;
                                s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x1E, 0); // Gate OFF
                                v.inUse = false;
                                v.noteNumber = -1;
                                printf("[GM] Released held note %d on CH%d\n", heldNote, ch);
                                break;
                            }
                        }
                    }
                    heldList.clear();
                }
            }
            // CC#7, CC#11受信時は全ONボイスの音量を即時更新
            if (msg.getControllerNumber() == 7 || msg.getControllerNumber() == 11 || msg.getControllerNumber() == 10) {
                for (int flat = 0; flat < numChips * numVoices; ++flat) {
                    auto& v = voiceSlots[flat];
                    if (v.inUse && v.midiChannel == ch) {
                        int chip = flat / numVoices;
                        int vIdx = flat % numVoices;
                        uint8 velocity = v.velocity;
                        uint8 expr = this->channelCC[ch][11];
                        uint8 volCC = this->channelCC[ch][7];
                        float volF = (static_cast<float>(velocity) / 127.0f)
                                   * (static_cast<float>(expr) / 127.0f)
                                   * (static_cast<float>(volCC) / 127.0f)
                                   * 255.0f;
                        uint8_t vol = static_cast<uint8_t>(std::min(std::max(volF, 0.0f), 255.0f));
                        v.volume = vol;
                        int baseAddr = 0x400000 + 0x40 * vIdx;
                        int bank = (baseAddr - 0x400000) / 0x40;
                        int progIdx = currentProgram[v.midiChannel-1];
                        auto regs = getEffectivePatch(currentBank[ch-1], progIdx).toRegValues(vol);
                        
                        for (size_t i = 0x10; i < 0x18; ++i) {
                        s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + static_cast<int>(i), regs[i]);
                        //printf("%02x: %02x, ", i, regs[i]);
                        }
                        //printf("\n");
                            // パンCC受信時は即時パン反映
                        if (msg.getControllerNumber() == 10) {
                            uint8 panCC = msg.getControllerValue();
                            float panNorm = static_cast<float>(panCC) / 127.0f;
                            uint8 left = static_cast<uint8>(std::round((1.0f - panNorm) * 15.0f));
                            uint8 right = static_cast<uint8>(std::round(panNorm * 15.0f));
                            uint8 panReg = (left << 4) | (right & 0x0F);
                            s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x1D, panReg);
                        }
                       
                    }
                }
            }
             // --- ピッチベンドレンジ処理 ---
            // RPN 0,0（ピッチベンドレンジ）を受信した場合、channelPitchBendRangeを更新
            // --- RPN処理 ---
            // RPN 0,0: ピッチベンドレンジ
            // RPN 0,1: ファインチューニング
            // RPN 0,2: コースチューニング
            if (ch >= 1 && ch <= 16) {
                if (msg.getControllerNumber() == 101) {
                    channelRpnMsb[ch-1] = msg.getControllerValue();
                    printf("[MIDI] RPN MSB Set: %d (ch %d)\n", channelRpnMsb[ch-1], ch);
                }
                if (msg.getControllerNumber() == 100) {
                    channelRpnLsb[ch-1] = msg.getControllerValue();
                    printf("[MIDI] RPN LSB Set: %d (ch %d)\n", channelRpnLsb[ch-1], ch);
                }
                // Data Entry MSB (CC#6) 受信時、RPN値ごとに処理
                if (msg.getControllerNumber() == 6) {
                    if (channelRpnMsb[ch-1] == 0 && channelRpnLsb[ch-1] == 0) {
                        channelPitchBendRange[ch-1] = msg.getControllerValue();
                        if (gsDrumChannels.find(ch) != gsDrumChannels.end() || ch == 10) {
                            printf("[DrumPCM] Pitch Bend Range Set: %d (drum ch %d)\n", channelPitchBendRange[ch-1], ch);
                        } else {
                            printf("[GM] Pitch Bend Range Set: %d (ch %d)\n", channelPitchBendRange[ch-1], ch);
                        }
                        channelRpnMsb[ch-1] = 127;
                        channelRpnLsb[ch-1] = 127;
                    } else if (channelRpnMsb[ch-1] == 0 && channelRpnLsb[ch-1] == 1) {
                        // ファインチューニング: -100～+100セント, 100/8192単位
                        int value = msg.getControllerValue();
                        // value: 0..127, center=64
                        int cents = ((value - 64) * 100) / 64; // -100～+100
                        channelFineTune[ch-1] = cents;
                        printf("[GM] Fine Tune Set: %d cents (ch %d)\n", channelFineTune[ch-1], ch);
                        channelRpnMsb[ch-1] = 127;
                        channelRpnLsb[ch-1] = 127;
                    } else if (channelRpnMsb[ch-1] == 0 && channelRpnLsb[ch-1] == 2) {
                        // コースチューニング: -6400～+6300セント, 100単位, MSBのみ
                        int value = msg.getControllerValue();
                        // value: 0..127, center=64
                        int cents = (value - 64) * 100; // -6400～+6300
                        channelCoarseTune[ch-1] = cents;
                        printf("[GM] Coarse Tune Set: %d cents (ch %d)\n", channelCoarseTune[ch-1], ch);
                        channelRpnMsb[ch-1] = 127;
                        channelRpnLsb[ch-1] = 127;
                    }
                }
            }

            //printf("[MIDI] CC# %d: %d (ch %d)\n", msg.getControllerNumber(), msg.getControllerValue(), ch);
        }
        // ピッチベンド処理
        if (msg.isPitchWheel())
        {
            int ch = msg.getChannel();
            int bend = msg.getPitchWheelValue() - 8192; // -8192～+8191
            if (ch >= 1 && ch <= 16)
                channelPitchBend[ch - 1] = bend;

            // ピッチベンド・チューニング即時反映: chの全inUseボイスの周波数レジスタを更新
            for (int flat = 0; flat < numChips * numVoices; ++flat) {
                auto& v = voiceSlots[flat];
                if (v.inUse && v.midiChannel == ch) {
                    int note = v.noteNumber;
                    int keyShift = (ch >= 1 && ch <= 16) ? channelKeyShift[ch - 1] : 0;
                    int bendRange = (ch >= 1 && ch <= 16) ? (channelPitchBendRange[ch - 1] ? channelPitchBendRange[ch - 1] : 2) : 2;
                    int bend = (ch >= 1 && ch <= 16) ? channelPitchBend[ch - 1] : 0;
                    float bendSemis = bendRange * (static_cast<float>(bend) / 8192.0f);

                    // チューニング値をセミトーン換算で加算
                    float coarse = (ch >= 1 && ch <= 16) ? static_cast<float>(channelCoarseTune[ch - 1]) : 0.0f;
                    float fine   = (ch >= 1 && ch <= 16) ? static_cast<float>(channelFineTune[ch - 1]) : 0.0f;
                    bendSemis += (coarse + fine) / 100.0f;

                    float freq = 440.0f * std::pow(2.0f, ((note + keyShift + bendSemis) - 69) / 12.0f);
                    int freqInt = static_cast<int>(freq);
                    int chip = flat / numVoices;
                    int vIdx = flat % numVoices;
                    int baseAddr = 0x400000 + 0x40 * vIdx;
                    int bank = (baseAddr - 0x400000) / 0x40;
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x00, (freqInt >> 8) & 0xFF);
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x01, freqInt & 0xFF);
                }
            }
            
            // --- ドラムPCMピッチベンド処理 ---
            // ドラムチャンネルの全アクティブPCMチャンネルにピッチベンドを適用
            if (gsDrumChannels.find(ch) != gsDrumChannels.end() || ch == 10) {
                int bendRange = (ch >= 1 && ch <= 16) ? (channelPitchBendRange[ch - 1] ? channelPitchBendRange[ch - 1] : 2) : 2;
                int bend = (ch >= 1 && ch <= 16) ? channelPitchBend[ch - 1] : 0;
                
                for (int i = 0; i < static_cast<int>(drumPcmChannelStates.size()); ++i) {
                    auto& drumState = drumPcmChannelStates[i];
                    if (drumState.inUse && drumState.midiChannel == ch) {
                        // ピッチベンド値を構造体に保存
                        drumState.pitchBendValue = bend + 8192; // 0x0000-0x3FFF形式に変換
                        drumState.pitchBendRange = static_cast<float>(bendRange);
                        
                        // 元のサンプルレートにピッチベンドを適用
                        float bendSemis = bendRange * (static_cast<float>(bend) / 8192.0f);
                        float pitchRatio = std::pow(2.0f, bendSemis / 12.0f);
                        
                        // 新しい周波数を計算
                        float modifiedSampleRate = drumState.sampleRate * pitchRatio;
                        int pcmFreq = static_cast<int>(std::floor(modifiedSampleRate));
                        
                        // S3HSレジスタに周波数を設定
                        int chip = i / 4;
                        int pcmChannel = i % 4;
                        s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x00, (pcmFreq >> 8) & 0xFF);
                        s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x01, pcmFreq & 0xFF);
                        
                        //printf("[DrumPCM] Pitch bend applied: CH%d PCM%d, bend=%d, ratio=%.3f, freq=%d\n",
                        //       ch, i, bend, pitchRatio, pcmFreq);
                    }
                }
            }
        }
        // プログラムチェンジ処理
        if (msg.isProgramChange())
        {
            int prog = msg.getProgramChangeNumber();
            if (prog >= 0 && prog < 128) {
                int ch = msg.getChannel() - 1;
                if (ch >= 0 && ch < 16) {
                    currentProgram[ch] = prog;
                    int bank = currentBank[ch];
                    printf("[MIDI] Program Change: Bank %d, Program %d, CH %d\n", bank, prog, msg.getChannel());
                    
                    // 代理発音の確認
                    auto& effectivePatch = getEffectivePatch(bank, prog);
                    if (bank != 0 && !PatchBanks[bank][prog].defined && PatchBanks[0][prog].defined) {
                        printf("[PatchBank] Using fallback from Bank 0 for Bank %d Program %d\n", bank, prog);
                    }
                }
            }
        }
        if (msg.isNoteOn())
        {
            int note = msg.getNoteNumber();
            int ch = msg.getChannel();

            // --- ドラムPCM再生処理 ---
            // ドラムチャンネル（例: ch==10）かつキーマップ登録済みノートの場合
            // ドラムチャンネルかつキーマップ登録済みノートはFM音源処理を完全スキップ
            if (gsDrumChannels.find(ch) != gsDrumChannels.end() || ch == 10) {
                auto info = drumKeymapManager.getSampleInfo(10, note);
                if (info.pcmIndex != -1 && info.sampleRate != -1 && info.pcmLength != -1) {
                    // PCM RAMアドレス取得
                    uint32_t pcmAddr_Start = info.pcmIndex;
                    uint32_t pcmAddr_End = pcmAddr_Start + info.pcmLength;
                    if (pcmAddr_End > g_pcmRamSize) {
                        printf("[Warning::DrumPCM] Drum PCM Sample out of bounds: %d-%d (size: %zu)\n", pcmAddr_Start, pcmAddr_End, g_pcmRamSize);
                    }
                    // 既存の同じMIDIチャンネル&ノート番号のドラムボイスを検索
                    int globalPcmChannel = -1;
                    int chip = -1;
                    int pcmChannel = -1;
                    
                    // まず既存のボイスがあるかチェック
                    for (int i = 0; i < static_cast<int>(drumPcmChannelStates.size()); ++i) {
                        const auto& drumState = drumPcmChannelStates[i];
                        if (drumState.inUse && drumState.midiChannel == ch && drumState.noteNumber == note) {
                            // 同じMIDIチャンネル&ノート番号の既存ボイスを発見、上書きする
                            globalPcmChannel = i;
                            chip = globalPcmChannel / 4;
                            pcmChannel = globalPcmChannel % 4;
                            //printf("[DrumPCM] Overwriting existing voice: MIDI ch %d, note %d, globalChannel %d\n", ch, note, globalPcmChannel);
                            break;
                        }
                    }
                    
                    // 既存ボイスがなければ新しいチャンネルを割り当て
                    if (globalPcmChannel == -1) {
                        int totalPcmChannels = numChips * 4; // 各チップ4チャンネル
                        globalPcmChannel = drumPcmChannelIndex;
                        drumPcmChannelIndex = (drumPcmChannelIndex + 1) % totalPcmChannels;
                        
                        // チップとローカルチャンネルを計算
                        chip = globalPcmChannel / 4;
                        pcmChannel = globalPcmChannel % 4;
                        printf("[DrumPCM] Assigning new voice: MIDI ch %d, note %d, globalChannel %d\n", ch, note, globalPcmChannel);
                    }

                    // PCM RAMアドレスをS3HS音源に設定（例: regwtやram_pokeでpcm_addr[pcmChannel]等を設定）
                    // s3hsSounds[0].ram_poke(..., ..., pcmAddr);

                    // PCM周波数レジスタ設定（ピッチベンド適用）
                    int bendRange = (ch >= 1 && ch <= 16) ? (channelPitchBendRange[ch - 1] ? channelPitchBendRange[ch - 1] : 2) : 2;
                    int bend = (ch >= 1 && ch <= 16) ? channelPitchBend[ch - 1] : 0;
                    float bendSemis = bendRange * (static_cast<float>(bend) / 8192.0f);
                    float pitchRatio = std::pow(2.0f, bendSemis / 12.0f);
                    float modifiedSampleRate = info.sampleRate * pitchRatio;
                    int pcmFreq = static_cast<int>(std::floor(modifiedSampleRate / 32.0));
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x00, (pcmFreq >> 8) & 0xFF);
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x01, pcmFreq & 0xFF);
                    
                    // PCM アドレスを書き込む (24bit ビッグエンディアン)
                    // PCM音源部のレジスタは0x30刻み
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x10, (pcmAddr_Start >> 16) & 0xFF);
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x11, (pcmAddr_Start >> 8) & 0xFF);
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x12, pcmAddr_Start & 0xFF);
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x13, (pcmAddr_End >> 16) & 0xFF);
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x14, (pcmAddr_End >> 8) & 0xFF);
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x15, pcmAddr_End & 0xFF);
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x16, 0xFF);
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x17, 0xFF);
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x18, 0xFF); // ループ開始アドレス（0xFFFFFFでワンショット）
                    
                    // 音量を書き込む
                    uint8 velocity = msg.getVelocity();
                    uint8 expr = this->channelCC[ch][11];
                    uint8 volCC = this->channelCC[ch][7];
                    float volF = (static_cast<float>(velocity) / 127.0f)
                                * (static_cast<float>(expr) / 127.0f)
                                * (static_cast<float>(volCC) / 127.0f)
                                * 255.0f;
                    uint8_t vol = static_cast<uint8_t>(std::min(std::max(volF, 0.0f), 255.0f));
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, 0x400200 + pcmChannel * 0x30 + 0x02, vol);
                    
                    // PCM再生トリガ
                    s3hsSounds[chip].wtSync(pcmChannel);
                    //printf("Drum Note on: note %d, freq %d, channel %d, pcmAddr %d-%d\n", note, pcmFreq, pcmChannel, pcmAddr_Start, pcmAddr_End);

                    // ドラムPCMチャンネル状態を更新
                    if (globalPcmChannel >= 0 && globalPcmChannel < static_cast<int>(drumPcmChannelStates.size())) {
                        auto& drumState = drumPcmChannelStates[globalPcmChannel];
                        drumState.inUse = true;
                        drumState.noteNumber = note;
                        drumState.midiChannel = ch;
                        drumState.velocity = velocity;
                        drumState.lastUsedTick = currentTick;
                        drumState.pcmAddr = pcmAddr_Start;
                        drumState.sampleRate = info.sampleRate;
                        drumState.pitchBendValue = bend + 8192; // 0x0000-0x3FFF形式で保存
                        drumState.pitchBendRange = static_cast<float>(bendRange);
                    }
                    
                    //printf("Drum Note on: note %d, chip %d, pcmChannel %d, globalChannel %d, pcmAddr %d-%d\n",
                    //       note, chip, pcmChannel, globalPcmChannel, pcmAddr_Start, pcmAddr_End);
                } else {// PCMファイルが無い場合は何も鳴らさない
                    printf("[Warning::DrumPCM] Drum PCM Sample not found for note %d on channel %d\n", note, ch);
                }
            } else {
            // それ以外はFM音源等の従来処理
                int keyShift = (ch >= 1 && ch <= 16) ? channelKeyShift[ch - 1] : 0;
                int bend = (ch >= 1 && ch <= 16) ? channelPitchBend[ch - 1] : 0;
                int bendRange = (ch >= 1 && ch <= 16) ? (channelPitchBendRange[ch - 1] ? channelPitchBendRange[ch - 1] : 2) : 2; // デフォルト2

                int progIdx = currentProgram[ch-1];
                auto patch = getEffectivePatch(currentBank[ch-1], progIdx);
                int totalKeyShift = keyShift + patch.keyShift;

                float bendSemis = bendRange * (static_cast<float>(bend) / 8192.0f);

                // チューニング値をセミトーン換算で加算
                float coarse = (ch >= 1 && ch <= 16) ? static_cast<float>(channelCoarseTune[ch - 1]) : 0.0f;
                float fine   = (ch >= 1 && ch <= 16) ? static_cast<float>(channelFineTune[ch - 1]) : 0.0f;
                bendSemis += (coarse + fine) / 100.0f;

                float freq = 440.0f * std::pow(2.0f, ((note + totalKeyShift + bendSemis) - 69) / 12.0f);
                int freqInt = static_cast<int>(freq);

                // 既存ノート（同じch/note）が再生中なら、そのスロットを再利用
                int voiceIndex = -1;
                for (int i = 0; i < numChips * numVoices; ++i) {
                    if (voiceSlots[i].inUse && voiceSlots[i].noteNumber == note + totalKeyShift && voiceSlots[i].midiChannel == ch) {
                        voiceIndex = i;
                        if (std::find(heldNotes[ch - 1].begin(), heldNotes[ch - 1].end(), note) != heldNotes[ch - 1].end()) {
                            printf("[Voice] Reusing voice slot %d for held note %d on channel %d, but key is held\n", voiceIndex, note, ch);
                        } else {
                            printf("[Warning::Voice] Reusing voice slot %d for note %d on channel %d, forgot note off?\n", voiceIndex, note, ch);
                        }
                    }
                    break;
                }
                // 見つからなければ空きスロットを探す
                if (voiceIndex < 0) {
                    for (int i = 0; i < numChips * numVoices; ++i) {
                        if (!voiceSlots[i].inUse) {
                            voiceIndex = i;
                            break;
                        }
                    }
                }
                // 空きもなければ最も古いスロットを強制的に使う
                if (voiceIndex < 0) {
                    uint64_t minTick = UINT64_MAX;
                    int oldestIndex = -1;
                    for (int i = 0; i < numChips * numVoices; ++i) {
                        if (voiceSlots[i].lastUsedTick < minTick) {
                            minTick = voiceSlots[i].lastUsedTick;
                            oldestIndex = i;
                        }
                    }
                    if (oldestIndex >= 0) {
                        voiceIndex = oldestIndex;
                    }
                    printf("[Warning::Voice] No free voice slots, reusing oldest slot %d\n", voiceIndex);
                }
                // tickカウンタを進める
                ++currentTick;
            
                // --- パンをリアルタイム反映 ---
                for (int flat = 0; flat < numChips * numVoices; ++flat) {
                    auto& v = voiceSlots[flat];
                    if (v.inUse) {
                        int ch = v.midiChannel;
                        int chip = flat / numVoices;
                        int vIdx = flat % numVoices;
                        int baseAddr = 0x400000 + 0x40 * vIdx;
                        uint8 panCC = this->channelCC[ch][10];
                        float panNorm = static_cast<float>(panCC) / 127.0f;
                        uint8 left = static_cast<uint8>(std::round((1.0f - panNorm) * 15.0f));
                        uint8 right = static_cast<uint8>(std::round(panNorm * 15.0f));
                        uint8 panReg = (left << 4) | (right & 0x0F);
                        int bank = (baseAddr - 0x400000) / 0x40;
                        s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x1D, panReg);
                    }
                }
                if (voiceIndex >= 0) {
                    // 割り当て

                    // もし該当するMIDIチャンネルのMIDIノートがすでに再生中なら、強制的にスロットを再利用する (ノートが重なるのを防止)

                    voiceSlots[voiceIndex].noteNumber = note + totalKeyShift;
                    voiceSlots[voiceIndex].midiChannel = ch;
                    voiceSlots[voiceIndex].inUse = true;
                    voiceSlots[voiceIndex].lastUsedTick = currentTick;

                    int chip = voiceIndex / numVoices;
                    std::lock_guard<std::mutex> lock(*voiceMutexes[chip]);
                    int vIdx = voiceIndex % numVoices;
                    int baseAddr = 0x400000 + 0x40 * vIdx;
                    uint8 velocity = msg.getVelocity(); // Velocity, 0 - 127
                    uint8 expr = this->channelCC[ch][11]; // CC#11 Expression, 0 - 127
                    uint8 volCC = this->channelCC[ch][7]; // CC#7 Channel Volume, 0 - 127
                    float volF = (static_cast<float>(velocity) / 127.0f)
                            * (static_cast<float>(expr) / 127.0f)
                            * (static_cast<float>(volCC) / 127.0f)
                            * 255.0f;
                    uint8_t vol = static_cast<uint8_t>(std::min(std::max(volF, 0.0f), 255.0f));
                    //printf("Note On: %d, chip# %d, chipch %d, MIDIch %d, Volume: %d (unclipped %f, vel %d, vol %d, expr %d)\n", note, chip, vIdx, ch, vol, volF, velocity, volCC, expr);
                    voiceSlots[voiceIndex].velocity = velocity;
                    voiceSlots[voiceIndex].volume = vol;
                    // voiceSlots, s3hsSounds へのアクセスはこのスコープ内で

                    // パッチ適用: PatchBank[currentProgram]のtoRegValues()で得たregValuesをレジスタに書き込む
                    // Patch.keyShiftはすでに上で取得済み
                    // volumeScalingMapに応じてuint8_t volでスケーリングされたレジスタ値を取得
                    auto regs = patch.toRegValues(vol);
                    int bank = (baseAddr - 0x400000) / 0x40;
                    for (size_t i = 0; i < regs.size(); ++i) {
                        s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + static_cast<int>(i), regs[i]);
                    }

                    // パン設定（CC#10, 0-127, デフォルト64中心）
                    uint8 panCC = this->channelCC[ch][10];
                    float panNorm = static_cast<float>(panCC) / 127.0f; // 0.0=Left, 1.0=Right
                    uint8 left = static_cast<uint8>(std::round((1.0f - panNorm) * 15.0f));
                    uint8 right = static_cast<uint8>(std::round(panNorm * 15.0f));
                    uint8 panReg = (left << 4) | (right & 0x0F);
                    bank = (baseAddr - 0x400000) / 0x40;
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x1D, panReg); // パンレジスタ書き込み
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x00, (freqInt >> 8) & 0xFF); // OP1 周波数 上位ビット
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x01, freqInt & 0xFF);   // OP1 周波数 下位ビット
                    /*s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x02, 0x10); // OP2 周波数倍率 上位ビット
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x03, 0x00);   // OP2 周波数倍率 下位ビット*/
                    // Gate ON
                    bank = (baseAddr - 0x400000) / 0x40;
                    s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x1E, 1);
                    s3hsSounds[chip].resetGate(vIdx);
                }
            }
        }
        if (msg.isNoteOff())
        {
            int note = msg.getNoteNumber();
            int ch = msg.getChannel();
            
            // Note ONと同様にkeyShiftを計算
            int keyShift = (ch >= 1 && ch <= 16) ? channelKeyShift[ch - 1] : 0;
            int progIdx = currentProgram[ch-1];
            auto patch = getEffectivePatch(currentBank[ch-1], progIdx);
            int totalKeyShift = keyShift + patch.keyShift;
            int adjustedNote = note + totalKeyShift;
            
            // Sustain Pedalの状態を確認
            bool sustainActive = (ch >= 1 && ch <= 16) ? channelSustainPedal[ch - 1] : false;
            
            if (sustainActive) {
                // ペダルが押されている場合、ノートをホールドリストに追加
                auto& heldList = heldNotes[ch - 1];
                if (std::find(heldList.begin(), heldList.end(), adjustedNote) == heldList.end()) {
                    heldList.push_back(adjustedNote);
                    printf("[GM] Note %d held on CH%d (Sustain Pedal active)\n", adjustedNote, ch);
                }
                // ノートは停止せずに継続
            } else {
                // 該当スロットを探して停止
                for (int flat = 0; flat < numChips * numVoices; ++flat) {
                    auto& v = voiceSlots[flat];
                    if (v.inUse && v.noteNumber == adjustedNote && v.midiChannel == ch) {
                        int chip = flat / numVoices;
                        std::lock_guard<std::mutex> lock(*voiceMutexes[chip]);
                        int vIdx = flat % numVoices;
                        int baseAddr = 0x400000 + 0x40 * vIdx;
                        int bank = (baseAddr - 0x400000) / 0x40;
                        s3hsSounds[chip].ram_poke(s3hsSounds[chip].ram, baseAddr + 0x1E, 0); // Gate OFF
                        v.inUse = false;
                        v.noteNumber = -1;
                        //printf("Note Off: %d (adjusted: %d), chip# %d, chipch %d, MIDIch %d\n", note, adjustedNote, chip, vIdx, ch);
                        break;
                    }
                }
            }
        }
    
    }
    
    // MIDI処理時間測定終了
    auto midiEndTime = std::chrono::high_resolution_clock::now();
    auto midiDuration = std::chrono::duration_cast<std::chrono::microseconds>(midiEndTime - midiStartTime);
    double midiTimeMs = midiDuration.count() / 1000.0;

    // 音声合成処理時間測定開始
    auto synthStartTime = std::chrono::high_resolution_clock::now();
    
    // 音声生成
    // 各チップの出力を合成
    std::vector<std::vector<std::vector<std::vector<float>>>> allFrames(numChips);
    for (int chip = 0; chip < numChips; ++chip) {
        allFrames[chip] = s3hsSounds[chip].AudioCallBack(buffer.getNumSamples());
    }
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        
        for (int chip = 0; chip < numChips; ++chip) {
            // 各チップの出力を加算
            
            /*for (int ch = 0; ch < 12; ++ch) { // 0-12までのフレーム
                int flat = chip * 12 + ch; // チップとチャンネルのフラットインデックス
                displayBufferL[flat].push_back(allFrames[chip][0][ch][i]);
                displayBufferR[flat].push_back(allFrames[chip][1][ch][i]);
                if (displayBufferL[flat].size() > DISPLAY_BUFFER_SIZE) {
                    displayBufferL[flat].erase(displayBufferL[flat].begin());
                    displayBufferR[flat].erase(displayBufferR[flat].begin());
                }
            }*/
                    
                    
        }
        float sumL = 0.0f, sumR = 0.0f;
        for (int chip = 0; chip < numChips; ++chip) {
            sumL += allFrames[chip][0][12][i]/1.5f;
            sumR += allFrames[chip][1][12][i]/1.5f;
        }
        left[i] = sumL / 32768.0f;
        if (right)
            right[i] = sumR / 32768.0f;
    }

    // DCオフセット除去フィルタの適用（最終出力）
    if (dcHighPassFilters.size() >= 1) {
        dcHighPassFilters[0].processSamples(left, buffer.getNumSamples());
    }
    if (right && dcHighPassFilters.size() >= 2) {
        dcHighPassFilters[1].processSamples(right, buffer.getNumSamples());
    }
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
    currentTick += buffer.getNumSamples(); // 現在のtickカウントを更新
    
    // 音声合成処理時間測定終了
    auto synthEndTime = std::chrono::high_resolution_clock::now();
    auto synthDuration = std::chrono::duration_cast<std::chrono::microseconds>(synthEndTime - synthStartTime);
    double synthTimeMs = synthDuration.count() / 1000.0;
    
    // パフォーマンス測定終了
    auto processEndTime = std::chrono::high_resolution_clock::now();
    auto processDuration = std::chrono::duration_cast<std::chrono::microseconds>(processEndTime - processStartTime);
    double processingTimeMs = processDuration.count() / 1000.0;
    
    // CPU使用率計算（処理時間 / バッファ時間）
    double bufferDurationMs = (buffer.getNumSamples() * 1000.0) / getSampleRate();
    double cpuUsage = (processingTimeMs / bufferDurationMs) * 100.0;
    double midiCpuUsage = (midiTimeMs / bufferDurationMs) * 100.0;
    double synthCpuUsage = (synthTimeMs / bufferDurationMs) * 100.0;
    
    // 移動平均でスムージング
    movingAverageProcessingTime = (1.0 - SMOOTHING_FACTOR) * movingAverageProcessingTime + SMOOTHING_FACTOR * processingTimeMs;
    movingAverageCpuUsage = (1.0 - SMOOTHING_FACTOR) * movingAverageCpuUsage + SMOOTHING_FACTOR * cpuUsage;
    movingAverageMidiTime = (1.0 - SMOOTHING_FACTOR) * movingAverageMidiTime + SMOOTHING_FACTOR * midiTimeMs;
    movingAverageSynthTime = (1.0 - SMOOTHING_FACTOR) * movingAverageSynthTime + SMOOTHING_FACTOR * synthTimeMs;
    
    // アトミック変数に保存
    audioProcessingTimeMs.store(movingAverageProcessingTime);
    cpuUsagePercent.store(movingAverageCpuUsage);
    midiProcessingTimeMs.store(movingAverageMidiTime);
    synthProcessingTimeMs.store(movingAverageSynthTime);
    
    lastProcessTime = processEndTime;
}

std::vector<std::vector<float>> _3HSPlugAudioProcessor::getChipAudioDataL(int chip) const
{
    if (chip < 0 || chip >= displayBufferL.size())
        return {};
    std::vector<std::vector<float>> slicedData;

    // flat index に基づいて、切り出す (0-11, 12-23, ...)
    for (int i = 0; i < 12; ++i) {
        int flatIndex = chip * 12 + i;
        if (flatIndex < displayBufferL.size()) {
            slicedData.push_back(displayBufferL[flatIndex]);
        } else {
            slicedData.push_back(std::vector<float>()); // 空のベクターを追加
        }
    }


    return slicedData;
}

std::vector<std::vector<float>> _3HSPlugAudioProcessor::getChipAudioDataR(int chip) const
{
    if (chip < 0 || chip >= displayBufferR.size())
        return {};
    std::vector<std::vector<float>> slicedData;

    // flat index に基づいて、切り出す (0-11, 12-23, ...)
    for (int i = 0; i < 12; ++i) {
        int flatIndex = chip * 12 + i;
        if (flatIndex < displayBufferR.size()) {
            slicedData.push_back(displayBufferR[flatIndex]);
        } else {
            slicedData.push_back(std::vector<float>()); // 空のベクターを追加
        }
    }

    return slicedData;
}


//==============================================================================
bool _3HSPlugAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* _3HSPlugAudioProcessor::createEditor()
{
    return new _3HSPlugAudioProcessorEditor (*this);
}

//==============================================================================
void _3HSPlugAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void _3HSPlugAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

uint64_t _3HSPlugAudioProcessor::getCurrentTick() const
{
    return currentTick;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new _3HSPlugAudioProcessor();
}

std::vector<uint8_t> _3HSPlugAudioProcessor::getRamDump() const
{
    std::vector<uint8_t> dump;
    if (s3hsSounds.size() == 0) return dump;
    const auto& ram = s3hsSounds[0].ram;
    if (ram.size() < 0x400400) return dump;
    dump.reserve(0x400);
    for (int addr = 0x400000; addr < 0x400400; ++addr) {
        dump.push_back(ram[addr]);
    }
    return dump;
}

std::vector<uint8_t> _3HSPlugAudioProcessor::getRamDumpPCM() const
{
    std::vector<uint8_t> dump;
    if (s3hsSounds.size() == 0) return dump;
    const auto& ram = s3hsSounds[0].ram;
    if (ram.size() < 0x1000) return dump;
    dump.reserve(0x1000);
    for (int addr = 0x0; addr < 0x1000; ++addr) {
        dump.push_back(ram[addr]);
    }
    return dump;
}


// DrumPcmChannelDebugInfo取得
DrumPcmChannelDebugInfo _3HSPlugAudioProcessor::getDrumPcmChannelInfo(int ch) const
{
    DrumPcmChannelDebugInfo info;
    // ch: 0～3（PCMチャンネル）
    // ここでは仮実装。実際のPCM再生状態やアドレス取得は必要に応じて修正
    info.noteNumber = -1;
    info.pcmAddr = 0;
    info.sampleRate = 0;
    info.active = false;

    // 例: drumKeymapManagerやPCM再生状態から情報取得
    // 実際のPCMチャンネル管理構造体があればそこから取得すること
    // info.noteNumber = ...;
    // info.pcmAddr = ...;
    // info.sampleRate = ...;
    // info.active = ...;

    return info;
}

// パンポット値取得関数
std::pair<int, int> _3HSPlugAudioProcessor::getVoicePanValues(int voiceIndex) const
{
    if (voiceIndex < 0 || voiceIndex >= static_cast<int>(voiceSlots.size())) {
        return {15, 15}; // デフォルト値（センター）
    }
    
    int chip = voiceIndex / numVoices;
    int vIdx = voiceIndex % numVoices;
    
    if (chip >= numChips || chip < 0) {
        return {15, 15}; // デフォルト値
    }
    
    // S3HSサウンドチップからパンポット値を取得
    // アドレス: 0x400000 + 0x40 * vIdx + 0x1d
    int baseAddr = 0x400000 + 0x40 * vIdx;
    
    // const_castを使用してconst制約を回避（読み取り専用操作なので安全）
    auto& nonConstSound = const_cast<S3HS_sound&>(s3hsSounds[chip]);
    auto ramDump = nonConstSound.ram_peek2array(nonConstSound.ram, baseAddr + 0x1d, 1);
    
    if (ramDump.empty()) {
        return {15, 15}; // デフォルト値
    }
    
    int panL = (ramDump[0] >> 4) & 0xF;  // 上位4ビット
    int panR = ramDump[0] & 0xF;         // 下位4ビット
    
    // 0の場合はセンター扱い
    if (panL == 0 && panR == 0) {
        panL = 15;
        panR = 15;
    }
    
    return {panL, panR};
}

// ドラムPCMチャンネルデバッグ情報取得
