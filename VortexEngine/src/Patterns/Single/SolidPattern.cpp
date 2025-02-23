#include "SolidPattern.h"

SolidPattern::SolidPattern(VortexEngine &engine, const PatternArgs &args) :
  BasicPattern(engine, args),
  m_colIndex(0)
{
  m_patternID = PATTERN_SOLID;
  REGISTER_ARG(m_colIndex);
  setArgs(args);
}

// callbacks for blinking on/off, can be overridden by derived classes
void SolidPattern::onBlinkOn()
{
  if (m_colIndex == 0) {
    m_colorset.resetIndex();
  } else {
    m_colorset.setCurIndex(m_colIndex - 1);
  }
  BasicPattern::onBlinkOn();
}
