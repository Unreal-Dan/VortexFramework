#include "BackStrobePattern.h"

#include "../../VortexEngine.h"

BackStrobePattern::BackStrobePattern(VortexEngine &engine, const PatternArgs &args) :
  CompoundPattern(engine, args),
  m_stepSpeed(0),
  m_stepTimer(engine),
  m_switch(),
  m_firstPatternArgs(0, 0, 0),
  m_secondPatternArgs(0, 0, 0),
  m_firstPat(PATTERN_DOPS),
  m_secPat(PATTERN_STROBE)
{
  m_patternID = PATTERN_BACKSTROBE;
  REGISTER_ARG(m_firstPatternArgs.arg1);
  REGISTER_ARG(m_firstPatternArgs.arg2);
  REGISTER_ARG(m_firstPatternArgs.arg3);
  REGISTER_ARG(m_secondPatternArgs.arg1);
  REGISTER_ARG(m_secondPatternArgs.arg2);
  REGISTER_ARG(m_stepSpeed);
  REGISTER_ARG(m_firstPat);
  REGISTER_ARG(m_secPat);
  setArgs(args);
}

BackStrobePattern::~BackStrobePattern()
{
}

// init the pattern to initial state
void BackStrobePattern::init()
{
  CompoundPattern::init();

  // timer for switch
  m_stepTimer.reset();
  m_stepTimer.addAlarm(m_stepSpeed * 100);
  m_stepTimer.start();

  // initialize the sub patterns one time first
  setEvensOdds((PatternID)m_firstPat, (PatternID)m_secPat, &m_firstPatternArgs, &m_secondPatternArgs);
}

void BackStrobePattern::play()
{
  switch (m_engine.leds().ledCount()) {
  case 28:
    playOrbit();
    break;
  default:
    playNormal();
    break;
  }
}

void BackStrobePattern::playOrbit()
{
  // orbit version
}

void BackStrobePattern::playNormal()
{
  if (m_stepTimer.alarm() == 0) {
    // switch which patterns are displayed
    m_switch = !m_switch;
    // update the tip/top patterns based on the switch
    setEvensOdds(m_switch ? (PatternID)m_firstPat : (PatternID)m_secPat,
                m_switch ? (PatternID)m_secPat : (PatternID)m_firstPat,
                m_switch ? &m_firstPatternArgs : &m_secondPatternArgs,
                m_switch ? &m_secondPatternArgs : &m_firstPatternArgs);
  }
  CompoundPattern::play();
}
