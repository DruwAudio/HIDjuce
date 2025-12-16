/*
  ==============================================================================

   Touch Parser - Utility functions for parsing touch data

  ==============================================================================
*/

#pragma once

namespace bs_hid
{

/**
    Static utility class for parsing touch data from HID reports.
    Contains device-specific parsing logic for different touchscreen models.
*/
class TouchParser
{
public:
    /** Parse ELO Touch (Atmel maXTouch) data */
    static TouchData parseELOTouch(const unsigned char* data, int length, unsigned char reportId);

    /** Parse standard HID multi-touch digitizer data (returns first touch only) */
    static TouchData parseStandardTouch(const unsigned char* data, int length,
                                       unsigned char reportId, int maxTouchPoints);

    /** Parse all touches from standard HID multi-touch digitizer data */
    static std::vector<TouchData> parseStandardTouchMulti(const unsigned char* data, int length,
                                                          unsigned char reportId, int maxTouchPoints);

    /** Validate coordinate ranges */
    static bool isValidCoordinate(uint16_t x, uint16_t y);

private:
    static constexpr uint16_t minValidCoord = 0;
    static constexpr uint16_t maxValidCoord = 60000;

    TouchParser() = delete;  // Static class only
};

} // namespace bs_hid
