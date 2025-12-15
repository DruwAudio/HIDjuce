/*******************************************************************************

  bs_hid module implementation

*******************************************************************************/

#include "bs_hid.h"

// Note: hidapi/mac/hid.c should be compiled separately by the build system
// It's added to target_sources in CMakeLists.txt

// Include module implementations
#include "bs_hid_TouchParser.cpp"
#include "bs_hid_HIDDeviceManager.cpp"
