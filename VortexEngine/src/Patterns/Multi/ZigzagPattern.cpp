#include "ZigzagPattern.h"

#include "../../VortexEngine.h"

#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

// Mapping of LED positions to steps.
// The lights runs across evens, then back across odds.
// Index this array with m_step in order to get correct LedPos
const LedPos ZigzagPattern::ledStepPositions[] = {
  LED_1,
  LED_3,
  LED_5,
  LED_7,
  LED_9,

  LED_8,
  LED_6,
  LED_4,
  LED_2,
  LED_0,
};

// There just happens to be LED_COUNT steps in the pattern
#define NUM_ZIGZAG_STEPS (sizeof(ledStepPositions) / sizeof(ledStepPositions[0]))
#define HALF_ZIGZAG_STEPS (NUM_ZIGZAG_STEPS / 2)

ZigzagPattern::ZigzagPattern(VortexEngine &engine, const PatternArgs &args) :
  MultiLedPattern(engine, args),
  m_onDuration(0),
  m_offDuration(0),
  m_stepDuration(0),
  m_snakeSize(0),
  m_fadeAmount(0),
  m_stepTimer(engine),
  m_snake1(engine),
  m_snake2(engine)
{
  m_patternID = PATTERN_ZIGZAG;
  REGISTER_ARG(m_onDuration);
  REGISTER_ARG(m_offDuration);
  REGISTER_ARG(m_stepDuration);
  REGISTER_ARG(m_snakeSize);
  REGISTER_ARG(m_fadeAmount);
  setArgs(args);
}

ZigzagPattern::~ZigzagPattern()
{
}

// init the pattern to initial state
void ZigzagPattern::init()
{
  MultiLedPattern::init();

  // reset and add alarm
  m_stepTimer.reset();
  m_stepTimer.addAlarm(m_stepDuration);
  m_stepTimer.start();

  // initialize the snakes with dops timing
  m_snake1.init(m_onDuration, m_offDuration, m_colorset, 0, 0, m_snakeSize, m_fadeAmount, 3);
  m_snake2.init(m_onDuration, m_offDuration, m_colorset, 1, HALF_ZIGZAG_STEPS, m_snakeSize, m_fadeAmount, 8);
}

// pure virtual must override the play function
void ZigzagPattern::play()
{
  // when the step timer triggers
  if (m_stepTimer.alarm() == 0) {
    m_snake1.step();
    m_snake2.step();
  }

  m_snake1.draw();
  m_snake2.draw();
}

// ===================
//  Snake code

ZigzagPattern::Snake::Snake(VortexEngine &engine) :
  m_engine(engine),
  m_blinkTimer(engine),
  m_colorset(),
  m_step(0),
  m_snakeSize(1),
  m_fadeAmount(55),
  m_changeBoundary(3)
{
}

void ZigzagPattern::Snake::init(uint8_t onDuration, uint8_t offDuration, const Colorset &colorset, uint32_t colorOffset,
  uint8_t step, uint8_t snakeSize, uint8_t fadeAmount, uint8_t changeBoundary)
{
  // reset the blink timer entirely
  m_blinkTimer.reset();
  // dops timing
  m_blinkTimer.addAlarm(onDuration);
  m_blinkTimer.addAlarm(offDuration);
  // start the blink timer from the next frame
  m_blinkTimer.start();
  // start on the first color so that cur() works immediately
  m_colorset = colorset;
  m_colorset.setCurIndex(colorOffset);
  // set the starting step
  m_step = step;
  // set the snake size
  m_snakeSize = snakeSize;
  // set fade amount and change boundary
  m_fadeAmount = fadeAmount;
  m_changeBoundary = changeBoundary;
}

void ZigzagPattern::Snake::step()
{
  // increment to the next step
  m_step = (m_step + 1) % NUM_ZIGZAG_STEPS;
  // if the step is on a change boundary then increment the snake color
  if (m_step == m_changeBoundary) {
    m_colorset.getNext();
  }
}

void ZigzagPattern::Snake::draw()
{
  // when the blinkTimer triggers
  switch (m_blinkTimer.alarm()) {
  case 0: // blinking on
    // draw two snakes, one on the current step, one on the opposite side
    drawSnake();
    break;
  case 1: // blinking off
    m_engine.leds().clearAll();
    break;
  default:
  case -1:
    // nothing
    break;
  }
}

void ZigzagPattern::Snake::drawSnake()
{
  uint8_t segment_position;
  int32_t colIndex = 0;
  for (uint8_t segment = 0; segment < m_snakeSize; ++segment) {
    // calculate the position of this segment, if the segment being rendered is
    // greater than the head position it means the snake head is at an index that
    // won't support it's entire tail without wrapping below 0
    if (segment > m_step) {
      segment_position = NUM_ZIGZAG_STEPS - (segment - m_step);
    } else {
      segment_position = m_step - segment;
    }
    // adjust the brightness of the color before setting
    RGBColor col = m_colorset.peek(colIndex);
    if (segment < m_snakeSize) {
      col.adjustBrightness(m_fadeAmount * segment);
    }
    // lookup the target in the step positions array and turn it on with given color/brightness
    m_engine.leds().setIndex(ledStepPositions[segment_position], col);
    // if this segment is on the step where the color changes
    if (segment_position == m_changeBoundary) {
      // then decrement the color index for the rest of the snake so that the
      // tail of the snake has the old color till it reaches the change point
      colIndex--;
    }
  }
}

