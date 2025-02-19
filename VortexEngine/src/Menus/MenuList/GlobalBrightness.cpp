#include "GlobalBrightness.h"

#include "../../VortexEngine.h"

#include "../../Patterns/PatternArgs.h"
#include "../../Patterns/Pattern.h"
#include "../../Time/TimeControl.h"
#include "../../Colors/Colorset.h"
#include "../../Buttons/Button.h"
#include "../../Time/Timings.h"
#include "../../Menus/Menus.h"
#include "../../Modes/Modes.h"
#include "../../Time/Timer.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

// allow the number of brightness options to be adjusted dynamically
#define NUM_BRIGHTNESS_OPTIONS (sizeof(m_brightnessOptions) / sizeof(m_brightnessOptions[0]))

GlobalBrightness::GlobalBrightness(const RGBColor &col, bool advanced) :
  Menu(col, advanced),
  m_keychain_modeState(KEYCHAIN_MODE_STATE_OFF),
  m_lastStateChange(0),
  m_colorIndex(0)
{
}

GlobalBrightness::~GlobalBrightness()
{
  // ensure force sleep is turned back on when we leave
  VortexEngine::toggleForceSleep(true);
}

bool GlobalBrightness::init()
{
  if (!Menu::init()) {
    return false;
  }
  // bypass led selection
  m_ledSelected = true;
  // would be nice if there was a more elegant way to do this
  for (uint8_t i = 0; i < NUM_BRIGHTNESS_OPTIONS; ++i) {
    if (m_brightnessOptions[i] == Leds::getBrightness()) {
      // make sure the default selection matches cur value
      m_curSelection = i;
    }
  }
  if (m_advanced) {
    // turn off force sleep while running keychain mode
    VortexEngine::toggleForceSleep(false);
    // enable keychain mode so if the device sleeps it will wake into keychain mode
    Modes::setKeychainMode(true);
    // start in the off position, this call is necessary to update the
    // lastStateChange time to the current time of init
    setKeychainModeState(KEYCHAIN_MODE_STATE_SOLID);
    // reset the color index to ensure it starts on first color
    m_colorIndex = 0;
  }
  DEBUG_LOG("Entered global brightness");
  return true;
}

Menu::MenuAction GlobalBrightness::run()
{
  MenuAction result = Menu::run();
  if (result != MENU_CONTINUE) {
    return result;
  }
  if (m_advanced) {
    return runKeychainMode();
  }
  // show the current brightness
  showBrightnessSelection();
  // show selections
  Menus::showSelection();
  // continue
  return MENU_CONTINUE;
}

void GlobalBrightness::onShortClick()
{
  if (m_advanced) {
    return;
  }
  // include one extra option for the exit slot
  m_curSelection = (m_curSelection + 1) % (NUM_BRIGHTNESS_OPTIONS + 1);
}

void GlobalBrightness::onLongClick()
{
  if (m_advanced) {
    return;
  }
  if (m_curSelection >= NUM_BRIGHTNESS_OPTIONS) {
    // no save exit
    leaveMenu();
    return;
  }
  // set the global brightness
  Leds::setBrightness(m_brightnessOptions[m_curSelection]);
  // done here, save settings with new brightness
  leaveMenu(true);
}

void GlobalBrightness::showBrightnessSelection()
{
  if (m_curSelection >= NUM_BRIGHTNESS_OPTIONS) {
    showExit();
    return;
  }
  Leds::setAll(HSVColor(38, 255, m_brightnessOptions[m_curSelection]));
}

// ==================== KEYCHAIN_MODE STUFF ====================

// The general timer for keychain mode to remain active
#define KEYCHAIN_MODE_TIMER_MS      2100
#define KEYCHAIN_MODE_TIMER_TICKS   MS_TO_TICKS(KEYCHAIN_MODE_TIMER_MS)
// the number of clicks needed to exit keychain mode
#define KEYCHAIN_MODE_EXIT_CLICKS   8
// This is the duration of idle time on blank before it will auto-sleep
// give a solid 300 seconds (5 minutes) before it goes to sleep
#define KEYCHAIN_MODE_SLEEP_TICKS   SEC_TO_TICKS(300)

// bonus simulate keychain_mode in this menu
Menu::MenuAction GlobalBrightness::runKeychainMode()
{
  // check for exit
  if (g_pButton->onConsecutivePresses(KEYCHAIN_MODE_EXIT_CLICKS)) {
    // turn off keychain mode now
    Modes::setKeychainMode(false);
    return MENU_QUIT;
  }
  uint32_t now = Time::getCurtime();
  // whether the button was pressed before the timer expired
  // when the button is first pressed after the window has expired switch off
  if (g_pButton->onPress() && m_keychain_modeState != KEYCHAIN_MODE_STATE_OFF && now > (m_lastStateChange + KEYCHAIN_MODE_TIMER_TICKS)) {
    setKeychainModeState(KEYCHAIN_MODE_STATE_OFF);
    return MENU_CONTINUE;
  }
  // when the button is released, but only if they pressed it within this state
  if (g_pButton->onRelease() && g_pButton->pressTime() > m_lastStateChange) {
    // advance the state or turn off based on press time
    setKeychainModeState((keychain_mode_state)(m_keychain_modeState + 1));
    return MENU_CONTINUE;
  }
  // as long as the button is held down the leds clear
  if (g_pButton->isPressed()) {
    Leds::clearAll();
    return MENU_CONTINUE;
  }
  // check for sleep after some seconds
  if (m_keychain_modeState == KEYCHAIN_MODE_STATE_OFF && now > (m_lastStateChange + KEYCHAIN_MODE_SLEEP_TICKS)) {
    // Optional: indicate when the keychain is turning off (useful mainly for debugging)
    //Leds::holdAll(RGB_PURPLE);
    VortexEngine::enterSleep(false);
    return MENU_QUIT;
  }
  // render the signal mode in a different way
  if (m_keychain_modeState == KEYCHAIN_MODE_STATE_SIGNAL) {
    // use a custom blink because the SIGNAL_OFF duration is too large for uint8 params
    Leds::clearAll();
    Leds::blinkIndexOffset(LED_ALL,
      Time::getCurtime() - m_lastStateChange,
      SIGNAL_OFF_DURATION,
      SIGNAL_ON_DURATION,
      m_previewMode.getColorset().get(m_colorIndex));
  } else {
    // play the keychain_mode mode
    m_previewMode.play();
  }
  return MENU_CONTINUE;
}

void GlobalBrightness::setKeychainModeState(keychain_mode_state newState)
{
  // update the keychain_mode state
  m_keychain_modeState = (keychain_mode_state)(newState % KEYCHAIN_MODE_STATE_COUNT);
  // record the time of this statechange
  m_lastStateChange = Time::getCurtime();
  // this could
  uint8_t numCols = Modes::curMode()->getColorset().numColors();
  // the args that will define the timing of the blink on the solid pattern
  PatternArgs args;
  switch (m_keychain_modeState) {
  case KEYCHAIN_MODE_STATE_OFF:
  default:
    // iterate to next color and wrap to 0, don't use modulus because numCols could be 0
    m_colorIndex++;
    if (m_colorIndex >= numCols) {
      m_colorIndex = 0;
    }
    break;
  case KEYCHAIN_MODE_STATE_SOLID:
    args.init(200);
    break;
  case KEYCHAIN_MODE_STATE_DOPS:
    args.init(1, 10);
    break;
  case KEYCHAIN_MODE_STATE_SIGNAL:
    // unfortunately the signal blink timing is too large to fit into the uint8 params
    // so we just use a custom blink to render that one, but use the previewMode for
    // the other two states
    break;
  }
  // update the mode and ensure current colorset is always used, use PATTERN_SOLID
  // because that will never iterate to the next color and allows us to force
  // the color index via argument 6
  args.arg6 = m_colorIndex;
  m_previewMode.setPattern(PATTERN_SOLID, LED_ALL, &args);
  m_previewMode.init();
}
