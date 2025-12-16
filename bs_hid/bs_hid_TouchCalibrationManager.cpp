/*
  ==============================================================================

   Touch Calibration Manager Implementation

  ==============================================================================
*/

#include "bs_hid.h"
#include "bs_hid_TouchCalibrationManager.h"


namespace bs
{

TouchCalibrationManager::TouchCalibrationManager()
{
    // Initialize with defaults
    resetToDefaults();
}

juce::File TouchCalibrationManager::getConfigDirectory() const
{
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    auto configDir = appDataDir.getChildFile("HIDModule");

    // Create directory if it doesn't exist
    if (!configDir.exists())
        configDir.createDirectory();

    return configDir;
}

juce::File TouchCalibrationManager::getCalibrationFile() const
{
    return getConfigDirectory().getChildFile("touchScreen.xml");
}

std::unique_ptr<juce::XmlElement> TouchCalibrationManager::createDefaultXml() const
{
    auto xml = std::make_unique<juce::XmlElement>("TouchScreenCalibration");
    xml->setAttribute("version", "1.0");

    auto* bounds = xml->createNewChildElement("Bounds");
    bounds->createNewChildElement("MinX")->addTextElement(juce::String(currentBounds.minX));
    bounds->createNewChildElement("MaxX")->addTextElement(juce::String(currentBounds.maxX));
    bounds->createNewChildElement("MinY")->addTextElement(juce::String(currentBounds.minY));
    bounds->createNewChildElement("MaxY")->addTextElement(juce::String(currentBounds.maxY));

    auto* metadata = xml->createNewChildElement("Metadata");
    metadata->createNewChildElement("CalibrationDate")->addTextElement(juce::Time::getCurrentTime().toISO8601(true));
    metadata->createNewChildElement("IsCalibrated")->addTextElement(currentBounds.isCalibrated ? "true" : "false");

    return xml;
}

bool TouchCalibrationManager::loadFromFile()
{
    juce::ScopedLock sl(lock);

    auto file = getCalibrationFile();

    if (!file.existsAsFile())
    {
        DBG("TouchCalibrationManager: No calibration file found, using defaults");
        return false;
    }

    // Parse XML file
    juce::XmlDocument xmlDoc(file);
    auto xml = xmlDoc.getDocumentElement();

    if (xml == nullptr)
    {
        DBG("TouchCalibrationManager: Failed to parse XML, using defaults");
        return false;
    }

    // Parse bounds
    parseBoundsFromXml(xml.get());

    DBG("TouchCalibrationManager: Loaded calibration from file");
    DBG("  X range: " << currentBounds.minX << " to " << currentBounds.maxX);
    DBG("  Y range: " << currentBounds.minY << " to " << currentBounds.maxY);

    return true;
}

void TouchCalibrationManager::parseBoundsFromXml(const juce::XmlElement* xml)
{
    if (xml == nullptr)
        return;

    auto* boundsElement = xml->getChildByName("Bounds");
    if (boundsElement == nullptr)
    {
        DBG("TouchCalibrationManager: Missing Bounds element, using defaults");
        return;
    }

    // Parse individual bounds
    auto* minXElem = boundsElement->getChildByName("MinX");
    auto* maxXElem = boundsElement->getChildByName("MaxX");
    auto* minYElem = boundsElement->getChildByName("MinY");
    auto* maxYElem = boundsElement->getChildByName("MaxY");

    if (minXElem && maxXElem && minYElem && maxYElem)
    {
        float minX = minXElem->getAllSubText().getFloatValue();
        float maxX = maxXElem->getAllSubText().getFloatValue();
        float minY = minYElem->getAllSubText().getFloatValue();
        float maxY = maxYElem->getAllSubText().getFloatValue();

        // Validate ranges
        if (minX < maxX && minY < maxY)
        {
            currentBounds.minX = minX;
            currentBounds.maxX = maxX;
            currentBounds.minY = minY;
            currentBounds.maxY = maxY;
            currentBounds.isCalibrated = true;
        }
        else
        {
            DBG("TouchCalibrationManager: Invalid bounds in XML, using defaults");
        }
    }
    else
    {
        DBG("TouchCalibrationManager: Missing bound elements, using defaults");
    }
}

bool TouchCalibrationManager::saveToFile()
{
    juce::ScopedLock sl(lock);

    auto file = getCalibrationFile();
    auto xml = createDefaultXml();

    if (!xml->writeTo(file))
    {
        DBG("TouchCalibrationManager: Failed to write calibration file");
        return false;
    }

    DBG("TouchCalibrationManager: Saved calibration to file");
    DBG("  X range: " << currentBounds.minX << " to " << currentBounds.maxX);
    DBG("  Y range: " << currentBounds.minY << " to " << currentBounds.maxY);

    return true;
}

void TouchCalibrationManager::setCalibrationPoints(const TouchData& topLeft, const TouchData& bottomRight)
{
    // Validate both points
    if (!topLeft.isValid() || !bottomRight.isValid())
    {
        DBG("TouchCalibrationManager: Invalid calibration points");
        return;
    }

    juce::ScopedLock sl(lock);

    // Calculate bounds (handle any order of points)
    currentBounds.minX = juce::jmin((float)topLeft.x, (float)bottomRight.x);
    currentBounds.maxX = juce::jmax((float)topLeft.x, (float)bottomRight.x);
    currentBounds.minY = juce::jmin((float)topLeft.y, (float)bottomRight.y);
    currentBounds.maxY = juce::jmax((float)topLeft.y, (float)bottomRight.y);

    // Validate separation
    float deltaX = currentBounds.maxX - currentBounds.minX;
    float deltaY = currentBounds.maxY - currentBounds.minY;

    if (deltaX < 20000.0f || deltaY < 20000.0f)
    {
        DBG("TouchCalibrationManager: Calibration points too close together");
        resetToDefaults();
        return;
    }

    currentBounds.isCalibrated = true;

    DBG("TouchCalibrationManager: Calibration set successfully");

    // Save to file (lock will auto-release when sl goes out of scope)
    saveToFile();
}

TouchCalibrationManager::CalibrationBounds TouchCalibrationManager::getBounds() const
{
    juce::ScopedLock sl(lock);
    return currentBounds;
}

void TouchCalibrationManager::resetToDefaults()
{
    juce::ScopedLock sl(lock);
    currentBounds.minX = 101.0f;
    currentBounds.maxX = 29947.0f;
    currentBounds.minY = 133.0f;
    currentBounds.maxY = 29986.0f;
    currentBounds.isCalibrated = false;
}

juce::Point<float> TouchCalibrationManager::convertTouchToNormalized(const TouchData &touch) const
{
    juce::ScopedLock sl(lock);

    if (currentBounds.isCalibrated)
    {
        return juce::Point<float>((touch.x - currentBounds.minX) / (currentBounds.maxX - currentBounds.minX),
                                  (touch.y - currentBounds.minY) / (currentBounds.maxY - currentBounds.minY));
    }
    else
    {
        jassert(false);
        return juce::Point<float>(touch.x / 32768.0f, touch.y / 32768.0f);
    }
}


} // namespace bs
