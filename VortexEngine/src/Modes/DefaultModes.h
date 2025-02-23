#ifndef DEFAULT_MODES_H
#define DEFAULT_MODES_H

#include "../Patterns/Patterns.h"
#include "../VortexConfig.h"
#include "../Leds/LedTypes.h"

// structure of the entries in the default modes array
struct default_mode_entry
{
  PatternID patternID;
  LedMap map;
  uint8_t numColors;
  uint32_t cols[MAX_COLOR_SLOTS];
  PatternID patternID2;
  LedMap map2;
  uint8_t numColors2;
  uint32_t cols2[MAX_COLOR_SLOTS];
};

// exposed global array of default modes
extern const default_mode_entry default_modes[MAX_MODES];

// exposed size of the default modes array
extern const uint8_t num_default_modes;

#endif
