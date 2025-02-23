#include "BouncePattern.h"

#include "../../Serial/ByteStream.h"
#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

// safety to prevent divide by 0
#define TOTAL_STEPS ((((PAIR_COUNT + 2) * 2) - 2) ? (((PAIR_COUNT + 2) * 2) - 2) : 1)
#define HALF_STEPS (TOTAL_STEPS / 2)

BouncePattern::BouncePattern(const PatternArgs &args) :
  BlinkStepPattern(args),
  m_progress(0),
  m_fadeAmount(0)
{
  m_patternID = PATTERN_BOUNCE;
  REGISTER_ARG(m_fadeAmount);
  setArgs(args);
}

BouncePattern::~BouncePattern()
{
}

// init the pattern to initial state
void BouncePattern::init()
{
  BlinkStepPattern::init();
  // reset progress
  m_progress = 0;
  // start colorset at index 0 so cur() works
  m_colorset.setCurIndex(0);
}

void BouncePattern::blinkOn()
{
  Leds::setAll(m_colorset.cur());
  if (m_progress != 0 && m_progress != HALF_STEPS) {
    if (m_progress < (PAIR_COUNT + 1)) {
      Leds::setPair((Pair)(m_progress - 1), m_colorset.peekNext());
    } else {
      Leds::setPair((Pair)(TOTAL_STEPS - (m_progress + 1)), m_colorset.peekNext());
    }
  }
}

void BouncePattern::poststep()
{
  m_progress = (m_progress + 1) % TOTAL_STEPS;
  if (m_progress == 0 || m_progress == HALF_STEPS) {
    m_colorset.getNext();
  }
}
