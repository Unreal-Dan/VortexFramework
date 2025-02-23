#include "HueShiftPattern.h"

#include "../../VortexEngine.h"

#include "../../Serial/ByteStream.h"
#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

HueShiftPattern::HueShiftPattern(VortexEngine &engine, const PatternArgs &args) :
  MultiLedPattern(engine, args),
  m_blinkOnDuration(0),
  m_blinkOffDuration(0),
  m_blendDelay(0),
  m_delayCounter(0),
  m_blinkTimer(engine),
  m_cur(0),
  m_next(0)
{
  m_patternID = PATTERN_HUE_SCROLL;
  REGISTER_ARG(m_blinkOnDuration);
  REGISTER_ARG(m_blinkOffDuration);
  REGISTER_ARG(m_blendDelay);
  setArgs(args);
}

HueShiftPattern::~HueShiftPattern()
{
}

// init the pattern to initial state
void HueShiftPattern::init()
{
  MultiLedPattern::init();
  m_cur = m_colorset.getNext();
  m_next = m_colorset.getNext();
  // reset the blink timer entirely
  m_blinkTimer.reset();
  // dops timing
  m_blinkTimer.addAlarm(m_blinkOnDuration);
  m_blinkTimer.addAlarm(m_blinkOffDuration);
  // start the blink timer from the next frame
  m_blinkTimer.start();
}

// pure virtual must override the play function
void HueShiftPattern::play()
{
  switch (m_blinkTimer.alarm()) {
  case 0: // turn on the leds for given mapping
    break;
  case 1: // turn off the leds
    m_engine.leds().clearAll();
  case -1: // just break and still run post-step
    return;
  }

  // if the current hue has reached the next hue
  if (m_cur.hue == m_next.hue) {
    // copy over the sat/val
    m_cur = m_next;
    // get the next color and convert to hsv
    m_next = m_colorset.getNext();
  }
  // check which direction is closer for the next hue
  // tbh idk really how this even works anymore but it does
  int sign = ((((m_next.hue + 255) - m_cur.hue) % 256) <= 128) ? 1 : -1;
  // move hue in the direction of next hue at chosen speed
  // NOTE: if the speed isn't a multiple of the hue values then
  //       it will cause oscillation around the target hue
  //       because it will never reach the target hue and
  //       always over/under shoot
  // only increment every blendDelay times
  ++m_delayCounter;
  if (m_delayCounter >= m_blendDelay) {
    m_delayCounter = 0;
    m_cur.hue += sign;
  }
  HSVColor showColor = HSVColor(m_cur.hue, 255, 255);
  // set the target led with the current HSV color
  for (LedPos pos = LED_FIRST; pos < LED_COUNT; ++pos) {
    m_engine.leds().setIndex(pos, hsv_to_rgb_generic(showColor));
    showColor.hue = (showColor.hue + 5) % 256;
  }
}
