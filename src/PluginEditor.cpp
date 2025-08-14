/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// HexDumpViewer Implementation
HexDumpViewer::HexDumpViewer()
    : monoFont(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain)
{
    // スクロール可能にするため、マウスホイールイベントを有効にする
    setWantsKeyboardFocus(false);
}

HexDumpViewer::~HexDumpViewer()
{
}

void HexDumpViewer::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setFont(monoFont);
    
    if (hexData.empty()) {
        g.setColour(juce::Colours::white);
        g.drawText("No data available", getLocalBounds(), juce::Justification::centred);
        return;
    }
    
    int width = getWidth();
    int height = getHeight();
    int maxLines = height / lineHeight;
    int totalLines = (static_cast<int>(0x2C0) + bytesPerLine - 1) / bytesPerLine;
    
    // スクロールオフセットの制限
    scrollOffset = juce::jlimit(0, juce::jmax(0, totalLines - maxLines), scrollOffset);
    
    int x = 5;
    int y = 5;
    
    for (int line = 0; line < maxLines && (line + scrollOffset) < totalLines; ++line) {
        int lineIndex = line + scrollOffset;
        int startByte = lineIndex * bytesPerLine;
        int endByte = juce::jmin(startByte + bytesPerLine, static_cast<int>(hexData.size()));
        
        // アドレス表示
        g.setColour(juce::Colours::lightblue);
        juce::String addrStr = juce::String::toHexString(static_cast<int>(baseAddr + startByte)).paddedLeft('0', 8).toUpperCase();
        g.drawText(addrStr + ":", x, y + line * lineHeight, 80, lineHeight, juce::Justification::centredLeft);
        
        // 16進データ表示
        g.setColour(juce::Colours::white);
        juce::String hexStr;
        juce::String asciiStr;
        
        for (int i = startByte; i < endByte; ++i) {
            uint8_t byte = hexData[i];
            hexStr += juce::String::toHexString(static_cast<int>(byte)).paddedLeft('0', 2).toUpperCase() + " ";
            
            // ASCII文字表示（印刷可能文字のみ）
            /*if (byte >= 32 && byte <= 126) {
                asciiStr += static_cast<char>(byte);
            } else {
                asciiStr += ".";
            }*/
        }
        
        // 16進データを表示
        g.drawText(hexStr, x + 90, y + line * lineHeight, 400, lineHeight, juce::Justification::centredLeft);
        
        // ASCII文字を表示
        //g.setColour(juce::Colours::lightgreen);
        //g.drawText(asciiStr, x + 500, y + line * lineHeight, 200, lineHeight, juce::Justification::centredLeft);
    }
    
    // スクロールバー的な表示
    if (totalLines > maxLines) {
        int scrollBarX = width - 15;
        int scrollBarHeight = height - 10;
        
        g.setColour(juce::Colours::darkgrey);
        g.fillRect(scrollBarX, 5, 10, scrollBarHeight);
        
        float scrollRatio = static_cast<float>(scrollOffset) / static_cast<float>(totalLines - maxLines);
        int thumbY = static_cast<int>(5 + scrollRatio * (scrollBarHeight - 20));
        
        g.setColour(juce::Colours::lightgrey);
        g.fillRect(scrollBarX, thumbY, 10, 20);
    }
}

void HexDumpViewer::resized()
{
    // リサイズ時の処理（必要に応じて）
}

void HexDumpViewer::updateData(const std::vector<uint8_t>& data, uint32_t baseAddress)
{
    hexData = data;
    baseAddr = baseAddress;
    repaint();
}

void HexDumpViewer::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    int totalLines = (static_cast<int>(hexData.size()) + bytesPerLine - 1) / bytesPerLine;
    int maxLines = getHeight() / lineHeight;
    
    if (totalLines > maxLines) {
        scrollOffset -= static_cast<int>(wheel.deltaY * 3); // スクロール感度調整
        scrollOffset = juce::jlimit(0, totalLines - maxLines, scrollOffset);
        repaint();
    }
}

//==============================================================================

//==============================================================================
_3HSPlugAudioProcessorEditor::_3HSPlugAudioProcessorEditor (_3HSPlugAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // オシロスコープコンポーネントを作成
    //multiOscilloscope = std::make_unique<MultiOscilloscopeComponent>();
    //addAndMakeVisible(*multiOscilloscope);

    // 16進ダンプビューアーコンポーネントを作成
    hexDumpViewer = std::make_unique<HexDumpViewer>();
    addAndMakeVisible(*hexDumpViewer);

    // ウィンドウサイズを横側に拡張（既存の情報表示 + 16進ダンプビューアー）
    setSize (1200, 950);
    setResizable(true, true);
    setResizeLimits(800, 600, 2560, 1440); // 最小サイズも調整
    startTimerHz(60); // 60HzでtimerCallback()を呼ぶ
    // 時間経過による減衰計算
    currentDisplayTick = 0;
}

_3HSPlugAudioProcessorEditor::~_3HSPlugAudioProcessorEditor()
{
    stopTimer();
}

// パンポット表示用描画関数（LR分離バー表示）
void _3HSPlugAudioProcessorEditor::drawPanBars(juce::Graphics& g, int x, int y, int width, int height, int panL, int panR)
{
    // バーの幅は半分ずつ
    int barWidth = width / 2;
    int barHeight = height;
    
    // 背景色で全体をクリア
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(x, y, width, height);
    
    // Lチャンネルバー（左側）
    int lBarX = x;
    int lBarY = y;
    float lLevel = static_cast<float>(panL) / 15.0f; // 0-15の範囲を0.0-1.0に正規化
    int lBarFillHeight = static_cast<int>(barHeight * lLevel);
    
    // Lチャンネルの背景（グレー）
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(lBarX, lBarY, barWidth - 1, barHeight);
    
    // Lチャンネルの値表示（下から上に向かって描画）
    g.setColour(juce::Colours::cyan);
    g.fillRect(lBarX, lBarY + barHeight - lBarFillHeight, barWidth - 1, lBarFillHeight);
    
    // Rチャンネルバー（右側）
    int rBarX = x + barWidth;
    int rBarY = y;
    float rLevel = static_cast<float>(panR) / 15.0f; // 0-15の範囲を0.0-1.0に正規化
    int rBarFillHeight = static_cast<int>(barHeight * rLevel);
    
    // Rチャンネルの背景（グレー）
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(rBarX, rBarY, barWidth, barHeight);
    
    // Rチャンネルの値表示（下から上に向かって描画）
    g.setColour(juce::Colours::orange);
    g.fillRect(rBarX, rBarY + barHeight - rBarFillHeight, barWidth, rBarFillHeight);
    
    // L/Rラベル表示
    //g.setColour(juce::Colours::white);
    //g.setFont(juce::Font(8.0f));
    //g.drawText("L", lBarX, y - 10, barWidth - 1, 8, juce::Justification::centred);
    //g.drawText("R", rBarX, y - 10, barWidth, 8, juce::Justification::centred);
    
    // 外枠の描画
    //g.setColour(juce::Colours::lightgrey);
    //g.drawRect(juce::Rectangle<int>(x, y, width, height), 1.0f);
    
    // 中央の区切り線
    //g.drawLine(x + barWidth, y, x + barWidth, y + height, 1.0f);
}

//==============================================================================
void _3HSPlugAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font(30.0f));
    g.drawFittedText("3HSPlug", 5, 5, 100, 20, juce::Justification::centredLeft, 1);
    g.setFont (juce::Font(15.0f));
    g.drawFittedText("Harmonic Synthesizer with 3SGUC2X (3HS88PWN4) Emulation", 110, 5, 400, 20, juce::Justification::bottomLeft, 1);
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font(15.0f));
    juce::String debugText = "Voice Slot Debug Info\n";
    const auto& slots = audioProcessor.getVoiceSlots();
    int totalVoices = static_cast<int>(slots.size());
    int numChips = audioProcessor.getNumChips();
    int numVoices = audioProcessor.numVoices;
    // グラフィカルなバー表示
    int barHeight = 20;
    int barSpacing = 8;
    int barWidthMax = 192;
    int barStartX = 10;
    int barStartY = 40;
    for (int i = 0; i < totalVoices; ++i) {
        int chip = i / numVoices;
        int vIdx = i % numVoices;
        const auto& v = slots[i];

        // バーの色（ON:緑, OFF:灰色）

        // ボリューム値をバー幅に反映（0～255想定）
        int barWidth = barWidthMax * v.volume / 255;

        int y = barStartY + i * (barHeight + barSpacing);
        g.setColour(juce::Colours::darkgrey);
        g.fillRect(barStartX, y, barWidthMax, barHeight);
        if (v.inUse) {
            g.setColour(juce::Colours::limegreen);
        }
        g.fillRect(barStartX, y, barWidth, barHeight);
        if (v.inUse) {
            g.setColour(juce::Colours::red);
        }
        g.fillRect(barStartX+(v.noteNumber * barWidthMax / 128), y, 4, barHeight);
        if (v.inUse) {
            g.setColour(juce::Colours::red);
        
            int bendbarval = (audioProcessor.getCurrentPitchBendScaled(v.midiChannel) * barWidthMax) / 128;
            //printf("Bend Bar Value (Channel %d): %d\n", v.midiChannel, bendbarval);
            if (bendbarval > 0) {
                g.fillRect(barStartX+(v.noteNumber * barWidthMax / 128 + 2), y+barHeight/2, bendbarval+1, barHeight/2);
            } else if (bendbarval < 0) {
                g.fillRect(barStartX+(v.noteNumber * barWidthMax / 128 + 2) + bendbarval, y+barHeight/2, -(bendbarval)+1, barHeight/2);
            }
        }

        // パンポット表示を追加（LR分離バー表示）
        auto panValues = audioProcessor.getVoicePanValues(i);
        int panBarWidth = 20; // バー表示の幅
        int panBarHeight = barHeight - 2;
        int panBarX = barStartX + barWidthMax + 10;
        int panBarY = y + 1;
        
        drawPanBars(g, panBarX, panBarY, panBarWidth, panBarHeight, panValues.first, panValues.second);
        
        // テキスト情報（Chip, Note, Ch, Vol, Prg等）をパンポットの右に表示
        g.setColour(juce::Colours::white);
        juce::String infoText = "Chip " + juce::String(chip) + " " + juce::String(vIdx) + ": ";
        infoText += (v.inUse ? "ON  " : "OFF ");
        infoText += "Note=" + juce::String(v.noteNumber)
                  + " Ch=" + juce::String(v.midiChannel)
                  + " Vol=" + juce::String(v.volume)
                  + " Prg=" + juce::String(audioProcessor.getCurrentProgramForChannel(v.midiChannel))
                  + " Pan=" + juce::String(panValues.first) + "/" + juce::String(panValues.second)
                  ;
        g.drawFittedText(infoText, panBarX + panBarWidth + 25, y, 400, barHeight, juce::Justification::centredLeft, 1);
    }

    // ドラムPCMチャンネル情報のグラフィカルなバー表示
    auto drumInfos = audioProcessor.getDrumPcmChannelInfoAll();
    int drumBarStartY = barStartY + totalVoices * (barHeight + barSpacing) + 20; // ボイススロットの下に配置
    
    for (size_t i = 0; i < drumInfos.size(); ++i) {
        const auto& info = drumInfos[i];
        
        currentDisplayTick = audioProcessor.getCurrentTick();
        
        float decayFactor = 1.0f;
        if (info.active && info.lastUsedTick > 0) {
            // 最後に使用されてからの経過時間を計算（適当な減衰率）
            uint64_t ticksSinceUse = currentDisplayTick - info.lastUsedTick;
            const uint64_t decayTime = 24000;
            
            if (ticksSinceUse < decayTime) {
                decayFactor = 1.0f - (static_cast<float>(ticksSinceUse) / static_cast<float>(decayTime));
            } else {
                decayFactor = 0.0f;
            }
        }

        // velocity値とdecayFactorをバー幅に反映
        int baseWidth = info.active ? (barWidthMax * info.velocity / 127) : 0;
        int drumBarWidth = static_cast<int>(baseWidth * decayFactor);
        
        int y = drumBarStartY + static_cast<int>(i) * (barHeight + barSpacing);
        g.setColour(juce::Colours::darkgrey);
        g.fillRect(barStartX, y, barWidthMax, barHeight);
        
        // バーの色（Active:青系、Inactive:灰色）、減衰に応じて色も薄くする
        if (info.active && decayFactor > 0.0f) {
            float alpha = decayFactor;
            g.setColour(juce::Colours::cyan);
        } else {
            g.setColour(juce::Colours::darkgrey);
        }
        g.fillRect(barStartX, y, drumBarWidth, barHeight);

        // テキスト情報（ChID, Note, Ch, PCM等）をバーの右に表示
        g.setColour(juce::Colours::white);
        juce::String drumInfoText = "DrumCh " + juce::String(static_cast<int>(i)) + ": ";
        drumInfoText += (info.active ? "ON  " : "OFF ");
        drumInfoText += "Note=" + juce::String(info.noteNumber)
                      + " MIDICh=" + juce::String(info.midiChannel)
                      + " Vel=" + juce::String(info.velocity)
                      + " PCM=0x" + juce::String::toHexString(static_cast<int>(info.pcmAddr))
                      + " Rate=" + juce::String(static_cast<int>(info.sampleRate))
                     ;
        g.drawFittedText(drumInfoText, barStartX + barWidthMax + 10, y, 450, barHeight, juce::Justification::centredLeft, 1);
    }

    // CPU使用率メーターとパフォーマンス情報の表示
    int performanceStartY = drumBarStartY + static_cast<int>(drumInfos.size()) * (barHeight + barSpacing) + 30;
    
    // 処理時間の取得
    double totalCpuUsage = audioProcessor.getCpuUsagePercent();
    double totalProcessingTime = audioProcessor.getAudioProcessingTimeMs();
    double midiProcessingTime = audioProcessor.getMidiProcessingTimeMs();
    double synthProcessingTime = audioProcessor.getSynthProcessingTimeMs();
    
    // パフォーマンスモニタータイトル
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawFittedText("Performance Monitor", barStartX, performanceStartY, 200, 20, juce::Justification::centredLeft, 1);
    
    // 全体CPU使用率バー
    int cpuBarY = performanceStartY + 25;
    int cpuBarWidth = 300;
    int cpuBarHeight = 18;
    
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(barStartX, cpuBarY, cpuBarWidth, cpuBarHeight);
    
    // CPU使用率に応じた色分け（緑→黄→赤）
    juce::Colour cpuBarColour;
    if (totalCpuUsage < 50.0) {
        cpuBarColour = juce::Colours::limegreen;
    } else if (totalCpuUsage < 80.0) {
        cpuBarColour = juce::Colours::yellow;
    } else {
        cpuBarColour = juce::Colours::red;
    }
    
    g.setColour(cpuBarColour);
    int totalFillWidth = static_cast<int>((totalCpuUsage / 100.0) * cpuBarWidth);
    g.fillRect(barStartX, cpuBarY, totalFillWidth, cpuBarHeight);
    
    // 全体CPU使用率テキスト
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f));
    juce::String cpuText = "Total CPU: " + juce::String(totalCpuUsage, 1) + "%";
    g.drawFittedText(cpuText, barStartX + cpuBarWidth + 10, cpuBarY, 120, cpuBarHeight, juce::Justification::centredLeft, 1);
    
    // MIDI処理時間バー
    int midiBarY = cpuBarY + cpuBarHeight + 5;
    double bufferDurationMs = (audioProcessor.getBlockSize() / audioProcessor.getSampleRate() * 1000.0);
    double midiCpuUsage = (midiProcessingTime / bufferDurationMs) * 100.0;
    
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(barStartX, midiBarY, cpuBarWidth, cpuBarHeight);
    
    g.setColour(juce::Colours::cyan);
    int midiFillWidth = static_cast<int>((midiCpuUsage / 100.0) * cpuBarWidth);
    g.fillRect(barStartX, midiBarY, midiFillWidth, cpuBarHeight);
    
    g.setColour(juce::Colours::white);
    juce::String midiText = "MIDI: " + juce::String(midiCpuUsage, 1) + "%";
    g.drawFittedText(midiText, barStartX + cpuBarWidth + 10, midiBarY, 120, cpuBarHeight, juce::Justification::centredLeft, 1);
    
    // 音声合成処理時間バー
    int synthBarY = midiBarY + cpuBarHeight + 5;
    double synthCpuUsage = (synthProcessingTime / bufferDurationMs) * 100.0;
    
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(barStartX, synthBarY, cpuBarWidth, cpuBarHeight);
    
    g.setColour(juce::Colours::orange);
    int synthFillWidth = static_cast<int>((synthCpuUsage / 100.0) * cpuBarWidth);
    g.fillRect(barStartX, synthBarY, synthFillWidth, cpuBarHeight);
    
    g.setColour(juce::Colours::white);
    juce::String synthText = "Synth: " + juce::String(synthCpuUsage, 1) + "%";
    g.drawFittedText(synthText, barStartX + cpuBarWidth + 10, synthBarY, 120, cpuBarHeight, juce::Justification::centredLeft, 1);
    
    // 処理時間詳細表示
    int timeTextY = synthBarY + cpuBarHeight + 10;
    juce::String timeText = "Total: " + juce::String(totalProcessingTime, 2) + " ms / " + juce::String(bufferDurationMs, 2) + " ms";
    g.drawFittedText(timeText, barStartX, timeTextY, 400, 16, juce::Justification::centredLeft, 1);
    
    int midiTimeTextY = timeTextY + 18;
    juce::String midiTimeText = "MIDI: " + juce::String(midiProcessingTime, 3) + " ms, Synth: " + juce::String(synthProcessingTime, 3) + " ms";
    g.drawFittedText(midiTimeText, barStartX, midiTimeTextY, 400, 16, juce::Justification::centredLeft, 1);
    
    // サンプルレート情報
    int sampleRateTextY = midiTimeTextY + 20;
    juce::String sampleRateText = "Sample Rate: " + juce::String(audioProcessor.getSampleRate(), 0) + " Hz";
    g.drawFittedText(sampleRateText, barStartX, sampleRateTextY, 300, 16, juce::Justification::centredLeft, 1);
    
    // バッファサイズ情報
    int bufferSizeTextY = sampleRateTextY + 18;
    juce::String bufferText = "Buffer Size: " + juce::String(static_cast<int>(audioProcessor.getBlockSize())) + " samples";
    g.drawFittedText(bufferText, barStartX, bufferSizeTextY, 300, 16, juce::Justification::centredLeft, 1);

}

void _3HSPlugAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // 左側600pxを既存のGUI用に確保
    //auto leftPanel = bounds.removeFromLeft(600);
    
    // 右側に16進ダンプビューアーを配置
    //if (hexDumpViewer != nullptr) {
    //    hexDumpViewer->setBounds(bounds.reduced(5));
    //}
    
    // オシロスコープは無効化
    //if (multiOscilloscope != nullptr) {
    //    multiOscilloscope->setBounds(bounds.reduced(5));
    //}
}

void _3HSPlugAudioProcessorEditor::timerCallback()
{
    // 既存のGUI要素を再描画
    repaint();
    
    // 16進ダンプビューアーにサウンドレジスタデータを送信
    //if (hexDumpViewer != nullptr) {
    //    auto ramDump = audioProcessor.getRamDump();
    //    if (!ramDump.empty()) {
    //        hexDumpViewer->updateData(ramDump, 0x400000);
    //    }
    //}
    
    // オシロスコープは無効化
    //if (multiOscilloscope != nullptr) {
    //    updateOscilloscopeData();
    //}
}

/*
void _3HSPlugAudioProcessorEditor::updateOscilloscopeData()
{
    // 各チップからオーディオデータを取得してオシロスコープに送信
    for (int chip = 0; chip < audioProcessor.getNumChips(); ++chip) {
        auto audioDataL = audioProcessor.getChipAudioDataL(chip);
        auto audioDataR = audioProcessor.getChipAudioDataR(chip);

        // 各チャンネルのデータを更新
        for (int channel = 0; channel < 12; ++channel) {
            if (audioDataL.size() >= 2 &&
                channel < static_cast<int>(audioDataL[0].size()) &&
                channel < static_cast<int>(audioDataR[1].size())) {

                multiOscilloscope->updateChannelData(chip, channel,
                                                   audioDataL[channel],
                                                   audioDataR[channel]);
            }
        }
    }
}*/
