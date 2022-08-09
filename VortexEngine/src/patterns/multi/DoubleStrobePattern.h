#ifndef DOUBLESTROBE_PATTERN_H
#define DOUBLESTROBE_PATTERN_H

#include "multiledpattern.h"

#include "../../Timer.h"

class DoubleStrobePattern : public MultiLedPattern
{
public:
  DoubleStrobePattern();
  virtual ~DoubleStrobePattern();

  // init the pattern to initial state
  virtual void init() override;

  // pure virtual must override the play function
  virtual void play() override;

  // must override the serialize routine to save the pattern
  virtual void serialize(SerialBuffer& buffer) const override;
  virtual void unserialize(SerialBuffer& buffer) override;

private:
  Timer m_blinkTimer;
  Timer m_colorTimer;
};
#endif
