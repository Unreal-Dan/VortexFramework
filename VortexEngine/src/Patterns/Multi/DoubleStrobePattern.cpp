#include "DoubleStrobePattern.h"

#include "../../VortexEngine.h"

#include "../../Serial/ByteStream.h"

#include "../../Leds/Leds.h"

DoubleStrobePattern::DoubleStrobePattern(VortexEngine &engine, const PatternArgs &args) :
  BlinkStepPattern(engine, args)
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
  m_engine.leds().setAllOdds(m_colorset.cur());
  m_engine.leds().setAllEvens(m_colorset.peekNext());
}
