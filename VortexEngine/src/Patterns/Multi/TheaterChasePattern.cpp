#include "TheaterChasePattern.h"

#include "../../VortexEngine.h"

#include "../../Leds/Leds.h"

#define THEATER_CHASE_STEPS 10

TheaterChasePattern::TheaterChasePattern(VortexEngine &engine, const PatternArgs &args) :
  BlinkStepPattern(engine, args),
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
  m_ledPositions = MAP_PAIR_ODD_EVENS;
  m_stepCounter = 0;
}

void TheaterChasePattern::blinkOn()
{
  m_engine.leds().setMap(m_ledPositions, m_colorset.getNext());
}

void TheaterChasePattern::poststep()
{
  // the first 5 steps are odd evens/odds alternating each step
  if (m_stepCounter < 5) {
    m_ledPositions = (m_stepCounter % 2) ? MAP_PAIR_ODD_ODDS : MAP_PAIR_ODD_EVENS;
  } else {
    // the end 5 steps are even evens/odds alternating each step
    m_ledPositions = (m_stepCounter % 2) ? MAP_PAIR_EVEN_ODDS : MAP_PAIR_EVEN_EVENS;
  }
  // increment step counter
  m_stepCounter = (m_stepCounter + 1) % THEATER_CHASE_STEPS;
}
