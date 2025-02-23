#include "MultiLedPattern.h"

#include "../../Time/Timings.h"
#include "../../Time/Timer.h"

class PulsishPattern : public MultiLedPattern
{
public:
  PulsishPattern(VortexEngine &engine, const PatternArgs &args);
  virtual ~PulsishPattern();

  // init the pattern to initial state
  virtual void init() override;

  // pure virtual must override the play function
  virtual void play() override;

private:
  // how much the warp has progressed
  uint8_t m_progress;

  // the step duration
  uint8_t m_stepDuration;
  // the step timer
  Timer m_stepTimer;
  // the on and off durations
  uint8_t m_onDuration1;
  uint8_t m_offDuration1;
  uint8_t m_onDuration2;
  uint8_t m_offDuration2;
  // the blink timer
  Timer m_blinkTimer;
  Timer m_blink2Timer;
};
