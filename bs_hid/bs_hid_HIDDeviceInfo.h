/*
  ==============================================================================

   HID Device Information Structure

  ==============================================================================
*/

#pragma once

namespace bs
{

/** Structure to hold information about a HID device */
struct HIDDeviceInfo
{
    juce::String path;
    unsigned short vendorId;
    unsigned short productId;
    juce::String manufacturer;
    juce::String product;
    juce::String serialNumber;

    HIDDeviceInfo() = default;

    HIDDeviceInfo(const juce::String& p, unsigned short vid, unsigned short pid,
                  const juce::String& mfg, const juce::String& prod, const juce::String& serial)
        : path(p), vendorId(vid), productId(pid),
          manufacturer(mfg), product(prod), serialNumber(serial)
    {}
};

} // namespace bs
