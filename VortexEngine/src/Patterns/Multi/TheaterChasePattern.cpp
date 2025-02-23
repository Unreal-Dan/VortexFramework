#include "TheaterChasePattern.h"

#include "../../Leds/Leds.h"

#define THEATER_CHASE_STEPS (LED_COUNT / 4)

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
  m_ledPositions = MAP_OPPOSITES_1;
  m_stepCounter = 0;
}

void TheaterChasePattern::blinkOn()
{
  Leds::setMap(m_ledPositions, m_colorset.getNext());
}

void TheaterChasePattern::poststep()
{
  if (m_stepCounter == 0) {
    m_ledPositions = MAP_OPPOSITES_1;
  }
  if (m_stepCounter == 1) {
    m_ledPositions = MAP_OPPOSITES_2;
  }
  if (m_stepCounter == 2) {
    m_ledPositions = MAP_OPPOSITES_3;
  }
  if (m_stepCounter == 3) {
    m_ledPositions = MAP_OPPOSITES_4;
  }
  if (m_stepCounter == 4) {
    m_ledPositions = MAP_OPPOSITES_5;
  }
  // increment step counter
  m_stepCounter = (m_stepCounter + 1) % THEATER_CHASE_STEPS;
}
