#pragma once

// CMake builds don't use an AppConfig.h, so it's safe to include juce module headers
// directly. If you need to remain compatible with Projucer-generated builds, and
// have called `juce_generate_juce_header(<thisTarget>)` in your CMakeLists.txt,
// you could `#include <JuceHeader.h>` here instead, to make all your module headers visible.
#include <juce_gui_extra/juce_gui_extra.h>

#include "hidapi/hidapi/hidapi.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
struct HIDDeviceInfo {
    juce::String path;
    unsigned short vendorId;
    unsigned short productId;
    juce::String manufacturer;
    juce::String product;
    juce::String serialNumber;
};

class MainComponent final : public juce::Component, public juce::Button::Listener, public juce::Timer
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void buttonClicked (juce::Button* button) override;
    void timerCallback() override;

private:
    //==============================================================================
    void enumerateHIDDevices();
    void createDeviceButtons();
    void connectToDevice(const HIDDeviceInfo& device);
    void disconnectFromDevice();
    void readHIDEvents();
    void parseInputReport(unsigned char* data, int length);

    std::vector<HIDDeviceInfo> hidDevices;
    juce::OwnedArray<juce::TextButton> deviceButtons;

    hid_device* connectedDevice;
    HIDDeviceInfo connectedDeviceInfo;
    bool isDeviceConnected;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
