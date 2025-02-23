#include "BlinkStepPattern.h"

#include "../../Time/Timings.h"

class WarpPattern : public BlinkStepPattern
{
public:
  WarpPattern(VortexEngine &engine, const PatternArgs &args);
  virtual ~WarpPattern();

  // init the pattern to initial state
  virtual void init() override;

protected:
  virtual void blinkOn() override;
  virtual void poststep() override;

private:
  // how much the warp has progressed
  uint8_t m_progress;
};

