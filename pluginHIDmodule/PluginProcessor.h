#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <bs_hid/bs_hid.h>

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor,
                                       public bs::HIDDeviceManager::Listener
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
    std::vector<bs::HIDDeviceInfo> getAvailableHIDDevices();
    void connectToDevice(const bs::HIDDeviceInfo& device);
    void disconnectFromDevice();
    bool isDeviceConnected() const;
    const bs::HIDDeviceInfo& getConnectedDeviceInfo() const;

    // HID Device Manager access
    bs::HIDDeviceManager& getHIDDeviceManager() { return hidDeviceManager; }

    // Listener callback from HIDDeviceManager
    void touchDetected(const bs::TouchData& touchData) override;

private:
    //==============================================================================
    bs::HIDDeviceManager hidDeviceManager;

    // Touch state for audio processing
    bool previousTouchState = false;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
