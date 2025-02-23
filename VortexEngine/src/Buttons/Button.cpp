#include "Button.h"

#include "../Time/TimeControl.h"
#include "../Time/Timings.h"
#include "../Log/Log.h"

#ifdef VORTEX_LIB
#include "VortexLib.h"
#endif

#ifdef VORTEX_EMBEDDED
#include <Arduino.h>
#endif

Button::Button() :
  m_pinNum(0),
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
  m_pinNum = 0;
  m_pressTime = 0;
  m_releaseTime = 0;
  m_holdDuration = 0;
  m_releaseDuration = 0;
  m_consecutivePresses = 0;
  m_releaseCount = 0;
  m_buttonState = false;
  m_newPress = false;
  m_newRelease = false;
  m_isPressed = m_buttonState;
  m_shortClick = false;
  m_longClick = false;

  m_pinNum = pin;
#ifdef VORTEX_EMBEDDED
  pinMode(m_pinNum, INPUT_PULLUP);
#endif
  return true;
}

bool Button::check()
{
#ifdef VORTEX_EMBEDDED
  return (digitalRead(m_pinNum) == 0);
#else
  return (Vortex::vcallbacks()->checkPinHook(m_pinNum) == 0);
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
      // the button was just released
      m_releaseTime = Time::getCurtime();
      m_newRelease = true;
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
