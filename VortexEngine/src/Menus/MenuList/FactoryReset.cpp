#include "FactoryReset.h"

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

FactoryReset::FactoryReset(const RGBColor &col, bool advanced) :
  Menu(col, advanced)
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
  Mode *cur = Modes::curMode();
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
  if (g_pButton->holdDuration() <= (FACTORY_RESET_THRESHOLD_TICKS + MS_TO_TICKS(10))) {
    return;
  }
  // the button was held down long enough so actually perform the factory reset
  // restore defaults and then leave menu and save
  if (m_advanced) {
    // reset the target mode slot on the target led
    const DefaultModeEntry &defMode = defaultModes[Modes::curModeIndex()];
    Mode *cur = Modes::curMode();
    MAP_FOREACH_LED(m_targetLeds) {
      const DefaultLedEntry &led = defMode.leds[pos];
      Colorset set(led.numColors, led.cols);
      cur->setPattern(led.patternID, pos, nullptr, &set);
    }
    cur->init();
  } else {
    Leds::setBrightness(DEFAULT_BRIGHTNESS);
    VortexEngine::setAutoCycle(false);
    Modes::setDefaults();
    Modes::setCurMode(0);
    Modes::resetFlags();
  }
  leaveMenu(true);
}

void FactoryReset::showReset()
{
  if (m_curSelection == 0) {
    Leds::clearAll();
    Leds::blinkAll(350, 350, RGB_WHITE0);
    return;
  }
  bool isPressed = g_pButton->isPressed();
  if (!isPressed) {
    Leds::clearAll();
    Leds::blinkAll(50, 50, RGB_RED4);
    return;
  }
  // don't start the fill until the button has been held for a bit
  uint32_t holdDur = g_pButton->holdDuration();
  if (holdDur < MS_TO_TICKS(100)) {
    return;
  }
  uint16_t progress = ((holdDur * 100) / FACTORY_RESET_THRESHOLD_TICKS);
  DEBUG_LOGF("progress: %d", progress);
  if (progress >= 100) {
    Leds::setAll(RGB_WHITE);
    return;
  }
  uint8_t offMs = 100;
  uint8_t onMs = (progress > 60) ? 30 : 100;
  uint8_t sat = (uint8_t)((progress * 5) >> 1); // Using bit shift for division by 2
  Leds::clearAll();
  Leds::blinkIndex(LED_0, offMs, onMs, HSVColor(0, 255 - sat, 180));
  Leds::blinkIndex(LED_1, offMs, onMs, RGB_WHITE0);
}

