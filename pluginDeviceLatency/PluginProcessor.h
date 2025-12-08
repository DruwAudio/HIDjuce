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
class AudioPluginAudioProcessor final : public juce::AudioProcessor, public juce::Timer
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

    // Timer callback for reading HID events
    void timerCallback() override;

private:
    //==============================================================================
    // HID functionality
    void enumerateHIDDevices();
    void readHIDEvents();
    void parseInputReport(unsigned char* data, int length);
    void parseELOTouchData(unsigned char* data, int length, unsigned char reportId);
    void parseStandardTouchData(unsigned char* data, int length, unsigned char reportId);

    // Optimized touch state packing/unpacking
    inline void setTouchState(uint16_t x, uint16_t y, bool active) {
        uint64_t packed = ((uint64_t)x) | (((uint64_t)y) << 16) |
                         (active ? (1ULL << 32) : 0) |
                         (((uint64_t)(juce::Time::currentTimeMillis() & 0x7FFFFFFF)) << 33);
        touchState.store(packed, std::memory_order_release);
    }

    inline bool getTouchState(uint16_t& x, uint16_t& y) {
        uint64_t packed = touchState.load(std::memory_order_acquire);
        x = (uint16_t)(packed & 0xFFFF);
        y = (uint16_t)((packed >> 16) & 0xFFFF);
        return (packed & (1ULL << 32)) != 0;
    }

    std::vector<HIDDeviceInfo> hidDevices;
    hid_device* connectedDevice = nullptr;
    HIDDeviceInfo connectedDeviceInfo;

    // Optimized touch data communication using single atomic
    std::atomic<uint64_t> touchState{0}; // Packed: x(16) + y(16) + active(1) + timestamp(31)

    // Touch state tracking for click generation
    bool previousTouchState = false;
    juce::int64 lastTouchTime = 0;
    const juce::int64 touchTimeoutMs = 50; // Reset touch state if no events for 50ms

    // Cached coordinate validation ranges
    static constexpr uint16_t minValidCoord = 100;
    static constexpr uint16_t maxValidCoord = 30000;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
