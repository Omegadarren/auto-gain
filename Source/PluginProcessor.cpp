#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
AutoGainAudioProcessor::buildLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "outputGain", "Output Gain",
        juce::NormalisableRange<float> (-40.0f, 0.0f, 0.1f),
        -18.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "mix", "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    return layout;
}

//==============================================================================
AutoGainAudioProcessor::AutoGainAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , apvts (*this, nullptr, "Parameters", buildLayout())
{}

//==============================================================================
bool AutoGainAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    auto ins = layouts.getMainInputChannelSet();
    return (ins == juce::AudioChannelSet::mono() ||
            ins == juce::AudioChannelSet::stereo());
}

//==============================================================================
void AutoGainAudioProcessor::prepareToPlay (double sr, int)
{
    currentSampleRate = sr;

    // RMS envelope: 300ms — slow enough that individual notes don't affect it
    rmsCoeff  = std::exp (-1.0f / (0.30f * (float)sr));
    // Gain smoothing: 600ms — very gradual, "no dynamics" transitions
    gainCoeff = std::exp (-1.0f / (0.60f * (float)sr));

    rmsPower        = 0.f;
    gainSmooth      = 1.f;
    inputRmsSmooth  = 0.f;
    outputRmsSmooth = 0.f;
    gainHistory.fill (0.f);
    historyWritePos.store (0);
}

void AutoGainAudioProcessor::releaseResources() {}

//==============================================================================
void AutoGainAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int   numSamples    = buffer.getNumSamples();
    const int   numCh         = juce::jmin (buffer.getNumChannels(), 2);
    const float sr            = (float)currentSampleRate;

    const float targetDb      = apvts.getRawParameterValue ("outputGain")->load();
    const float targetLinear  = std::pow (10.f, targetDb / 20.f);
    const float mixFrac       = apvts.getRawParameterValue ("mix")->load() / 100.f;

    static constexpr float kEpsilon = 1.0e-5f;   // -100 dBFS noise floor
    static constexpr float kMaxGain = 100.f;      // +40 dB max boost

    float inSumSq    = 0.f;
    float outSumSq   = 0.f;
    float gainSumLin = 0.f;

    for (int n = 0; n < numSamples; ++n)
    {
        // ── Read input samples (save to local so we don't re-read after write) ──
        float inp[2] = { 0.f, 0.f };
        float monoIn = 0.f;
        for (int ch = 0; ch < numCh; ++ch)
        {
            inp[ch] = buffer.getReadPointer (ch)[n];
            monoIn += inp[ch];
        }
        monoIn /= (float)numCh;

        // ── Slow RMS envelope follower ────────────────────────────────────────
        rmsPower = rmsCoeff * rmsPower + (1.f - rmsCoeff) * monoIn * monoIn;
        float rmsLin = std::sqrt (juce::jmax (rmsPower, kEpsilon * kEpsilon));

        // ── Desired gain to reach target level ───────────────────────────────
        float desired = juce::jlimit (1.f / kMaxGain, kMaxGain, targetLinear / rmsLin);

        // ── Smooth the gain (avoids any audible dynamics artifacts) ───────────
        gainSmooth = gainCoeff * gainSmooth + (1.f - gainCoeff) * desired;

        // ── Apply with wet/dry blend ──────────────────────────────────────────
        float outSample0 = 0.f;
        for (int ch = 0; ch < numCh; ++ch)
        {
            float wet = inp[ch] * gainSmooth;
            float out = inp[ch] + (wet - inp[ch]) * mixFrac;
            buffer.getWritePointer (ch)[n] = out;
            if (ch == 0) outSample0 = out;
        }

        inSumSq    += monoIn    * monoIn;
        outSumSq   += outSample0 * outSample0;
        gainSumLin += gainSmooth;
    }

    // Copy mono to right channel if needed
    if (buffer.getNumChannels() >= 2 && numCh == 1)
        buffer.copyFrom (1, 0, buffer.getReadPointer (0), numSamples);

    // ── Update meters ─────────────────────────────────────────────────────────
    {
        const float rmsIn  = std::sqrt (inSumSq  / (float)numSamples);
        const float rmsOut = std::sqrt (outSumSq / (float)numSamples);

        // Faster smoothing for the visible meter (50ms)
        const float mc = std::exp (-1.f / (0.05f * sr / juce::jmax (numSamples, 1)));
        inputRmsSmooth  = mc * inputRmsSmooth  + (1.f - mc) * rmsIn;
        outputRmsSmooth = mc * outputRmsSmooth + (1.f - mc) * rmsOut;

        auto toDb = [](float x) noexcept {
            return x > 1.0e-6f ? 20.f * std::log10 (x) : -120.f;
        };
        inputLevelDb .store (juce::jlimit (-120.f,  6.f, toDb (inputRmsSmooth)));
        outputLevelDb.store (juce::jlimit (-120.f,  6.f, toDb (outputRmsSmooth)));

        float avgGainLin = gainSumLin / (float)numSamples;
        float avgGainDb  = avgGainLin > 1e-6f ? 20.f * std::log10 (avgGainLin) : -120.f;
        gainAppliedDb.store (juce::jlimit (-40.f, 40.f, avgGainDb));

        // Push one history value per block
        int wp = historyWritePos.load();
        gainHistory[wp] = juce::jlimit (-40.f, 40.f, avgGainDb);
        historyWritePos.store ((wp + 1) % kHistorySize);
    }
}

//==============================================================================
void AutoGainAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    xml->setAttribute ("editorZoom", editorZoomIndex);
    copyXmlToBinary (*xml, destData);
}

void AutoGainAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
    {
        editorZoomIndex = juce::jlimit (0, 2, xml->getIntAttribute ("editorZoom", 0));
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

//==============================================================================
juce::AudioProcessorEditor* AutoGainAudioProcessor::createEditor()
{
    return new AutoGainAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AutoGainAudioProcessor();
}
