#ifndef BUTTON_H
#define BUTTON_H

#include <inttypes.h>

// although there is only one button on the VortexFramework
// I am still opting for a non-static button class
class Button
{
private:
  // private unimplemented copy and assignment constructors to prevent copies
  Button(Button const &);
  void operator=(Button const &);

public:
  Button();
  ~Button();

  // initialize a new button object with a pin number
  bool init(uint8_t pin);
  // directly poll the pin for whether it's pressed right now
  bool check();
  // poll the button pin and update the state of the button object
  void update();

  // whether the button was pressed this tick
  bool onPress() const { return m_newPress; }
  // whether the button was released this tick
  bool onRelease() const { return m_newRelease; }
  // whether the button is currently pressed
  bool isPressed() const { return m_isPressed; }

  // whether the button was shortclicked this tick
  bool onShortClick() const { return m_shortClick; }
  // whether the button was long clicked this tick
  bool onLongClick() const { return m_longClick; }
  // fired when a certain number of presses is reached, the consecutive press
  // counter is automatically reset when that happens
  bool onConsecutivePresses(uint8_t numPresses);

  // when the button was last pressed
  uint32_t pressTime() const { return m_pressTime; }
  // when the button was last released
  uint32_t releaseTime() const { return m_releaseTime; }

  // how long the button is currently or was last held down (in ticks)
  uint32_t holdDuration() const { return m_holdDuration; }
  // how long the button is currently or was last released for (in ticks)
  uint32_t releaseDuration() const { return m_releaseDuration; }

  // the number of consecutive presses
  uint8_t consecutivePresses() const { return m_consecutivePresses; }
  // the number of releases
  uint8_t releaseCount() const { return m_releaseCount; }

private:
  // the pin number that is read
  uint8_t m_pinNum;

  // ========================================
  // state data that is populated each check

  // the timestamp of when the button was pressed
  uint32_t m_pressTime;
  // the timestamp of when the button was released
  uint32_t m_releaseTime;

  // the last hold duration
  uint32_t m_holdDuration;
  // the last release duration
  uint32_t m_releaseDuration;

  // the number of repeated presses (automatically detects rapid presses)
  uint8_t m_consecutivePresses;
  // the number of times released, will overflow at 255
  uint8_t m_releaseCount;

  // the active state of the button
  bool m_buttonState;

  // whether pressed this tick
  bool m_newPress;
  // whether released this tick
  bool m_newRelease;
  // whether currently pressed
  bool m_isPressed;
  // whether a short click occurred
  bool m_shortClick;
  // whether a long click occurred
  bool m_longClick;

#ifdef VORTEX_LIB
  // allow the VortexLib class to manipulate the members of this
  // class so that it can inject button events to make apis like
  // Vortex::shortClick() and Vortex::longClick() possible
  friend class Vortex;
#endif
};

// See Button.cpp for info about this
extern Button *g_pButton;
extern Button *g_pButton2;

#endif
