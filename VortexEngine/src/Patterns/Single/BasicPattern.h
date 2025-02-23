#ifndef BASIC_PATTERN_H
#define BASIC_PATTERN_H

#include "SingleLedPattern.h"
#include "../../Time/Timer.h"

class BasicPattern : public SingleLedPattern
{
public:
  BasicPattern(VortexEngine &engine, const PatternArgs &args);
  virtual ~BasicPattern();

  virtual void init() override;
  virtual void play() override;

protected:
  // when blinking off
  virtual void onBlinkOn();
  virtual void onBlinkOff();
  virtual void beginGap();
  virtual void beginDash();

  // iterate to next state
  void nextState(uint8_t timing);

  // the parameters of the pattern
  uint8_t m_onDuration;
  uint8_t m_offDuration;
  uint8_t m_gapDuration;
  uint8_t m_dashDuration;
  uint8_t m_groupSize;

  uint8_t m_groupCounter;

  // the various different blinking states the pattern can be in
  enum PatternState : uint8_t
  {
    // the led is disabled (there is no on or dash)
    STATE_DISABLED,

    // the pattern is blinking on the next color in the set
    STATE_BLINK_ON,
    STATE_ON,

    // the pattern is blinking off
    STATE_BLINK_OFF,
    STATE_OFF,

    // the pattern is starting a gap after a colorset
    STATE_BEGIN_GAP,
    STATE_IN_GAP,

    // the pattern is beginning a dash after a colorset or gap
    STATE_BEGIN_DASH,
    STATE_IN_DASH,

    // the pattern is starting a gap after a dash
    STATE_BEGIN_GAP2,
    STATE_IN_GAP2,
  };

  // the state of the current pattern
  PatternState m_state;

  // the blink timer used to measure blink timings
  Timer m_blinkTimer;
};

#endif
