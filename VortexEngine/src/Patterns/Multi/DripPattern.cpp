#include "DripPattern.h"

#include "../../VortexEngine.h"

#include "../../Leds/Leds.h"

DripPattern::DripPattern(VortexEngine &engine, const PatternArgs &args) :
  BlinkStepPattern(engine, args),
  m_sync(true)
{
  m_patternID = PATTERN_DRIP;
  setArgs(args);
}

DripPattern::~DripPattern()
{
}

void DripPattern::init()
{
  BlinkStepPattern::init();
  // reset sync bool
  m_sync = true;
  // start colorset at index 0 so cur() works
  m_colorset.setCurIndex(0);
}

void DripPattern::blinkOn()
{
  if (!m_sync) {
    m_engine.leds().setAllEvens(m_colorset.cur());
    m_engine.leds().setAllOdds(m_colorset.peekNext());
  } else {
    m_engine.leds().setAll(m_colorset.cur());
  }
}

void DripPattern::poststep()
{
  m_sync = !m_sync;
  if (m_sync) {
    m_colorset.getNext();
  }
}
