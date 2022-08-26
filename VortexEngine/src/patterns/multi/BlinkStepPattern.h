#ifndef BLINKSTEP_PATTERN_H
#define BLINKSTEP_PATTERN_H

#include "MultiLedPattern.h"

#include "../../Timer.h"
#include "../../LedTypes.h"

class BlinkStepPattern : public MultiLedPattern
{
public:
  BlinkStepPattern(uint8_t blinkOn, uint8_t blinkOff, uint8_t stepDuration);
  virtual ~BlinkStepPattern();

  // init the pattern to initial state
  virtual void init() override;

  // pure virtual must override the play function
  virtual void play() override;

  // must override the serialize routine to save the pattern
  virtual void serialize(SerialBuffer& buffer) const override;
  virtual void unserialize(SerialBuffer& buffer) override;

protected:
  // overrideable members:
  virtual void blinkOn();  // when the leds blink on
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

