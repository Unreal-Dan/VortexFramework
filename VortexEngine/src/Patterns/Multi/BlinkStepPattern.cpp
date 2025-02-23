#include "BlinkStepPattern.h"

#include "../../VortexEngine.h"

#include "../../Serial/ByteStream.h"
#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

BlinkStepPattern::BlinkStepPattern(VortexEngine &engine, const PatternArgs &args) :
  MultiLedPattern(engine, args),
  m_blinkOnDuration(0),
  m_blinkOffDuration(0),
  m_stepDuration(0),
  m_blinkTimer(engine),
  m_stepTimer(engine)
{
  // BlinkStep is an abstract class it cannot be directly
  // instantiated so we do not need to assign a pattern id
  REGISTER_ARG(m_blinkOnDuration);
  REGISTER_ARG(m_blinkOffDuration);
  REGISTER_ARG(m_stepDuration);
}

BlinkStepPattern::~BlinkStepPattern()
{
}

// init the pattern to initial state
void BlinkStepPattern::init()
{
  MultiLedPattern::init();

  // reset the blink timer entirely
  m_blinkTimer.reset();
  // dops timing
  m_blinkTimer.addAlarm(m_blinkOnDuration);
  m_blinkTimer.addAlarm(m_blinkOffDuration);
  // start the blink timer from the next frame
  m_blinkTimer.start();

  // reset and add alarm
  m_stepTimer.reset();
  m_stepTimer.addAlarm(m_stepDuration);
  m_stepTimer.start();
}

// pure virtual must override the play function
void BlinkStepPattern::play()
{
  // whether to trigger a step event based on the step timer
  bool shouldStep = (m_stepTimer.alarm() == 0);
  if (shouldStep) {
    prestep();
  }
  switch (m_blinkTimer.alarm()) {
  case -1: // just break and still run post-step
    break;
  case 0: // turn on the leds for given mapping
    blinkOn();
    break;
  case 1: // turn off the leds
    blinkOff();
    break;
  }
  if (shouldStep) {
    poststep();
  }
}

void BlinkStepPattern::blinkOn()
{
  // override me
}

void BlinkStepPattern::blinkOff()
{
  // override me
  m_engine.leds().clearAll();
}

void BlinkStepPattern::prestep()
{
  // override me
}

void BlinkStepPattern::poststep()
{
  // override me
  m_colorset.getNext();
}
