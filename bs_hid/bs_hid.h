/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION

  ID:               bs_hid
  vendor:           Beatsurfing
  version:          1.0.0
  name:             Beatsurfing HID Module
  description:      HID device management and touch input parsing
  website:          https://www.beatsurfing.com
  license:          Proprietary
  dependencies:     juce_core, juce_events, juce_graphics

 END_JUCE_MODULE_DECLARATION
*******************************************************************************/

#pragma once
#define BS_HID_H_INCLUDED

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>

// Include hidapi header
#include "../hidapi/hidapi/hidapi.h"

namespace bs_hid
{
    using namespace juce;
}

#include "bs_hid_HIDDeviceInfo.h"
#include "bs_hid_TouchData.h"
#include "bs_hid_HIDDeviceManager.h"
#include "bs_hid_TouchParser.h"
#include "bs_hid_TouchCalibrationManager.h"
#include "bs_hid_TouchVisualizerComponent.h"
