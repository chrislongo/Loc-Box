#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class BlackKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BlackKnobLookAndFeel();

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;
};

//==============================================================================
class LocBoxAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit LocBoxAudioProcessorEditor (LocBoxAudioProcessor&);
    ~LocBoxAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    BlackKnobLookAndFeel blackKnobLAF;

    juce::Slider inputKnob, limitKnob, outputKnob;
    juce::Label  inputLabel, limitLabel, outputLabel;

    juce::AudioProcessorValueTreeState::SliderAttachment inputAttachment,
                                                         limitAttachment,
                                                         outputAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LocBoxAudioProcessorEditor)
};
