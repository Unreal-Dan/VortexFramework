#ifndef SPARKLETRACE_PATTERN_H
#define SPARKLETRACE_PATTERN_H

#include "BlinkStepPattern.h"

#include "../../Random/Random.h"

class SparkleTracePattern : public BlinkStepPattern
{
public:
  SparkleTracePattern(const PatternArgs &args);
  virtual ~SparkleTracePattern();

protected:
  virtual void blinkOn() override;
  virtual void blinkOff() override;
  virtual void poststep() override;

  Random m_randCtx;
};
#endif