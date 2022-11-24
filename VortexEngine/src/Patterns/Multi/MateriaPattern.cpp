#include "MateriaPattern.h"

#include "../../Serial/ByteStream.h"
#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

MateriaPattern::MateriaPattern(uint8_t onDuration1, uint8_t offDuration1, uint8_t onDuration2, uint8_t offDuration2, uint16_t stepSpeed) :
  MultiLedPattern(),
  m_onDuration1(onDuration1),
  m_offDuration1(offDuration1),
  m_onDuration2(onDuration2),
  m_offDuration2(offDuration2),
  m_stepSpeed(stepSpeed),
  m_stepTimer(),
  m_ledMap(0),
  m_switch(false)
{
}

MateriaPattern::~MateriaPattern()
{
}

// init the pattern to initial state
void MateriaPattern::init()
{
  MultiLedPattern::init();
  
  // start on index 1
  m_colorset.setCurIndex(1);
  // reset the blink timer entirely
  m_blinkTimer1.reset();
  // dops timing
  m_blinkTimer1.addAlarm(m_onDuration1);
  m_blinkTimer1.addAlarm(m_offDuration1);
  // start the blink timer from the next frame
  m_blinkTimer1.start();

  // reset the blink timer entirely
  m_blinkTimer2.reset();
  // dops timing
  m_blinkTimer2.addAlarm(m_onDuration2);
  m_blinkTimer2.addAlarm(m_offDuration2);
  // start the blink timer from the next frame
  m_blinkTimer2.start();

  // reset and add alarm
  m_stepTimer.reset();
  m_stepTimer.addAlarm(m_stepSpeed);
  m_stepTimer.start();

  // map of the Thumb, Index, Ring, and Pinkie Tops
  m_ledMap = MAP_LED(THUMB_TOP) | MAP_LED(INDEX_TOP) | MAP_LED(RING_TOP) | MAP_LED(PINKIE_TOP);
  
  //Set the middle top to solid color 0
  Leds::setIndex(MIDDLE_TOP, m_colorset.get(0));
}

// pure virtual must override the play function
void MateriaPattern::play()
{
  // get next color on stepTimer alarm, skip color 0
  if (m_stepTimer.alarm() == 0) {
    m_colorset.getNext();
    if (m_colorset.curIndex() == 0) {
      m_colorset.getNext();
    }
  }
  switch (m_blinkTimer1.alarm()) {
  case -1: // just break and still run post-step
    break;
  case 0: // turn on the leds for given mapping
    m_switch = !m_switch;
    // alternate between showing the current and next color
    if (m_switch) {
      // if this is the last color of the set, skip color 0 (color 0 is for Middle top only)
      if (m_colorset.curIndex() != m_colorset.numColors() - 1) {
        Leds::setAllTips(m_colorset.peekNext());
      } else {
        Leds::setAllTips(m_colorset.peek(2));
      }
    }
    else {
      Leds::setAllTips(m_colorset.cur());
    }
    break;
  case 1: // turn off the leds
    Leds::clearAllTips();
    break;
  }

  switch (m_blinkTimer2.alarm()) {
  case -1: // just break and still run post-step
    break;
  case 0: // turn on the leds for given mapping
    Leds::setMap(m_ledMap, m_colorset.cur());
    break;
  case 1: // turn off the leds
    Leds::clearMap(m_ledMap);
    break;
  }
 }

// must override the serialize routine to save the pattern
void MateriaPattern::serialize(ByteStream& buffer) const
{
  MultiLedPattern::serialize(buffer);
  buffer.serialize(m_onDuration1);
  buffer.serialize(m_offDuration1);
  buffer.serialize(m_onDuration2);
  buffer.serialize(m_offDuration2);
  buffer.serialize(m_stepSpeed);
}

void MateriaPattern::unserialize(ByteStream& buffer)
{
  MultiLedPattern::unserialize(buffer);
  buffer.serialize(m_onDuration1);
  buffer.serialize(m_offDuration1);
  buffer.serialize(m_onDuration2);
  buffer.serialize(m_offDuration2);
  buffer.serialize(m_stepSpeed);
}

#ifdef TEST_FRAMEWORK
void MateriaPattern::saveTemplate(int level) const
{
  MultiLedPattern::saveTemplate(level);
  IndentMsg(level + 1, "\"onDuration1\": %d,", m_onDuration1);
  IndentMsg(level + 1, "\"offDuration1\": %d,", m_offDuration1);
  IndentMsg(level + 1, "\"onDuration2\": %d,", m_onDuration2);
  IndentMsg(level + 1, "\"offDuration2\": %d,", m_offDuration2);
  IndentMsg(level + 1, "\"stepSpeed\": %d,", m_stepSpeed);
}
#endif


