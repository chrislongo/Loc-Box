#include "PluginEditor.h"

//==============================================================================
// BlackKnobLookAndFeel
//==============================================================================

BlackKnobLookAndFeel::BlackKnobLookAndFeel() {}

void BlackKnobLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                              int x, int y, int width, int height,
                                              float sliderPosProportional,
                                              float rotaryStartAngle,
                                              float rotaryEndAngle,
                                              juce::Slider&)
{
    const float cx = x + width  * 0.5f;
    const float cy = y + height * 0.5f;
    const float r  = juce::jmin (width, height) * 0.5f - 12.0f;

    // Tick marks
    constexpr int kNumTicks = 7;
    g.setColour (juce::Colour (0xff888888));
    for (int i = 0; i < kNumTicks; ++i)
    {
        const float t     = (float) i / (float) (kNumTicks - 1);
        const float angle = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
        const float sinA  = std::sin (angle);
        const float cosA  = std::cos (angle);
        g.drawLine (cx + sinA * (r + 4.0f),  cy - cosA * (r + 4.0f),
                    cx + sinA * (r + 10.0f), cy - cosA * (r + 10.0f),
                    1.0f);
    }

    // Black body
    g.setColour (juce::Colour (0xff111111));
    g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);

    // White indicator line
    {
        const float angle = rotaryStartAngle
                          + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        const float sinA  = std::sin (angle);
        const float cosA  = -std::cos (angle);

        const float innerR = r * 0.30f;
        const float outerR = r * 0.78f;

        juce::Path line;
        line.startNewSubPath (cx + sinA * innerR, cy + cosA * innerR);
        line.lineTo          (cx + sinA * outerR, cy + cosA * outerR);

        g.setColour (juce::Colours::white);
        g.strokePath (line, juce::PathStrokeType (2.2f));
    }
}

//==============================================================================
// LocBoxAudioProcessorEditor
//==============================================================================

LocBoxAudioProcessorEditor::LocBoxAudioProcessorEditor (LocBoxAudioProcessor& p)
    : AudioProcessorEditor (&p),
      inputAttachment  (p.apvts, "input",  inputKnob),
      limitAttachment  (p.apvts, "limit",  limitKnob),
      outputAttachment (p.apvts, "output", outputKnob)
{
    setSize (320, 230);

    juce::Slider* knobs[] = { &inputKnob, &limitKnob, &outputKnob };
    for (auto* knob : knobs)
    {
        knob->setSliderStyle (juce::Slider::RotaryVerticalDrag);
        knob->setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        knob->setLookAndFeel (&blackKnobLAF);
        addAndMakeVisible (knob);
    }

    auto setupLabel = [this] (juce::Label& label, const char* text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setFont (juce::Font (juce::FontOptions().withHeight (10.0f).withStyle ("Bold")));
        label.setColour (juce::Label::textColourId, juce::Colour (0xff111111));
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
    };

    setupLabel (inputLabel,  "Input");
    setupLabel (limitLabel,  "Limit");
    setupLabel (outputLabel, "Output");
}

LocBoxAudioProcessorEditor::~LocBoxAudioProcessorEditor()
{
    juce::Slider* knobs[] = { &inputKnob, &limitKnob, &outputKnob };
    for (auto* knob : knobs)
        knob->setLookAndFeel (nullptr);
}

void LocBoxAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xffd8d8d8));
    g.fillAll();

    juce::ColourGradient overlay (juce::Colour (0x18ffffff), 0.0f, 0.0f,
                                   juce::Colour (0x18000000), 0.0f, static_cast<float> (getHeight()), false);
    g.setGradientFill (overlay);
    g.fillAll();

    // Plugin name
    g.setColour (juce::Colour (0xff111111));
    g.setFont (juce::Font (juce::FontOptions().withHeight (16.0f).withStyle ("Bold")));
    g.drawText ("Loc-Box", 12, 6, 150, 20, juce::Justification::bottomLeft);
}

void LocBoxAudioProcessorEditor::resized()
{
    constexpr int cx[3]  = { 53, 160, 267 };
    constexpr int r      = 48;  // 36 knob body + 12 tick margin
    constexpr int gap    = 4;
    constexpr int labelH = 15;
    constexpr int labelW = 100;

    const int groupH = r * 2 + gap + labelH;
    const int cy     = (getHeight() - groupH) / 2 + r;

    juce::Slider* knobs[]  = { &inputKnob,  &limitKnob,  &outputKnob  };
    juce::Label*  labels[] = { &inputLabel, &limitLabel, &outputLabel };

    for (int i = 0; i < 3; ++i)
    {
        knobs[i] ->setBounds (cx[i] - r,        cy - r,        r * 2,  r * 2);
        labels[i]->setBounds (cx[i] - labelW/2, cy + r + gap,  labelW, labelH);
    }
}
