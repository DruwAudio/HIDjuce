/*
  ==============================================================================

   Touch Parser Implementation

  ==============================================================================
*/

// This file should only be included via bs_hid.cpp
// But if compiled standalone, include the necessary headers
#ifndef BS_HID_H_INCLUDED
    #include "bs_hid.h"
#endif

namespace bs
{

TouchData TouchParser::parseELOTouch(const unsigned char* data, int length, unsigned char reportId)
{
    if (reportId != 1 || length < 59)
        return TouchData();

    // Extract primary touch coordinates
    uint16_t touch_x = data[2] | (data[3] << 8);
    uint16_t touch_y = data[6] | (data[7] << 8);

    // Validate coordinates
    bool isValid = isValidCoordinate(touch_x, touch_y);

    // ELO doesn't have explicit contact ID in this position, use 0 for primary touch
    return TouchData(touch_x, touch_y, isValid, 0, juce::Time::currentTimeMillis());
}

TouchData TouchParser::parseStandardTouch(const unsigned char* data, int length,
                                         unsigned char reportId, int maxTouchPoints)
{
    if (reportId != 1 || length < 44)
        return TouchData();

    // Extract contact count (last byte before scan time)
    unsigned char contactCount = data[length - 1];
    printf("Contact Count: %d\n", contactCount);


    // Parse each touch point (limited by maxTouchPoints for better latency)
    for (int i = 0; i < maxTouchPoints && (1 + i * 4 + 3) < length - 3; ++i)
    {
        int offset = 1 + i * 4; // Start after report ID

        // Byte structure per touch point:
        // Byte 0: Tip switch (bit 0) + padding (bits 1-2) + Contact ID (bits 3-7)
        // Bytes 1-2: X coordinate (16 bits, little endian)
        // Bytes 3-4: Y coordinate (16 bits, little endian)

        unsigned char firstByte = data[offset];
        bool tipSwitch = (firstByte & 0x01) != 0;
        uint8_t contactId = (firstByte >> 3) & 0x1F; // Extract bits 3-7 (contact ID)

        // Early exit if no tip switch
        if (!tipSwitch)
            continue;

        // X coordinate (2 bytes, little endian)
        uint16_t x = data[offset + 1] | (data[offset + 2] << 8);
        // Y coordinate (2 bytes, little endian)
        uint16_t y = data[offset + 3] | (data[offset + 4] << 8);

        // Validate coordinates
        if (isValidCoordinate(x, y))
        {
            return TouchData(x, y, true, contactId, juce::Time::currentTimeMillis());
        }
    }

    // No active touches found
    return TouchData(0, 0, false, 0, juce::Time::currentTimeMillis());
}

std::vector<TouchData> TouchParser::parseStandardTouchMulti(const unsigned char* data, int length,
                                                             unsigned char reportId, int maxTouchPoints)
{
    std::vector<TouchData> touches;

    if (reportId != 1 || length < 44)
        return touches;

    juce::int64 timestamp = juce::Time::currentTimeMillis();

    // Extract contact count for debugging
    unsigned char contactCount = data[length - 1];
    printf("parseStandardTouchMulti - Contact Count: %d, length: %d, maxTouchPoints: %d\n",
           contactCount, length, maxTouchPoints);

    // Parse each touch point (limited by maxTouchPoints for better latency)
    for (int i = 0; i < maxTouchPoints && (1 + i * 5 + 4) < length - 1; ++i)
    {
        int offset = 1 + i * 5; // Start after report ID (5 bytes per touch)

        // Byte structure per touch point:
        // Byte 0: Tip switch (bit 0) + padding (bits 1-2) + Contact ID (bits 3-7)
        // Bytes 1-2: X coordinate (16 bits, little endian)
        // Bytes 3-4: Y coordinate (16 bits, little endian)

        unsigned char firstByte = data[offset];
        bool tipSwitch = (firstByte & 0x01) != 0;
        uint8_t contactId = (firstByte >> 3) & 0x1F; // Extract bits 3-7 (contact ID)

        // Skip if no tip switch
        if (!tipSwitch)
            continue;

        // X coordinate (2 bytes, little endian)
        uint16_t x = data[offset + 1] | (data[offset + 2] << 8);
        // Y coordinate (2 bytes, little endian)
        uint16_t y = data[offset + 3] | (data[offset + 4] << 8);

        // Add touch if coordinates are valid
        if (isValidCoordinate(x, y))
        {
            printf("  Found valid touch %d: ID=%d, x=%d, y=%d\n", i, contactId, x, y);
            touches.push_back(TouchData(x, y, true, contactId, timestamp));
        }
        else if (tipSwitch)
        {
            printf("  Touch %d has tipSwitch but invalid coords: x=%d, y=%d\n", i, x, y);
        }
    }

    printf("  Total touches collected: %d\n", (int)touches.size());
    return touches;
}

bool TouchParser::isValidCoordinate(uint16_t x, uint16_t y)
{
    return x >= minValidCoord && x <= maxValidCoord &&
           y >= minValidCoord && y <= maxValidCoord;
}

} // namespace bs
