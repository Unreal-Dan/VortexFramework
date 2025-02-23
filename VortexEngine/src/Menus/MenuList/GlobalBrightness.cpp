#include "GlobalBrightness.h"

#include "../../Modes/Modes.h"
#include "../../Menus/Menus.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

#define NUM_BRIGHTNESS_OPTIONS (sizeof(m_brightnessOptions) / sizeof(m_brightnessOptions[0]))

GlobalBrightness::GlobalBrightness(const RGBColor &col, bool advanced) :
  Menu(col, advanced)
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
    if (m_brightnessOptions[i] == Leds::getBrightness()) {
      // make sure the default selection matches cur value
      m_curSelection = (Quadrant)i;
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

  // blink the current selection
  blinkSelection();

  // continue
  return MENU_CONTINUE;
}

void GlobalBrightness::onShortClick()
{
  // include one extra option for the exit slot
  m_curSelection = (Quadrant)(((uint32_t)m_curSelection + 1) % (NUM_BRIGHTNESS_OPTIONS + 1));
}

void GlobalBrightness::onShortClick2()
{
  // four options in global brightness
  if (!m_curSelection) {
    m_curSelection = QUADRANT_LAST;
  } else {
    m_curSelection = m_curSelection - 1;
  }
}

void GlobalBrightness::onLongClick()
{
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

void GlobalBrightness::onLongClick2()
{
  leaveMenu();
}

void GlobalBrightness::showBrightnessSelection()
{
  // display brightnesses on each finger
  for (Quadrant f = QUADRANT_FIRST; f <= QUADRANT_4; ++f) {
    Leds::setQuadrant(f, HSVColor(0, 0, m_brightnessOptions[f]));
  }
}
