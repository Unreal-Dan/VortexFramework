#include "DefaultModes.h"

#include "../Colors/ColorTypes.h"

// Here is the array of 'default modes' that are assigned to
// the gloveset upon factory reset
const default_mode_entry default_modes[MAX_MODES] = {
  {
    PATTERN_VORTEX, MAP_LED_ALL, 5, {
      RGB_RED,
      RGB_GREEN,
      RGB_BLUE,
      0xABAA00,
      0x5500AB
    },
    PATTERN_VORTEX, MAP_LED_NONE, 5, {
      RGB_RED,
      RGB_GREEN,
      RGB_BLUE,
      0xABAA00,
      0x5500AB
    }
  },

  {
    PATTERN_GAPCYCLE, MAP_LED_ALL, 4, {
      0x00553A,
      0x00CFFF,
      0x55001D,
      0x551200
    },
    PATTERN_GAPCYCLE, MAP_LED_NONE, 4, {
      HSV(114, 255, 43),
      HSV(159, 255, 128),
      HSV(240, 255, 43),
      HSV(9, 255, 43)
    }
  },

  {
    PATTERN_TRACER, MAP_RINGS_EVEN, 3, {
      0x62007F,
      0x62007F,
      0x33FE00
    },
    PATTERN_BLENDSTROBE, MAP_RINGS_ODD, 8, {
      0xFF0019,
      0x553600,
      0xABFF00,
      0x005502,
      0x00FFBA,
      0x003155,
      0x1E00FF,
      0x460055
    }
  },

  {
    PATTERN_BLENDSTROBEGAP, MAP_LINE_1, 8, {
      0xFFAE00,
      0x3D5500,
      0x1BFF00,
      0x00552A,
      0x00E7FF,
      0x001955,
      0x4E00FF,
      0x4e0055
    },
    PATTERN_GHOSTCRUSH, MAP_LINE_2, 8, {
      0xFFAE00,
      0x3D5500,
      0x1BFF00,
      0x00552A,
      0x00E7FF,
      0x001955,
      0x4E00FF,
      0x4e0055
    }
  },

  {
    PATTERN_BLENDSTROBE, MAP_RINGS_EVEN, 6, {
      0x554A00,
      0x554A00,
      0x00AA90,
      0x00AA90,
      0xD200FF,
      0xD200FF
    },
    PATTERN_DOUBLEDOPS, MAP_RINGS_ODD, 8, {
      0xFF0033,
      0xFF0033,
      0xDB2F00,
      0xB76B00,
      0x938D00,
      0x4A6F00,
      0x164B00,
      0x002702
    }
  },

  {
    PATTERN_CHASER, MAP_LED_ALL, 5, {
      0x9FFF00,
      0x00FF66,
      0x009FFF,
      0x5A00FF,
      0xFF009F
    },
    PATTERN_CHASER, MAP_LED_NONE, 5, {
      HSV(0, 0, 255),
      HSV(0, 0, 255),
      HSV(0, 0, 0),
      HSV(140, 255, 170),
      HSV(0, 0, 0)
    }
  },

  {
    PATTERN_METEOR, MAP_LED_ALL, 5, {
      0x00AA90,
      0x00B1FF,
      0x300055,
      0xFF002D,
      0x541B00
    },
    PATTERN_METEOR, MAP_LED_NONE, 5, {
      HSV(0, 0, 255),
      HSV(0, 0, 255),
      HSV(0, 0, 0),
      HSV(170, 255, 170),
      HSV(0, 0, 0)
    }
  },

  {
    PATTERN_DRIP, MAP_LED_ALL, 5, {
      0x0000FF,
      0x400055,
      0x54000B,
      0x525400,
      0x26AA00
    },
    PATTERN_DRIP, MAP_LED_NONE, 5, {
      HSV(0, 0, 255),
      HSV(0, 0, 255),
      HSV(0, 0, 0),
      HSV(200, 255, 170),
      HSV(0, 0, 0)
    }
  },

  {
    PATTERN_HYPERSTROBE, MAP_LED_ALL, 2, {
      0xFFF600,
      0x000880
    },
    PATTERN_HYPERSTROBE, MAP_LED_NONE, 5, {
      HSV(0, 0, 255),
      HSV(0, 0, 255),
      HSV(0, 0, 0),
      HSV(230, 255, 170),
      HSV(0, 0, 0)
    }
  },

  {
    PATTERN_SPARKLETRACE, MAP_LED_ALL, 6, {
      0x003755,
      0x003755,
      0xAA0072,
      0xAA0072,
      0xA5FF00,
      0xA5FF00
    },
    PATTERN_SPARKLETRACE, MAP_LED_NONE, 5, {
      HSV(0, 0, 255),
      HSV(0, 0, 255),
      HSV(0, 0, 0),
      HSV(0, 255, 170),
      HSV(0, 0, 0)
    }
  },

  {
    PATTERN_DASHDOPS, MAP_LED_ALL, 5, {
      0xE2E7FF,
      0x010015,
      0x13090E,
      0x010015,
      0xE2E7FF
    },
    PATTERN_DASHDOPS, MAP_LED_NONE, 8, {
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
    PATTERN_ULTRADOPS, MAP_LED_ALL, 8, {
      0x1C0000,
      0x4B2600,
      0xABAA00,
      0x001C00,
      0x00130A,
      0x00001C,
      0x26004B,
      0x13000A
    },
    PATTERN_ULTRADOPS, MAP_LED_NONE, 8, {
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
    PATTERN_GHOSTCRUSH, MAP_LED_ALL, 7, {
      0x26004B,
      RGB_OFF,
      RGB_GREEN,
      RGB_WHITE,
      RGB_GREEN,
      RGB_OFF,
      0x26004B,
    },
    PATTERN_GHOSTCRUSH, MAP_LED_NONE, 7, {
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
    PATTERN_COMPLEMENTARY_BLEND, MAP_LED_ALL, 3, {
      RGB_RED,
      RGB_GREEN,
      RGB_BLUE
    },
    PATTERN_COMPLEMENTARY_BLEND, MAP_LED_NONE, 3, {
      RGB_RED,
      RGB_GREEN,
      RGB_BLUE
    }
  }
};

// exposed size of the default modes array
const uint8_t num_default_modes = (sizeof(default_modes) / sizeof(default_modes[0]));
