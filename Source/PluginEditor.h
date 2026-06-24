#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Ui/PlateLookAndFeel.h"

//==============================================================================
class AutoGainAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit AutoGainAudioProcessorEditor (AutoGainAudioProcessor&);
    ~AutoGainAudioProcessorEditor() override;

    void paint                  (juce::Graphics&) override;
    void resized                () override;
    void mouseDown              (const juce::MouseEvent&) override;
    void visibilityChanged      () override;
    void parentHierarchyChanged () override;

private:
    void timerCallback () override;
    void applyZoom     ();

    AutoGainAudioProcessor& processorRef;

    std::unique_ptr<juce::LookAndFeel_V4> laf;
    std::unique_ptr<juce::Component>      gainDisplay;  // GainDisplay (defined in .cpp)

    // Controls
    juce::Slider attackSlider         { juce::Slider::RotaryVerticalDrag,
                                        juce::Slider::TextBoxBelow };
    juce::Label  attackLabel;
    juce::Slider releaseSlider        { juce::Slider::RotaryVerticalDrag,
                                        juce::Slider::TextBoxBelow };
    juce::Label  releaseLabel;
    juce::Slider outputGainSlider     { juce::Slider::RotaryVerticalDrag,
                                        juce::Slider::TextBoxBelow };
    juce::Label  outputGainLabel;
    juce::Slider mixSlider            { juce::Slider::RotaryVerticalDrag,
                                        juce::Slider::TextBoxBelow };
    juce::Label  mixLabel;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAtt> attackAtt, releaseAtt, outputGainAtt, mixAtt;

    // Zoom
    int zoomIndex = 0;
    bool centred = false;
    static constexpr float       kZoomFactors[] = { 1.0f, 1.5f, 2.0f };
    static constexpr const char* kZoomLabels[]  = { "1x", "1.5x", "2x" };
    static constexpr int kBaseW = 600;
    static constexpr int kBaseH = 460;
    juce::Rectangle<int> zoomButtonBounds;

    juce::TooltipWindow tooltipWindow { this, 700 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoGainAudioProcessorEditor)
};
