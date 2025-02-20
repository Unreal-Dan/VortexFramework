#include "ChaserPattern.h"

// This controls the ratio of chaser dots to LED_COUNT. Default 1 chaser per 7 LEDs. Range: 1-LED_COUNT.
#define CHASER_RATIO 5


// This pattern aims to be a demonstration of the sequencer.
// There are always many ways to implement a pattern, it's best
// to choose the method that is most suitable for the pattern.
ChaserPattern::ChaserPattern(const PatternArgs &args) :
  SequencedPattern(args)
{
  setArgs(args);
  uint32_t numChasers = LED_COUNT / CHASER_RATIO;
  if (!numChasers) {
    numChasers = 1;
  }
  for (uint8_t i = 0; i < (LED_COUNT / numChasers); ++i) {
    PatternMap patMap(PATTERN_DOPS);
    for (uint8_t chaserCount = 0; chaserCount < numChasers; ++chaserCount) {
      patMap.setPatternAt(PATTERN_SOLID, MAP_LED((i + (chaserCount * CHASER_RATIO)) % LED_COUNT));
    }
    m_sequence.addStep(150, patMap);
  }
}
