/*
  ==============================================================================

   This file is part of the JUCE examples.
   Copyright (c) 2020 - Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             MidiDemo
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Handles incoming and outcoming midi messages.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2019, linux_make, androidstudio, xcode_iphone

 moduleFlags:      JUCE_STRICT_REFCOUNTEDPOINTER=1

 type:             Component
 mainClass:        MidiDemo

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once

//==============================================================================
class MidiDemo  : public Component,
                  private MidiKeyboardState::Listener,
                  private AsyncUpdater
{
public:
    //==============================================================================
    MidiDemo()
        : midiKeyboard (keyboardState, MidiKeyboardComponent::horizontalKeyboard)
    {
        
        // start the virtual port

        virtualPort = MidiOutput::createNewDevice("Pitch Innovations MIDI Keyboard");

        if (virtualPort)
        {
            virtualPort->startBackgroundThread();
        }
        
        midiKeyboard.setName ("MIDI Keyboard");
        addAndMakeVisible (midiKeyboard);

        midiMonitor.setMultiLine (true);
        midiMonitor.setReturnKeyStartsNewLine (false);
        midiMonitor.setReadOnly (true);
        midiMonitor.setScrollbarsShown (true);
        midiMonitor.setCaretVisible (false);
        midiMonitor.setPopupMenuEnabled (false);
        midiMonitor.setText ({});
        addAndMakeVisible (midiMonitor);

        
        keyboardState.addListener (this);

        setSize (732, 520);

    }

    ~MidiDemo() override
    {
        keyboardState.removeListener (this);
    }

    //==============================================================================

    void handleNoteOn (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
    {
        MidiMessage m (MidiMessage::noteOn (midiChannel, midiNoteNumber, velocity));
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
    }

    void handleNoteOff (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
    {
        MidiMessage m (MidiMessage::noteOff (midiChannel, midiNoteNumber, velocity));
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
    }

    void paint (Graphics&) override {}

    void resized() override
    {
        auto margin = 10;
        midiKeyboard.setBounds (margin, (24 + margin), getWidth() - (2 * margin), 64);

        auto y = ((2 * 24) + (3 * margin) + 64);
        midiMonitor.setBounds (margin, y,
                               getWidth() - (2 * margin), getHeight() - y - margin);
    }

private:
    //==============================================================================
    
    void addToMonitor (MidiInput* /*source*/, const MidiMessage& message)
    {
        // This is called on the MIDI thread
        const ScopedLock sl (midiMonitorLock);
        incomingMessages.add (message);
        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override
    {
        // This is called on the message loop
        Array<MidiMessage> messages;

        {
            const ScopedLock sl (midiMonitorLock);
            messages.swapWith (incomingMessages);
        }

        String messageText;

        for (auto& m : messages)
            messageText << m.getDescription() << "\n";

        midiMonitor.insertTextAtCaret (messageText);
    }

    void sendToOutputs (const MidiMessage& msg)
    {
        if (virtualPort != nullptr)
        {
            virtualPort->sendMessageNow (msg);
            addToMonitor(nullptr, msg);
        }
    }

    //==============================================================================
    MidiKeyboardState keyboardState;
    MidiKeyboardComponent midiKeyboard;
    TextEditor midiMonitor  { "MIDI Monitor" };

    CriticalSection midiMonitorLock;
    Array<MidiMessage> incomingMessages;
    
    std::unique_ptr<MidiOutput> virtualPort;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiDemo)
};
