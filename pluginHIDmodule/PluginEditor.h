#pragma once

#include "PluginProcessor.h"

//==============================================================================
/** Component that visualizes touch events */
class TouchVisualizerComponent : public juce::Component,
                                 public juce::Timer
{
public:
    TouchVisualizerComponent(AudioPluginAudioProcessor& p)
        : processor(p)
    {
        startTimerHz(60); // 60 FPS refresh
    }

    void paint(juce::Graphics& g) override
    {
        // Draw background
        g.fillAll(juce::Colours::black);

        // Draw border
        g.setColour(juce::Colours::grey);
        g.drawRect(getLocalBounds(), 2);

        // Draw connection status
        bool isConnected = processor.getHIDDeviceManager().isDeviceConnected();
        g.setColour(isConnected ? juce::Colours::green : juce::Colours::red);
        g.fillEllipse(10, 10, 15, 15);
        g.setColour(juce::Colours::white);
        g.drawText(isConnected ? "Connected" : "Disconnected", 30, 10, 150, 15, juce::Justification::left);

        // Get current touch data
        auto touch = processor.getHIDDeviceManager().getLatestTouchData();

        // Debug: Always show raw touch data
        g.setColour(juce::Colours::yellow);
        g.drawText(juce::String::formatted("Raw: x=%d, y=%d, active=%s, valid=%s",
                                          touch.x, touch.y,
                                          touch.isActive ? "true" : "false",
                                          touch.isValid() ? "true" : "false"),
                  10, 35, 400, 20, juce::Justification::left);

        // Draw touch visualization even if not "valid" - just to see something
        if (touch.isActive)
        {
            // Scale touch coordinates to component bounds
            // Assuming touch coordinates are 0-32767 range (typical for HID digitizers)
            float normalizedX = touch.x / 32767.0f;
            float normalizedY = touch.y / 32767.0f;

            float screenX = normalizedX * getWidth();
            float screenY = normalizedY * getHeight();

            // Draw touch point as a circle
            g.setColour(touch.isValid() ? juce::Colours::cyan : juce::Colours::orange);
            float radius = 20.0f;
            g.fillEllipse(screenX - radius, screenY - radius, radius * 2, radius * 2);

            // Draw crosshair
            g.setColour(juce::Colours::white);
            g.drawLine(screenX - 30, screenY, screenX + 30, screenY, 2.0f);
            g.drawLine(screenX, screenY - 30, screenX, screenY + 30, 2.0f);

            // Draw coordinates text
            g.setColour(juce::Colours::lightgreen);
            g.drawText(juce::String::formatted("Touch: (%d, %d) -> Screen: (%.1f, %.1f)",
                                              touch.x, touch.y, screenX, screenY),
                      10, 60, 500, 20, juce::Justification::left);
        }
        else
        {
            // No touch detected
            g.setColour(juce::Colours::grey);
            g.drawText("Touch the screen", getLocalBounds(),
                      juce::Justification::centred);
        }
    }

    void timerCallback() override
    {
        repaint(); // Refresh display at 60 FPS
    }

private:
    AudioPluginAudioProcessor& processor;
};

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;

    // Touch visualizer component
    TouchVisualizerComponent touchVisualizer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
