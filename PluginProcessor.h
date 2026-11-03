#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <vector>

class NoteDetectorProcessor : public juce::AudioProcessor
{
public:
    NoteDetectorProcessor();
    ~NoteDetectorProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Note Detector"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    // read by the editor
    std::atomic<int>   detectedNote { -1 };   // MIDI note number, -1 = none
    std::atomic<float> detectedFreq { 0.0f };
    std::atomic<float> detectedConf { 0.0f };

private:
    static constexpr int frameSize = 2048;    // analysis window
    static constexpr int hopSize   = 512;     // analyse every ~11 ms @ 44.1k

    void analyse (int sampleIndex, juce::MidiBuffer& midi);
    float yin (const float* x, float& confidence);

    double sr = 44100.0;
    std::vector<float> ring, frame, cmnd;
    int writePos = 0, sinceLast = 0;

    int activeNote = -1, candidate = -1, candCount = 0, missCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoteDetectorProcessor)
};
