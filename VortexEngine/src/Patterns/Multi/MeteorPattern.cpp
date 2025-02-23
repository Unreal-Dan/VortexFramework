#include "MeteorPattern.h"

#include "../../VortexEngine.h"

#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

MeteorPattern::MeteorPattern(VortexEngine &engine, const PatternArgs &args) :
  BlinkStepPattern(engine, args),
  m_fadeAmount(0),
  m_randCtx(),
  m_stash(engine)
{
  m_patternID = PATTERN_METEOR;
  REGISTER_ARG(m_fadeAmount);
  setArgs(args);
}

MeteorPattern::~MeteorPattern()
{
}

void MeteorPattern::blinkOn()
{
  for (int i = 0; i < LED_COUNT; ++i) {
    m_stash[i].adjustBrightness(m_fadeAmount);
  }
  m_engine.leds().restoreAll(m_stash);
}

void MeteorPattern::blinkOff()
{
  m_engine.leds().clearAll();
}

void MeteorPattern::poststep()
{
  for (uint8_t meteorCount = 0; meteorCount < (LED_COUNT / 2); ++meteorCount) {
    // when a new meteor is created it is incerted into the stash so the blinking pattern is not interrupted
    LedPos target = (LedPos)m_randCtx.next8(LED_FIRST, LED_LAST);
    RGBColor col = m_colorset.getNext();
    m_stash.setIndex(target, col);
  }
}
