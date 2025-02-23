#include "BlinkStepPattern.h"

#include "../../Time/Timings.h"

class SnowballPattern : public BlinkStepPattern
{
public:
  SnowballPattern(VortexEngine &engine, const PatternArgs &args);
  virtual ~SnowballPattern();

  // init the pattern to initial state
  virtual void init() override;

protected:
  virtual void blinkOn() override;
  virtual void poststep() override;

private:
  // how much the warp has progressed
  uint8_t m_progress;
};
