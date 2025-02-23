#include "PulsishPattern.h"

#include "../../VortexEngine.h"

#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"

PulsishPattern::PulsishPattern(VortexEngine &engine, const PatternArgs &args) :
  MultiLedPattern(engine, args),
  m_progress(),
  m_stepDuration(0),
  m_stepTimer(engine),
  m_onDuration1(0),
  m_offDuration1(0),
  m_onDuration2(0),
  m_offDuration2(0),
  m_blinkTimer(engine),
  m_blink2Timer(engine)
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
    m_progress = (m_progress + 1) % PAIR_COUNT;
  }

  switch (m_blinkTimer.alarm()) {
  case -1: // just return
    return;
  case 0: // turn on the leds
    for (Pair pair = PAIR_FIRST; pair < PAIR_COUNT; ++pair) {
      if (pair != m_progress) {
        m_engine.leds().setPair(pair, m_colorset.cur());
      }
    }
    m_colorset.skip();
    if (m_colorset.curIndex() == 0) {
      m_colorset.skip();
    }
    break;
  case 1:
    for (Pair pair = PAIR_FIRST; pair < PAIR_COUNT; ++pair) {
      if (pair != m_progress) {
        m_engine.leds().clearPair(pair);
      }
    }
    break;
  }

  switch (m_blink2Timer.alarm()) {
  case -1: // just return
    return;
  case 0: // turn on the leds
    m_engine.leds().setPair((Pair)m_progress, m_colorset.get(0));
    break;
  case 1:
    m_engine.leds().clearPair((Pair)m_progress);
    break;
  }
}
