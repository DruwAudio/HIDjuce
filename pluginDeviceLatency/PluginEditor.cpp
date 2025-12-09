#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    deviceLabel.setText("HID Device:", juce::dontSendNotification);
    addAndMakeVisible(deviceLabel);

    deviceComboBox.addListener(this);
    addAndMakeVisible(deviceComboBox);

    statusLabel.setText("Disconnected", juce::dontSendNotification);
    addAndMakeVisible(statusLabel);

    // Setup latency optimization controls
    optimizeButton.setButtonText("Optimize for Low Latency");
    optimizeButton.addListener(this);
    optimizeButton.setEnabled(false); // Disabled until device connected
    addAndMakeVisible(optimizeButton);

    restoreButton.setButtonText("Restore Original Settings");
    restoreButton.addListener(this);
    restoreButton.setEnabled(false);
    addAndMakeVisible(restoreButton);

    twoFingerToggle.setButtonText("2-Finger Mode (Faster)");
    twoFingerToggle.addListener(this);
    twoFingerToggle.setToggleState(true, juce::dontSendNotification); // Default to 2-finger
    addAndMakeVisible(twoFingerToggle);

    optimizationStatus.setText("Ready for optimization", juce::dontSendNotification);
    optimizationStatus.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(optimizationStatus);

    // Setup diagnostic display
    diagnosticsHeader.setText("HID Report Diagnostics:", juce::dontSendNotification);
    diagnosticsHeader.setFont(juce::Font(14.0f, juce::Font::bold));
    diagnosticsHeader.setColour(juce::Label::textColourId, juce::Colours::lightblue);
    addAndMakeVisible(diagnosticsHeader);

    reportRateLabel.setText("Report Rate: --", juce::dontSendNotification);
    reportRateLabel.setFont(juce::Font(12.0f, juce::Font::plain));
    addAndMakeVisible(reportRateLabel);

    avgIntervalLabel.setText("Avg Interval: --", juce::dontSendNotification);
    avgIntervalLabel.setFont(juce::Font(12.0f, juce::Font::plain));
    addAndMakeVisible(avgIntervalLabel);

    minMaxIntervalLabel.setText("Min/Max: --", juce::dontSendNotification);
    minMaxIntervalLabel.setFont(juce::Font(12.0f, juce::Font::plain));
    addAndMakeVisible(minMaxIntervalLabel);

    audioLatencyLabel.setText("Audio Latency: --", juce::dontSendNotification);
    audioLatencyLabel.setFont(juce::Font(12.0f, juce::Font::plain));
    audioLatencyLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);
    addAndMakeVisible(audioLatencyLabel);

    populateDeviceComboBox();

    // Start timer to update diagnostic display (100ms refresh rate)
    startTimer(100);

    setSize (450, 340);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    deviceComboBox.removeListener(this);
    optimizeButton.removeListener(this);
    restoreButton.removeListener(this);
    twoFingerToggle.removeListener(this);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    // Device selection section
    deviceLabel.setBounds(area.removeFromTop(20));
    deviceComboBox.setBounds(area.removeFromTop(25));
    area.removeFromTop(5);
    statusLabel.setBounds(area.removeFromTop(20));

    area.removeFromTop(10); // Separator

    // Optimization controls section
    optimizeButton.setBounds(area.removeFromTop(30));
    area.removeFromTop(5);
    restoreButton.setBounds(area.removeFromTop(30));
    area.removeFromTop(5);
    twoFingerToggle.setBounds(area.removeFromTop(25));
    area.removeFromTop(5);
    optimizationStatus.setBounds(area.removeFromTop(20));

    area.removeFromTop(10); // Separator

    // Diagnostic section
    diagnosticsHeader.setBounds(area.removeFromTop(20));
    reportRateLabel.setBounds(area.removeFromTop(18));
    avgIntervalLabel.setBounds(area.removeFromTop(18));
    minMaxIntervalLabel.setBounds(area.removeFromTop(18));
    audioLatencyLabel.setBounds(area.removeFromTop(18));
}

void AudioPluginAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &deviceComboBox)
    {
        int selectedId = deviceComboBox.getSelectedId();
        if (selectedId == 1)
        {
            processorRef.disconnectFromDevice();
            statusLabel.setText("Disconnected", juce::dontSendNotification);

            // Disable optimization controls when disconnected
            optimizeButton.setEnabled(false);
            restoreButton.setEnabled(false);
            optimizationStatus.setText("Connect a device to optimize", juce::dontSendNotification);
            optimizationStatus.setColour(juce::Label::textColourId, juce::Colours::grey);
        }
        else if (selectedId > 1)
        {
            auto devices = processorRef.getAvailableHIDDevices();
            int deviceIndex = selectedId - 2;
            if (deviceIndex >= 0 && deviceIndex < devices.size())
            {
                processorRef.connectToDevice(devices[deviceIndex]);
                statusLabel.setText("Connected to: " + devices[deviceIndex].product, juce::dontSendNotification);

                // Enable optimization controls when connected
                optimizeButton.setEnabled(true);
                optimizationStatus.setText("Ready for optimization", juce::dontSendNotification);
                optimizationStatus.setColour(juce::Label::textColourId, juce::Colours::orange);
            }
        }
    }
}

void AudioPluginAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &optimizeButton)
    {
        optimizationStatus.setText("Optimizing touchscreen settings...", juce::dontSendNotification);
        optimizationStatus.setColour(juce::Label::textColourId, juce::Colours::yellow);

        bool success = processorRef.optimizeForLowLatency();

        if (success)
        {
            optimizationStatus.setText("✅ Optimization applied successfully!", juce::dontSendNotification);
            optimizationStatus.setColour(juce::Label::textColourId, juce::Colours::green);
            restoreButton.setEnabled(true);
            optimizeButton.setEnabled(false);
        }
        else
        {
            optimizationStatus.setText("Optimization completed with warnings", juce::dontSendNotification);
            optimizationStatus.setColour(juce::Label::textColourId, juce::Colours::orange);
            restoreButton.setEnabled(true);
        }
    }
    else if (button == &restoreButton)
    {
        processorRef.restoreSettings();
        optimizationStatus.setText("Original settings restored", juce::dontSendNotification);
        optimizationStatus.setColour(juce::Label::textColourId, juce::Colours::blue);

        restoreButton.setEnabled(false);
        optimizeButton.setEnabled(true);
    }
    else if (button == &twoFingerToggle)
    {
        bool twoFingerMode = twoFingerToggle.getToggleState();
        processorRef.setMaxTouchPoints(twoFingerMode ? 2 : 10);

        optimizationStatus.setText(twoFingerMode ?
                                  "2-finger mode: Faster parsing" :
                                  "10-finger mode: Full multi-touch",
                                  juce::dontSendNotification);
        optimizationStatus.setColour(juce::Label::textColourId,
                                    twoFingerMode ? juce::Colours::green : juce::Colours::grey);
    }
}

void AudioPluginAudioProcessorEditor::populateDeviceComboBox()
{
    deviceComboBox.clear();
    deviceComboBox.addItem("Disconnect", 1);

    auto devices = processorRef.getAvailableHIDDevices();
    for (size_t i = 0; i < devices.size(); ++i)
    {
        juce::String itemText = devices[i].manufacturer + " - " + devices[i].product;
        deviceComboBox.addItem(itemText, static_cast<int>(i + 2));
    }

    deviceComboBox.setSelectedId(1);
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    updateDiagnosticDisplay();
}

void AudioPluginAudioProcessorEditor::updateDiagnosticDisplay()
{
    // Get audio setup info from processor
    auto audioInfo = processorRef.getAudioSetupInfo();

    if (audioInfo.sampleRate > 0)
    {
        double bufferMs = (audioInfo.bufferSize * 1000.0) / audioInfo.sampleRate;
        double totalLatencyMs = (audioInfo.totalLatencySamples * 1000.0) / audioInfo.sampleRate;

        audioLatencyLabel.setText(
            juce::String::formatted("Audio: Buffer=%d smp (%.2f ms), Total Latency=%.2f ms @ %.0f Hz",
                                   audioInfo.bufferSize,
                                   bufferMs,
                                   totalLatencyMs,
                                   audioInfo.sampleRate),
            juce::dontSendNotification);

        // Calculate expected minimum end-to-end latency
        auto touchStats = processorRef.getLatencyStats();
        if (touchStats.sampleCount > 0)
        {
            double expectedMinLatency = touchStats.avgIntervalMs + bufferMs + 2.0; // +2ms for USB/processing overhead
            double unexplainedLatency = 37.0 - expectedMinLatency; // Your measured 37ms

            if (unexplainedLatency > 5.0)
            {
                audioLatencyLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
            }
            else
            {
                audioLatencyLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
            }
        }
    }
    else
    {
        audioLatencyLabel.setText("Audio: Not initialized", juce::dontSendNotification);
    }

    if (!processorRef.isDeviceConnected())
    {
        reportRateLabel.setText("Report Rate: -- (no device)", juce::dontSendNotification);
        avgIntervalLabel.setText("Avg Interval: --", juce::dontSendNotification);
        minMaxIntervalLabel.setText("Min/Max: --", juce::dontSendNotification);
        return;
    }

    auto stats = processorRef.getLatencyStats();

    if (stats.sampleCount > 0)
    {
        // Display report rate in Hz
        reportRateLabel.setText(
            juce::String::formatted("Report Rate: %.1f Hz (%.2f ms)",
                                   stats.currentReportRateHz,
                                   stats.avgIntervalMs),
            juce::dontSendNotification);

        // Display average interval
        avgIntervalLabel.setText(
            juce::String::formatted("Avg Interval: %.2f ms (%d samples)",
                                   stats.avgIntervalMs,
                                   stats.sampleCount),
            juce::dontSendNotification);

        // Display min/max intervals
        minMaxIntervalLabel.setText(
            juce::String::formatted("Min/Max: %.2f / %.2f ms",
                                   stats.minIntervalMs,
                                   stats.maxIntervalMs),
            juce::dontSendNotification);

        // Color code based on report rate quality
        juce::Colour rateColor;
        if (stats.currentReportRateHz >= 200.0) {
            rateColor = juce::Colours::green;  // Excellent
        } else if (stats.currentReportRateHz >= 120.0) {
            rateColor = juce::Colours::lightgreen;  // Good
        } else if (stats.currentReportRateHz >= 60.0) {
            rateColor = juce::Colours::orange;  // Moderate
        } else {
            rateColor = juce::Colours::red;  // Poor
        }
        reportRateLabel.setColour(juce::Label::textColourId, rateColor);
    }
    else
    {
        reportRateLabel.setText("Report Rate: Waiting for touch events...", juce::dontSendNotification);
        reportRateLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
        avgIntervalLabel.setText("Avg Interval: --", juce::dontSendNotification);
        minMaxIntervalLabel.setText("Min/Max: --", juce::dontSendNotification);
    }
}
