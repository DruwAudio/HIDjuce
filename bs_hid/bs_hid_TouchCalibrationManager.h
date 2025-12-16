/*
  ==============================================================================

   Touch Calibration Manager
   Manages persistent touch screen calibration settings

  ==============================================================================
*/

#pragma once

namespace bs
{

/** Manages touch screen calibration data with persistent XML storage */
class TouchCalibrationManager
{
public:
    /** Structure to hold calibration boundary values */
    struct CalibrationBounds
    {
        float minX = 101.0f;
        float maxX = 29947.0f;
        float minY = 133.0f;
        float maxY = 29986.0f;
        bool isCalibrated = false;
    };

    TouchCalibrationManager();
    ~TouchCalibrationManager() = default;

    /** Load calibration from XML file. Returns false if file doesn't exist (uses defaults) */
    bool loadFromFile();

    /** Save current calibration bounds to XML file */
    bool saveToFile();

    /** Set calibration from two touch points (top-left and bottom-right)
        Automatically validates points and saves to file if valid */
    void setCalibrationPoints(const TouchData& topLeft, const TouchData& bottomRight);

    /** Get current calibration bounds (thread-safe) */
    CalibrationBounds getBounds() const;

    /** Reset calibration to factory defaults */
    void resetToDefaults();

    /** Get the calibration file path for diagnostics */
    juce::File getCalibrationFile() const;

    juce::Point<float> convertTouchToNormalized(const TouchData& touch) const;

private:
    /** Get configuration directory, creating it if needed */
    juce::File getConfigDirectory() const;

    /** Create default XML structure */
    std::unique_ptr<juce::XmlElement> createDefaultXml() const;

    /** Parse bounds from XML element */
    void parseBoundsFromXml(const juce::XmlElement* xml);

    mutable juce::CriticalSection lock;
    CalibrationBounds currentBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TouchCalibrationManager)
};

} // namespace bs
