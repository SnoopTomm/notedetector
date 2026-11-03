#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

NoteDetectorProcessor::NoteDetectorProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

bool NoteDetectorProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void NoteDetectorProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    ring.assign (frameSize, 0.0f);
    frame.assign (frameSize, 0.0f);
    cmnd.assign (frameSize / 2, 1.0f);
    writePos = sinceLast = 0;
    activeNote = candidate = -1;
    candCount = missCount = 0;
    detectedNote = -1;
}

void NoteDetectorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();                       // audio passes through untouched

    const int n  = buffer.getNumSamples();
    const int ch = buffer.getNumChannels();

    for (int i = 0; i < n; ++i)
    {
        float s = 0.0f;
        for (int c = 0; c < ch; ++c) s += buffer.getSample (c, i);
        ring[(size_t) writePos] = s / (float) juce::jmax (1, ch);
        writePos = (writePos + 1) % frameSize;

        if (++sinceLast >= hopSize)
        {
            sinceLast = 0;
            analyse (i, midi);
        }
    }
}

// YIN pitch detection (de Cheveigne & Kawahara, 2002)
float NoteDetectorProcessor::yin (const float* x, float& confidence)
{
    const int W = frameSize / 2;
    cmnd[0] = 1.0f;
    float running = 0.0f;

    for (int tau = 1; tau < W; ++tau)
    {
        float sum = 0.0f;
        for (int j = 0; j < W; ++j)
        {
            const float d = x[j] - x[j + tau];
            sum += d * d;
        }
        running += sum;
        cmnd[(size_t) tau] = running > 0.0f ? sum * (float) tau / running : 1.0f;
    }

    int tauEst = -1;
    for (int tau = 2; tau < W - 1; ++tau)
    {
        if (cmnd[(size_t) tau] < 0.15f)
        {
            while (tau + 1 < W - 1 && cmnd[(size_t) tau + 1] < cmnd[(size_t) tau]) ++tau;
            tauEst = tau;
            break;
        }
    }
    if (tauEst < 0) return -1.0f;

    confidence = 1.0f - cmnd[(size_t) tauEst];

    const float s0 = cmnd[(size_t) tauEst - 1], s1 = cmnd[(size_t) tauEst], s2 = cmnd[(size_t) tauEst + 1];
    const float denom = 2.0f * (2.0f * s1 - s2 - s0);
    const float shift = denom != 0.0f ? (s2 - s0) / denom : 0.0f;

    return (float) sr / ((float) tauEst + shift);
}

void NoteDetectorProcessor::analyse (int sampleIndex, juce::MidiBuffer& midi)
{
    float energy = 0.0f;
    for (int k = 0; k < frameSize; ++k)
    {
        frame[(size_t) k] = ring[(size_t) ((writePos + k) % frameSize)];   // oldest -> newest
        energy += frame[(size_t) k] * frame[(size_t) k];
    }
    const float rms = std::sqrt (energy / (float) frameSize);

    int note = -1;
    float freq = -1.0f, conf = 0.0f;

    if (rms > 0.003f)                                    // ~ -50 dB gate
    {
        freq = yin (frame.data(), conf);
        if (freq > 0.0f)
        {
            note = (int) std::lround (69.0 + 12.0 * std::log2 (freq / 440.0));
            if (note < 21 || note > 108) note = -1;
        }
    }

    auto noteOff = [&]
    {
        if (activeNote >= 0)
            midi.addEvent (juce::MidiMessage::noteOff (1, activeNote), sampleIndex);
        activeNote = -1;
    };

    if (note < 0)
    {
        candidate = -1; candCount = 0;
        if (++missCount >= 4) noteOff();                 // short dropouts don't cut the note
    }
    else
    {
        missCount = 0;
        if (note != activeNote)
        {
            if (note == candidate) ++candCount; else { candidate = note; candCount = 1; }

            if (candCount >= 2)                           // must be stable for 2 frames
            {
                noteOff();
                const auto vel = (juce::uint8) juce::jlimit (30, 127, (int) (rms * 600.0f));
                midi.addEvent (juce::MidiMessage::noteOn (1, note, vel), sampleIndex);
                activeNote = note;
            }
        }
    }

    detectedNote = activeNote;
    if (activeNote >= 0) { detectedFreq = freq; detectedConf = conf; }
}

juce::AudioProcessorEditor* NoteDetectorProcessor::createEditor()
{
    return new NoteDetectorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NoteDetectorProcessor();
}
