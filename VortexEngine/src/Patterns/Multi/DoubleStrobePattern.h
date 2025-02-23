#ifndef DOUBLESTROBE_PATTERN_H
#define DOUBLESTROBE_PATTERN_H

#include "BlinkStepPattern.h"

#include "../../Time/Timings.h"

class DoubleStrobePattern : public BlinkStepPattern
{
public:
  DoubleStrobePattern(VortexEngine &engine, const PatternArgs &args);
  virtual ~DoubleStrobePattern();

  virtual void init() override;

protected:
  virtual void blinkOn() override;
};
#endif
