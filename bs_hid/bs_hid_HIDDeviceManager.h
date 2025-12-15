/*
  ==============================================================================

   HID Device Manager - Handles HID device enumeration and connection

  ==============================================================================
*/

#pragma once

namespace bs
{

/**
    Manages HID device connections and provides callbacks for touch events.

    This class handles:
    - Enumerating available HID devices
    - Connecting/disconnecting from devices
    - Running a high-priority polling thread
    - Parsing HID reports and generating touch callbacks
*/
class HIDDeviceManager : public juce::Thread
{
public:
    //==============================================================================
    /** Callback interface for touch events */
    class Listener
    {
    public:
        virtual ~Listener() = default;

        /** Called when a touch event is detected */
        virtual void touchDetected(const TouchData& touchData) = 0;
    };

    //==============================================================================
    HIDDeviceManager();
    ~HIDDeviceManager() override;

    //==============================================================================
    /** Enumerates all available HID devices */
    std::vector<HIDDeviceInfo> getAvailableDevices();

    /** Connects to a specific HID device */
    bool connectToDevice(const HIDDeviceInfo& device);

    /** Disconnects from the current device */
    void disconnectFromDevice();

    /** Returns true if a device is currently connected */
    bool isDeviceConnected() const { return connectedDevice != nullptr; }

    /** Returns information about the connected device */
    const HIDDeviceInfo& getConnectedDeviceInfo() const { return connectedDeviceInfo; }

    //==============================================================================
    /** Adds a listener to receive touch events */
    void addListener(Listener* listener);

    /** Removes a listener */
    void removeListener(Listener* listener);

    //==============================================================================
    /** Set the maximum number of touch points to parse (default: 2) */
    void setMaxTouchPoints(int maxPoints) { maxTouchPoints = maxPoints; }

    /** Get the maximum number of touch points */
    int getMaxTouchPoints() const { return maxTouchPoints; }

    //==============================================================================
    /** Diagnostics: Get the most recent touch data */
    TouchData getLatestTouchData() const;

    /** Get all current active touches */
    std::vector<TouchData> getAllTouches() const;

    /** Diagnostics: Get HID report statistics */
    struct ReportStats
    {
        double reportRateHz = 0.0;
        double minIntervalMs = 0.0;
        double maxIntervalMs = 0.0;
        double avgIntervalMs = 0.0;
        int sampleCount = 0;
    };
    ReportStats getReportStats() const;

private:
    //==============================================================================
    // Thread run method
    void run() override;

    // HID reading and parsing
    void readHIDEvents();
    void parseInputReport(unsigned char* data, int length);
    void parseELOTouchData(unsigned char* data, int length, unsigned char reportId);
    void parseStandardTouchData(unsigned char* data, int length, unsigned char reportId);

    // Touch state management
    void updateTouchState(const TouchData& newTouch);
    void notifyListeners(const TouchData& touch);

    //==============================================================================
    hid_device* connectedDevice = nullptr;
    HIDDeviceInfo connectedDeviceInfo;

    // Listener management
    juce::ListenerList<Listener> listeners;

    // Touch state (using atomic for thread-safe communication)
    std::atomic<uint64_t> packedTouchState{0};  // Packed: x(16) + y(16) + active(1) + contactId(8) + timestamp(23)

    // Multi-touch state (protected by mutex)
    mutable juce::CriticalSection touchArrayLock;
    std::vector<TouchData> currentTouches;

    // Configuration
    int maxTouchPoints = 10;

    // Diagnostic timing
    juce::int64 lastReportTimeTicks = 0;
    std::atomic<double> reportIntervalMs{0.0};
    std::atomic<double> minReportIntervalMs{999999.0};
    std::atomic<double> maxReportIntervalMs{0.0};
    std::atomic<double> avgReportIntervalMs{0.0};
    std::atomic<int> reportCount{0};
    double runningIntervalSum = 0.0;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HIDDeviceManager)
};

} // namespace bs
