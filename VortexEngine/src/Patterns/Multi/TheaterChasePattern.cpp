#include "TheaterChasePattern.h"

#include "../../Leds/Leds.h"

#define THEATER_CHASE_STEPS LED_COUNT

TheaterChasePattern::TheaterChasePattern(const PatternArgs &args) :
  BlinkStepPattern(args),
  m_ledPositions(0),
  m_stepCounter(0)
{
  m_patternID = PATTERN_THEATER_CHASE;
  setArgs(args);
}

TheaterChasePattern::~TheaterChasePattern()
{
}

void TheaterChasePattern::init()
{
  BlinkStepPattern::init();
  // starts on odd evens
  m_ledPositions = MAP_PAIR_EVENS;
  m_stepCounter = 0;
}

void TheaterChasePattern::blinkOn()
{
  Leds::setMap(m_ledPositions, m_colorset.getNext());
}

void TheaterChasePattern::poststep()
{
  // the steps are odd evens/odds alternating each step
  m_ledPositions = (m_stepCounter % 2) ? MAP_PAIR_ODDS : MAP_PAIR_EVENS;
  // increment step counter
  m_stepCounter = (m_stepCounter + 1) % THEATER_CHASE_STEPS;
}
