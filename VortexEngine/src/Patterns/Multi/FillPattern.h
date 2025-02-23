#ifndef FILL_PATTERN_H
#define FILL_PATTERN_H

#include "BlinkStepPattern.h"

#include "../../Time/Timings.h"

class FillPattern : public BlinkStepPattern
{
public:
  FillPattern(VortexEngine &engine, const PatternArgs &args);
  virtual ~FillPattern();

  virtual void init() override;

protected:
  virtual void blinkOn() override;
  virtual void poststep() override;

private:
  // how much the fill has progressed
  uint8_t m_progress;
};

#endif
