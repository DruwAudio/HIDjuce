#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    setSize (600, 400);

    // Enumerate HID devices when the component is created
    enumerateHIDDevices();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setFont (juce::FontOptions (16.0f));
    g.setColour (juce::Colours::white);
    g.drawText ("Hello World!", getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}

void MainComponent::enumerateHIDDevices()
{
    printf("=== HID Device Enumeration ===\n");

    // Initialize the HID API
    if (hid_init() != 0) {
        printf("Error: Failed to initialize HID API\n");
        return;
    }

    // Enumerate HID devices
    struct hid_device_info* devices = hid_enumerate(0x0, 0x0);  // 0,0 means enumerate all devices
    struct hid_device_info* current_device = devices;

    int device_count = 0;
    while (current_device) {
        device_count++;

        printf("\nDevice #%d:\n", device_count);
        printf("  Path: %s\n", current_device->path);
        printf("  Vendor ID: 0x%04X\n", current_device->vendor_id);
        printf("  Product ID: 0x%04X\n", current_device->product_id);

        if (current_device->manufacturer_string)
            printf("  Manufacturer: %ls\n", current_device->manufacturer_string);

        if (current_device->product_string)
            printf("  Product: %ls\n", current_device->product_string);

        if (current_device->serial_number)
            printf("  Serial Number: %ls\n", current_device->serial_number);

        printf("  Release Number: 0x%04X\n", current_device->release_number);
        printf("  Interface Number: %d\n", current_device->interface_number);
        printf("  Usage Page: 0x%04X\n", current_device->usage_page);
        printf("  Usage: 0x%04X\n", current_device->usage);

        current_device = current_device->next;
    }

    if (device_count == 0) {
        printf("No HID devices found.\n");
    } else {
        printf("\nTotal devices found: %d\n", device_count);
    }

    // Free the enumeration list
    hid_free_enumeration(devices);

    // Clean up the HID API
    hid_exit();

    printf("=== End of HID Device Enumeration ===\n");
}
