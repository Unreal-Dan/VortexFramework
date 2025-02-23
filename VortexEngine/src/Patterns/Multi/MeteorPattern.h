#ifndef METEOR_PATTERN_H
#define METEOR_PATTERN_H

#include "BlinkStepPattern.h"

#include "../../Time/Timings.h"
#include "../../Leds/LedStash.h"
#include "../../Random/Random.h"

class MeteorPattern : public BlinkStepPattern
{
public:
  MeteorPattern(VortexEngine &engine, const PatternArgs &args);
  virtual ~MeteorPattern();

protected:
  virtual void blinkOn() override;
  virtual void blinkOff() override;
  virtual void poststep() override;

private:
  // the fade amount
  uint8_t m_fadeAmount;

  // for random operations
  Random m_randCtx;

  LedStash m_stash;
};
#endif
