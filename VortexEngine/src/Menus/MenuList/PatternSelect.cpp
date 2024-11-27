#include "PatternSelect.h"

#include "../../Patterns/PatternBuilder.h"
#include "../../Patterns/Pattern.h"
#include "../../Time/TimeControl.h"
#include "../../Time/Timings.h"
#include "../../Buttons/Button.h"
#include "../../Menus/Menus.h"
#include "../../Modes/Modes.h"
#include "../../Modes/Mode.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

PatternSelect::PatternSelect(const RGBColor &col, bool advanced) :
  Menu(col, advanced),
  m_state(STATE_PICK_LIST),
  m_newPatternID(PATTERN_FIRST),
  m_started(false)
{
}

PatternSelect::~PatternSelect()
{
}

bool PatternSelect::init()
{
  if (!Menu::init()) {
    return false;
  }
  m_state = STATE_PICK_LIST;
  DEBUG_LOG("Entered pattern select");
  return true;
}

Menu::MenuAction PatternSelect::run()
{
  MenuAction result = Menu::run();
  if (result != MENU_CONTINUE) {
    return result;
  }

  switch (m_state) {
  case STATE_PICK_LIST:
    // display lists
    showListSelection();
    blinkSelection();
    break;
  case STATE_PICK_PATTERN:
    // display patterns
    showPatternSelection();
    break;
  }

  // show selections
  Menus::showSelection();
  return MENU_CONTINUE;
}

void PatternSelect::showListSelection()
{
  Leds::clearAll();
  for (Quadrant quad = QUADRANT_FIRST; quad < QUADRANT_LAST; ++quad) {
    // hue split into 4 quadrants of 90
    Leds::breatheRange(quadrantFirstLed(quad), quadrantLastLed(quad), quad * (255/4), (uint32_t)Time::getCurtime() / 3, 10, 255, 255);
  }
  Leds::setQuadrant(QUADRANT_5, RGB_WHITE6);
}

void PatternSelect::showPatternSelection()
{
  m_previewMode.play();
  if (g_pButton->isPressed() && g_pButton->holdDuration() > SHORT_CLICK_THRESHOLD_TICKS) {
    Leds::setAll(RGB_WHITE4);
  }
}

void PatternSelect::onLedSelected()
{
  // need to ready up the preview mode for picking patterns, this can look different based on
  // which pattern was already on this mode, and which leds they decided to pick
  // for example if they had a multi-led pattern and they are targetting some grouping of singles now
  // then we need to convert the multi into singles, maybe in the future we can allow singles to overlay
  if (m_previewMode.isMultiLed() && m_targetLeds != MAP_LED_ALL && m_targetLeds != MAP_LED(LED_MULTI)) {
    Colorset curSet = m_previewMode.getColorset();
    m_previewMode.setPattern(PATTERN_FIRST, LED_ALL_SINGLE, nullptr, &curSet);
    // todo: clear multi a better way, automatically when setting singles?
    m_previewMode.clearPattern(LED_MULTI);
    m_previewMode.init();
    DEBUG_LOG("Converted existing multi-led pattern to singles for given led selection");
  }
}

void PatternSelect::onShortClick()
{
  switch (m_state) {
  case STATE_PICK_LIST:
    // wrap at the index back to pinkie
    m_curSelection = (Quadrant)((m_curSelection + 1) % QUADRANT_COUNT);
    break;
  case STATE_PICK_PATTERN:
    nextPattern();
    break;
  }
}

void PatternSelect::onShortClick2()
{
  switch (m_state) {
  case STATE_PICK_LIST:
    if (m_curSelection > QUADRANT_FIRST) {
      m_curSelection = (Quadrant)((m_curSelection - 1));
    } else {
      m_curSelection = QUADRANT_LAST;
    }
    break;
  case STATE_PICK_PATTERN:
    previousPattern();
    break;
  }
}

void PatternSelect::nextPatternID()
{
  // increment to next pattern
  PatternID endList = PATTERN_SINGLE_LAST;
  PatternID beginList = PATTERN_SINGLE_FIRST;
#if VORTEX_SLIM == 0
  // if targeted multi led or all singles, iterate through multis
  if ((m_targetLeds == MAP_LED_ALL) || (m_targetLeds == MAP_LED(LED_MULTI))) {
    endList = PATTERN_MULTI_LAST;
  }
  // if targeted multi then start at multis and only iterate multis
  if ((m_targetLeds == MAP_LED(LED_MULTI))) {
    beginList = PATTERN_MULTI_FIRST;
  }
#endif
  m_newPatternID = (PatternID)((m_newPatternID + 1) % endList);
  if (m_newPatternID > endList || m_newPatternID < beginList) {
    m_newPatternID = beginList;
  }
}

void PatternSelect::nextPattern()
{
  if (m_started) {
    nextPatternID();
  } else {
    m_started = true;
    // Do not modify m_newPatternID Here! It has been set in the long click handler
    // to be the start of the list we want to iterate
  }
  // set the new pattern id
  if (isMultiLedPatternID(m_newPatternID)) {
    m_previewMode.setPattern(m_newPatternID);
  } else {
    // if the user selected multi then just put singles on all leds
    LedMap setLeds = (m_targetLeds == MAP_LED(LED_MULTI)) ? LED_ALL : m_targetLeds;
    m_previewMode.setPatternMap(setLeds, m_newPatternID);
    // TODO: clear multi a better way
    m_previewMode.clearPattern(LED_MULTI);
  }
  m_previewMode.init();
  DEBUG_LOGF("Iterated to pattern id %d", m_newPatternID);
}

void PatternSelect::previousPatternID() 
{
  // increment to next pattern
  PatternID endList = PATTERN_SINGLE_LAST;
  PatternID beginList = PATTERN_SINGLE_FIRST;
#if VORTEX_SLIM == 0
  // if targeted multi led or all singles, iterate through multis
  if ((m_targetLeds == MAP_LED_ALL) || (m_targetLeds == MAP_LED(LED_MULTI))) {
    endList = PATTERN_MULTI_LAST;
  }
  // if targeted multi then start at multis and only iterate multis
  if ((m_targetLeds == MAP_LED(LED_MULTI))) {
    beginList = PATTERN_MULTI_FIRST;
  }
#endif
  if (m_newPatternID > beginList) {
    m_newPatternID = (PatternID)(m_newPatternID - 1);
  } else {
    m_newPatternID = endList;
  }
}

void PatternSelect::previousPattern() 
{
  if (m_started) {
    previousPatternID();
  } else {
    m_started = true;
    // Do not modify m_newPatternID Here! It has been set in the long click handler
    // to be the start of the list we want to iterate
  }
  // set the new pattern id
  if (isMultiLedPatternID(m_newPatternID)) {
    m_previewMode.setPattern(m_newPatternID);
  } else {
    // if the user selected multi then just put singles on all leds
    LedMap setLeds = (m_targetLeds == MAP_LED(LED_MULTI)) ? LED_ALL : m_targetLeds;
    m_previewMode.setPatternMap(setLeds, m_newPatternID);
    // TODO: clear multi a better way
    m_previewMode.clearPattern(LED_MULTI);
  }
  m_previewMode.init();
  DEBUG_LOGF("Iterated to pattern id %d", m_newPatternID);
}

void PatternSelect::onLongClick()
{
  bool needsSave = false;
  Mode *cur = Modes::curMode();
  needsSave = !cur->equals(&m_previewMode);
  if (needsSave) {
    // update the current mode with the new pattern
    Modes::updateCurMode(&m_previewMode);
  }
  DEBUG_LOGF("Saving pattern %u", m_newPatternID);
  // done in the pattern select menu
  leaveMenu(needsSave);
}
