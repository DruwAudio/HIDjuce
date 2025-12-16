/*
  ==============================================================================

   Touch Data Structure

  ==============================================================================
*/

#pragma once

namespace bs
{

/** Structure to hold touch state data */
struct TouchData
{
    uint16_t x = 0;
    uint16_t y = 0;
    bool isActive = false;
    uint8_t contactId = 0;  // Touch index/finger ID
    juce::int64 timestamp = 0;

    TouchData() = default;

    TouchData(uint16_t xPos, uint16_t yPos, bool active, uint8_t id = 0, juce::int64 time = 0)
        : x(xPos), y(yPos), isActive(active), contactId(id), timestamp(time)
    {}

    bool isValid() const
    {
        static constexpr uint16_t minValidCoord = 0;
        static constexpr uint16_t maxValidCoord = 300000;
        return x >= minValidCoord && x <= maxValidCoord &&
               y >= minValidCoord && y <= maxValidCoord;
    }
};

} // namespace bs
