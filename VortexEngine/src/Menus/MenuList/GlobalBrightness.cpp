#include "GlobalBrightness.h"

#include "../../VortexEngine.h"

#include "../../Modes/Modes.h"
#include "../../Menus/Menus.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

// allow the number of brightness options to be adjusted dynamically
#define NUM_BRIGHTNESS_OPTIONS (sizeof(m_brightnessOptions) / sizeof(m_brightnessOptions[0]))

GlobalBrightness::GlobalBrightness(VortexEngine &engine, const RGBColor &col, bool advanced) :
  Menu(engine, col, advanced)
{
}

GlobalBrightness::~GlobalBrightness()
{
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
    if (m_brightnessOptions[i] == m_engine.leds().getBrightness()) {
      // make sure the default selection matches cur value
      m_curSelection = i;
    }
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
  // show the current brightness
  showBrightnessSelection();
  // show selections
  m_engine.menus().showSelection();
  // continue
  return MENU_CONTINUE;
}

void GlobalBrightness::onShortClick()
{
  // include one extra option for the exit slot
  m_curSelection = (m_curSelection + 1) % (NUM_BRIGHTNESS_OPTIONS + 1);
}

void GlobalBrightness::onLongClick()
{
  if (m_curSelection >= NUM_BRIGHTNESS_OPTIONS) {
    // no save exit
    leaveMenu();
    return;
  }
  // set the global brightness
  m_engine.leds().setBrightness(m_brightnessOptions[m_curSelection]);
  // done here, save settings with new brightness
  leaveMenu(true);
}

void GlobalBrightness::showBrightnessSelection()
{
  if (m_curSelection >= NUM_BRIGHTNESS_OPTIONS) {
    showExit();
    return;
  }
  m_engine.leds().setAll(HSVColor(38, 255, m_brightnessOptions[m_curSelection]));
}
