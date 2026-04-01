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

    // Attack/release coefficients (computed in prepareToPlay)
    float attackCoeff  = 0.0f;
    float releaseCoeff = 0.0f;

    // Compression constants
    static constexpr float kRatio    = 7.0f;            // ~7:1 (40dB in -> 6dB out)
    static constexpr float kCompExp  = 6.0f / 7.0f;     // (ratio-1)/ratio
    static constexpr float kAttackS  = 0.0005f;          // 500 us
    static constexpr float kReleaseS = 0.7f;             // 700 ms

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LocBoxAudioProcessor)
};
