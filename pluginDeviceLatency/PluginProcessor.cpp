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

    // Parse each touch point (10 touch points max, each touch point is 4 bytes)
    for (int i = 0; i < 10 && (1 + i * 4 + 3) < length - 3; ++i) {
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
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
