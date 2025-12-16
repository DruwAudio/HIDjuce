#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <bs_hid/bs_hid.h>

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor,
                                       public bs_hid::HIDDeviceManager::Listener
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // HID Device Management
    std::vector<bs_hid::HIDDeviceInfo> getAvailableHIDDevices();
    void connectToDevice(const bs_hid::HIDDeviceInfo& device);
    void disconnectFromDevice();
    bool isDeviceConnected() const;
    const bs_hid::HIDDeviceInfo& getConnectedDeviceInfo() const;

    // HID Device Manager access
    bs_hid::HIDDeviceManager& getHIDDeviceManager() { return hidDeviceManager; }

    // Touch Calibration Manager access
    bs_hid::TouchCalibrationManager& getCalibrationManager() { return calibrationManager; }

    // Listener callback from HIDDeviceManager
    void touchDetected(const bs_hid::TouchData& touchData) override;

private:
    // Attempt to connect to known touch devices
    void attemptTouchDeviceConnection();

    //==============================================================================
    bs_hid::HIDDeviceManager hidDeviceManager;
    bs_hid::TouchCalibrationManager calibrationManager;

    // Touch state for audio processing
    bool previousTouchState = false;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
