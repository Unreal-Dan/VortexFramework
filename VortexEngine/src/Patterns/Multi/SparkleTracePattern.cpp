#include "SparkleTracePattern.h"

#include "../../VortexEngine.h"

#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

SparkleTracePattern::SparkleTracePattern(VortexEngine &engine, const PatternArgs &args) :
  BlinkStepPattern(engine, args),
  m_randCtx()
{
  m_patternID = PATTERN_SPARKLETRACE;
  setArgs(args);
}

SparkleTracePattern::~SparkleTracePattern()
{
}

void SparkleTracePattern::blinkOn()
{
  m_engine.leds().setAll(m_colorset.get(0));
}

void SparkleTracePattern::poststep()
{
  for (uint8_t dot = 0; dot < 4; ++dot) {
    m_engine.leds().setPair((Pair)m_randCtx.next8(PAIR_FIRST, PAIR_LAST), m_colorset.cur());
  }
  m_colorset.skip();
  if (m_colorset.curIndex() == 0) {
    m_colorset.skip();
  }
}
