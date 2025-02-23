#ifndef HUE_SHIFT_PATTERN_H
#define HUE_SHIFT_PATTERN_H

#include "MultiLedPattern.h"

#include "../../Time/Timings.h"
#include "../../Time/Timer.h"

class HueShiftPattern : public MultiLedPattern
{
public:
  HueShiftPattern(VortexEngine &engine, const PatternArgs &args);
  virtual ~HueShiftPattern();

  // init the pattern to initial state
  virtual void init() override;

  // pure virtual must override the play function
  virtual void play() override;

private:
  uint8_t m_blinkOnDuration;
  uint8_t m_blinkOffDuration;
  uint8_t m_blendDelay;
  uint8_t m_delayCounter;

  Timer m_blinkTimer;

  // current color and target blend color
  HSVColor m_cur;
  HSVColor m_next;
};
#endif
