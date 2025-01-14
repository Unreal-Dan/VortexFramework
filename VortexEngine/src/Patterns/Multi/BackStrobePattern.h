#ifndef BACKSTROBE_PATTERN_H
#define BACKSTROBE_PATTERN_H

#include "CompoundPattern.h"

#include "../../Time/Timer.h"
#include "../../Time/Timings.h"

class BackStrobePattern : public CompoundPattern
{
public:
  BackStrobePattern(VortexEngine &engine, const PatternArgs &args);
  virtual ~BackStrobePattern();

  // init the pattern to initial state
  virtual void init() override;
  virtual void play() override;

private:
  void playOrbit();
  void playNormal();

  // the speed for the step timer in x100 ms chunks, so a value of 10
  // will be 1000ms
  uint8_t m_stepSpeed;
  // the step timer
  Timer m_stepTimer;

  bool m_switch;

  PatternArgs m_firstPatternArgs;
  PatternArgs m_secondPatternArgs;
  uint8_t m_firstPat;
  uint8_t m_secPat;
};

#endif
