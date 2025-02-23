#include "CrossDopsPattern.h"

#include "../../Leds/Leds.h"

CrossDopsPattern::CrossDopsPattern(const PatternArgs &args) :
  BlinkStepPattern(args),
  m_ledMap(0)
{
  m_patternID = PATTERN_CROSSDOPS;
  setArgs(args);
}

CrossDopsPattern::~CrossDopsPattern()
{
}

// init the pattern to initial state
void CrossDopsPattern::init()
{
  BlinkStepPattern::init();
  // Alternating evens and odds mapping of leds to turn on/off
  m_ledMap = MAP_LINE_1;
  // start colorset at index 0 so cur() works
  m_colorset.setCurIndex(0);
}

void CrossDopsPattern::blinkOn()
{
  // set the current color on all the given leds
  Leds::setMap(m_ledMap, m_colorset.cur());
}

void CrossDopsPattern::poststep()
{
  // iterate to next color and invert of all the evens/odds
  m_colorset.getNext();
  m_ledMap = MAP_INVERSE(m_ledMap);
}
