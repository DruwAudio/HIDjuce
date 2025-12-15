/*
  ==============================================================================

   HID Device Manager Implementation

  ==============================================================================
*/

// This file should only be included via bs_hid.cpp
// But if compiled standalone, include the necessary headers
#ifndef BS_HID_H_INCLUDED
    #include "bs_hid.h"
#endif

namespace bs
{

//==============================================================================
HIDDeviceManager::HIDDeviceManager()
    : juce::Thread("HIDPollingThread")
{
}

HIDDeviceManager::~HIDDeviceManager()
{
    disconnectFromDevice();
    signalThreadShouldExit();
    waitForThreadToExit(2000);
}

//==============================================================================
std::vector<HIDDeviceInfo> HIDDeviceManager::getAvailableDevices()
{
    std::vector<HIDDeviceInfo> devices;

    if (hid_init() != 0)
        return devices;

    struct hid_device_info* deviceList = hid_enumerate(0x0, 0x0);
    struct hid_device_info* current = deviceList;

    while (current)
    {
        HIDDeviceInfo info;
        info.path = juce::String(current->path);
        info.vendorId = current->vendor_id;
        info.productId = current->product_id;
        info.manufacturer = current->manufacturer_string ? juce::String(current->manufacturer_string) : "Unknown";
        info.product = current->product_string ? juce::String(current->product_string) : "Unknown Product";
        info.serialNumber = current->serial_number ? juce::String(current->serial_number) : "No Serial";

        devices.push_back(info);
        current = current->next;
    }

    hid_free_enumeration(deviceList);
    hid_exit();

    return devices;
}

bool HIDDeviceManager::connectToDevice(const HIDDeviceInfo& device)
{
    disconnectFromDevice();

    if (hid_init() != 0)
        return false;

    connectedDevice = hid_open_path(device.path.toUTF8());
    if (!connectedDevice)
    {
        hid_exit();
        return false;
    }

    hid_set_nonblocking(connectedDevice, 1);
    connectedDeviceInfo = device;

    // Reset diagnostic statistics
    lastReportTimeTicks = 0;
    reportIntervalMs.store(0.0, std::memory_order_relaxed);
    minReportIntervalMs.store(999999.0, std::memory_order_relaxed);
    maxReportIntervalMs.store(0.0, std::memory_order_relaxed);
    avgReportIntervalMs.store(0.0, std::memory_order_relaxed);
    reportCount.store(0, std::memory_order_relaxed);
    runningIntervalSum = 0.0;

    // Start real-time thread for minimal latency HID polling
    juce::Thread::RealtimeOptions realtimeOptions;
    realtimeOptions.withPriority(8); // High priority (0-10 scale)
    startRealtimeThread(realtimeOptions);

    return true;
}

void HIDDeviceManager::disconnectFromDevice()
{
    if (connectedDevice)
    {
        signalThreadShouldExit();
        waitForThreadToExit(1000);
        hid_close(connectedDevice);
        hid_exit();
        connectedDevice = nullptr;
    }
}

//==============================================================================
void HIDDeviceManager::addListener(Listener* listener)
{
    listeners.add(listener);
}

void HIDDeviceManager::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

//==============================================================================
TouchData HIDDeviceManager::getLatestTouchData() const
{
    uint64_t packed = packedTouchState.load(std::memory_order_acquire);

    TouchData data;
    data.x = (uint16_t)(packed & 0xFFFF);
    data.y = (uint16_t)((packed >> 16) & 0xFFFF);
    data.isActive = (packed & (1ULL << 32)) != 0;
    data.contactId = (uint8_t)((packed >> 33) & 0xFF);
    data.timestamp = (juce::int64)((packed >> 41) & 0x7FFFFF);

    return data;
}

std::vector<TouchData> HIDDeviceManager::getAllTouches() const
{
    juce::ScopedLock lock(touchArrayLock);
    return currentTouches;
}

HIDDeviceManager::ReportStats HIDDeviceManager::getReportStats() const
{
    ReportStats stats;
    double avgInterval = avgReportIntervalMs.load(std::memory_order_relaxed);
    stats.avgIntervalMs = avgInterval;
    stats.minIntervalMs = minReportIntervalMs.load(std::memory_order_relaxed);
    stats.maxIntervalMs = maxReportIntervalMs.load(std::memory_order_relaxed);
    stats.sampleCount = reportCount.load(std::memory_order_relaxed);

    if (avgInterval > 0.0)
        stats.reportRateHz = 1000.0 / avgInterval;

    return stats;
}

//==============================================================================
void HIDDeviceManager::run()
{
    while (!threadShouldExit())
    {
        if (connectedDevice)
            readHIDEvents();

        wait(1); // Sleep for 1ms between polls
    }
}

void HIDDeviceManager::readHIDEvents()
{
    if (!connectedDevice)
        return;

    unsigned char buffer[256];
    int bytesRead = hid_read(connectedDevice, buffer, sizeof(buffer));

    if (bytesRead > 0)
    {
        parseInputReport(buffer, bytesRead);
    }
    else if (bytesRead < 0)
    {
        disconnectFromDevice();
    }
}

void HIDDeviceManager::parseInputReport(unsigned char* data, int length)
{
    if (length <= 0)
        return;

    unsigned char reportId = data[0];

    // Store previous touch state
    TouchData previousTouch = getLatestTouchData();
    bool wasTouchActive = previousTouch.isActive;

    // Parse based on device type
    TouchData newTouch;
    std::vector<TouchData> allTouches;

    // ELO Touch (Atmel maXTouch)
    if (connectedDeviceInfo.vendorId == 0x03EB && connectedDeviceInfo.productId == 0x8A6E)
    {
        newTouch = TouchParser::parseELOTouch(data, length, reportId);
        if (newTouch.isActive)
            allTouches.push_back(newTouch);
    }
    // Standard HID multi-touch digitizer
    else if (connectedDeviceInfo.vendorId == 0x2575 && connectedDeviceInfo.productId == 0x7317 && reportId == 1)
    {
        newTouch = TouchParser::parseStandardTouch(data, length, reportId, maxTouchPoints);
        allTouches = TouchParser::parseStandardTouchMulti(data, length, reportId, maxTouchPoints);
    }

    // Update multi-touch state
    {
        juce::ScopedLock lock(touchArrayLock);
        currentTouches = allTouches;
    }

    // Update single touch state (for backward compatibility)
    updateTouchState(newTouch);

    // Measure HID report timing ONLY for active touch reports
    if (newTouch.isActive)
    {
        juce::int64 currentTimeTicks = juce::Time::getHighResolutionTicks();

        // Only measure interval if previous report also had active touch
        if (wasTouchActive && lastReportTimeTicks > 0)
        {
            // Calculate interval between reports (convert ticks to milliseconds)
            juce::int64 ticksPerSecond = juce::Time::getHighResolutionTicksPerSecond();
            double intervalSeconds = (double)(currentTimeTicks - lastReportTimeTicks) / (double)ticksPerSecond;
            double intervalMs = intervalSeconds * 1000.0;

            // Update statistics
            reportIntervalMs.store(intervalMs, std::memory_order_relaxed);

            double currentMin = minReportIntervalMs.load(std::memory_order_relaxed);
            if (intervalMs < currentMin)
                minReportIntervalMs.store(intervalMs, std::memory_order_relaxed);

            double currentMax = maxReportIntervalMs.load(std::memory_order_relaxed);
            if (intervalMs > currentMax)
                maxReportIntervalMs.store(intervalMs, std::memory_order_relaxed);

            runningIntervalSum += intervalMs;
            int count = reportCount.fetch_add(1, std::memory_order_relaxed) + 1;
            avgReportIntervalMs.store(runningIntervalSum / count, std::memory_order_relaxed);
        }

        lastReportTimeTicks = currentTimeTicks;
    }

    // Notify listeners if touch state changed
    if (newTouch.isActive || wasTouchActive)
    {
        notifyListeners(newTouch);
    }
}

void HIDDeviceManager::updateTouchState(const TouchData& newTouch)
{
    // Pack touch data into single atomic 64-bit value
    // Layout: x(16) + y(16) + active(1) + contactId(8) + timestamp(23)
    uint64_t packed = ((uint64_t)newTouch.x) |
                      (((uint64_t)newTouch.y) << 16) |
                      (newTouch.isActive ? (1ULL << 32) : 0) |
                      (((uint64_t)newTouch.contactId) << 33) |
                      (((uint64_t)(newTouch.timestamp & 0x7FFFFF)) << 41);

    packedTouchState.store(packed, std::memory_order_release);
}

void HIDDeviceManager::notifyListeners(const TouchData& touch)
{
    listeners.call([&](Listener& l) { l.touchDetected(touch); });
}

} // namespace bs
