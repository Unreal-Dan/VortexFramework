#include "Button.h"

#include "../Time/TimeControl.h"
#include "../Time/Timings.h"
#include "../Log/Log.h"

#ifdef VORTEX_LIB
#include "VortexLib.h"
#endif

#ifdef VORTEX_EMBEDDED
#include "../VortexEngine.h"
#include <avr/interrupt.h>
#include <avr/io.h>

// Update here to change button pin/port
#define PIN_NUM 2
#define PORT_LETTER C

// expands out details to make the macros work
#define CONCATENATE_DETAIL(x, y) x##y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)
#define CONCATENATE_DETAIL_3(x, y, z) x##y##z
#define CONCATENATE_3(x, y, z) CONCATENATE_DETAIL_3(x, y, z)

// macros for the button pin/port
#define BUTTON_PORT   CONCATENATE(PORT, PORT_LETTER)
#define BUTTON_VPORT  CONCATENATE(VPORT, PORT_LETTER)
#define BUTTON_PIN    CONCATENATE_3(PIN, PIN_NUM, _bm)
#define PIN_CTRL      CONCATENATE_3(PIN, PIN_NUM, CTRL)
#define PORT_VECT     CONCATENATE_3(PORT, PORT_LETTER, _PORT_vect)

// interrupt handler to wakeup device on button press
ISR(PORT_VECT)
{
  // mark the interrupt as handled
  BUTTON_PORT.INTFLAGS = BUTTON_PIN;
  // turn off the interrupt
  BUTTON_PORT.PIN_CTRL &= ~PORT_ISC_gm;
  // call the wakeup routine in the engine
  VortexEngine::wakeup();
}

void Button::enableWake()
{
  // turn on the above interrupt for FALLING edge, maintain the pullup enabled
  BUTTON_PORT.PIN_CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
}
#endif

Button::Button() :
  m_pressTime(0),
  m_releaseTime(0),
  m_holdDuration(0),
  m_releaseDuration(0),
  m_consecutivePresses(0),
  m_releaseCount(0),
  m_buttonState(false),
  m_newPress(false),
  m_newRelease(false),
  m_isPressed(false),
  m_shortClick(false),
  m_longClick(false)
{
}

Button::~Button()
{
}

bool Button::init(uint8_t pin)
{
  m_pressTime = 0;
  m_releaseTime = 0;
  m_holdDuration = 0;
  m_releaseDuration = 0;
  m_consecutivePresses = 0;
  m_newPress = false;
  m_newRelease = false;
  m_shortClick = false;
  m_longClick = false;
  // this is weird, when I did m_releaseCount = !m_buttonState the
  // compiler generated a huge amount of assembly, but not !check()
  m_buttonState = check();
  m_releaseCount = !check();
  m_isPressed = m_buttonState;
#ifdef VORTEX_EMBEDDED
  BUTTON_PORT.PIN_CTRL = PORT_PULLUPEN_bm;
#endif
  return true;
}

bool Button::check()
{
#ifdef VORTEX_LIB
  return (Vortex::vcallbacks()->checkPinHook(0) == 0);
#else
  return ((BUTTON_VPORT.IN & BUTTON_PIN) == 0);
#endif
}

void Button::update()
{
  // reset the new press/release members this tick
  m_newPress = false;
  m_newRelease = false;

  // read the new button state
  bool newButtonState = check();

  // did the button change (press/release occurred)
  if (newButtonState != m_buttonState) {
    // set the new state
    m_buttonState = newButtonState;
    // update the currently pressed member
    m_isPressed = m_buttonState;

    // update the press/release times and newpress/newrelease members
    if (m_isPressed) {
      // the button was just pressed
      m_pressTime = Time::getCurtime();
      m_newPress = true;
    } else {
      // simply ignore the first release, always. Because they just turned the device
      // on and we don't want them to cycle to the next mode or something
      if (m_releaseCount > 0) {
        // the button was just released
        m_releaseTime = Time::getCurtime();
        m_newRelease = true;
      }
      // count releases even inside the ignore window so that
      // we can tell if the button was released to stop ignoring
      m_releaseCount++;
    }
  }

  // calculate new hold/release durations if currently held/released
  if (m_isPressed) {
    // update the hold duration as long as the button is pressed
    if (Time::getCurtime() >= m_pressTime) {
      m_holdDuration = (uint32_t)(Time::getCurtime() - m_pressTime);
    }
  } else {
    // update the release duration as long as the button is released
    if (Time::getCurtime() >= m_releaseTime) {
      m_releaseDuration = (uint32_t)(Time::getCurtime() - m_releaseTime);
      if (m_releaseDuration > CONSECUTIVE_WINDOW_TICKS) {
        // if the release duration is greater than the threshold, reset the consecutive presses
        m_consecutivePresses = 0;
      }
    }
  }

  if (m_newRelease) {
    m_consecutivePresses++;
  }

  // whether a shortclick or long click just occurred
  m_shortClick = (m_newRelease && (m_holdDuration <= SHORT_CLICK_THRESHOLD_TICKS));
  m_longClick = (m_newRelease && (m_holdDuration > SHORT_CLICK_THRESHOLD_TICKS));

  if (m_shortClick) {
    DEBUG_LOG("Short click");
  }
  if (m_longClick) {
    DEBUG_LOG("Long click");
  }
}

bool Button::onConsecutivePresses(uint8_t numPresses)
{
  if (m_consecutivePresses >= numPresses) {
    m_consecutivePresses = 0;
    return true;
  }
  return false;
}
