#ifndef VORTEX_PATTERN_H
#define VORTEX_PATTERN_H

#include "BlinkStepPattern.h"

#include "../../Time/Timings.h"

class VortexPattern : public BlinkStepPattern
{
public:
  VortexPattern(VortexEngine &engine, const PatternArgs& args);
  virtual ~VortexPattern();

  // init the pattern to initial state
  virtual void init() override;

protected:
  virtual void blinkOn() override;
  virtual void poststep() override;

private:
  // how much the fill has progressed
  uint8_t m_progress;
};

#endif
