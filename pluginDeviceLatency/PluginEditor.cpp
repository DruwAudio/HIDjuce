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
    optimizeButton.setButtonText("🚀 Optimize for Low Latency");
    optimizeButton.addListener(this);
    optimizeButton.setEnabled(false); // Disabled until device connected
    addAndMakeVisible(optimizeButton);

    restoreButton.setButtonText("🔄 Restore Original Settings");
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

    populateDeviceComboBox();

    setSize (450, 220);
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
            optimizationStatus.setText("⚠️ Optimization completed with warnings", juce::dontSendNotification);
            optimizationStatus.setColour(juce::Label::textColourId, juce::Colours::orange);
            restoreButton.setEnabled(true);
        }
    }
    else if (button == &restoreButton)
    {
        processorRef.restoreSettings();
        optimizationStatus.setText("🔄 Original settings restored", juce::dontSendNotification);
        optimizationStatus.setColour(juce::Label::textColourId, juce::Colours::blue);

        restoreButton.setEnabled(false);
        optimizeButton.setEnabled(true);
    }
    else if (button == &twoFingerToggle)
    {
        bool twoFingerMode = twoFingerToggle.getToggleState();
        processorRef.setMaxTouchPoints(twoFingerMode ? 2 : 10);

        optimizationStatus.setText(twoFingerMode ?
                                  "🎯 2-finger mode: Faster parsing" :
                                  "👐 10-finger mode: Full multi-touch",
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
