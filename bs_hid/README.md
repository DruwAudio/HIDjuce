# bs_hid - Beetronics HID Module

A JUCE module for low-latency HID device management and touch input parsing.

## Features

- **Low-latency HID polling** with real-time priority threading
- **Touch event callbacks** with optimized atomic state communication
- **Device enumeration** and connection management
- **Built-in touch parsers** for ELO Touch and standard HID digitizers
- **Diagnostic reporting** (report rate, timing statistics)
- **Thread-safe** design with lock-free communication

## Installation

### As a JUCE Module

1. Copy the `bs_hid` folder to your project's modules directory
2. In your CMakeLists.txt:

```cmake
# Add hidapi C implementation to sources (compile separately)
target_sources(YourTarget
    PRIVATE
        path/to/hidapi/mac/hid.c  # or hidapi/windows/hid.c, hidapi/linux/hid.c
)

# Add bs_hid module
juce_add_module(path/to/bs_hid)

# Add hidapi include path
target_include_directories(YourTarget PRIVATE
    path/to/hidapi/hidapi
)

# Link the module
target_link_libraries(YourTarget
    PRIVATE
        bs_hid
        juce::juce_audio_utils
)
```

## Usage

### Basic Setup

```cpp
#include <bs_hid/bs_hid.h>

class MyProcessor : public juce::AudioProcessor,
                   public bs::HIDDeviceManager::Listener
{
public:
    MyProcessor()
    {
        // Register as listener
        hidManager.addListener(this);
    }

    ~MyProcessor()
    {
        hidManager.removeListener(this);
    }

    // Implement the touch callback
    void touchDetected(const bs::TouchData& touch) override
    {
        // Called from HID polling thread when touch events occur
        DBG("Touch: x=" << touch.x << " y=" << touch.y
            << " active=" << touch.isActive);
    }

private:
    bs::HIDDeviceManager hidManager;
};
```

### Enumerating and Connecting to Devices

```cpp
// Get list of available devices
auto devices = hidManager.getAvailableDevices();

for (const auto& device : devices)
{
    DBG("Found: " << device.manufacturer << " - " << device.product);
}

// Connect to a device
if (!devices.empty())
{
    hidManager.connectToDevice(devices[0]);
}

// Check connection status
if (hidManager.isDeviceConnected())
{
    auto info = hidManager.getConnectedDeviceInfo();
    DBG("Connected to: " << info.product);
}
```

### Using Touch Data in Audio Processing

```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    // Get latest touch state (thread-safe)
    auto touch = hidManager.getLatestTouchData();

    // Generate click on touch start
    bool touchStarted = touch.isActive && !previousTouchActive;
    previousTouchActive = touch.isActive;

    if (touchStarted)
    {
        // Generate audio impulse
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            buffer.getWritePointer(ch)[0] = 0.5f;
        }
    }
}
```

### Getting Diagnostic Statistics

```cpp
// Get HID report rate statistics
auto stats = hidManager.getReportStats();

DBG("Report Rate: " << stats.reportRateHz << " Hz");
DBG("Avg Interval: " << stats.avgIntervalMs << " ms");
DBG("Min/Max: " << stats.minIntervalMs << " / " << stats.maxIntervalMs << " ms");
```

### Configuration

```cpp
// Set maximum touch points to parse (default: 2)
// Lower values = faster parsing = lower latency
hidManager.setMaxTouchPoints(2);  // 2-finger mode
// or
hidManager.setMaxTouchPoints(10); // 10-finger mode
```

## Architecture

### Thread Safety

The module uses **lock-free atomic operations** for touch state communication between the HID polling thread and the audio thread:

- Touch state is packed into a single `uint64_t` atomic variable
- `std::memory_order_release` / `acquire` semantics ensure proper synchronization
- No mutexes or locks in the hot path

### Touch State Packing

Touch data is efficiently packed into 64 bits:
```
[Timestamp (31 bits)] [Active (1 bit)] [Y (16 bits)] [X (16 bits)]
```

This allows atomic read/write of the entire touch state without locking.

## Supported Devices

Currently supports parsing for:
- **ELO Touch** (Atmel maXTouch) - VID: 0x03EB, PID: 0x8A6E
- **Standard HID Multi-touch Digitizers** - VID: 0x2575, PID: 0x7317

### Adding Support for New Devices

1. Implement a parser in `bs_hid_TouchParser.cpp`
2. Add device detection in `HIDDeviceManager::parseInputReport()`

## API Reference

### Classes

- **`HIDDeviceManager`** - Main class for device management and polling
- **`TouchParser`** - Static utility class for parsing touch data
- **`HIDDeviceInfo`** - Device information structure
- **`TouchData`** - Touch state data structure

### Key Methods

#### HIDDeviceManager
- `getAvailableDevices()` - Enumerate HID devices
- `connectToDevice(device)` - Connect to a device
- `disconnectFromDevice()` - Disconnect
- `getLatestTouchData()` - Get current touch state (thread-safe)
- `getReportStats()` - Get diagnostic statistics
- `addListener(listener)` - Register for callbacks
- `removeListener(listener)` - Unregister

## Dependencies

- **JUCE** (juce_core, juce_events)
- **hidapi** - Low-level HID library (included in parent project)

## License

Proprietary - Beetronics

## Version

1.0.0
