#pragma once
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <cstdint>
#include <JuceHeader.h>
#include "DrumKeymapManager.h"
#include "s3hs_core/sound.cpp"

// ドラムPCMチャンネルデバッグ情報構造体
struct DrumPcmChannelDebugInfo {
    int noteNumber = -1;
    uint32_t pcmAddr = 0;
    uint32_t sampleRate = 0;
    bool active = false;
    int midiChannel = -1;
    int velocity = 0;
    uint64_t lastUsedTick = 0;
};

// ドラムPCMチャンネル状態管理構造体
struct DrumPcmChannelState {
    bool inUse = false;
    int noteNumber = -1;
    int midiChannel = -1;
    int velocity = 0;
    uint64_t lastUsedTick = 0;
    uint32_t pcmAddr = 0;
    uint32_t sampleRate = 0;
};

// デバッグ文字列取得

// Patch構造体はPatchBankData.hで定義されています（重複定義を削除）

//==============================================================================
/**
*/
class _3HSPlugAudioProcessor  : public juce::AudioProcessor
{
    // GSドラムパートとして扱うMIDIチャンネル集合
    std::set<uint8_t> gsDrumChannels;

    // DCオフセット除去用ハイパスフィルタ（チャンネルごと）
    std::vector<juce::IIRFilter> dcHighPassFilters;
public:
    DrumKeymapManager drumKeymapManager;
    int drumPcmChannelIndex = 0; // 4ボイス用ラウンドロビンインデックス
    
    // ドラムPCMチャンネル状態管理（各チップごと4チャンネル）
    std::vector<DrumPcmChannelState> drumPcmChannelStates;

    // デバッグ文字列取得
    std::string getDrumPcmDebugText() const;
    DrumPcmChannelDebugInfo getDrumPcmChannelInfo(int ch) const;
    std::vector<DrumPcmChannelDebugInfo> getDrumPcmChannelInfoAll() const;

    //==============================================================================
    _3HSPlugAudioProcessor();
    ~_3HSPlugAudioProcessor() override;

    void resetGM();

    // 指定チャンネルの現在のプログラム番号を取得
    int getCurrentProgramForChannel(int channel) const;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    uint64_t getCurrentTick() const;

    // デバッグ用: ボイス割り当て状況取得
    struct VoiceSlot {
        int noteNumber = -1;   // 割り当てノート番号（未使用時は-1）
        int midiChannel = 0;   // MIDIチャンネル
        bool inUse = false;    // 使用中フラグ
        uint8_t velocity = 0;  // ベロシティ（ノートON時値）
        uint8_t volume = 0;    // 実際に書き込んだ音量値
        uint64_t lastUsedTick = 0; // 最終使用時刻（ノートON時に更新）
    };
    static constexpr int numVoices = 8;
    const std::vector<VoiceSlot>& getVoiceSlots() const noexcept { return voiceSlots; }
    int getNumVoices() const noexcept { return numChips * numVoices; }
    int getNumChips() const noexcept { return numChips; }

    // RAMダンプ取得（1チップ目、0x400000～0x4003FF）
    std::vector<uint8_t> getRamDump() const;
    // RAMダンプ取得（PCM, 1チップ目、0x000000～0x000FFF）
    std::vector<uint8_t> getRamDumpPCM() const;

private:
        //==============================================================================
    // S3HS音源エンジン
    std::vector<S3HS_sound> s3hsSounds;
    int numChips = 1;

    // パラメータ管理（最小限：音色/ボリューム/ADSR/ゲート）
    juce::AudioProcessorValueTreeState parameters;

    // 8ボイス分のノート割り当て管理
    std::vector<VoiceSlot> voiceSlots; // フラットな全ボイス

    // チップごとの排他制御用ミューテックス
    std::vector<std::unique_ptr<std::mutex>> voiceMutexes;

    // ボイスアロケーション用tickカウンタ
    uint64_t currentTick = 0;

    // パッチバンク機能
    // std::vector<Patch> patchBank = std::vector<Patch>(128); // 外部定義に切り替え
    std::array<int, 16> currentProgram{{0}};

    // ピッチベンド・レンジ・キーシフト
    std::array<int, 16> channelPitchBend{};        // -8192～+8191
    std::array<int, 16> channelPitchBendRange{};   // 半音単位
    std::array<int, 16> channelKeyShift{};         // 半音単位

    // パラメータ値キャッシュ
    float paramTone = 0.0f;

    // RPN状態管理
    std::array<uint8_t, 16> channelRpnMsb{};
    std::array<uint8_t, 16> channelRpnLsb{};

    // チューニング用
    std::array<int, 16> channelFineTune{};   // -100～+100セント
    std::array<int, 16> channelCoarseTune{}; // -6400～+6300セント

    // 各MIDIチャンネル・CC番号ごとの値 [1-16][0-127]（全要素127で初期化）
    uint8_t channelCC[17][128] = {{127}};
    
    // Hold/Sustain Pedal状態管理
    std::array<bool, 16> channelSustainPedal{};  // CC#64の状態
    std::array<std::vector<int>, 16> heldNotes;  // ペダル押下中のノート番号リスト
};
