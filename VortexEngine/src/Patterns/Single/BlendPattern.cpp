#include "BlendPattern.h"

#include "../../VortexEngine.h"

#include "../../Time/TimeControl.h"
#include "../../Colors/Colorset.h"
#include "../../Leds/Leds.h"

BlendPattern::BlendPattern(VortexEngine &engine, const PatternArgs &args) :
  BasicPattern(engine, args),
  m_blendSpeed(0),
  m_numFlips(0),
  m_cur(),
  m_next(),
  m_flip(0)
{
  m_patternID = PATTERN_BLEND;
  REGISTER_ARG(m_blendSpeed);
  REGISTER_ARG(m_numFlips);
  setArgs(args);
}

BlendPattern::~BlendPattern()
{
}

void BlendPattern::init()
{
  // run basic pattern init logic
  BasicPattern::init();
  // convert current/next colors to HSV
  m_cur = m_colorset.getNext();
  m_next = m_colorset.getNext();
  // reset the flip count
  m_flip = 0;
}

void BlendPattern::onBlinkOn()
{
  if (m_cur == m_next) {
    m_next = m_colorset.getNext();
  }
  interpolate(m_cur.red, m_next.red);
  interpolate(m_cur.green, m_next.green);
  interpolate(m_cur.blue, m_next.blue);
  RGBColor col = m_cur;
  // it should be impossible for m_numFlips to be 0 and m_flip to be 1
  // unless the pattern arg is changed mid-play which could happen with
  // and editor or something, so check both for 0
  if (m_flip && m_numFlips) {
    // convert to hsv
    HSVColor hsvCol = m_cur;
    // shift the hue by a flip size
    hsvCol.hue += (m_flip * (127 / m_numFlips));
    // convert the hsv color back to RGB
    col = hsvCol;
  }
  // set the color
  m_engine.leds().setIndex(m_ledPos, col);
  // increment the flip count
  m_flip++;
  // modulate the flip count DO NOT USE MODULO OPERATOR BECAUSE
  // THE FLIP COUNT COULD BE 0 THAT WILL DIVIDE BY ZERO
  if (m_flip > m_numFlips) {
    m_flip = 0;
  }
}

void BlendPattern::interpolate(uint8_t &current, const uint8_t next)
{
  if (current < next) {
    uint8_t step = (next - current) > m_blendSpeed ? m_blendSpeed : (next - current);
    current += step;
  } else if (current > next) {
    uint8_t step = (current - next) > m_blendSpeed ? m_blendSpeed : (current - next);
    current -= step;
  }
}