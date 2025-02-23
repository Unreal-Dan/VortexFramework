#include "DripMorphPattern.h"

#include "../../Serial/ByteStream.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

DripMorphPattern::DripMorphPattern(const PatternArgs &args) :
  MultiLedPattern(args),
  m_blinkOnDuration(0),
  m_blinkOffDuration(0),
  m_blinkTimer(),
  m_speed(0),
  m_cur(),
  m_next()
{
  m_patternID = PATTERN_DRIPMORPH;
  REGISTER_ARG(m_blinkOnDuration);
  REGISTER_ARG(m_blinkOffDuration);
  REGISTER_ARG(m_speed);
  setArgs(args);
}

DripMorphPattern::~DripMorphPattern()
{
}

void DripMorphPattern::init()
{
  MultiLedPattern::init();
  // reset the blink timer entirely
  m_blinkTimer.reset();
  // dops timing
  m_blinkTimer.addAlarm(m_blinkOnDuration);
  m_blinkTimer.addAlarm(m_blinkOffDuration);
  // start the blink timer from the next frame
  m_blinkTimer.start();

  // convert current/next colors to HSV
  m_cur = m_colorset.getNext();
  m_next = m_colorset.getNext();
}

// pure virtual must override the play function
void DripMorphPattern::play()
{
  switch (m_blinkTimer.alarm()) {
  case -1: // just break
    break;
  case 0: // turn on the leds for given mapping
    blinkOn();
    break;
  case 1: // turn off the leds
    blinkOff();
    break;
  }
}

void DripMorphPattern::blinkOn()
{
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
  m_cur.hue += m_speed * sign;
  // set the target led with the current HSV color
  Leds::setMap(MAP_RINGS_EVEN, m_cur);
  Leds::setMap(MAP_RINGS_ODD, m_colorset.cur());
}

void DripMorphPattern::blinkOff()
{
  Leds::clearAll();
}
