#include "PatternSelect.h"

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

PatternSelect::PatternSelect(const RGBColor &col, bool advanced) :
  Menu(col, advanced),
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
  Menus::showSelection(RGB_WHITE5);
  return MENU_CONTINUE;
}

void PatternSelect::onLedSelected()
{
  m_srcLed = ledmapGetFirstLed(m_targetLeds);
}

void PatternSelect::onShortClick()
{
  PatternID newID = (PatternID)(m_previewMode.getPatternID(m_srcLed) + 1);
  if (newID > PATTERN_SINGLE_LAST) {
    newID = PATTERN_SINGLE_FIRST;
    Leds::holdAll(RGB_WHITE);
  }
  if (!m_started) {
    m_started = true;
    newID = PATTERN_FIRST;
  }
  // set the new pattern id
  if (isMultiLedPatternID(newID)) {
    m_previewMode.setPattern(newID);
  } else {
    m_previewMode.setPatternMap(m_targetLeds, newID);
  }
  m_previewMode.init();
  DEBUG_LOGF("Iterated to pattern id %d", newID);
}

void PatternSelect::onLongClick()
{
  // store the mode as current mode
  Modes::updateCurMode(&m_previewMode);
  leaveMenu(true);
}
