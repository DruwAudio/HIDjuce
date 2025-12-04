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
                       )
{
    // Initialize HID devices list
    enumerateHIDDevices();
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
    disconnectFromDevice();
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
    if (touchActive.load() && (currentTime - lastTouchTime > touchTimeoutMs)) {
        touchActive.store(false);
    }

    // Generate click impulse on touch start
    bool currentTouchState = touchActive.load();
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

    // Start timer to read events periodically
    startTimer(1); // Read every 1ms for lowest latency
}

void AudioPluginAudioProcessor::disconnectFromDevice()
{
    if (connectedDevice) {
        stopTimer();
        hid_close(connectedDevice);
        hid_exit();
        connectedDevice = nullptr;
    }
}

void AudioPluginAudioProcessor::timerCallback()
{
    if (connectedDevice) {
        readHIDEvents();
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
    }
}

void AudioPluginAudioProcessor::parseELOTouchData(unsigned char* data, int length, unsigned char reportId)
{
    if (reportId != 1 || length < 59) return;

    // Extract primary touch coordinates
    uint16_t touch1_x = data[2] | (data[3] << 8);
    uint16_t touch1_y = data[6] | (data[7] << 8);

    // Detect active touches (reasonable coordinate range)
    bool touch1_active = (touch1_x > 0 && touch1_x < 32000 && touch1_y > 0 && touch1_y < 32000);

    if (touch1_active) {
        touchActive.store(true);
        touchX.store(touch1_x);
        touchY.store(touch1_y);
        lastTouchTime = juce::Time::currentTimeMillis();
    } else {
        touchActive.store(false);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
