#ifndef THEATER_CHASE_PATTERN_H
#define THEATER_CHASE_PATTERN_H

#include "BlinkStepPattern.h"

#include "../../Time/Timings.h"

class TheaterChasePattern : public BlinkStepPattern
{
public:
  TheaterChasePattern(VortexEngine &engine, const PatternArgs &args);
  virtual ~TheaterChasePattern();

  virtual void init() override;

protected:
  virtual void blinkOn() override;
  virtual void poststep() override;

  LedMap m_ledPositions;
  uint8_t m_stepCounter;
};

#endif
