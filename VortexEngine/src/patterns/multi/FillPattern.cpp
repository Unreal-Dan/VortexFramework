#include "FillPattern.h"

#include "../../SerialBuffer.h"
#include "../../TimeControl.h"
#include "../../Leds.h"
#include "../../Log.h"

FillPattern::FillPattern() :
  BlinkStepPattern(3, 12, 50),
  m_progress()
{
}

FillPattern::~FillPattern()
{
}

void FillPattern::init()
{
  BlinkStepPattern::init();
  // start colorset at index 0 so cur() works
  m_colorset.setCurIndex(0);
}

void FillPattern::blinkOn()
{
  Leds::setFingers(FINGER_FIRST, (Finger)m_progress, m_colorset.peekNext());
  Leds::setFingers((Finger)m_progress, FINGER_COUNT, m_colorset.cur());
}

void FillPattern::poststep()
{
  m_progress = (m_progress + 1) % FINGER_COUNT;
  if (m_progress == 0) {
    m_colorset.getNext();
  }
}
