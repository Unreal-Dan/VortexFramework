#include "DefaultModes.h"

#include "../Colors/ColorTypes.h"

// Here is the array of 'default modes' that are assigned to
// the gloveset upon factory reset
const default_mode_entry default_modes[] = {
  {
    PATTERN_DOPS, 5, {
      RGB_RED,
      RGB_GREEN,
      RGB_BLUE,
      0xABAA00,
      0x5500AB
    }
  }
};

// exposed size of the default modes array
const uint8_t num_default_modes = (sizeof(default_modes) / sizeof(default_modes[0]));
