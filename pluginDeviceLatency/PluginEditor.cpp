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

    populateDeviceComboBox();

    setSize (400, 120);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    deviceComboBox.removeListener(this);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    deviceLabel.setBounds(area.removeFromTop(20));
    deviceComboBox.setBounds(area.removeFromTop(25));
    area.removeFromTop(5);
    statusLabel.setBounds(area.removeFromTop(20));
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
        }
        else if (selectedId > 1)
        {
            auto devices = processorRef.getAvailableHIDDevices();
            int deviceIndex = selectedId - 2;
            if (deviceIndex >= 0 && deviceIndex < devices.size())
            {
                processorRef.connectToDevice(devices[deviceIndex]);
                statusLabel.setText("Connected to: " + devices[deviceIndex].product, juce::dontSendNotification);
            }
        }
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
