#include "BasicPattern.h"

#include "../../VortexEngine.h"

#include "../../Time/TimeControl.h"
#include "../../Colors/Colorset.h"
#include "../../Leds/Leds.h"

// uncomment me to print debug labels on the pattern states, this is useful if you
// are debugging a pattern strip from the command line and want to see what state
// the pattern is in each tick of the pattern
//#define DEBUG_BASIC_PATTERN

#ifdef DEBUG_BASIC_PATTERN
#include <stdio.h>
// print out the current state of the pattern
#define PRINT_STATE(state) printState(state)
static void printState(PatternState state)
{
  static uint64_t lastPrint = 0;
  if (lastPrint == Time::getCurtime()) return;
  switch (m_state) {
  case STATE_ON: printf("on  "); break;
  case STATE_OFF: printf("off "); break;
  case STATE_IN_GAP: printf("gap1"); break;
  case STATE_IN_DASH: printf("dash"); break;
  case STATE_IN_GAP2: printf("gap2"); break;
  default: return;
  }
  lastPrint = Time::getCurtime();
}
#else
#define PRINT_STATE(state) // do nothing
#endif

BasicPattern::BasicPattern(VortexEngine &engine, const PatternArgs &args) :
  SingleLedPattern(engine, args),
  m_onDuration(0),
  m_offDuration(0),
  m_gapDuration(0),
  m_dashDuration(0),
  m_groupSize(0),
  m_groupCounter(0),
  m_state(STATE_BLINK_ON),
  m_blinkTimer(engine)
{
  m_patternID = PATTERN_STROBE;
  REGISTER_ARG(m_onDuration);
  REGISTER_ARG(m_offDuration);
  REGISTER_ARG(m_gapDuration);
  REGISTER_ARG(m_dashDuration);
  REGISTER_ARG(m_groupSize);
  setArgs(args);
}

BasicPattern::~BasicPattern()
{
}

void BasicPattern::init()
{
  // run base pattern init logic
  Pattern::init();

  // the default state to begin with
  m_state = STATE_BLINK_ON;
  // if a dash is present then always start with the dash because
  // it consumes the first color in the colorset
  if (m_dashDuration > 0) {
    m_state = STATE_BEGIN_DASH;
  }
  // if there's no on duration or dash duration the led is just disabled
  if ((!m_onDuration && !m_dashDuration) || !m_colorset.numColors()) {
    m_state = STATE_DISABLED;
  }
  m_groupCounter = m_groupSize ? m_groupSize : (m_colorset.numColors() - (m_dashDuration != 0));
}

void BasicPattern::nextState(uint8_t timing)
{
  m_blinkTimer.init(TIMER_1_ALARM | TIMER_START, timing);
  m_state = (PatternState)(m_state + 1);
}

void BasicPattern::play()
{
  // Sometimes the pattern needs to cycle multiple states in a single frame so
  // instead of using a loop or recursion I have just used a simple goto
replay:

  // its kinda evolving as i go
  switch (m_state) {
  case STATE_DISABLED:
    return;
  case STATE_BLINK_ON:
    if (m_onDuration > 0) {
      onBlinkOn();
      --m_groupCounter;
      nextState(m_onDuration);
      return;
    }
    m_state = STATE_BLINK_OFF;
  case STATE_BLINK_OFF:
    // the whole 'should blink off' situation is tricky because we might need
    // to go back to blinking on if our colorset isn't at the end yet
    if (m_groupCounter > 0 || (!m_gapDuration && !m_dashDuration)) {
      if (m_offDuration > 0) {
        onBlinkOff();
        nextState(m_offDuration);
        return;
      }
      if (m_groupCounter > 0 && m_onDuration > 0) {
        m_state = STATE_BLINK_ON;
        goto replay;
      }
    }
    m_state = STATE_BEGIN_GAP;
  case STATE_BEGIN_GAP:
    m_groupCounter = m_groupSize ? m_groupSize : (m_colorset.numColors() - (m_dashDuration != 0));
    if (m_gapDuration > 0) {
      beginGap();
      nextState(m_gapDuration);
      return;
    }
    m_state = STATE_BEGIN_DASH;
  case STATE_BEGIN_DASH:
    if (m_dashDuration > 0) {
      beginDash();
      nextState(m_dashDuration);
      return;
    }
    m_state = STATE_BEGIN_GAP2;
  case STATE_BEGIN_GAP2:
    if (m_dashDuration > 0 && m_gapDuration > 0) {
      beginGap();
      nextState(m_gapDuration);
      return;
    }
    m_state = STATE_BLINK_ON;
    goto replay;
  default:
    break;
  }

  if (m_blinkTimer.alarm() == -1) {
    // no alarm triggered just stay in current state, return and don't transition states
    PRINT_STATE(m_state);
    return;
  }

  // this just transitions the state into the next state, with some edge conditions for
  // transitioning to different states under certain circumstances. Honestly this is
  // a nightmare to read now and idk how to fix it
  if (m_state == STATE_IN_GAP2 || (m_state == STATE_OFF && m_groupCounter > 0)) {
    // this is an edge condition for when in the second gap or off in the non-last off blink
    // then the state actually needs to jump backwards rather than iterate
    m_state = m_onDuration ? STATE_BLINK_ON : (m_dashDuration ? STATE_BEGIN_DASH : STATE_BEGIN_GAP);
  } else if (m_state == STATE_OFF && (!m_groupCounter || m_colorset.numColors() == 1)) {
    // this is an edge condition when the state is off but this is the last off blink in the
    // group or there's literally only one color in the group then if there is more blinks
    // left in the group we need to cycle back to blink on instead of to the next state
    m_state = (m_groupCounter > 0) ? STATE_BLINK_ON : STATE_BEGIN_GAP;
  } else {
    // this is the standard case, iterate to the next state
    m_state = (PatternState)(m_state + 1);
  }
  // poor-mans recurse with the new state change (this transitions to a new state within the same tick)
  goto replay;
}

void BasicPattern::onBlinkOn()
{
  PRINT_STATE(STATE_ON);
  m_engine.leds().setIndex(m_ledPos, m_colorset.getNext());
}

void BasicPattern::onBlinkOff()
{
  PRINT_STATE(STATE_OFF);
  m_engine.leds().clearIndex(m_ledPos);
}

void BasicPattern::beginGap()
{
  PRINT_STATE(STATE_IN_GAP);
  m_engine.leds().clearIndex(m_ledPos);
}

void BasicPattern::beginDash()
{
  PRINT_STATE(STATE_IN_DASH);
  m_engine.leds().setIndex(m_ledPos, m_colorset.getNext());
}
