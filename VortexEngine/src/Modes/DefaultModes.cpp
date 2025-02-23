#include "DefaultModes.h"

#include "../Colors/ColorTypes.h"

// Here is the array of 'default modes' that are assigned to
// the gloveset upon factory reset
const default_mode_entry default_modes[MAX_MODES] = {
  {
    PATTERN_DOPS, 5, {
      RGB_RED,
      RGB_GREEN,
      RGB_BLUE,
      0xABAA00,
      0x5500AB
    }
  },
  {
    PATTERN_GHOSTCRUSH, 5, {
      RGB_WHITE,
      RGB_WHITE,
      RGB_OFF,
      0x700000,
      RGB_OFF,
    }
  },
  {
    PATTERN_WARPWORM, 2, {
      RGB_GREEN,
      0x26004B,
    }
  },
  {
    PATTERN_PULSISH, 3, {
      0x00AB55,
      0x8D1C55,
      0x00001C
    }
  },

  {
    PATTERN_ZIGZAG, 6, {
      RGB_OFF,
      0x56D400,
      0x5500AB,
      RGB_OFF,
      RGB_RED,
      0x700000
    }
  },
  {
    PATTERN_STROBE, 8, {
      0xD4002B,
      RGB_OFF,
      0x0056AA,
      RGB_OFF,
      0x8E711C,
      RGB_OFF,
      0x0056AA,
      RGB_OFF
    }
  },
  {
    PATTERN_SNOWBALL, 3, {
      0x170600,
      0x00840A,
      0x12002A
    }
  },
  {
    PATTERN_ULTRADOPS, 8, {
      0x1C0000,
      0x4B2600,
      0xABAA00,
      0x001C00,
      0x00130A,
      0x00001C,
      0x26004B,
      0x13000A
    }
  },
  {
    PATTERN_VORTEX, 4, {
      0xAA0055,
      0x7070C5,
      0x0A0013,
      0x1C8E55,
    }
  },
  {
    PATTERN_VORTEXWIPE, 8, {
      RGB_RED,
      0x00001C,
      0x00001C,
      0x00001C,
      0x00001C,
      0x00001C,
      0x00001C,
      0x00001C,
    }
  },
  {
    PATTERN_GHOSTCRUSH, 7, {
      0x26004B,
      RGB_OFF,
      RGB_GREEN,
      RGB_WHITE,
      RGB_GREEN,
      RGB_OFF,
      0x26004B,
    }
  },
  {
    PATTERN_VORTEXWIPE, 3, {
      0x00AB55,
      0x7F0081,
      0xAA381C,
    }
  },
  {
    PATTERN_COMPLEMENTARY_BLEND, 3, {
      RGB_RED,
      RGB_GREEN,
      RGB_BLUE
    }
  },
  {
    PATTERN_STROBE, 2, {
      RGB_RED,
      RGB_GREEN,
    }
  },
  {
    PATTERN_DRIP, 4, {
      RGB_RED,
      RGB_GREEN,
      RGB_BLUE,
      RGB_YELLOW
    }
  },
  {
    PATTERN_DASHCYCLE, 3, {
      RGB_RED,
      RGB_GREEN,
      RGB_BLUE
    }
  }
};

// exposed size of the default modes array
const uint8_t num_default_modes = (sizeof(default_modes) / sizeof(default_modes[0]));
