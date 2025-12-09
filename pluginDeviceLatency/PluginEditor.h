#pragma once

#include "PluginProcessor.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             public juce::ComboBox::Listener,
                                             public juce::Button::Listener,
                                             public juce::Timer
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked (juce::Button* button) override;
    void timerCallback() override;

private:
    void populateDeviceComboBox();
    void updateDiagnosticDisplay();

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;

    juce::Label deviceLabel;
    juce::ComboBox deviceComboBox;
    juce::Label statusLabel;

    // Latency optimization controls
    juce::TextButton optimizeButton;
    juce::TextButton restoreButton;
    juce::ToggleButton twoFingerToggle;
    juce::Label optimizationStatus;

    // Diagnostic display
    juce::Label diagnosticsHeader;
    juce::Label reportRateLabel;
    juce::Label avgIntervalLabel;
    juce::Label minMaxIntervalLabel;
    juce::Label audioLatencyLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
