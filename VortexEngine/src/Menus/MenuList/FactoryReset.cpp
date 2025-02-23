#include "FactoryReset.h"

#include "../../VortexEngine.h"

#include "../../VortexEngine.h"
#include "../../Modes/DefaultModes.h"
#include "../../Time/TimeControl.h"
#include "../../Patterns/Pattern.h"
#include "../../Buttons/Button.h"
#include "../../Time/Timings.h"
#include "../../Modes/Modes.h"
#include "../../Modes/Mode.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

#include "../../VortexConfig.h"

FactoryReset::FactoryReset(VortexEngine &engine, const RGBColor &col, bool advanced) :
  Menu(engine, col, advanced)
{
}

FactoryReset::~FactoryReset()
{
}

bool FactoryReset::init()
{
  if (!Menu::init()) {
    return false;
  }
  // bypass led selection for fac reset if a multi was set on
  // the current slot because it doesn't make sense to pick
  Mode *cur = m_engine.modes().curMode();
  if (!cur) {
    return false;
  }
  if (cur->isMultiLed()) {
    m_ledSelected = true;
    m_targetLeds = MAP_LED(LED_MULTI);
  }
  if (!m_advanced) {
    // skip led selection
    m_ledSelected = true;
  }
  // Start on exit by default
  m_curSelection = false;
  DEBUG_LOG("Entered factory reset");
  return true;
}

Menu::MenuAction FactoryReset::run()
{
  MenuAction result = Menu::run();
  if (result != MENU_CONTINUE) {
    return result;
  }
  showReset();
  return MENU_CONTINUE;
}

void FactoryReset::onShortClick()
{
  m_curSelection = (uint8_t)!m_curSelection;
}

void FactoryReset::onLongClick()
{
  if (m_curSelection == 0) {
    // if the selection isn't actually on factory reset then just leave
    leaveMenu();
    return;
  }
  // if the button hasn't been held long enough just return
  if (m_engine.button().holdDuration() <= (FACTORY_RESET_THRESHOLD_TICKS + MS_TO_TICKS(10))) {
    return;
  }
  // the button was held down long enough so actually perform the factory reset
  // restore defaults and then leave menu and save
  if (m_advanced) {
    uint8_t curModeIndex = m_engine.modes().curModeIndex();
    // reset the target mode slot on the target led
    const default_mode_entry &def = default_modes[curModeIndex];
    Colorset set(def.numColors, def.cols);
    Mode *cur = m_engine.modes().curMode();
    cur->setPatternMap(m_targetLeds, def.patternID, nullptr, &set);
    // re-initialize the current mode
    cur->init();
  } else {
    m_engine.leds().setBrightness(DEFAULT_BRIGHTNESS);
    m_engine.setAutoCycle(false);
    m_engine.modes().setDefaults();
    m_engine.modes().setCurMode(0);
    m_engine.modes().resetFlags();
  }
  leaveMenu(true);
}

void FactoryReset::showReset()
{
  if (m_curSelection == 0) {
    m_engine.leds().clearAll();
    m_engine.leds().blinkAll(350, 350, RGB_WHITE0);
    return;
  }
  bool isPressed = m_engine.button().isPressed();
  if (!isPressed) {
    m_engine.leds().clearAll();
    m_engine.leds().blinkAll(50, 50, RGB_RED4);
    return;
  }
  // don't start the fill until the button has been held for a bit
  uint32_t holdDur = m_engine.button().holdDuration();
  if (holdDur < MS_TO_TICKS(100)) {
    return;
  }
  uint16_t progress = ((holdDur * 100) / FACTORY_RESET_THRESHOLD_TICKS);
  DEBUG_LOGF("progress: %d", progress);
  if (progress >= 100) {
    m_engine.leds().setAll(RGB_WHITE);
    return;
  }
  uint8_t offMs = 100;
  uint8_t onMs = (progress > 60) ? 30 : 100;
  uint8_t sat = (uint8_t)((progress * 5) >> 1); // Using bit shift for division by 2
  m_engine.leds().clearAll();
  m_engine.leds().blinkAll(offMs, onMs, HSVColor(0, 255 - sat, 180));
}

