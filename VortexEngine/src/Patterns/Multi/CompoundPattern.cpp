#include "CompoundPattern.h"

#include "../Single/SingleLedPattern.h"
#include "../../Colors/Colorset.h"
#include "../PatternBuilder.h"

CompoundPattern::CompoundPattern(const PatternArgs &args) :
  MultiLedPattern(args),
  m_ledPatterns()
{
  // Hybrid is an abstract class it cannot be directly
  // instantiated so we do not need to assign a pattern id
}

CompoundPattern::~CompoundPattern()
{
  // clean up all the sub patterns
  for (LedPos pos = LED_FIRST; pos <= LED_LAST; pos++) {
    if (m_ledPatterns[pos]) {
      delete m_ledPatterns[pos];
      m_ledPatterns[pos] = nullptr;
    }
  }
}

// init the pattern to initial state
void CompoundPattern::init()
{
  MultiLedPattern::init();
}

// play the hybrid pattern (play all led patterns)
void CompoundPattern::play()
{
  for (LedPos pos = LED_FIRST; pos <= LED_LAST; pos++) {
    SingleLedPattern *pat = m_ledPatterns[pos];
    if (!pat) {
      // should never happen, but just in case
      continue;
    }
    pat->play();
  }
}

void CompoundPattern::clearPatterns()
{
  for (LedPos pos = LED_FIRST; pos <= LED_LAST; pos++) {
    if (!m_ledPatterns[pos]) {
      continue;
    }
    delete m_ledPatterns[pos];
    m_ledPatterns[pos] = nullptr;
  }
}

void CompoundPattern::setPatternAt(LedPos pos, SingleLedPattern *pat,
  const Colorset *set)
{
  if (!pat || pos >= LED_COUNT) {
    return;
  }
  pat->bind(pos);
  pat->setColorset(set ? *set : m_colorset);
  pat->init();
  // handle re-initialization and prevent leaks
  if (m_ledPatterns[pos]) {
    delete m_ledPatterns[pos];
  }
  // store the pattern for the given led
  m_ledPatterns[pos] = pat;
}

void CompoundPattern::setPatternAt(LedPos pos, PatternID id,
  const PatternArgs *args, const Colorset *set)
{
  if (pos >= LED_COUNT) {
    return;
  }
  setPatternAt(pos, PatternBuilder::makeSingle(id, args), set);
}

void CompoundPattern::setEvensOdds(PatternID evenPattern, PatternID oddPattern,
  const PatternArgs *evenArgs, const PatternArgs *oddArgs)
{
  // Set the evenPattern on all evens and oddPattern on all odds
  for (LedPos p = LED_FIRST; p <= LED_LAST; p++) {
    const PatternArgs *args = isOrbitEven(p) ? evenArgs : oddArgs;
    PatternID id = isOrbitEven(p) ? evenPattern : oddPattern;
    setPatternAt(p, PatternBuilder::makeSingle(id, args));
  }
}
