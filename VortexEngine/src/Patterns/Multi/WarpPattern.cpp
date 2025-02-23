#include "WarpPattern.h"

#include "../../VortexEngine.h"

#include "../../Serial/ByteStream.h"
#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

WarpPattern::WarpPattern(VortexEngine &engine, const PatternArgs &args) :
  BlinkStepPattern(engine, args),
  m_progress(0)
{
  m_patternID = PATTERN_WARP;
  setArgs(args);
}

WarpPattern::~WarpPattern()
{
}

// init the pattern to initial state
void WarpPattern::init()
{
  BlinkStepPattern::init();
  // reset progress
  m_progress = 0;
  // start colorset at index 0 so cur() works
  m_colorset.setCurIndex(0);
}

void WarpPattern::blinkOn()
{
  m_engine.leds().setAll(m_colorset.cur());
  m_engine.leds().setPair((Pair)m_progress, m_colorset.peekNext());
}

void WarpPattern::poststep()
{
  m_progress = (m_progress + 1) % PAIR_COUNT;
  if (m_progress == 0) {
    m_colorset.getNext();
  }
}
