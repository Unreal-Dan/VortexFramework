#include "MainMenu.h"

#include "../Time/TimeControl.h"
#include "../Storage/Storage.h"
#include "../Buttons/Buttons.h"
#include "../Leds/LedTypes.h"
#include "../Modes/Modes.h"
#include "../Leds/Leds.h"
#include "../Log/Log.h"

bool MainMenu::m_isOpen = true;
uint8_t MainMenu::m_curSelection = 0;

#define NUM_SELECTIONS (LED_COUNT / 2)

bool MainMenu::init()
{
  // main menus start open
  m_isOpen = true;
  m_curSelection = 0;
  return true;
}

bool MainMenu::run()
{
  // if the main menus aren't open then nothing to do here
  if (!m_isOpen) {
    return false;
  }

  // press <
  if (g_pButtonL->onShortClick()) {
    pressLeft();
  }
  // press o
  if (g_pButtonM->onShortClick()) {
    select();
  }
  if (g_pButtonM->onLongClick()) {
    select();
  }
  // press >
  if (g_pButtonR->onShortClick()) {
    pressRight();
  }

  // render
  show();
  return true;
}

void MainMenu::show()
{
  Leds::clearAll();
  // render the main menu
  uint8_t hue = 0;
  uint32_t now = Time::getCurtime();
  MAP_FOREACH_LED(MAP_OUTER_RING) {
    Leds::breatheIndex(pos, hue, (now / 2), 8, 255, 180);
    hue += (255 / (LED_COUNT / 2));
  }
  hue = 0;
  MAP_FOREACH_LED(MAP_INNER_RING) {
    Leds::breatheIndex(pos, hue, (now / 2), 8, 255, 180);
    hue += (255 / (LED_COUNT / 2));
  }
  Leds::blinkIndex((LedPos)m_curSelection);
  Leds::blinkIndex((LedPos)(m_curSelection + 10));
}

void MainMenu::open()
{
  m_isOpen = true;
}

void MainMenu::close()
{
  m_isOpen = false;
}

bool MainMenu::isOpen()
{
  return m_isOpen;
}

void MainMenu::pressLeft()
{
  if (!m_curSelection) {
    m_curSelection = NUM_SELECTIONS - 1;
  } else {
    m_curSelection--;
  }
}

void MainMenu::pressRight()
{
  m_curSelection = (m_curSelection + 1) % NUM_SELECTIONS;
}

void MainMenu::select()
{
  m_isOpen = false;
  Storage::setStoragePage(m_curSelection);
  if (!Modes::loadStorage()) {
    Modes::setDefaults();
  }
  DEBUG_LOGF("Selected storage page: %u", m_curSelection);
}
