#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cmath>

//==============================================================================
/**
 * Auto Gain — automatic level normaliser.
 *
 * Measures the input RMS with a slow envelope follower (no dynamics processing)
 * and applies a smoothed inverse gain so the output stays at the target level.
 *
 * Parameters:
 *   outputGain  - target output RMS level  -40 to 0 dB  (default -18 dBFS)
 *   mix         - wet/dry blend             0 to 100 %   (default 100%)
 */
class AutoGainAudioProcessor : public juce::AudioProcessor
{
public:
    // Ring-buffer size for gain history display
    static constexpr int kHistorySize = 512;

    AutoGainAudioProcessor();
    ~AutoGainAudioProcessor() override = default;

    void prepareToPlay  (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock   (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName()    const override { return JucePlugin_Name; }
    bool  acceptsMidi()             const override { return false; }
    bool  producesMidi()            const override { return false; }
    bool  isMidiEffect()            const override { return false; }
    double getTailLengthSeconds()   const override { return 0.0; }

    int  getNumPrograms()  override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    double getCurrentSampleRate() const noexcept { return currentSampleRate; }
    int editorZoomIndex = 1;

    // ── Thread-safe meters (audio thread writes, UI thread reads) ────────────
    std::atomic<float> inputLevelDb  { -120.f };
    std::atomic<float> outputLevelDb { -120.f };
    std::atomic<float> gainAppliedDb { 0.f };

    // ── Gain history ring buffer ─────────────────────────────────────────────
    // Audio thread writes one value per processBlock (gain in dB).
    // UI thread reads sequentially for the history display.
    std::array<float, kHistorySize> gainHistory {};
    std::atomic<int> historyWritePos { 0 };

private:
    double currentSampleRate = 44100.0;

    // Attack/release envelope follower for note-level tracking
    float envFollower    = 1.f;   // smoothed peak level (linear)
    float gainSmooth     = 1.f;   // smoothed gain (linear)
    float attackCoeff    = 0.f;   // fast attack  (~15 ms)
    float releaseCoeff   = 0.f;   // slow release (~600 ms)
    float gainCoeff      = 0.f;   // tiny gain smooth (~2 ms) to prevent zippering

    // Separate meter smoothers (faster, for UI display)
    float inputRmsSmooth  = 0.f;
    float outputRmsSmooth = 0.f;

    static juce::AudioProcessorValueTreeState::ParameterLayout buildLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoGainAudioProcessor)
};
