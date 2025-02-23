#ifndef SOLID_PATTERN_H
#define SOLID_PATTERN_H

#include "BasicPattern.h"

#include "../../Time/Timings.h"

class SolidPattern : public BasicPattern
{
public:
  SolidPattern(VortexEngine &engine, const PatternArgs &args);

protected:
  // callbacks for blinking on/off, can be overridden by derived classes
  virtual void onBlinkOn() override;

  uint8_t m_colIndex;
};

#endif
