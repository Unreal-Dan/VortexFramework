#include "PulsishPattern.h"

#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"

PulsishPattern::PulsishPattern(const PatternArgs &args) :
  MultiLedPattern(args),
  m_progress(),
  m_stepDuration(0),
  m_stepTimer(),
  m_onDuration1(0),
  m_offDuration1(0),
  m_onDuration2(0),
  m_offDuration2(0),
  m_blinkTimer(),
  m_blink2Timer()
{
  m_patternID = PATTERN_PULSISH;
  REGISTER_ARG(m_onDuration1);
  REGISTER_ARG(m_offDuration1);
  REGISTER_ARG(m_onDuration2);
  REGISTER_ARG(m_offDuration2);
  REGISTER_ARG(m_stepDuration);
  setArgs(args);
}

PulsishPattern::~PulsishPattern()
{
}

// init the pattern to initial state
void PulsishPattern::init()
{
  MultiLedPattern::init();

  // reset and add alarm
  m_stepTimer.reset();
  m_stepTimer.addAlarm(m_stepDuration);
  m_stepTimer.start();

  // make blink timer
  m_blinkTimer.reset();
  m_blinkTimer.addAlarm(m_onDuration1);
  m_blinkTimer.addAlarm(m_offDuration1);
  m_blinkTimer.start();

  // make 2nd blink timer
  m_blink2Timer.reset();
  m_blink2Timer.addAlarm(m_onDuration2);
  m_blink2Timer.addAlarm(m_offDuration2);
  m_blink2Timer.start();
}
// pure virtual must override the play function
void PulsishPattern::play()
{
  // when the step timer triggers
  if (m_stepTimer.alarm() == 0) {
    m_progress = (m_progress + 1) % LED_COUNT;
  }

  switch (m_blinkTimer.alarm()) {
  case -1: // just return
    return;
  case 0: // turn on the leds
    for (LedPos pos = LED_FIRST; pos < LED_COUNT; ++pos) {
      if (pos != m_progress) {
        Leds::setIndex(pos, m_colorset.cur());
      }
    }
    m_colorset.skip();
    if (m_colorset.curIndex() == 0) {
      m_colorset.skip();
    }
    break;
  case 1:
    for (LedPos pos = LED_FIRST; pos < LED_COUNT; ++pos) {
      if (pos != m_progress) {
        Leds::clearIndex(pos);
      }
    }
    break;
  }

  switch (m_blink2Timer.alarm()) {
  case -1: // just return
    return;
  case 0: // turn on the leds
    Leds::setIndex((LedPos)m_progress, m_colorset.get(0));
    break;
  case 1:
    Leds::clearIndex((LedPos)m_progress);
    break;
  }
}
