#include "FactoryReset.h"

#include "../../VortexEngine.h"
#include "../../Modes/DefaultModes.h"
#include "../../Time/TimeControl.h"
#include "../../Time/Timings.h"
#include "../../Colors/Colorset.h"
#include "../../Buttons/Button.h"
#include "../../Modes/DefaultModes.h"
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
  // start on exit by default
  m_curSelection = QUADRANT_LAST;
  // bypass led selection for fac reset if a multi was set on
  // the current slot because it doesn't make sense to pick
  if (Modes::curMode()->isMultiLed()) {
    m_ledSelected = true;
    m_targetLeds = MAP_LED(LED_MULTI);
  }
  DEBUG_LOG("Entered factory reset");
  return true;
}

Menu::MenuAction FactoryReset::run()
{
  MenuAction result = Menu::run();
  if (result != MENU_CONTINUE) {
    return result;
  }

  // show the reset menu
  showReset();

  // blink the selection
  blinkSelection();

  // continue
  return MENU_CONTINUE;
}

void FactoryReset::onShortClick()
{
  if (m_curSelection == QUADRANT_LAST) {
    m_curSelection = QUADRANT_COUNT;
  } else {
    m_curSelection = QUADRANT_LAST;
  }
}

void FactoryReset::onShortClick2()
{
  onShortClick();
}

void FactoryReset::onLongClick()
{
  switch (m_curSelection) {
  case QUADRANT_LAST:
  default:
    leaveMenu();
    return;
  case QUADRANT_COUNT:
    // must wait till all empty + one white blink
    if (g_pButton->holdDuration() > (FACTORY_RESET_THRESHOLD_TICKS + MS_TO_TICKS(700))) {
      Modes::setDefaults();
      leaveMenu(true);
    }
    break;
  }
  // if the button hasn't been held long enough just return
  if (g_pButton->holdDuration() <= (FACTORY_RESET_THRESHOLD_TICKS + MS_TO_TICKS(10))) {
    return;
  }
  // the button was held down long enough so actually perform the factory reset
  // restore defaults and then leave menu and save
  if (m_advanced) {
    uint8_t curModeIndex = Modes::curModeIndex();
    // reset the target mode slot on the target led
    const default_mode_entry &def = default_modes[curModeIndex];
    Colorset set(def.numColors, def.cols);
    Mode *cur = Modes::curMode();
    cur->setPatternMap(m_targetLeds, def.patternID, nullptr, &set);
    // re-initialize the current mode
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

void FactoryReset::onLongClick2()
{
  leaveMenu();
}

void FactoryReset::showReset()
{
  // if we're on the thumb just set the rest to blank
  if (m_curSelection == QUADRANT_LAST) {
    Leds::setRange(LED_FIRST, LED_LAST, RGB_WHITE0);
    return;
  }
  // otherwise we're not on thumb, if the button isn't pressed
  if (!g_pButton->isPressed()) {
    // just idle blink from clear to blank
    Leds::clearRange(LED_FIRST, LED_LAST);
    Leds::blinkRange(LED_FIRST, LED_LAST, 250, 150, RGB_RED0);
    return;
  }

  // the button is pressed so show the reset countdown timer

  // the progress is how long the hold duration has been held
  // relative to the factory reset threshold time
  float progress = (float)g_pButton->holdDuration() / FACTORY_RESET_THRESHOLD_TICKS;
  // prevents the countdown timer from showing unless button is held longer than 3% of the reset Threshold (this is for short clicks)
  if (progress < 0.03) {
    return;
  }
  // the ledProgress is just an LED from pinky tip to index top based on progress
  LedPos ledProgress = (LedPos)(progress * LED_LAST);
  // max the led progress at index top (don't include thumb)
  if (ledProgress > LED_LAST) {
    // when we reach the end of the progress bar just blink white
    Leds::blinkRange(LED_FIRST, LED_LAST, 80, 60, RGB_WHITE6);
    return;
  }

  // the off/on ms blink faster based on the progress
  uint32_t offMs = 150 - ((65 / LED_COUNT) * ledProgress);
  uint32_t onMs = 200 - ((25 / LED_COUNT) * ledProgress);
  // the 'endled' is the tip of the reset progress bar, since the progress
  // bar starts full red and empties down to the pinky that means it is
  // inverted from the 'ledProgress' which starts at 0 and grows
  LedPos endLed = (LedPos)(LED_LAST - ledProgress);
  // clear all the leds so that 'blinkRange' will blink from off to the designated color
  Leds::clearRange(LED_FIRST, LED_LAST);
  // blink to the calculated redish hue from pinky to the end led
  Leds::blinkRange(LED_FIRST, endLed, offMs, onMs, HSVColor(0, 255 - (progress * 170), 180));
  // and blink the background the regular blank color
  Leds::blinkRange((LedPos)(endLed + 1), LED_LAST, offMs, onMs, RGB_WHITE0);
}

