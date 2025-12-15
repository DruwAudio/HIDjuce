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

        // Get all current touches
        auto allTouches = processor.getHIDDeviceManager().getAllTouches();

        // Debug: Show touch count
        g.setColour(juce::Colours::yellow);
        g.drawText(juce::String::formatted("Active Touches: %d", (int)allTouches.size()),
                  10, 35, 200, 20, juce::Justification::left);

        // Draw all touch points
        if (!allTouches.empty())
        {
            // Define colors for different touch IDs
            juce::Colour touchColors[] = {
                juce::Colours::cyan,
                juce::Colours::magenta,
                juce::Colours::yellow,
                juce::Colours::lime,
                juce::Colours::orange,
                juce::Colours::pink,
                juce::Colours::lightblue,
                juce::Colours::lightgreen,
                juce::Colours::violet,
                juce::Colours::gold
            };

            int yOffset = 60;
            for (const auto& touch : allTouches)
            {
                // Scale touch coordinates to component bounds
                // Assuming touch coordinates are 0-32767 range (typical for HID digitizers)
                float normalizedX = touch.x / 32767.0f;
                float normalizedY = touch.y / 32767.0f;

                float screenX = normalizedX * getWidth();
                float screenY = normalizedY * getHeight();

                // Select color based on contact ID
                juce::Colour touchColor = touchColors[touch.contactId % 10];

                // Draw touch point as a circle
                g.setColour(touchColor.withAlpha(0.8f));
                float radius = 25.0f;
                g.fillEllipse(screenX - radius, screenY - radius, radius * 2, radius * 2);

                // Draw border
                g.setColour(touchColor);
                g.drawEllipse(screenX - radius, screenY - radius, radius * 2, radius * 2, 3.0f);

                // Draw crosshair
                g.setColour(juce::Colours::white);
                g.drawLine(screenX - 30, screenY, screenX + 30, screenY, 2.0f);
                g.drawLine(screenX, screenY - 30, screenX, screenY + 30, 2.0f);

                // Draw contact ID on the circle
                g.setColour(juce::Colours::black);
                g.setFont(juce::Font(20.0f, juce::Font::bold));
                g.drawText(juce::String(touch.contactId),
                          screenX - 15, screenY - 15, 30, 30,
                          juce::Justification::centred);

                // Draw coordinates text
                g.setColour(touchColor);
                g.setFont(12.0f);
                g.drawText(juce::String::formatted("ID %d: (%d, %d) -> (%.0f, %.0f)",
                                                  touch.contactId, touch.x, touch.y, screenX, screenY),
                          10, yOffset, 600, 18, juce::Justification::left);
                yOffset += 18;
            }
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
