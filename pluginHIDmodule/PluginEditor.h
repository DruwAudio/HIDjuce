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

    void startCalibration()
    {
        calibrationState = WaitingForTopLeft;
        topLeftCalibration = bs::TouchData();
        bottomRightCalibration = bs::TouchData();
        repaint();
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

        // Handle calibration mode
        if (calibrationState != NotCalibrating)
        {
            drawCalibrationOverlay(g, allTouches);
            return;
        }

        // Debug: Show touch count and coordinate info
        g.setColour(juce::Colours::yellow);
        g.drawText(juce::String::formatted("Active Touches: %d | Window: %dx%d",
                                          (int)allTouches.size(), getWidth(), getHeight()),
                  10, 35, 400, 20, juce::Justification::left);

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

            // Track actual coordinate ranges
            for (const auto& touch : allTouches)
            {
                if (touch.x < minX) minX = touch.x;
                if (touch.x > maxX) maxX = touch.x;
                if (touch.y < minY) minY = touch.y;
                if (touch.y > maxY) maxY = touch.y;
            }

            // Display actual coordinate ranges observed
            g.setColour(juce::Colours::orange);
            g.drawText(juce::String::formatted("Touch Range: X[%d-%d] Y[%d-%d]",
                                              minX, maxX, minY, maxY),
                      10, 55, 400, 20, juce::Justification::left);
            yOffset = 80;

            // Use calibration bounds
            auto bounds = processor.getCalibrationManager().getBounds();
            const float touchMinX = bounds.minX;
            const float touchMaxX = bounds.maxX;
            const float touchMinY = bounds.minY;
            const float touchMaxY = bounds.maxY;
            const float touchRangeX = touchMaxX - touchMinX;
            const float touchRangeY = touchMaxY - touchMinY;

            for (const auto& touch : allTouches)
            {
                // Scale touch coordinates to component bounds using actual ranges
                float normalizedX = (touch.x - touchMinX) / touchRangeX;
                float normalizedY = (touch.y - touchMinY) / touchRangeY;

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
    void drawCalibrationOverlay(juce::Graphics& g, const std::vector<bs::TouchData>& touches)
    {
        // Semi-transparent overlay
        g.setColour(juce::Colours::black.withAlpha(0.7f));
        g.fillRect(getLocalBounds());

        // Update pulse animation
        crosshairPulsePhase += 0.1f;
        if (crosshairPulsePhase > juce::MathConstants<float>::twoPi)
            crosshairPulsePhase -= juce::MathConstants<float>::twoPi;

        // Determine target position and instruction text
        juce::Point<float> targetPos;
        juce::String instructionText;

        if (calibrationState == WaitingForTopLeft)
        {
            targetPos = {50.0f, 50.0f};
            instructionText = "Touch the TOP-LEFT crosshair";
        }
        else if (calibrationState == WaitingForBottomRight)
        {
            targetPos = {getWidth() - 50.0f, getHeight() - 50.0f};
            instructionText = "Touch the BOTTOM-RIGHT crosshair";
        }
        else if (calibrationState == CalibrationComplete)
        {
            // Show success message
            g.setColour(juce::Colours::green);
            g.setFont(juce::Font(30.0f, juce::Font::bold));
            g.drawText("Calibration Saved!", getLocalBounds(), juce::Justification::centred);

            // Auto-dismiss after 1 second
            if (juce::Time::getCurrentTime().toMilliseconds() > calibrationCompleteTime + 1000)
            {
                calibrationState = NotCalibrating;
                repaint();
            }
            return;
        }

        // Draw pulsing crosshair
        drawCrosshairTarget(g, targetPos);

        // Draw instruction text
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(24.0f, juce::Font::bold));
        g.drawText(instructionText, 0, getHeight() - 100, getWidth(), 50, juce::Justification::centred);

        // Process touch input
        processCalibratorTouch(touches);
    }

    void drawCrosshairTarget(juce::Graphics& g, juce::Point<float> pos)
    {
        // Pulsing outer ring
        float pulseRadius = 40.0f + 10.0f * std::sin(crosshairPulsePhase);
        g.setColour(juce::Colours::cyan.withAlpha(0.5f));
        g.drawEllipse(pos.x - pulseRadius, pos.y - pulseRadius, pulseRadius * 2, pulseRadius * 2, 3.0f);

        // Static crosshair lines
        g.setColour(juce::Colours::white);
        g.drawLine(pos.x - 30, pos.y, pos.x + 30, pos.y, 3.0f);
        g.drawLine(pos.x, pos.y - 30, pos.x, pos.y + 30, 3.0f);

        // Center dot
        g.fillEllipse(pos.x - 5, pos.y - 5, 10, 10);
    }

    void processCalibratorTouch(const std::vector<bs::TouchData>& touches)
    {
        // Only process first active touch
        if (touches.empty() || !touches[0].isActive)
            return;

        const auto& touch = touches[0];

        if (calibrationState == WaitingForTopLeft)
        {
            if (!topLeftCalibration.isActive)
            {
                topLeftCalibration = touch;
                DBG("Top-left captured: x=" << touch.x << " y=" << touch.y);
                calibrationState = WaitingForBottomRight;
                repaint();
            }
        }
        else if (calibrationState == WaitingForBottomRight)
        {
            if (!bottomRightCalibration.isActive)
            {
                bottomRightCalibration = touch;
                DBG("Bottom-right captured: x=" << touch.x << " y=" << touch.y);

                // Validate and save calibration
                if (validateCalibration())
                {
                    saveCalibration();
                    calibrationState = CalibrationComplete;
                    calibrationCompleteTime = juce::Time::getCurrentTime().toMilliseconds();
                }
                else
                {
                    DBG("Invalid calibration points, restarting...");
                    startCalibration();
                }
                repaint();
            }
        }
    }

    bool validateCalibration()
    {
        // Ensure both points have coordinates
        if (topLeftCalibration.x == 0 && topLeftCalibration.y == 0)
            return false;
        if (bottomRightCalibration.x == 0 && bottomRightCalibration.y == 0)
            return false;

        // Ensure points are sufficiently separated (at least 10000 units)
        int deltaX = std::abs((int)bottomRightCalibration.x - (int)topLeftCalibration.x);
        int deltaY = std::abs((int)bottomRightCalibration.y - (int)topLeftCalibration.y);

        return deltaX > 10000 && deltaY > 10000;
    }

    void saveCalibration()
    {
        // Extrapolate calibration bounds from crosshair positions to screen edges
        // Crosshairs are at (50, 50) and (width-50, height-50)
        // We need to calculate what the raw coordinates would be at (0, 0) and (width, height)

        float screenX1 = 50.0f;
        float screenY1 = 50.0f;
        float screenX2 = getWidth() - 50.0f;
        float screenY2 = getHeight() - 50.0f;

        float rawX1 = topLeftCalibration.x;
        float rawY1 = topLeftCalibration.y;
        float rawX2 = bottomRightCalibration.x;
        float rawY2 = bottomRightCalibration.y;

        // Calculate pixels per raw unit
        float pixelsPerRawX = (screenX2 - screenX1) / (rawX2 - rawX1);
        float pixelsPerRawY = (screenY2 - screenY1) / (rawY2 - rawY1);

        // Extrapolate to find raw coordinates at screen edges (0, 0) and (width, height)
        float extrapolatedMinX = rawX1 - (screenX1 / pixelsPerRawX);
        float extrapolatedMinY = rawY1 - (screenY1 / pixelsPerRawY);
        float extrapolatedMaxX = rawX2 + ((getWidth() - screenX2) / pixelsPerRawX);
        float extrapolatedMaxY = rawY2 + ((getHeight() - screenY2) / pixelsPerRawY);

        // Create extrapolated touch points
        bs::TouchData extrapolatedTopLeft;
        extrapolatedTopLeft.x = static_cast<uint16_t>(extrapolatedMinX);
        extrapolatedTopLeft.y = static_cast<uint16_t>(extrapolatedMinY);
        extrapolatedTopLeft.isActive = true;

        bs::TouchData extrapolatedBottomRight;
        extrapolatedBottomRight.x = static_cast<uint16_t>(extrapolatedMaxX);
        extrapolatedBottomRight.y = static_cast<uint16_t>(extrapolatedMaxY);
        extrapolatedBottomRight.isActive = true;

        DBG("Calibration extrapolation:");
        DBG("  Measured: (" << rawX1 << "," << rawY1 << ") to (" << rawX2 << "," << rawY2 << ")");
        DBG("  Extrapolated: (" << extrapolatedMinX << "," << extrapolatedMinY << ") to (" << extrapolatedMaxX << "," << extrapolatedMaxY << ")");

        auto& calibrationMgr = processor.getCalibrationManager();
        calibrationMgr.setCalibrationPoints(extrapolatedTopLeft, extrapolatedBottomRight);
        DBG("Calibration saved to XML");
    }

    AudioPluginAudioProcessor& processor;

    // Track actual coordinate ranges for calibration
    uint16_t minX = 65535, maxX = 0;
    uint16_t minY = 65535, maxY = 0;

    // Calibration state
    enum CalibrationState { NotCalibrating, WaitingForTopLeft, WaitingForBottomRight, CalibrationComplete };
    CalibrationState calibrationState = NotCalibrating;
    bs::TouchData topLeftCalibration;
    bs::TouchData bottomRightCalibration;
    juce::int64 calibrationCompleteTime = 0;
    float crosshairPulsePhase = 0.0f;
};

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
    TouchVisualizerComponent touchVisualizer;

    // Fullscreen state
    bool isFullscreenFlag = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
