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
    // Register as listener for touch events
    hidDeviceManager.addListener(this);

    // Load touch calibration
    bool calibLoaded = calibrationManager.loadFromFile();
    DBG("Touch calibration: " << (calibLoaded ? "Loaded from file" : "Using defaults"));

    // Attempt initial connection
    attemptTouchDeviceConnection();

    // Enable auto-reconnect for known touch devices
    hidDeviceManager.enableAutoReconnect({
        {0x03EB, 0x8A6E},  // ELO Touch (Atmel maXTouch)
        {0x2575, 0x7317}   // Standard touch digitizer
    }, 2000); // Check every 2 seconds
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
    hidDeviceManager.disableAutoReconnect();
    hidDeviceManager.removeListener(this);
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

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Get current touch state
    auto touchData = hidDeviceManager.getLatestTouchData();
    bool touchStarted = touchData.isActive && !previousTouchState;
    previousTouchState = touchData.isActive;

    // Generate click impulse on touch start
    if (touchStarted)
    {
        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            if (buffer.getNumSamples() > 0)
            {
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

std::vector<bs::HIDDeviceInfo> AudioPluginAudioProcessor::getAvailableHIDDevices()
{
    return hidDeviceManager.getAvailableDevices();
}

void AudioPluginAudioProcessor::connectToDevice(const bs::HIDDeviceInfo& device)
{
    hidDeviceManager.connectToDevice(device);
}

void AudioPluginAudioProcessor::disconnectFromDevice()
{
    hidDeviceManager.disconnectFromDevice();
}

bool AudioPluginAudioProcessor::isDeviceConnected() const
{
    return hidDeviceManager.isDeviceConnected();
}

const bs::HIDDeviceInfo& AudioPluginAudioProcessor::getConnectedDeviceInfo() const
{
    return hidDeviceManager.getConnectedDeviceInfo();
}

void AudioPluginAudioProcessor::touchDetected(const bs::TouchData& touchData)
{
    // This callback is called from the HID polling thread
    // You can log, update UI, or trigger other events here
    //DBG("Touch detected: x=" << touchData.x << " y=" << touchData.y << " active=" << (touchData.isActive ? "true" : "false"));
}

void AudioPluginAudioProcessor::attemptTouchDeviceConnection()
{
    // Don't try to connect if already connected
    if (hidDeviceManager.isDeviceConnected())
        return;

    // Get available devices
    auto devices = hidDeviceManager.getAvailableDevices();

    DBG("Found " << devices.size() << " HID devices:");
    for (const auto& device : devices)
    {
        DBG("  - VID:0x" << juce::String::toHexString((int)device.vendorId)
            << " PID:0x" << juce::String::toHexString((int)device.productId)
            << " : " << device.manufacturer << " - " << device.product);
    }

    // Look for known touch devices (ELO or the standard one)
    bs::HIDDeviceInfo* touchDevice = nullptr;
    for (auto& device : devices)
    {
        // ELO Touch (Atmel maXTouch)
        if (device.vendorId == 0x03EB && device.productId == 0x8A6E)
        {
            touchDevice = &device;
            DBG("Found ELO Touch device!");
            break;
        }
        // Standard touch digitizer
        else if (device.vendorId == 0x2575 && device.productId == 0x7317)
        {
            touchDevice = &device;
            DBG("Found standard touch digitizer!");
            break;
        }
    }

    if (touchDevice != nullptr)
    {
        if (hidDeviceManager.connectToDevice(*touchDevice))
        {
            DBG("Successfully connected to: " << touchDevice->manufacturer << " - " << touchDevice->product);
        }
        else
        {
            DBG("Failed to connect to touch device");
        }
    }
    else
    {
        DBG("No known touch device found. Please check vendor/product IDs.");
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
