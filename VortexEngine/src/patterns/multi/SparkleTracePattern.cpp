#include "SparkleTracePattern.h"

#include "../../SerialBuffer.h"
#include "../../TimeControl.h"
#include "../../Leds.h"
#include "../../Log.h"

SparkleTracePattern::SparkleTracePattern() :
  BlinkStepPattern(5, 0, 50)
{
}

SparkleTracePattern::~SparkleTracePattern()
{
}

void SparkleTracePattern::blinkOn()
{
  Leds::setAll(m_colorset.get(0));
}

void SparkleTracePattern::poststep()
{
  for (int dot = 0; dot < 4; ++dot) {
    Leds::setFinger((Finger)random(FINGER_FIRST, FINGER_LAST + 1), m_colorset.cur());
  }
  m_colorset.skip();
  if (m_colorset.curIndex() == 0) {
    m_colorset.skip();
  }
}
