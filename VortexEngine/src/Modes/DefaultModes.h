#ifndef DEFAULT_MODES_H
#define DEFAULT_MODES_H

#include "../Patterns/Patterns.h"
#include "../VortexConfig.h"

// structure of the entries in the default modes array
struct default_mode_entry
{
  PatternID patternID;
  uint8_t numColors;
  uint32_t cols[MAX_COLOR_SLOTS];
};

// exposed global array of default modes
extern const default_mode_entry default_modes[];

// exposed size of the default modes array
extern const uint8_t num_default_modes;

#endif
