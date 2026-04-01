#pragma once

#include <JuceHeader.h>

class LocBoxAudioProcessor : public juce::AudioProcessor
{
public:
    LocBoxAudioProcessor();
    ~LocBoxAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Loc-Box"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.7; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

private:
    // Smoothed parameter values (per channel)
    std::array<juce::SmoothedValue<float>, 2> inputGainSmoothed;
    std::array<juce::SmoothedValue<float>, 2> thresholdSmoothed;
    std::array<juce::SmoothedValue<float>, 2> outputGainSmoothed;

    // Envelope follower state (per channel)
    float envelope[2] { 0.0f, 0.0f };

    // Input transformer HPF state (per channel, ~80 Hz roll-off)
    float inputXfmrState[2] { 0.0f, 0.0f };

    // Sidechain HPF state (per channel, ~18 Hz AC coupling)
    float scHpfState[2] { 0.0f, 0.0f };

    // Coefficients (computed in prepareToPlay)
    float attackCoeff     = 0.0f;
    float releaseCoeff    = 0.0f;
    float inputXfmrCoeff  = 0.0f;   // input transformer HPF
    float scHpfCoeff      = 0.0f;   // sidechain AC-coupling HPF
    float piOverSr        = 0.0f;

    // Compression exponent (ratio-dependent, computed once)
    static constexpr float kCompExp = 19.0f / 20.0f;  // ratio 20:1

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LocBoxAudioProcessor)
};
