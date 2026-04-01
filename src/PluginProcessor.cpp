#include "PluginProcessor.h"
#include "PluginEditor.h"

LocBoxAudioProcessor::LocBoxAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

LocBoxAudioProcessor::~LocBoxAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout
LocBoxAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // INPUT: signal level into the limiter (audio taper)
    juce::NormalisableRange<float> inputRange (0.0f, 100.0f, 0.01f);
    inputRange.setSkewForCentre (25.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "input", 1 }, "Input", inputRange, 50.0f,
        juce::AudioParameterFloatAttributes().withLabel (" %")));

    // LIMIT: compression amount (threshold, 0 = off, 100 = heavy)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "limit", 1 }, "Limit",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel (" %")));

    // OUTPUT: makeup gain (audio taper)
    juce::NormalisableRange<float> outputRange (0.0f, 100.0f, 0.01f);
    outputRange.setSkewForCentre (35.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "output", 1 }, "Output", outputRange, 50.0f,
        juce::AudioParameterFloatAttributes().withLabel (" %")));

    return layout;
}

void LocBoxAudioProcessor::prepareToPlay (double sampleRate, int)
{
    // Envelope detector coefficients
    attackCoeff  = std::exp (-1.0f / (static_cast<float> (sampleRate) * kAttackS));
    releaseCoeff = std::exp (-1.0f / (static_cast<float> (sampleRate) * kReleaseS));

    const float inp = apvts.getRawParameterValue ("input")->load();
    const float lim = apvts.getRawParameterValue ("limit")->load();
    const float out = apvts.getRawParameterValue ("output")->load();

    const float normInp    = inp / 100.0f;
    const float initInput  = normInp * normInp * 16.0f;
    const float threshDB   = -48.0f * (lim / 100.0f);
    const float initThresh = std::pow (10.0f, threshDB / 20.0f);
    const float normOut    = out / 100.0f;
    const float initOutput = normOut * normOut * 16.0f;

    for (size_t ch = 0; ch < 2; ++ch)
    {
        inputGainSmoothed[ch].reset (sampleRate, 0.05);
        inputGainSmoothed[ch].setCurrentAndTargetValue (initInput);

        thresholdSmoothed[ch].reset (sampleRate, 0.05);
        thresholdSmoothed[ch].setCurrentAndTargetValue (initThresh);

        outputGainSmoothed[ch].reset (sampleRate, 0.02);
        outputGainSmoothed[ch].setCurrentAndTargetValue (initOutput);

        envelope[ch] = 0.0f;
    }
}

void LocBoxAudioProcessor::releaseResources() {}

bool LocBoxAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

void LocBoxAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float inp = apvts.getRawParameterValue ("input")->load();
    const float lim = apvts.getRawParameterValue ("limit")->load();
    const float out = apvts.getRawParameterValue ("output")->load();

    // Input gain: audio taper (squared), max +12 dB
    const float normInp         = inp / 100.0f;
    const float targetInputGain = normInp * normInp * 16.0f;

    // Threshold: 0% limit = 0 dBFS (no limiting), 100% limit = -48 dBFS (heavy)
    const float threshDB        = -48.0f * (lim / 100.0f);
    const float targetThreshold = std::pow (10.0f, threshDB / 20.0f);

    // Output gain: audio taper (squared), max +12 dB
    const float normOut          = out / 100.0f;
    const float targetOutputGain = normOut * normOut * 16.0f;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels && ch < 2; ++ch)
    {
        const auto chi = static_cast<size_t> (ch);

        inputGainSmoothed[chi].setTargetValue (targetInputGain);
        thresholdSmoothed[chi].setTargetValue (targetThreshold);
        outputGainSmoothed[chi].setTargetValue (targetOutputGain);

        float* data = buffer.getWritePointer (ch);

        for (int i = 0; i < numSamples; ++i)
        {
            const float ig = inputGainSmoothed[chi].getNextValue();
            const float th = thresholdSmoothed[chi].getNextValue();
            const float og = outputGainSmoothed[chi].getNextValue();

            float x = data[i] * ig;

            // --- Envelope follower (peak detector) ---
            // Models the full-wave rectifier + RC network of the Level Loc sidechain.
            // Attack:  39 ohm into 2 uF -> ~500 us
            // Release: 1M ohm from 2 uF -> ~700 ms
            const float absX = std::abs (x);
            if (absX > envelope[chi])
                envelope[chi] = absX + attackCoeff * (envelope[chi] - absX);
            else
                envelope[chi] = absX + releaseCoeff * (envelope[chi] - absX);

            // --- Gain reduction ---
            // Level Loc spec: 40 dB input change -> ~6 dB output change.
            // Modelled as feedforward compressor with ratio ~7:1.
            // gain = (threshold / envelope) ^ ((ratio-1)/ratio)
            float gain = 1.0f;
            if (envelope[chi] > th && th > 1.0e-10f)
                gain = std::pow (th / envelope[chi], kCompExp);

            // --- JFET nonlinearity (2N5458 variable resistor) ---
            // The FET adds subtle even-harmonic distortion that increases
            // with gain reduction.  Below ~100 mV it behaves as a linear
            // variable resistor; above that it transitions to a variable
            // current source, introducing asymmetric distortion.
            float compressed = x * gain;
            const float grAmount = juce::jlimit (0.0f, 1.0f, 1.0f - gain);
            compressed *= 1.0f + grAmount * 0.12f * compressed;

            // --- Transistor stage soft saturation ---
            // Models the discrete 2N5088 amplifier stages clipping
            // when driven hard (3% THD spec).
            if (std::abs (compressed) > 0.8f)
            {
                const float softClip = std::tanh (compressed * 1.25f);
                const float blend    = juce::jlimit (0.0f, 1.0f,
                                           (std::abs (compressed) - 0.8f) * 5.0f);
                compressed = compressed * (1.0f - blend) + softClip * blend;
            }

            data[i] = compressed * og;
        }
    }
}

juce::AudioProcessorEditor* LocBoxAudioProcessor::createEditor()
{
    return new LocBoxAudioProcessorEditor (*this);
}

void LocBoxAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void LocBoxAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LocBoxAudioProcessor();
}
