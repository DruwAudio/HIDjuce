#pragma once

#include "PluginProcessor.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                               public juce::KeyListener
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    // KeyListener to handle Escape key to exit fullscreen
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

    // Toggle fullscreen mode
    void toggleFullscreen();

    // Check if currently fullscreen
    bool isFullscreen() const { return isFullscreenFlag; }

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;

    // Touch visualizer component
    bs::TouchVisualizerComponent touchVisualizer;

    // Fullscreen state
    bool isFullscreenFlag = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
