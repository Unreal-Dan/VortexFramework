#ifndef BLINKSTEP_PATTERN_H
#define BLINKSTEP_PATTERN_H

#include "MultiLedPattern.h"

#include "../../Time/Timer.h"
#include "../../Leds/LedTypes.h"

class BlinkStepPattern : public MultiLedPattern
{
protected:
  // You must derive from BlinkStep to use it
  BlinkStepPattern(VortexEngine &engine, const PatternArgs &args);

public:
  virtual ~BlinkStepPattern();

  // init the pattern to initial state
  virtual void init() override;

  // pure virtual must override the play function
  virtual void play() override;

protected:
  // overrideable members:
  virtual void blinkOn();  // when the leds blink on (must override)
  virtual void blinkOff(); // when the leds blink off
  virtual void prestep();  // when step triggers (before blink on)
  virtual void poststep(); // when step triggers (after blink on)

private:
  uint8_t m_blinkOnDuration;
  uint8_t m_blinkOffDuration;
  uint8_t m_stepDuration;
  Timer m_blinkTimer;
  Timer m_stepTimer;
};
#endif

