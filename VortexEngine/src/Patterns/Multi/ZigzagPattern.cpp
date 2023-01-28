#include "ZigzagPattern.h"

#include "../../Serial/ByteStream.h"
#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

// Mapping of LED positions to steps.
// The lights runs across tips, then back across tops.
// Index this array with m_step in order to get correct LedPos
const LedPos ZigzagPattern::ledStepPositions[] = {
  PINKIE_TOP,
  RING_TOP,
  MIDDLE_TOP,
  INDEX_TOP,
  THUMB_TOP,

  THUMB_TIP,
  INDEX_TIP,
  MIDDLE_TIP,
  RING_TIP,
  PINKIE_TIP,

  // PaLm LiGhTs?>??!?
#if USE_PALM_LIGHTS == 1
  PALM_LEFT,
  PALM_UP,
  PALM_RIGHT,
  PALM_DOWN
#endif
};

// There just happens to be LED_COUNT steps in the pattern
#define NUM_ZIGZAG_STEPS (sizeof(ledStepPositions) / sizeof(ledStepPositions[0]))
#define HALF_ZIGZAG_STEPS (NUM_ZIGZAG_STEPS / 2)

ZigzagPattern::ZigzagPattern(uint8_t onDuration, uint8_t offDuration, uint8_t stepDuration, uint8_t snakeSize, uint8_t fadeAmount) :
  MultiLedPattern(),
  m_onDuration(onDuration),
  m_offDuration(offDuration),
  m_stepDuration(stepDuration),
  m_snakeSize(snakeSize),
  m_fadeAmount(fadeAmount),
  m_stepTimer(),
  m_snake1(),
  m_snake2()
{
  m_patternID = PATTERN_ZIGZAG;
}

ZigzagPattern::ZigzagPattern(const PatternArgs &args) :
  ZigzagPattern()
{
  setArgs(args);
  // WORKAROUND:
  //  Unfortunately snakesize and fadeamount cannot be set/get through
  //  the setArgs and getArgs apis which means they are not exposed as
  //  params beyond the constructor above. Because of this we need to
  //  manually handle snakeSize and fadeAmount in order to properly
  //  construct a ZigZag pattern from PatternArgs
  m_snakeSize = args.arg4;
  m_fadeAmount = args.arg5;
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

void ZigzagPattern::setArgs(const PatternArgs &args)
{
  MultiLedPattern::setArgs(args);
  m_onDuration = args.arg1;
  m_offDuration = args.arg2;
  m_stepDuration = args.arg3;
}

void ZigzagPattern::getArgs(PatternArgs &args) const
{
  MultiLedPattern::getArgs(args);
  args.arg1 = m_onDuration;
  args.arg2 = m_offDuration;
  args.arg3 = m_stepDuration;
  args.numArgs += 3;
}

#if SAVE_TEMPLATE == 1
void ZigzagPattern::saveTemplate(int level) const
{
  MultiLedPattern::saveTemplate(level);
  IndentMsg(level + 1, "\"OnDuration\": %d,", m_onDuration);
  IndentMsg(level + 1, "\"OffDuraiton\": %d,", m_offDuration);
  IndentMsg(level + 1, "\"StepDuration\": %d,", m_stepDuration);
}
#endif

// ===================
//  Snake code

ZigzagPattern::Snake::Snake() :
  m_blinkTimer(),
  m_colorset(),
  m_step(0),
  m_snakeSize(1),
  m_fadeAmount(55),
  m_changeBoundary(3)
{
}

void ZigzagPattern::Snake::init(uint32_t onDuration, uint32_t offDuration, const Colorset &colorset, uint32_t colorOffset,
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
    Leds::clearAll();
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
    Leds::setIndex(ledStepPositions[segment_position], col);
    // if this segment is on the step where the color changes
    if (segment_position == m_changeBoundary) {
      // then decrement the color index for the rest of the snake so that the
      // tail of the snake has the old color till it reaches the change point
      colIndex--;
    }
  }
}

