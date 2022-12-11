#ifndef MATERIA_PATTERN_H
#define MATERIA_PATTERN_H

#include "MultiLedPattern.h"

#include "../../Time/Timer.h"
#include "../../Leds/LedTypes.h"

class MateriaPattern : public MultiLedPattern
{
public:
  MateriaPattern(uint8_t onDuration1 = 5, uint8_t offDuration1 = 8, uint8_t onDuration2 = 3, uint8_t offDuration2 = 35, uint16_t stepSpeed = 800);
; MateriaPattern(const PatternArgs &args);
  virtual ~MateriaPattern();

  // init the pattern to initial state
  virtual void init() override;

  // pure virtual must override the play function
  virtual void play() override;

  // must override the serialize routine to save the pattern
  virtual void serialize(ByteStream& buffer) const override;
  virtual void unserialize(ByteStream& buffer) override;

#if SAVE_TEMPLATE == 1
  virtual void saveTemplate(int level = 0) const override;
#endif

private:
  // blink durations
  uint8_t m_onDuration1;
  uint8_t m_offDuration1;
  uint8_t m_onDuration2;
  uint8_t m_offDuration2;
  // the speed for the step timer
  uint16_t m_stepSpeed;
  // the step timer
  Timer m_blinkTimer1;
  Timer m_blinkTimer2;
  Timer m_stepTimer;

  LedMap m_ledMap;

  bool m_switch;
};
#endif
