#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), touchVisualizer(p)
{
    // Add and make the touch visualizer visible
    addAndMakeVisible(touchVisualizer);

    // Make the editor resizable
    setResizable(true, true);
    setResizeLimits(400, 300, 4096, 4096);

    // Add key listener to handle keyboard shortcuts
    addKeyListener(this);
    setWantsKeyboardFocus(true);

    setSize (800, 600);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    removeKeyListener(this);

    // Exit fullscreen if active
    if (isFullscreenFlag)
    {
        juce::Desktop::getInstance().setKioskModeComponent(nullptr);
    }
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void AudioPluginAudioProcessorEditor::resized()
{
    // Make the touch visualizer fill the entire editor
    touchVisualizer.setBounds(getLocalBounds());
}

//==============================================================================
bool AudioPluginAudioProcessorEditor::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    // Press 'F' to toggle fullscreen/kiosk mode
    if (key == juce::KeyPress('f', juce::ModifierKeys::noModifiers, 0))
    {
        toggleFullscreen();
        return true;
    }
    // Press Escape to exit fullscreen (if currently in fullscreen)
    else if (key == juce::KeyPress::escapeKey && isFullscreenFlag)
    {
        toggleFullscreen();
        return true;
    }
    // Press 'C' to enter calibration mode
    else if (key == juce::KeyPress('c', juce::ModifierKeys::noModifiers, 0))
    {
        touchVisualizer.startCalibration();
        return true;
    }

    return false;
}

void AudioPluginAudioProcessorEditor::toggleFullscreen()
{
    if (isFullscreenFlag)
    {
        // Exit fullscreen/kiosk mode
        juce::Desktop::getInstance().setKioskModeComponent(nullptr);
        isFullscreenFlag = false;
    }
    else
    {
        // Enter fullscreen/kiosk mode
        // This will make the window cover the entire screen and capture all input
        juce::Desktop::getInstance().setKioskModeComponent(getTopLevelComponent(), true);
        isFullscreenFlag = true;
    }
}
