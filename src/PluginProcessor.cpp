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
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 25.0f,
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
    const float sr = static_cast<float> (sampleRate);
    piOverSr = juce::MathConstants<float>::pi / sr;

    // Input transformer HPF at ~80 Hz (cheap transformer LF roll-off)
    const float xfmrK = std::tan (piOverSr * 80.0f);
    inputXfmrCoeff = xfmrK / (1.0f + xfmrK);

    // Sidechain HPF at ~18 Hz (2 uF AC-coupling cap in detector path)
    const float scK = std::tan (piOverSr * 18.0f);
    scHpfCoeff = scK / (1.0f + scK);

    // Envelope detector coefficients (base values, modulated per-block by limit)
    attackCoeff  = std::exp (-1.0f / (sr * 0.0005f));
    releaseCoeff = std::exp (-1.0f / (sr * 0.7f));

    const float inp = apvts.getRawParameterValue ("input")->load();
    const float lim = apvts.getRawParameterValue ("limit")->load();
    const float out = apvts.getRawParameterValue ("output")->load();

    const float normInp    = inp / 100.0f;
    const float initInput  = normInp * normInp * 16.0f;
    const float threshDB   = -24.0f * (lim / 100.0f);
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

        envelope[ch]       = 0.0f;
        inputXfmrState[ch] = 0.0f;
        scHpfState[ch]     = 0.0f;
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

    // Input gain: audio taper (squared), max +24 dB
    const float normInp         = inp / 100.0f;
    const float targetInputGain = normInp * normInp * 16.0f;

    // Threshold: 0% limit = 0 dBFS (no limiting), 100% limit = -24 dBFS (heavy)
    const float threshDB        = -24.0f * (lim / 100.0f);
    const float targetThreshold = std::pow (10.0f, threshDB / 20.0f);

    // Output gain: audio taper (squared), max +24 dB
    const float normOut          = out / 100.0f;
    const float targetOutputGain = normOut * normOut * 16.0f;

    // Attack/release modulated by limit setting (distance selector interaction):
    // Higher limit -> faster attack (more aggressive), slightly faster release.
    // Attack: 800 us at limit=0 down to 300 us at limit=100
    // Release: fixed at 700 ms
    const float normLim   = lim / 100.0f;
    const float attackS   = 0.0008f - 0.0005f * normLim;   // 800 us -> 300 us
    const float sr        = juce::MathConstants<float>::pi / piOverSr;  // recover sample rate
    const float atkCoeff  = std::exp (-1.0f / (sr * attackS));

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

            // --- Input transformer ---
            // Cheap transformer: HPF at ~80 Hz (LF roll-off) + soft saturation.
            // "The transformers are a huge part of this sound" — Dan Korneff
            inputXfmrState[chi] += inputXfmrCoeff * (x - inputXfmrState[chi]);
            x -= inputXfmrState[chi];

            // Input transformer saturation (asymmetric, cheap core)
            const float xAbs = std::abs (x);
            if (xAbs > 0.4f)
            {
                const float sat   = std::tanh (x);
                const float blend = juce::jlimit (0.0f, 1.0f, (xAbs - 0.4f) * 2.5f);
                x = x * (1.0f - blend) + sat * blend;
            }

            // --- Sidechain with HPF (AC-coupling cap, ~18 Hz) ---
            // The 2 uF cap in the detector path means the limiter is
            // less sensitive to sub-bass content.
            scHpfState[chi] += scHpfCoeff * (x - scHpfState[chi]);
            const float scSignal = x - scHpfState[chi];
            const float scAbs    = std::abs (scSignal);

            // --- Envelope follower (peak detector) ---
            // Attack/release modulated by limit setting (distance selector).
            if (scAbs > envelope[chi])
                envelope[chi] = scAbs + atkCoeff * (envelope[chi] - scAbs);
            else
                envelope[chi] = scAbs + releaseCoeff * (envelope[chi] - scAbs);

            // --- Gain reduction ---
            // Brickwall limiter, ratio ~20:1.  The original's "locked" output
            // behavior is closer to infinity:1; the apparent 40 dB -> 6 dB spec
            // includes transformer softening.
            float gain = 1.0f;
            if (envelope[chi] > th && th > 1.0e-10f)
                gain = std::pow (th / envelope[chi], kCompExp);

            // --- JFET nonlinearity (2N5458 variable resistor) ---
            // Even-harmonic distortion proportional to gain reduction.
            float compressed = x * gain;
            const float grAmount = juce::jlimit (0.0f, 1.0f, 1.0f - gain);
            compressed *= 1.0f + grAmount * 0.12f * compressed;

            // --- Output transformer ---
            // No buffer stage: FET output drives step-up transformer directly.
            // Saturation varies with gain reduction (impedance changes).
            const float outDrive = 1.5f + grAmount * 2.0f;
            compressed = std::tanh (compressed * outDrive) / outDrive;

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
