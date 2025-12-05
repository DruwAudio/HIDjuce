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
    enumerateHIDDevices();
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
    stopHidPollingThread();
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
    juce::ScopedNoDenormals noDenormals;
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear output channels
    for (auto i = 0; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Fast MIDI scan - early exit on first note-on
    bool midiTriggered = midiTriggerPending;
    if (!midiTriggered && !midiMessages.isEmpty()) {
        for (const auto& metadata : midiMessages) {
            if (metadata.getMessage().isNoteOn()) {
                midiTriggered = true;
                break;
            }
        }
    }
    midiTriggerPending = false;

    // Check for touch events from polling thread
    bool currentTouchState = touchActive.load(std::memory_order_relaxed);
    bool touchStarted = touchTriggerPending || (currentTouchState && !previousTouchState);
    previousTouchState = currentTouchState;
    touchTriggerPending = false;

    // Generate optimized click impulse
    if (touchStarted || midiTriggered) {
        const int numSamples = buffer.getNumSamples();
        if (numSamples > 0) {
            // Generate on output channels for maximum compatibility
            for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
                auto* channelData = buffer.getWritePointer(channel);
                channelData[0] = 0.5f;

                // Optional: add slight decay for better click sound
                if (numSamples > 1) {
                    channelData[1] = 0.1f;
                }
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

    // Start high-priority polling thread
    startHidPollingThread();
}

void AudioPluginAudioProcessor::disconnectFromDevice()
{
    stopHidPollingThread();

    if (connectedDevice) {
        hid_close(connectedDevice);
        hid_exit();
        connectedDevice = nullptr;
    }
}


void AudioPluginAudioProcessor::readHIDEvents()
{
    if (!connectedDevice) return;

    int bytesRead = hid_read(connectedDevice, hidBuffer, sizeof(hidBuffer));

    if (bytesRead > 0) {
        parseInputReport(hidBuffer, bytesRead);
    } else if (bytesRead < 0) {
        shouldStopHidThread = true;
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

    // Extract primary touch coordinates with optimized bit operations
    const uint16_t touch1_x = static_cast<uint16_t>(data[2]) | (static_cast<uint16_t>(data[3]) << 8);
    const uint16_t touch1_y = static_cast<uint16_t>(data[6]) | (static_cast<uint16_t>(data[7]) << 8);

    // Optimized touch detection
    const bool touch1_active = (touch1_x > 0 && touch1_x < 32000 && touch1_y > 0 && touch1_y < 32000);
    const bool previousTouch = touchActive.load(std::memory_order_relaxed);

    if (touch1_active) {
        touchX.store(touch1_x, std::memory_order_relaxed);
        touchY.store(touch1_y, std::memory_order_relaxed);
        lastTouchTime = juce::Time::currentTimeMillis();

        // Signal touch start for immediate audio response
        if (!previousTouch) {
            touchTriggerPending = true;
        }
        touchActive.store(true, std::memory_order_relaxed);
    } else {
        touchActive.store(false, std::memory_order_relaxed);
    }
}

//==============================================================================
// HID Polling Thread Functions

void AudioPluginAudioProcessor::startHidPollingThread()
{
    stopHidPollingThread();
    shouldStopHidThread.store(false);

    hidThread = std::make_unique<HIDPollingThread>(*this);
    hidThread->startThread(juce::Thread::Priority::high);
}

void AudioPluginAudioProcessor::stopHidPollingThread()
{
    shouldStopHidThread.store(true);
    if (hidThread && hidThread->isThreadRunning()) {
        hidThread->stopThread(1000);
    }
    hidThread.reset();
}

void AudioPluginAudioProcessor::hidPollingThreadFunction()
{
    while (!shouldStopHidThread.load() && connectedDevice) {
        readHIDEvents();
        juce::Thread::sleep(1);
    }
}

void AudioPluginAudioProcessor::HIDPollingThread::run()
{
    processor.hidPollingThreadFunction();
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
