#pragma once
#include "PluginProcessor.h"

class NoteDetectorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NoteDetectorEditor (NoteDetectorProcessor& p)
        : juce::AudioProcessorEditor (&p), proc (p)
    {
        setSize (320, 180);
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff1b1b22));
        auto area = getLocalBounds();

        const int note = proc.detectedNote.load();
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (72.0f, juce::Font::bold));
        // octave 5 = FL Studio's naming (middle C = C5)
        g.drawText (note >= 0 ? juce::MidiMessage::getMidiNoteName (note, true, true, 5) : "--",
                    area.removeFromTop (120), juce::Justification::centred);

        g.setColour (juce::Colours::grey);
        g.setFont (juce::FontOptions (16.0f));
        auto f = proc.detectedFreq.load();
        g.drawText (note >= 0 ? juce::String (f, 1) + " Hz   |   MIDI " + juce::String (note)
                              : "Listening...",
                    area, juce::Justification::centredTop);
    }

private:
    void timerCallback() override { repaint(); }
    NoteDetectorProcessor& proc;
};
