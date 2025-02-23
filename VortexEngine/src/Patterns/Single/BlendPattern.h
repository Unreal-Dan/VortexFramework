#ifndef BLEND_PATTERN_H
#define BLEND_PATTERN_H

#include "BasicPattern.h"

#include "../../Time/Timings.h"

class BlendPattern : public BasicPattern
{
public:
  BlendPattern(VortexEngine &engine, const PatternArgs &args);
  virtual ~BlendPattern();

  virtual void init() override;

protected:
  // only override the onBlinkOn so we can control the color it blinks
  virtual void onBlinkOn() override;
  void interpolate(uint8_t &current, const uint8_t next);

  // offset of starting hue and number of flips
  uint8_t m_blendSpeed;
  uint8_t m_numFlips;

  // current color and target blend color
  RGBColor m_cur;
  RGBColor m_next;

  // the current flip counter
  uint8_t m_flip;
};

#endif

