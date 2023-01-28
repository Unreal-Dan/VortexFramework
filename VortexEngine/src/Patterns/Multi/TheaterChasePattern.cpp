#include "TheaterChasePattern.h"

#include "MultiLedPattern.h"

#include "../../Time/TimeControl.h"
#include "../../Colors/Colorset.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

#define THEATER_CHASE_STEPS 10

TheaterChasePattern::TheaterChasePattern(uint8_t onDuration, uint8_t offDuration, uint8_t stepDuration) :
  BlinkStepPattern(onDuration, offDuration, stepDuration),
  m_ledPositions(0),
  m_stepCounter(0)
{
  m_patternID = PATTERN_THEATER_CHASE;
}

TheaterChasePattern::TheaterChasePattern(const PatternArgs &args) :
  TheaterChasePattern()
{
  setArgs(args);
}

TheaterChasePattern::~TheaterChasePattern()
{
}

void TheaterChasePattern::init()
{
  BlinkStepPattern::init();
  // starts on odd tips
  m_ledPositions = MAP_FINGER_ODD_TIPS;
  m_stepCounter = 0;
}

void TheaterChasePattern::blinkOn()
{
  Leds::setMap(m_ledPositions, m_colorset.getNext());
}

void TheaterChasePattern::poststep()
{
  // the first 5 steps are odd tips/tops alternating each step
  if (m_stepCounter < 5) {
    m_ledPositions = (m_stepCounter % 2) ? MAP_FINGER_ODD_TOPS : MAP_FINGER_ODD_TIPS;
  } else {
    // the end 5 steps are even tips/tops alternating each step
    m_ledPositions = (m_stepCounter % 2) ? MAP_FINGER_EVEN_TOPS : MAP_FINGER_EVEN_TIPS;
  }
  // increment step counter
  m_stepCounter = (m_stepCounter + 1) % THEATER_CHASE_STEPS;
}
