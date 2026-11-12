#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include "BBDEngine.h"

namespace yoko
{
// Interval knob detents (noon = unison).
static constexpr int kNumIntervals = 11;
static constexpr int kIntervalSemis[kNumIntervals] = { -24, -12, -7, -5, -4, 0, 4, 5, 7, 12, 24 };
inline const char* intervalName (int i)
{
    static const char* names[kNumIntervals] = { "2 octaves down", "1 octave down", "5th down", "4th down",
                                                "3rd down", "same note", "3rd up", "4th up", "5th up",
                                                "1 octave up", "2 octaves up" };
    return names[juce::jlimit (0, kNumIntervals - 1, i)];
}
inline int intervalIndex (float norm) { return juce::jlimit (0, kNumIntervals - 1, (int) std::round (norm * (kNumIntervals - 1))); }

namespace ID
{
    static const juce::String mix = "mix", lpf = "lpf", regen = "regen", glide = "glide",
                              int1 = "int1", int2 = "int2",
                              len1 = "len1", len2 = "len2", len3 = "len3",
                              memory = "memory", time = "time", on = "on",
                              sync = "sync", modulation = "modulation", step = "step", slowdown = "slowdown",
                              rampLpf = "rampLpf", rampRegen = "rampRegen", rampGlide = "rampGlide",
                              trails = "trails", momentary = "momentary";
}
} // namespace yoko

class YokokokoAudioProcessor : public juce::AudioProcessor,
                               private juce::AudioProcessorValueTreeState::Listener,
                               private juce::AsyncUpdater
{
public:
    YokokokoAudioProcessor();
    ~YokokokoAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Yokoko Onono Pitch Delay"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 40.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // Called from the GUI (message thread)
    void tap();
    void saveToCurrentSlot();
    bool slotHasData (int slot) const { return slots[(size_t) slot].isValid(); }

    // Read by the GUI
    std::atomic<int>   currentStep { 0 };
    std::atomic<float> currentDelayMs { 500.0f }, currentClockHz { 16384.0f }, currentSemis { 0.0f };
    std::atomic<float> baseDelayMs { 500.0f };
    std::atomic<bool>  followingHost { false };

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    void parameterChanged (const juce::String& id, float newValue) override;
    void handleAsyncUpdate() override;
    juce::ValueTree takeSnapshot() const;
    void applySnapshot (const juce::ValueTree&);

    yoko::WetPath wet;
    double fs = 48000.0;

    // clock / glide (log2 Hz)
    double logF = std::log2 (16384.0), glideTarget = logF, glideRate = 0.0;
    bool   clockInit = false;

    // sequencer
    double freeBeatPos = 0.0;
    int    manualStep = 0, lastStep = -1;
    std::atomic<int>  pendingAdvance { 0 };
    std::atomic<bool> pendingReset { false };

    // modulation LFO / ramp
    double lfoPhase = 0.0, rampPhase = 0.0, rndTarget = 0.0, rndSmooth = 0.0, rndHeld = 0.0;
    juce::Random rng;

    // smoothing
    juce::SmoothedValue<float> sMix, sLpf, sRegen, sOnWet, sInGate;

    // tap tempo
    juce::Array<double> tapTimes;

    // memory slots
    std::array<juce::ValueTree, 2> slots;
    std::atomic<int> pendingRecall { -1 };
    bool applyingSnapshot = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YokokokoAudioProcessor)
};
