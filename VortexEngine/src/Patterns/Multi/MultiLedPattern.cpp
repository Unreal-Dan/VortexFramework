#include "MultiLedPattern.h"

#include "../../VortexEngine.h"

MultiLedPattern::MultiLedPattern(VortexEngine &engine, const PatternArgs &args) :
  Pattern(engine, args)
{
  // this is a multi led pattern
  m_patternFlags |= PATTERN_FLAG_MULTI;

  // MultiLed is an abstract class it cannot be directly
  // instantiated so we do not need to assign a pattern id
}

MultiLedPattern::~MultiLedPattern()
{
}

void MultiLedPattern::bind(LedPos pos)
{
  // the 'LedPos' is LED_COUNT when binding a multi-led pattern
  // as opposed to being an individual LED when it's single
  Pattern::bind(LED_COUNT);
}

// init the pattern to initial state
void MultiLedPattern::init()
{
  Pattern::init();
}
