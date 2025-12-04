#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "../hidapi/hidapi/hidapi.h"

//==============================================================================
struct HIDDeviceInfo {
    juce::String path;
    unsigned short vendorId;
    unsigned short productId;
    juce::String manufacturer;
    juce::String product;
    juce::String serialNumber;
};

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
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
    std::vector<HIDDeviceInfo> getAvailableHIDDevices();
    void connectToDevice(const HIDDeviceInfo& device);
    void disconnectFromDevice();
    bool isDeviceConnected() const { return connectedDevice != nullptr; }
    const HIDDeviceInfo& getConnectedDeviceInfo() const { return connectedDeviceInfo; }


private:
    //==============================================================================
    // HID functionality
    void enumerateHIDDevices();
    void readHIDEvents();
    void parseInputReport(unsigned char* data, int length);
    void parseELOTouchData(unsigned char* data, int length, unsigned char reportId);

    std::vector<HIDDeviceInfo> hidDevices;
    hid_device* connectedDevice = nullptr;
    HIDDeviceInfo connectedDeviceInfo;

    // Touch data for audio processing
    std::atomic<bool> touchActive{false};
    std::atomic<int> touchX{0};
    std::atomic<int> touchY{0};

    // Touch state tracking for click generation
    bool previousTouchState = false;
    juce::int64 lastTouchTime = 0;
    const juce::int64 touchTimeoutMs = 50; // Reset touch state if no events for 50ms
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
