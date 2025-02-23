#include "DoubleStrobePattern.h"

#include "../../Serial/ByteStream.h"

#include "../../Leds/Leds.h"

DoubleStrobePattern::DoubleStrobePattern(const PatternArgs &args) :
  BlinkStepPattern(args)
{
  m_patternID = PATTERN_DOUBLESTROBE;
  setArgs(args);
}

DoubleStrobePattern::~DoubleStrobePattern()
{
}

void DoubleStrobePattern::init()
{
  BlinkStepPattern::init();
  // start colorset at index 0 so cur() works
  m_colorset.setCurIndex(0);
}

void DoubleStrobePattern::blinkOn()
{
  Leds::setMap(MAP_RINGS_ODD, m_colorset.cur());
  Leds::setMap(MAP_RINGS_EVEN, m_colorset.peekNext());
}
