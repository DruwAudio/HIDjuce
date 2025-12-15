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
    juce::int64 timestamp = 0;

    TouchData() = default;

    TouchData(uint16_t xPos, uint16_t yPos, bool active, juce::int64 time = 0)
        : x(xPos), y(yPos), isActive(active), timestamp(time)
    {}

    bool isValid() const
    {
        static constexpr uint16_t minValidCoord = 100;
        static constexpr uint16_t maxValidCoord = 30000;
        return x >= minValidCoord && x <= maxValidCoord &&
               y >= minValidCoord && y <= maxValidCoord;
    }
};

} // namespace bs
