#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : connectedDevice(nullptr), isDeviceConnected(false)
{
    setSize (600, 400);

    // Enumerate HID devices when the component is created
    enumerateHIDDevices();
    createDeviceButtons();
}

MainComponent::~MainComponent()
{
    disconnectFromDevice();
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

            // Connect to this device for event reading
            connectToDevice(device);
            break;
        }
    }
}

void MainComponent::connectToDevice(const HIDDeviceInfo& device)
{
    // Disconnect from any currently connected device
    disconnectFromDevice();

    // Initialize HID API
    if (hid_init() != 0) {
        printf("Error: Failed to initialize HID API for device connection\n");
        return;
    }

    // Open the device
    connectedDevice = hid_open_path(device.path.toUTF8());
    if (!connectedDevice) {
        printf("Error: Failed to open device at path: %s\n", device.path.toUTF8());
        hid_exit();
        return;
    }

    // Set device to non-blocking mode for reading
    hid_set_nonblocking(connectedDevice, 1);

    connectedDeviceInfo = device;
    isDeviceConnected = true;

    printf("Successfully connected to device: %s %s\n",
           device.manufacturer.toUTF8(), device.product.toUTF8());

    // Get and display report descriptor information
    getReportDescriptor();

    printf("Starting event monitoring...\n");

    // Start timer to read events periodically
    startTimer(10); // Read every 10ms
}

void MainComponent::disconnectFromDevice()
{
    if (isDeviceConnected && connectedDevice) {
        stopTimer();
        hid_close(connectedDevice);
        hid_exit();
        connectedDevice = nullptr;
        isDeviceConnected = false;
        printf("Disconnected from device\n");
    }
}

void MainComponent::timerCallback()
{
    if (isDeviceConnected) {
        readHIDEvents();
    }
}

void MainComponent::readHIDEvents()
{
    if (!connectedDevice) return;

    unsigned char buffer[256];
    int bytesRead = hid_read(connectedDevice, buffer, sizeof(buffer));

    if (bytesRead > 0) {
        parseInputReport(buffer, bytesRead);
    } else if (bytesRead < 0) {
        printf("Error reading from device: %ls\n", hid_error(connectedDevice));
        disconnectFromDevice();
    }
    // bytesRead == 0 means no data available (non-blocking mode)
}

void MainComponent::parseInputReport(unsigned char* data, int length)
{
    printf("HID Event [%d bytes]: ", length);
    for (int i = 0; i < length; ++i) {
        printf("%02X ", data[i]);
    }
    printf("\n");

    // Basic parsing examples:
    if (length > 0) {
        unsigned char reportId = data[0];
        printf("  Report ID: 0x%02X\n", reportId);

        // For keyboards, bytes 2-7 typically contain key codes
        if (length >= 8 && reportId == 0x01) {
            printf("  Modifier keys: 0x%02X\n", data[1]);
            printf("  Key codes: ");
            for (int i = 2; i < 8; ++i) {
                if (data[i] != 0) {
                    printf("0x%02X ", data[i]);
                }
            }
            printf("\n");
        }

        // For mice, typically X/Y movement and button states
        if (length >= 4 && (reportId == 0x02 || reportId == 0x01)) {
            if (length > 1) printf("  Buttons: 0x%02X\n", data[1]);
            if (length > 2) printf("  X movement: %d\n", (signed char)data[2]);
            if (length > 3) printf("  Y movement: %d\n", (signed char)data[3]);
            if (length > 4) printf("  Wheel: %d\n", (signed char)data[4]);
        }
    }

    printf("---\n");
}

void MainComponent::getReportDescriptor()
{
    if (!connectedDevice) return;

    unsigned char descriptor[4096];
    int descriptorLength = hid_get_report_descriptor(connectedDevice, descriptor, sizeof(descriptor));

    if (descriptorLength > 0) {
        printf("\n=== HID Report Descriptor ===\n");
        printf("Descriptor length: %d bytes\n\n", descriptorLength);
        parseReportDescriptor(descriptor, descriptorLength);
        printf("=== End Report Descriptor ===\n\n");
    } else {
        printf("Failed to retrieve report descriptor\n");
    }
}

void MainComponent::parseReportDescriptor(unsigned char* descriptor, int length)
{
    printf("Raw descriptor bytes:\n");
    for (int i = 0; i < length; ++i) {
        printf("%02X ", descriptor[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    if (length % 16 != 0) printf("\n\n");

    printf("Parsed descriptor structure:\n");

    int i = 0;
    int indent = 0;
    unsigned short currentUsagePage = 0;
    unsigned short currentUsage = 0;
    int reportID = 0;
    int reportSize = 0;
    int reportCount = 0;

    while (i < length) {
        unsigned char item = descriptor[i];
        unsigned char tag = (item >> 4) & 0x0F;
        unsigned char type = (item >> 2) & 0x03;
        unsigned char size = item & 0x03;

        if (size == 3) size = 4; // Special case for 4-byte items

        // Print indentation
        for (int j = 0; j < indent; ++j) printf("  ");

        switch (type) {
            case 0: // Main items
                switch (tag) {
                    case 0x8: // Input
                        printf("Input (");
                        if (i + 1 < length) {
                            unsigned char flags = descriptor[i + 1];
                            if (flags & 0x01) printf("Constant, ");
                            else printf("Data, ");
                            if (flags & 0x02) printf("Variable, ");
                            else printf("Array, ");
                            if (flags & 0x04) printf("Relative");
                            else printf("Absolute");
                        }
                        printf(")\n");
                        break;
                    case 0x9: // Output
                        printf("Output\n");
                        break;
                    case 0xB: // Feature
                        printf("Feature\n");
                        break;
                    case 0xA: // Collection
                        printf("Collection (");
                        if (i + 1 < length) {
                            switch (descriptor[i + 1]) {
                                case 0x00: printf("Physical"); break;
                                case 0x01: printf("Application"); break;
                                case 0x02: printf("Logical"); break;
                                default: printf("0x%02X", descriptor[i + 1]); break;
                            }
                        }
                        printf(")\n");
                        indent++;
                        break;
                    case 0xC: // End Collection
                        indent--;
                        for (int j = 0; j < indent; ++j) printf("  ");
                        printf("End Collection\n");
                        break;
                }
                break;

            case 1: // Global items
                switch (tag) {
                    case 0x0: // Usage Page
                        if (i + 1 < length) {
                            currentUsagePage = descriptor[i + 1];
                            if (i + 2 < length && size > 1) {
                                currentUsagePage |= (descriptor[i + 2] << 8);
                            }
                            printf("Usage Page (0x%04X - ", currentUsagePage);
                            printUsageInfo(currentUsagePage, 0);
                            printf(")\n");
                        }
                        break;
                    case 0x1: // Logical Minimum
                        printf("Logical Minimum\n");
                        break;
                    case 0x2: // Logical Maximum
                        printf("Logical Maximum\n");
                        break;
                    case 0x8: // Report ID
                        if (i + 1 < length) {
                            reportID = descriptor[i + 1];
                            printf("Report ID (%d)\n", reportID);
                        }
                        break;
                    case 0x7: // Report Size
                        if (i + 1 < length) {
                            reportSize = descriptor[i + 1];
                            printf("Report Size (%d bits)\n", reportSize);
                        }
                        break;
                    case 0x9: // Report Count
                        if (i + 1 < length) {
                            reportCount = descriptor[i + 1];
                            printf("Report Count (%d)\n", reportCount);
                        }
                        break;
                }
                break;

            case 2: // Local items
                switch (tag) {
                    case 0x0: // Usage
                        if (i + 1 < length) {
                            currentUsage = descriptor[i + 1];
                            if (i + 2 < length && size > 1) {
                                currentUsage |= (descriptor[i + 2] << 8);
                            }
                            printf("Usage (0x%04X - ", currentUsage);
                            printUsageInfo(currentUsagePage, currentUsage);
                            printf(")\n");
                        }
                        break;
                    case 0x1: // Usage Minimum
                        printf("Usage Minimum\n");
                        break;
                    case 0x2: // Usage Maximum
                        printf("Usage Maximum\n");
                        break;
                }
                break;
        }

        i += 1 + size;
    }
}

void MainComponent::printUsageInfo(unsigned short usagePage, unsigned short usage)
{
    switch (usagePage) {
        case 0x01: // Generic Desktop
            printf("Generic Desktop");
            if (usage != 0) {
                printf(" - ");
                switch (usage) {
                    case 0x01: printf("Pointer"); break;
                    case 0x02: printf("Mouse"); break;
                    case 0x04: printf("Joystick"); break;
                    case 0x05: printf("Game Pad"); break;
                    case 0x06: printf("Keyboard"); break;
                    case 0x07: printf("Keypad"); break;
                    case 0x30: printf("X"); break;
                    case 0x31: printf("Y"); break;
                    case 0x32: printf("Z"); break;
                    case 0x38: printf("Wheel"); break;
                    default: printf("0x%02X", usage); break;
                }
            }
            break;
        case 0x07: // Keyboard/Keypad
            printf("Keyboard/Keypad");
            if (usage != 0) {
                printf(" - ");
                if (usage >= 0x04 && usage <= 0x1D) {
                    printf("Key %c", 'A' + (usage - 0x04));
                } else if (usage >= 0x1E && usage <= 0x27) {
                    printf("Key %d", (usage - 0x1E + 1) % 10);
                } else {
                    switch (usage) {
                        case 0x28: printf("Enter"); break;
                        case 0x29: printf("Escape"); break;
                        case 0x2A: printf("Backspace"); break;
                        case 0x2B: printf("Tab"); break;
                        case 0x2C: printf("Space"); break;
                        case 0xE0: printf("Left Ctrl"); break;
                        case 0xE1: printf("Left Shift"); break;
                        case 0xE2: printf("Left Alt"); break;
                        case 0xE3: printf("Left GUI"); break;
                        case 0xE4: printf("Right Ctrl"); break;
                        case 0xE5: printf("Right Shift"); break;
                        case 0xE6: printf("Right Alt"); break;
                        case 0xE7: printf("Right GUI"); break;
                        default: printf("0x%02X", usage); break;
                    }
                }
            }
            break;
        case 0x09: // Button
            printf("Button");
            if (usage != 0) {
                printf(" - Button %d", usage);
            }
            break;
        case 0x0C: // Consumer
            printf("Consumer");
            break;
        default:
            printf("Page 0x%04X", usagePage);
            break;
    }
}
