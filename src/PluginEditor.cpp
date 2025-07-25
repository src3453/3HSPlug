/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#define y true
#define _ false
// モジュレーションモードに応じて、音量に応じてボリュームレジスタをスケーリングすべきかを定義する (true: スケーリングする, false: しない)
// 可読性のために、y: true, _: false としている
bool volumeScalingMap[13][8] = {
    {y,y,y,y,y,y,y,y}, //  0: Additive
    {y,y,y,y,_,_,_,_}, //  1: 4x2OP FM
    {y,y,y,y,_,_,_,_}, //  2: 4x2 RingMod
    {y,y,_,_,_,_,_,_}, //  3: 2x4OP FM
    {y,_,_,_,_,_,_,_}, //  4: 8OP FM
    {y,y,_,_,y,y,_,_}, //  5: 4OP FM x2
    {y,_,y,_,y,_,y,_}, //  6: 2OP FM x4
    {y,_,_,_,_,_,_,_}, //  7: 4OP FMxRM x2
    {y,y,_,_,_,_,_,_}, //  8: 2x4 RingMod
    {y,_,_,_,_,_,_,_}, //  9: 2OP FMxRM x4
    {y,y,y,y,_,_,_,_}, // 10: 2OP DirectPhase
    {y,y,_,_,_,_,_,_}, // 11: 4OP DirectPhase
    {y,_,_,_,_,_,_,_}, // 12: 8OP DirectPhase
};
#undef y // 一時的なマクロ定義を削除
#undef _

//==============================================================================
_3HSPlugAudioProcessorEditor::_3HSPlugAudioProcessorEditor (_3HSPlugAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (600, 800);
    setResizable(true, true);
    setResizeLimits(300, 200, 1920, 1080);
    startTimerHz(60); // 60HzでtimerCallback()を呼ぶ
    // 時間経過による減衰計算
    currentDisplayTick = 0;
}

_3HSPlugAudioProcessorEditor::~_3HSPlugAudioProcessorEditor()
{
    stopTimer();
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
    int barHeight = 16;
    int barSpacing = 8;
    int barWidthMax = 120;
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

        // テキスト情報（Chip, Note, Ch, Vol, Prg等）をバーの下に表示
        g.setColour(juce::Colours::white);
        juce::String infoText = "Chip " + juce::String(chip) + " " + juce::String(vIdx) + ": ";
        infoText += (v.inUse ? "ON  " : "OFF ");
        infoText += "Note=" + juce::String(v.noteNumber)
                  + " Ch=" + juce::String(v.midiChannel)
                  + " Vol=" + juce::String(v.volume)
                  + " Prg=" + juce::String(audioProcessor.getCurrentProgramForChannel(v.midiChannel))
                  ;
        g.drawFittedText(infoText, barStartX + barWidthMax + 10, y, 320, barHeight, juce::Justification::centredLeft, 1);
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

}

void _3HSPlugAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}

void _3HSPlugAudioProcessorEditor::timerCallback()
{
    repaint();
}
