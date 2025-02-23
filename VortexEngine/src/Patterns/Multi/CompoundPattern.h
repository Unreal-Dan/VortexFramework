#ifndef HYBRID_PATTERN_H
#define HYBRID_PATTERN_H

#include "MultiLedPattern.h"

#include <vector>

class SingleLedPattern;

// A hybrid pattern is a type of multi-led pattern that works by displaying
// pre-existing patterns on different leds simultaneously
class CompoundPattern : public MultiLedPattern
{
protected:
  // CompoundPattern is an abstract class
  CompoundPattern(VortexEngine &engine, const PatternArgs &args);

public:
  virtual ~CompoundPattern();

  // init the pattern to initial state
  virtual void init() override;

  // play the hybrid pattern (play all led patterns)
  virtual void play() override;

protected:
  // set a pattern at an index of the hybrid pattern (optionally replace colorset)
  void setPatternAt(LedPos pos, SingleLedPattern *pat, const Colorset *set = nullptr);
  void setPatternAt(LedPos pos, PatternID id, const PatternArgs *args = nullptr, const Colorset *set = nullptr);
  void clearPatterns();

  // change the patterns on the evens/odds respectively
  void setEvensOdds(PatternID tipPattern, PatternID topPattern,
    const PatternArgs *tipArgs = nullptr, const PatternArgs *topArgs = nullptr);

  // array of single LED patterns, one for each LED
  std::vector<SingleLedPattern *> m_ledPatterns;
};

#endif
