#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    setSize (600, 400);

    // Enumerate HID devices when the component is created
    enumerateHIDDevices();
    createDeviceButtons();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    if (hidDevices.empty()) {
        g.setFont (juce::FontOptions (16.0f));
        g.setColour (juce::Colours::white);
        g.drawText ("No HID devices found", getLocalBounds(), juce::Justification::centred, true);
    }
}

void MainComponent::resized()
{
    // Layout the device buttons in a vertical list
    auto bounds = getLocalBounds();
    bounds.reduce(10, 10); // Add some padding

    const int buttonHeight = 30;
    const int buttonSpacing = 5;

    for (int i = 0; i < deviceButtons.size(); ++i) {
        auto buttonBounds = bounds.removeFromTop(buttonHeight);
        deviceButtons[i]->setBounds(buttonBounds);
        bounds.removeFromTop(buttonSpacing); // Add spacing between buttons
    }
}

void MainComponent::enumerateHIDDevices()
{
    printf("=== HID Device Enumeration ===\n");

    // Clear any existing devices
    hidDevices.clear();

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

        HIDDeviceInfo deviceInfo;
        deviceInfo.path = juce::String(current_device->path);
        deviceInfo.vendorId = current_device->vendor_id;
        deviceInfo.productId = current_device->product_id;

        if (current_device->manufacturer_string)
            deviceInfo.manufacturer = juce::String(current_device->manufacturer_string);
        else
            deviceInfo.manufacturer = "Unknown";

        if (current_device->product_string)
            deviceInfo.product = juce::String(current_device->product_string);
        else
            deviceInfo.product = "Unknown Product";

        if (current_device->serial_number)
            deviceInfo.serialNumber = juce::String(current_device->serial_number);
        else
            deviceInfo.serialNumber = "No Serial";

        hidDevices.push_back(deviceInfo);

        printf("\nDevice #%d:\n", device_count);
        printf("  Path: %s\n", current_device->path);
        printf("  Vendor ID: 0x%04X\n", current_device->vendor_id);
        printf("  Product ID: 0x%04X\n", current_device->product_id);
        printf("  Manufacturer: %s\n", deviceInfo.manufacturer.toUTF8());
        printf("  Product: %s\n", deviceInfo.product.toUTF8());
        printf("  Serial Number: %s\n", deviceInfo.serialNumber.toUTF8());

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

void MainComponent::createDeviceButtons()
{
    // Clear any existing buttons
    deviceButtons.clear();

    // Create a button for each HID device
    for (size_t i = 0; i < hidDevices.size(); ++i) {
        const auto& device = hidDevices[i];

        auto* button = new juce::TextButton();
        juce::String buttonText = device.manufacturer + " " + device.product +
                                 " (VID: " + juce::String::toHexString(device.vendorId) +
                                 ", PID: " + juce::String::toHexString(device.productId) + ")";
        button->setButtonText(buttonText);
        button->addListener(this);

        deviceButtons.add(button);
        addAndMakeVisible(button);
    }

    // Trigger a layout update
    resized();
}

void MainComponent::buttonClicked(juce::Button* button)
{
    // Find which device button was clicked
    for (int i = 0; i < deviceButtons.size(); ++i) {
        if (deviceButtons[i] == button) {
            const auto& device = hidDevices[i];

            printf("\n=== Device Selected ===\n");
            printf("Manufacturer: %s\n", device.manufacturer.toUTF8());
            printf("Product: %s\n", device.product.toUTF8());
            printf("Vendor ID: 0x%04X\n", device.vendorId);
            printf("Product ID: 0x%04X\n", device.productId);
            printf("Path: %s\n", device.path.toUTF8());
            printf("Serial: %s\n", device.serialNumber.toUTF8());
            printf("======================\n");
            break;
        }
    }
}
