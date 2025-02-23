#include "PatternSelect.h"

#include "../../VortexEngine.h"

#include "../../Patterns/PatternBuilder.h"
#include "../../Patterns/PatternArgs.h"
#include "../../Patterns/Pattern.h"
#include "../../Serial/ByteStream.h"
#include "../../Time/TimeControl.h"
#include "../../Buttons/Button.h"
#include "../../Random/Random.h"
#include "../../Time/Timings.h"
#include "../../Modes/Modes.h"
#include "../../Menus/Menus.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

PatternSelect::PatternSelect(VortexEngine &engine, const RGBColor &col, bool advanced) :
  Menu(engine, col, advanced),
  m_newPatternID(PATTERN_FIRST),
  m_srcLed(LED_FIRST),
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
  DEBUG_LOG("Entered pattern select");
  return true;
}

Menu::MenuAction PatternSelect::run()
{
  MenuAction result = Menu::run();
  if (result != MENU_CONTINUE) {
    return result;
  }
  // run the current mode
  m_previewMode.play();
  // show dimmer selections in advanced mode
  m_engine.menus().showSelection(RGB_WHITE5);
  return MENU_CONTINUE;
}

void PatternSelect::onLedSelected()
{
  m_srcLed = m_engine.leds().ledmapGetFirstLed(m_targetLeds);
}

void PatternSelect::onShortClick()
{
  nextPattern();
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
    m_previewMode.setPattern(m_newPatternID, LED_ANY);
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
  Mode *cur = m_engine.modes().curMode();
  needsSave = !cur || !cur->equals(&m_previewMode);
  if (needsSave) {
    // update the current mode with the new pattern
    m_engine.modes().updateCurMode(&m_previewMode);
  }
  DEBUG_LOGF("Saving pattern %u", m_newPatternID);
  // done in the pattern select menu
  leaveMenu(needsSave);
}
