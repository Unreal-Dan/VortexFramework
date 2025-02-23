#ifndef ZIGZAG_PATTERN_H
#define ZIGZAG_PATTERN_H

#include "MultiLedPattern.h"

#include "../../Time/Timings.h"
#include "../../Time/Timer.h"

class ZigzagPattern : public MultiLedPattern
{
public:
  ZigzagPattern(VortexEngine &engine, const PatternArgs &args);
  virtual ~ZigzagPattern();

  // init the pattern to initial state
  virtual void init() override;

  // pure virtual must override the play function
  virtual void play() override;

private:

  class Snake {
  public:
    Snake(VortexEngine &engine);

    void init(uint8_t onDuration, uint8_t offDuration, const Colorset &colorset, uint32_t colorOffset,
      uint8_t step = 0, uint8_t snakeSize = 1, uint8_t fadeAmount = 55, uint8_t changeBoundary = 3);
    void step();
    void draw();

  private:
    void drawSnake();

    VortexEngine &m_engine;
    Timer m_blinkTimer;
    Colorset m_colorset;
    uint8_t m_step;
    uint8_t m_snakeSize;
    uint8_t m_fadeAmount;
    uint8_t m_changeBoundary;
  };

  // path for leds to take, index this with m_step up to LED_COUNT steps
  static const LedPos ledStepPositions[];

  // blink on duration
  uint8_t m_onDuration;
  // blink off duration
  uint8_t m_offDuration;
  // how long each step takes
  uint8_t m_stepDuration;
  // the snake size
  uint8_t m_snakeSize;
  // the fade amount
  uint8_t m_fadeAmount;

  // the step timer
  Timer m_stepTimer;

  // the two snakes that zigzag
  Snake m_snake1;
  Snake m_snake2;
};

#endif
