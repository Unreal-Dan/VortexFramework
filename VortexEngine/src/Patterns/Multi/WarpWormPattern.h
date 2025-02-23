#include "BlinkStepPattern.h"

#include "../../Time/Timings.h"

class WarpWormPattern : public BlinkStepPattern
{
public:
  WarpWormPattern(VortexEngine &engine, const PatternArgs &args);
  virtual ~WarpWormPattern();

  // init the pattern to initial state
  virtual void init() override;

protected:
  virtual void blinkOn() override;
  virtual void poststep() override;

private:
  // how much the warp has progressed
  uint8_t m_progress;
};

