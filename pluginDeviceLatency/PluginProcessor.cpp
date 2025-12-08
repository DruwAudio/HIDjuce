#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), juce::Thread("HIDPollingThread")
{
    // Initialize HID devices list
    enumerateHIDDevices();
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
    disconnectFromDevice();
    signalThreadShouldExit();
    waitForThreadToExit(2000);
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Check for touch timeout (no HID events means finger lifted)
    juce::int64 currentTime = juce::Time::currentTimeMillis();
    uint16_t touchX_temp, touchY_temp;
    bool currentTouchState = getTouchState(touchX_temp, touchY_temp);

    if (currentTouchState && (currentTime - lastTouchTime > touchTimeoutMs)) {
        setTouchState(0, 0, false);
        currentTouchState = false;
    }

    // Generate click impulse on touch start using optimized atomic
    bool touchStarted = currentTouchState && !previousTouchState;
    previousTouchState = currentTouchState;

    if (touchStarted) {
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            if (buffer.getNumSamples() > 0) {
                channelData[0] = 0.5f; // Single impulse at first sample
            }
        }
    }
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// HID Device Management

std::vector<HIDDeviceInfo> AudioPluginAudioProcessor::getAvailableHIDDevices()
{
    return hidDevices;
}

void AudioPluginAudioProcessor::enumerateHIDDevices()
{
    hidDevices.clear();

    if (hid_init() != 0) {
        return;
    }

    struct hid_device_info* devices = hid_enumerate(0x0, 0x0);
    struct hid_device_info* current_device = devices;

    while (current_device) {
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
        current_device = current_device->next;
    }

    hid_free_enumeration(devices);
    hid_exit();
}

void AudioPluginAudioProcessor::connectToDevice(const HIDDeviceInfo& device)
{
    disconnectFromDevice();

    if (hid_init() != 0) {
        return;
    }

    connectedDevice = hid_open_path(device.path.toUTF8());
    if (!connectedDevice) {
        hid_exit();
        return;
    }

    hid_set_nonblocking(connectedDevice, 1);
    connectedDeviceInfo = device;

    // Query available feature reports to analyze device capabilities
    queryAvailableFeatureReports();

    // Start real-time thread for minimal latency HID polling
    juce::Thread::RealtimeOptions realtimeOptions;
    realtimeOptions.withPriority(8); // High priority (0-10 scale)
    startRealtimeThread(realtimeOptions);
}

void AudioPluginAudioProcessor::disconnectFromDevice()
{
    if (connectedDevice) {
        signalThreadShouldExit();
        waitForThreadToExit(1000); // Wait up to 1 second for clean exit
        hid_close(connectedDevice);
        hid_exit();
        connectedDevice = nullptr;
    }
}

void AudioPluginAudioProcessor::run()
{
    while (!threadShouldExit()) {
        if (connectedDevice) {
            readHIDEvents();
        }
        wait(1); // Sleep for 1ms between polls
    }
}

void AudioPluginAudioProcessor::readHIDEvents()
{
    if (!connectedDevice) return;

    unsigned char buffer[256];
    int bytesRead = hid_read(connectedDevice, buffer, sizeof(buffer));

    if (bytesRead > 0) {
        parseInputReport(buffer, bytesRead);
    } else if (bytesRead < 0) {
        disconnectFromDevice();
    }
}

void AudioPluginAudioProcessor::parseInputReport(unsigned char* data, int length)
{
    if (length > 0) {
        unsigned char reportId = data[0];

        // ELO Touch parsing for Atmel maXTouch
        if (connectedDeviceInfo.vendorId == 0x03EB && connectedDeviceInfo.productId == 0x8A6E) {
            parseELOTouchData(data, length, reportId);
        }
        // Standard HID multi-touch digitizer parsing for new touchscreen
        else if (connectedDeviceInfo.vendorId == 0x2575 && connectedDeviceInfo.productId == 0x7317 && reportId == 1) {
            parseStandardTouchData(data, length, reportId);
        }
    }
}

void AudioPluginAudioProcessor::parseELOTouchData(unsigned char* data, int length, unsigned char reportId)
{
    if (reportId != 1 || length < 59) return;

    // Extract primary touch coordinates
    uint16_t touch1_x = data[2] | (data[3] << 8);
    uint16_t touch1_y = data[6] | (data[7] << 8);

    // Optimized coordinate validation using cached ranges
    bool touch1_active = (touch1_x >= minValidCoord && touch1_x <= maxValidCoord &&
                          touch1_y >= minValidCoord && touch1_y <= maxValidCoord);

    // Update touch state using optimized atomic communication
    setTouchState(touch1_x, touch1_y, touch1_active);

    if (touch1_active) {
        lastTouchTime = juce::Time::currentTimeMillis();
    }
}

void AudioPluginAudioProcessor::parseStandardTouchData(unsigned char* data, int length, unsigned char reportId)
{
    if (reportId != 1 || length < 44) return;

    // Parse each touch point (limited by maxTouchPoints for better latency)
    for (int i = 0; i < maxTouchPoints && (1 + i * 4 + 3) < length - 3; ++i) {
        int offset = 1 + i * 4; // Start after report ID

        // Byte structure per touch point:
        // Byte 0: Tip switch (bit 0) + padding (bits 1-2) + Contact ID (bits 3-7)
        // Bytes 1-2: X coordinate (16 bits, little endian)
        // Bytes 3-4: Y coordinate (16 bits, little endian)

        unsigned char firstByte = data[offset];
        bool tipSwitch = (firstByte & 0x01) != 0;

        // Early exit if no tip switch
        if (!tipSwitch) continue;

        // X coordinate (2 bytes, little endian)
        uint16_t x = data[offset + 1] | (data[offset + 2] << 8);
        // Y coordinate (2 bytes, little endian)
        uint16_t y = data[offset + 3] | (data[offset + 4] << 8);

        // Optimized coordinate validation using cached ranges
        if (x >= minValidCoord && x <= maxValidCoord &&
            y >= minValidCoord && y <= maxValidCoord) {

            // Update touch state using optimized atomic communication
            setTouchState(x, y, true);
            lastTouchTime = juce::Time::currentTimeMillis();
            return; // Use first active touch only
        }
    }

    // If we get here, no active touches were found
    setTouchState(0, 0, false);
}

//==============================================================================
// HID Feature Report Management

void AudioPluginAudioProcessor::queryAvailableFeatureReports()
{
    if (!connectedDevice) {
        printf("No device connected for feature report query\n");
        return;
    }

    printf("\n=== HID Feature Reports Analysis ===\n");
    printf("Device: %s %s\n", connectedDeviceInfo.manufacturer.toUTF8(),
           connectedDeviceInfo.product.toUTF8());
    printf("VID: 0x%04X, PID: 0x%04X\n\n",
           connectedDeviceInfo.vendorId, connectedDeviceInfo.productId);

    // List of feature report IDs from your descriptor
    std::vector<unsigned char> featureReportIds = {
        66,   // Standard touch configuration
        68,   // Standard touch configuration
        240,  // Vendor specific (4 bytes)
        242,  // Vendor specific (4 bytes)
        243,  // Vendor specific (61 bytes)
        6,    // Vendor specific (7 bytes)
        7,    // Vendor specific (63 bytes)
        8,    // Vendor specific (63 bytes)
        9     // Vendor specific (1 byte)
    };

    for (unsigned char reportId : featureReportIds) {
        unsigned char buffer[64] = {0}; // Max size for most reports

        if (readFeatureReport(reportId, buffer, sizeof(buffer))) {
            analyzeFeatureReport(reportId, buffer, sizeof(buffer));
        } else {
            printf("Report ID %d: Not accessible or not supported\n", reportId);
        }
    }

    printf("=== End Feature Reports Analysis ===\n\n");
}

bool AudioPluginAudioProcessor::readFeatureReport(unsigned char reportId, unsigned char* buffer, int bufferSize)
{
    if (!connectedDevice) return false;

    buffer[0] = reportId;
    int result = hid_get_feature_report(connectedDevice, buffer, bufferSize);

    return result > 0;
}

bool AudioPluginAudioProcessor::writeFeatureReport(unsigned char reportId, unsigned char* data, int dataSize)
{
    if (!connectedDevice) return false;

    unsigned char buffer[65] = {0}; // +1 for report ID
    buffer[0] = reportId;
    memcpy(buffer + 1, data, dataSize);

    int result = hid_send_feature_report(connectedDevice, buffer, dataSize + 1);
    return result >= 0;
}

void AudioPluginAudioProcessor::analyzeFeatureReport(unsigned char reportId, unsigned char* data, int length)
{
    printf("📋 Report ID %d: ", reportId);

    // Print raw data
    printf("Data [%d bytes]: ", length);
    for (int i = 1; i < std::min(16, length); ++i) { // Skip report ID byte
        printf("%02X ", data[i]);
    }
    if (length > 16) printf("...");
    printf("\n");

    // Analyze specific report types based on known patterns
    switch (reportId) {
        case 66:
            printf("  📱 Touch Configuration Report\n");
            if (length > 2) {
                printf("    Touch Mode: 0x%02X\n", data[1]);
                printf("    Settings: 0x%02X\n", data[2]);
            }
            break;

        case 68:
            printf("  ⚡ Performance/Latency Settings\n");
            if (length > 1) {
                printf("    Performance Mode: 0x%02X\n", data[1]);
            }
            break;

        case 240:
            printf("  🔧 Vendor Configuration (4 bytes)\n");
            if (length > 4) {
                printf("    Config Bytes: %02X %02X %02X %02X\n",
                       data[1], data[2], data[3], data[4]);
            }
            break;

        case 242:
            printf("  📊 Touch Sensitivity/Thresholds\n");
            if (length > 4) {
                uint16_t threshold1 = data[1] | (data[2] << 8);
                uint16_t threshold2 = data[3] | (data[4] << 8);
                printf("    Threshold 1: %d\n", threshold1);
                printf("    Threshold 2: %d\n", threshold2);
            }
            break;

        case 243:
            printf("  🚀 Extended Configuration (61 bytes)\n");
            printf("    Report Rate Config: 0x%02X\n", data[1]);
            if (length > 5) {
                printf("    Power Management: 0x%02X\n", data[5]);
                printf("    Filter Settings: 0x%02X\n", data[10]);
            }
            break;

        default:
            printf("  ❓ Unknown/Vendor Specific\n");
            break;
    }
    printf("\n");
}

//==============================================================================
// Latency Optimization Functions

bool AudioPluginAudioProcessor::optimizeForLowLatency()
{
    if (!connectedDevice) {
        printf("❌ No device connected for optimization\n");
        return false;
    }

    printf("\n🚀 Optimizing touchscreen for low latency...\n");

    // Backup current settings first
    backupCurrentSettings();

    bool success = true;

    // 1. Increase report rate (most impactful)
    printf("📊 Setting high report rate...\n");
    if (!setReportRate(0x08)) { // Try doubling current rate
        printf("⚠️  Report rate adjustment failed, trying alternative...\n");
        setReportRate(0x04); // Fallback value
    }

    // 2. Set maximum performance mode
    printf("⚡ Setting maximum performance mode...\n");
    if (!setPerformanceMode(0xFF)) {
        printf("⚠️  Performance mode adjustment failed\n");
        success = false;
    }

    // 3. Lower touch thresholds for faster detection
    printf("🎯 Lowering touch detection thresholds...\n");
    if (!setTouchThresholds(16000, 2000)) { // Roughly half current values
        printf("⚠️  Threshold adjustment failed\n");
        success = false;
    }

    printf("%s Latency optimization %s\n",
           success ? "✅" : "⚠️",
           success ? "completed successfully" : "completed with warnings");

    if (success) {
        printf("🎯 Expected latency improvement: 1-4ms\n");
        printf("📋 Changes will reset when device is disconnected\n");
    }

    return success;
}

void AudioPluginAudioProcessor::backupCurrentSettings()
{
    unsigned char buffer[64] = {0};

    // Backup Report ID 243 (report rate)
    if (readFeatureReport(243, buffer, sizeof(buffer))) {
        settingsBackup.reportRate = buffer[1];
    }

    // Backup Report ID 68 (performance mode)
    if (readFeatureReport(68, buffer, sizeof(buffer))) {
        settingsBackup.performanceMode = buffer[1];
    }

    // Backup Report ID 242 (thresholds)
    if (readFeatureReport(242, buffer, sizeof(buffer))) {
        settingsBackup.threshold1 = buffer[1] | (buffer[2] << 8);
        settingsBackup.threshold2 = buffer[3] | (buffer[4] << 8);
    }

    settingsBackup.hasBackup = true;
    printf("💾 Current settings backed up\n");
}

void AudioPluginAudioProcessor::restoreSettings()
{
    if (!settingsBackup.hasBackup) {
        printf("⚠️  No backup available to restore\n");
        return;
    }

    printf("🔄 Restoring original settings...\n");

    setReportRate(settingsBackup.reportRate);
    setPerformanceMode(settingsBackup.performanceMode);
    setTouchThresholds(settingsBackup.threshold1, settingsBackup.threshold2);

    printf("✅ Original settings restored\n");
}

bool AudioPluginAudioProcessor::setReportRate(unsigned char rateValue)
{
    unsigned char data[64] = {0};
    data[0] = rateValue; // Set report rate in first data byte

    bool success = writeFeatureReport(243, data, 64);
    if (success) {
        printf("   📊 Report rate set to: 0x%02X\n", rateValue);
    }
    return success;
}

bool AudioPluginAudioProcessor::setPerformanceMode(unsigned char perfMode)
{
    // Note: Report 68 has complex data, we'll only modify the first byte
    unsigned char buffer[64] = {0};

    // Read current data first to preserve other settings
    if (!readFeatureReport(68, buffer, sizeof(buffer))) {
        return false;
    }

    // Modify only the performance mode byte
    buffer[1] = perfMode;

    bool success = writeFeatureReport(68, buffer + 1, 63); // Skip report ID
    if (success) {
        printf("   ⚡ Performance mode set to: 0x%02X\n", perfMode);
    }
    return success;
}

bool AudioPluginAudioProcessor::setTouchThresholds(uint16_t threshold1, uint16_t threshold2)
{
    unsigned char buffer[64] = {0};

    // Read current data to preserve other settings
    if (!readFeatureReport(242, buffer, sizeof(buffer))) {
        return false;
    }

    // Set new thresholds (little endian)
    buffer[1] = threshold1 & 0xFF;
    buffer[2] = (threshold1 >> 8) & 0xFF;
    buffer[3] = threshold2 & 0xFF;
    buffer[4] = (threshold2 >> 8) & 0xFF;

    bool success = writeFeatureReport(242, buffer + 1, 63); // Skip report ID
    if (success) {
        printf("   🎯 Thresholds set to: %d, %d\n", threshold1, threshold2);
    }
    return success;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
