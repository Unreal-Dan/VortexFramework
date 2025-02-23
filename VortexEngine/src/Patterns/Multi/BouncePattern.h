#include "BlinkStepPattern.h"

#include "../../Time/Timings.h"

class BouncePattern : public BlinkStepPattern
{
public:
  BouncePattern(VortexEngine &engine, const PatternArgs &args);
  virtual ~BouncePattern();

  // init the pattern to initial state
  virtual void init() override;

protected:
  virtual void blinkOn() override;
  virtual void poststep() override;

private:
  // how much the warp has progressed
  uint8_t m_progress;
  // the fade amount
  uint8_t m_fadeAmount;
};

