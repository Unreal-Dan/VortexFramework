#include "WarpWormPattern.h"

#include "../../Serial/ByteStream.h"
#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

WarpWormPattern::WarpWormPattern(const PatternArgs &args) :
  BlinkStepPattern(args),
  m_progress(0)
{
  m_patternID = PATTERN_WARPWORM;
  setArgs(args);
}

WarpWormPattern::~WarpWormPattern()
{
}

// init the pattern to initial state
void WarpWormPattern::init()
{
  BlinkStepPattern::init();
  // reset progress
  m_progress = 0;
  // start colorset at index 0 so cur() works
  m_colorset.setCurIndex(1);
}

void WarpWormPattern::blinkOn()
{
  int wormSize = LED_COUNT / 3;
  Leds::setAll(m_colorset.get(0));
  for (int body = 0; body < wormSize; ++body) {
    if (body + m_progress < LED_COUNT) {
      Leds::setIndex((LedPos)(body + m_progress), m_colorset.cur());
    } else {
      RGBColor col = m_colorset.peekNext();
      if (m_colorset.curIndex() == m_colorset.numColors() - 1) {
        col = m_colorset.peek(2);
      }
      Leds::setIndex((LedPos)((body + m_progress) % LED_COUNT), col);
    }
  }
}

void WarpWormPattern::poststep()
{
  m_progress = (m_progress + 1) % LED_COUNT;
  if (m_progress == 0) {
    m_colorset.getNext();
    if (m_colorset.curIndex() == 0) {
      m_colorset.getNext();
    }
  }
}
