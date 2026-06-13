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

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "attack", "Attack",
        juce::NormalisableRange<float> (1.0f, 500.0f, 0.1f, 0.4f),
        15.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "release", "Release",
        juce::NormalisableRange<float> (50.0f, 2000.0f, 1.0f, 0.4f),
        600.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

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

    // 15ms attack  — quickly catches new note levels
    attackCoeff  = std::exp (-1.f / (0.015f * (float)sr));
    // 600ms release — slowly drops between notes, avoids noise-floor pumping
    releaseCoeff = std::exp (-1.f / (0.600f * (float)sr));
    // 2ms gain smooth — just enough to prevent zipper noise, no audible lag
    gainCoeff    = std::exp (-1.f / (0.002f * (float)sr));

    // Start envelope at 1.0 so initial desired gain = targetLinear (no startup spike)
    envFollower     = 1.f;
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
    const float outGainLinear = std::pow (10.f, targetDb / 20.f);
    const float mixFrac       = apvts.getRawParameterValue ("mix")->load() / 100.f;

    // Recalculate attack/release coefficients from user params
    const float attackMs  = apvts.getRawParameterValue ("attack")->load();
    const float releaseMs = apvts.getRawParameterValue ("release")->load();
    attackCoeff  = std::exp (-1.f / (attackMs  * 0.001f * sr));
    releaseCoeff = std::exp (-1.f / (releaseMs * 0.001f * sr));

    static constexpr float kEpsilon = 1.0e-5f;   // -100 dBFS noise floor
    static constexpr float kMaxGain = 10.f;       // +20 dB max boost (prevents runaway on silence)

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

        // ── Attack/release envelope follower ─────────────────────────────────
        float absIn = std::abs (monoIn);
        if (absIn > envFollower)
            envFollower = attackCoeff  * envFollower + (1.f - attackCoeff)  * absIn;
        else
            envFollower = releaseCoeff * envFollower + (1.f - releaseCoeff) * absIn;

        // ── Desired gain: bring envelope level up to target ────────────────
        float level   = juce::jmax (envFollower, kEpsilon);
        float desired = juce::jlimit (1.f / kMaxGain, kMaxGain, outGainLinear / level);

        // ── Tiny gain smooth to avoid zipper noise ────────────────────────
        gainSmooth = gainCoeff * gainSmooth + (1.f - gainCoeff) * desired;

        // ── Signal path: dry blend + output trim ──────────────────────────────
        // Mix=0%  → pure dry (original signal, untouched)
        // Mix=100% → fully auto-gain-normalized, trimmed to target output level
        // outGainLinear only scales the wet (affected) portion
        float outSample0 = 0.f;
        for (int ch = 0; ch < numCh; ++ch)
        {
            float wet = inp[ch] * gainSmooth * outGainLinear;
            float out = inp[ch] * (1.f - mixFrac) + wet * mixFrac;
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
