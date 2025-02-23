#ifndef DEFAULT_MODES_H
#define DEFAULT_MODES_H

#include "../Patterns/Patterns.h"
#include "../Leds/LedTypes.h"
#include "../VortexConfig.h"

// structure of the entries in the default modes array
struct DefaultLedEntry
{
  PatternID patternID;
  uint8_t numColors;
  const uint32_t *cols;
};

struct DefaultModeEntry
{
  DefaultLedEntry leds[LED_COUNT];
};

// exposed global array of default modes
extern const DefaultModeEntry defaultModes[MAX_MODES];

#endif
