#include "Menu.h"

#include "../Time/TimeControl.h"
#include "../Time/Timings.h"
#include "../Buttons/Button.h"
#include "../Menus/Menus.h"
#include "../Modes/Modes.h"
#include "../Modes/Mode.h"
#include "../Leds/Leds.h"
#include "../Log/Log.h"

// this is an array of possible LED maps for LED selection
static LedMap ledPermutations[] = {
  MAP_LED_ALL,
  MAP_LED(LED_MULTI),
  MAP_RING_INNER,
  MAP_RING_OUTER,

  MAP_RING_INNER_EVEN,
  MAP_RING_INNER_ODD,
  MAP_RING_OUTER_EVEN,
  MAP_RING_OUTER_ODD,

  MAP_LINE_1,
  MAP_LINE_2,
  MAP_LINE_3,
  MAP_LINE_4,
  MAP_LINE_5,

  MAP_LED(LED_0),
  MAP_LED(LED_1),
  MAP_LED(LED_2),
  MAP_LED(LED_3),
  MAP_LED(LED_4),
  MAP_LED(LED_5),
  MAP_LED(LED_6),
  MAP_LED(LED_7),
  MAP_LED(LED_8),
  MAP_LED(LED_9),
  MAP_LED(LED_10),
  MAP_LED(LED_11),
  MAP_LED(LED_12),
  MAP_LED(LED_13),
  MAP_LED(LED_14),
  MAP_LED(LED_15),
  MAP_LED(LED_16),
  MAP_LED(LED_17),
  MAP_LED(LED_18),
  MAP_LED(LED_19),
};

#define NUM_PERMUTATIONS (sizeof(ledPermutations)/ sizeof(ledPermutations[0]))

Menu::Menu(const RGBColor &col, bool advanced) :
  m_previewMode(),
  m_menuColor(col),
  m_targetLeds(MAP_LED_NONE),
  m_ledSelection(0),
  m_curSelection(0),
  m_ledSelected(false),
  m_advanced(advanced),
  m_shouldClose(false)
{
}

Menu::~Menu()
{
}

bool Menu::init()
{
  // menu is initialized before being run
  if (!Modes::curMode()) {
    // if you enter a menu and there's no modes, it will add an empty one
    if (Modes::numModes() > 0) {
      // some kind of serious error
      return false;
    }
    if (!Modes::addMode(PATTERN_STROBE, RGBColor(RGB_OFF))) {
      // some kind of serious error
      return false;
    }
    if (!Modes::curMode()) {
      // serious error again
      return false;
    }
  }
  // reset the current selection
  m_curSelection = 0;
  // copy the current mode into the demo mode and initialize it
  m_previewMode = *Modes::curMode();
  m_previewMode.init();
  // just in case
  m_shouldClose = false;
  return true;
}

Menu::MenuAction Menu::run()
{
  // check whether the close flag has been set
  if (m_shouldClose) {
    // reset flag and close
    m_shouldClose = false;
    return MENU_QUIT;
  }

  // if the led has already been selected continue as normal
  // and allow derived menu logic to perform work
  if (m_ledSelected) {
    return MENU_CONTINUE;
  }

  // using manual onShortClick and onLongClick check here because
  // there is no guarantee the child class will call the parent
  // class's onShortClick and onLongClick functions so

  // every time a button is clicked, change the led selection
  if (g_pButtonR->onShortClick()) {
    m_ledSelection = (m_ledSelection + 1) % NUM_PERMUTATIONS;
  }
  if (g_pButtonL->onShortClick()) {
    m_ledSelection = (m_ledSelection > 0) ? (m_ledSelection - 1) : (NUM_PERMUTATIONS - 1);
  }
  // on a long press of the button, lock in the target led
  if (g_pButtonM->onLongClick() || g_pButtonM->onShortClick()) {
    // if no target, set at least cur mask
    if (m_targetLeds == 0) {
    //if (m_targetLeds == MAP_LED_NONE) {
      addSelectionMask();
    }
    m_ledSelected = true;
    // call led selected callback
    onLedSelected();
    // log about the selection
    DEBUG_LOGF("Led Selected: 0x%x (%s)", m_targetLeds,
      (m_targetLeds == MAP_LED(LED_MULTI)) ? "multi"
        : (m_targetLeds == MAP_LED_ALL) ? "all"
          : "some singles");
  }
  // on a long press of the 2nd button, add to selection
  if (g_pButtonR->onLongClick()) {
    addSelectionMask();
  }
  // render the bulb selection
  showBulbSelection();

  // return MENU_SKIP to indicate to derived menu classes that
  // they should skip their logic and just return so that the
  // bulb selection can be rendered
  return MENU_SKIP;
}

void Menu::showBulbSelection()
{
  Leds::clearAll();
  if (ledPermutations[m_ledSelection] == MAP_LED(LED_MULTI)) {
    LedPos pos = (LedPos)((Time::getCurtime() / 30) % LED_COUNT);
    for (int dots = 0; dots < 4; ++dots) {
      LedPos dotPos = (LedPos)((pos + (dots * (LED_COUNT / 4))) % LED_COUNT);
      Leds::blinkIndexOffset(dotPos, dotPos * 10, 50, 500, RGB_MAGENTA1);
    }
  } else {
    Leds::setMap(m_targetLeds, RGB_ORANGE);
    Leds::blinkMap(ledPermutations[m_ledSelection], BULB_SELECT_OFF_MS, BULB_SELECT_ON_MS, RGB_MAGENTA1);
  }
  // blink when selecting
  Menus::showSelection(RGB_MAGENTA1);
}

void Menu::showExit()
{
  if (g_pButtonM->isPressed() && g_pButtonM->holdDuration() > SHORT_CLICK_THRESHOLD_TICKS) {
    Leds::setAll(RGB_RED);
    return;
  }
  Leds::clearAll();
  Leds::setAll(RGB_WHITE0);
  Leds::blinkAll(EXIT_MENU_OFF_MS, EXIT_MENU_ON_MS, RGB_RED0);
}

void Menu::bypassLedSelection(LedMap map)
{
  m_ledSelected = true;
  m_targetLeds = map;
  onLedSelected();
}

void Menu::onLedSelected()
{
}

void Menu::onShortClickL()
{
}

void Menu::onShortClickM()
{
}

void Menu::onShortClickR()
{
}

void Menu::onLongClickL()
{
  leaveMenu(false);
}

void Menu::onLongClickM()
{
  leaveMenu(false);
}

void Menu::onLongClickR()
{
}

void Menu::leaveMenu(bool doSave)
{
  m_shouldClose = true;
  if (doSave) {
    Modes::saveStorage();
  }
}

// this adds the currently targeted ledPermutation to the selected leds
void Menu::addSelectionMask() {
  // if selecting any of the individual leds then toggle
  uint32_t mask = ledPermutations[m_ledSelection];
  // checks if only 1 b it is set in the target mask
  if ((mask & (mask - 1)) == 0) {
    // if there's only one bit set then toggle that location
    m_targetLeds ^= mask;
  } else {
    // otherwise just add the mask whatever it is
    m_targetLeds |= mask;
  }
}
