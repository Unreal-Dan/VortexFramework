#ifndef CHASER_PATTERN_H
#define CHASER_PATTERN_H

#include "SequencedPattern.h"

class ChaserPattern : public SequencedPattern
{
public:
  // Just provide a constructor to build the sequence
  ChaserPattern(VortexEngine &engine, const PatternArgs &args);
};

#endif
