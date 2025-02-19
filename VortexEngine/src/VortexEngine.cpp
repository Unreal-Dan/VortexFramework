#include "VortexEngine.h"

#include "Wireless/IRReceiver.h"
#include "Wireless/IRSender.h"
#include "Wireless/VLReceiver.h"
#include "Wireless/VLSender.h"
#include "Wireless/IRConfig.h"
#include "Wireless/VLConfig.h"
#include "Storage/Storage.h"
#include "Buttons/Buttons.h"
#include "Time/TimeControl.h"
#include "Time/Timings.h"
#include "Serial/Serial.h"
#include "Modes/Modes.h"
#include "Menus/Menus.h"
#include "Modes/Mode.h"
#include "Leds/Leds.h"

#ifdef VORTEX_EMBEDDED
#include <avr/interrupt.h>
#include <avr/sleep.h>
#endif

#ifdef VORTEX_LIB
#include "VortexLib.h"
#endif

// bool in vortexlib to simulate sleeping
volatile bool VortexEngine::m_sleeping = false;
// whether the 'force sleep' option is available (hold down for long time)
bool VortexEngine::m_forceSleepEnabled = true;

// auto cycling
bool VortexEngine::m_autoCycle = false;

bool VortexEngine::init()
{
#ifdef VORTEX_EMBEDDED
  // clear the output pins to initialize everything
  clearOutputPins();
#endif

  // all of the global controllers
  if (!SerialComs::init()) {
    DEBUG_LOG("Serial failed to initialize");
    return false;
  }
  if (!Time::init()) {
    DEBUG_LOG("Time failed to initialize");
    return false;
  }
  if (!Storage::init()) {
    DEBUG_LOG("Storage failed to initialize");
    return false;
  }
#if IR_ENABLE_RECEIVER == 1
  if (!IRReceiver::init()) {
    DEBUG_LOG("IRReceiver failed to initialize");
    return false;
  }
#endif
#if IR_ENABLE_SENDER == 1
  if (!IRSender::init()) {
    DEBUG_LOG("IRSender failed to initialize");
    return false;
  }
#endif
#if VL_ENABLE_RECEIVER == 1
  if (!VLReceiver::init()) {
    DEBUG_LOG("VLReceiver failed to initialize");
    return false;
  }
#endif
#if VL_ENABLE_SENDER == 1
  if (!VLSender::init()) {
    DEBUG_LOG("VLSender failed to initialize");
    return false;
  }
#endif
  if (!Leds::init()) {
    DEBUG_LOG("Leds failed to initialize");
    return false;
  }
  if (!Buttons::init()) {
    DEBUG_LOG("Buttons failed to initialize");
    return false;
  }
  if (!Menus::init()) {
    DEBUG_LOG("Menus failed to initialize");
    return false;
  }
  if (!Modes::init()) {
    DEBUG_LOG("Settings failed to initialize");
    return false;
  }

#if COMPRESSION_TEST == 1
  compressionTest();
#endif

#if SERIALIZATION_TEST == 1
  serializationTest();
#endif

#if TIMER_TEST == 1
  timerTest();
#endif

#ifdef VORTEX_EMBEDDED
  // setup TCB0 to track micros() and run ticks
  TCB0.CCMP = 10000;
  TCB0.INTCTRL = TCB_CAPT_bm;
  // Set clock source to CPU divided by 1, Keep running in sleep mode, Enable TCB
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_RUNSTDBY_bm | TCB_ENABLE_bm;
  // set the state of the mosfet based on whether the chip is locked or not
  enableMOSFET(!Modes::locked());
  // setup sleep mode for standby
  set_sleep_mode(SLEEP_MODE_STANDBY);
  // enable interrupts
  sei();
  // standby indefinitely while the ISR runs VortexEngine::tick
  while (!m_sleeping) {
    sleep_mode();
  }
#endif

  return true;
}

void VortexEngine::cleanup()
{
  // cleanup in reverse order
  // NOTE: the embedded doesn't actually cleanup,
  //       but the test frameworks do
#ifdef VORTEX_LIB
  Modes::cleanup();
  Menus::cleanup();
  Buttons::cleanup();
  Leds::cleanup();
#if VL_ENABLE_SENDER == 1
  VLSender::cleanup();
#endif
#if VL_ENABLE_RECEIVER == 1
  VLReceiver::cleanup();
#endif
#if IR_ENABLE_SENDER == 1
  IRSender::cleanup();
#endif
#if IR_ENABLE_RECEIVER == 1
  IRReceiver::cleanup();
#endif
  Storage::cleanup();
  Time::cleanup();
  SerialComs::cleanup();
#endif
}

void VortexEngine::tick()
{
#ifdef VORTEX_LIB
  if (m_sleeping) {
    // update the buttons to check for wake
    Buttons::update();
    // several fast clicks will unlock the device
    if (Modes::locked() && g_pButton->onConsecutivePresses(DEVICE_LOCK_CLICKS - 1)) {
      // turn off the lock flag and save it to disk
      Modes::setLocked(false);
    }
    // check for any kind of press to wakeup
    if (g_pButton->check() || g_pButton->onRelease() || !Vortex::sleepEnabled()) {
      wakeup();
    }
    return;
  }
#endif

  // tick the current time counter forward
  Time::tickClock();

  // poll the button(s) and update the button object states
  Buttons::update();

  // run the main logic for the engine
  runMainLogic();

  // update the leds
  Leds::update();
}

// warning this function is quite heavy at this point, it could probably be split
void VortexEngine::runMainLogic()
{
  // the current tick
  uint32_t now = Time::getCurtime();

  // if the device is locked then that takes priority over all, while locked the
  // device will only listen for clicks to wakeup momentarily then go back to sleep
  if (Modes::locked()) {
    // several fast clicks will unlock the device
    if (g_pButton->onConsecutivePresses(DEVICE_LOCK_CLICKS - 1)) {
      // turn off the lock flag and save it to disk
      Modes::setLocked(false);
#ifdef VORTEX_EMBEDDED
      // then enable the mosfet
      enableMOSFET(true);
#endif
    } else if (now > (CONSECUTIVE_WINDOW_TICKS * DEVICE_LOCK_CLICKS)) {
      // go back to sleep if they don't unlock in time, don't save
      enterSleep(false);
    }
    // OPTIONAL: render a dim led during unlock window waiting for clicks?
    //Leds::setIndex(LED_1, RGB_RED4);
    Leds::clearAll();
    // don't do anything else while locked, just return
    return;
  }

  // the device is not locked, proceed with regular logic

  // if the button hasn't been released since turning on then there is custom logic
  if (g_pButton->releaseCount() == 0) {
    if (!Modes::load()) {
      return;
    }
    // if the button is held for 2 seconds from off, switch to on click mode on
    // the last mode shown before sleep
    if (!Modes::keychainModeEnabled() && now == ONE_CLICK_THRESHOLD_TICKS && g_pButton->isPressed()) {
      // whether oneclick mode is now enabled
      bool isEnabledNow = !Modes::oneClickModeEnabled();
      // toggle one click mode
      Modes::setOneClickMode(isEnabledNow);
      // if we turned it on then switch to that mode
      if (isEnabledNow) {
        Modes::switchToStartupMode();
      } else {
        Modes::setCurMode(0);
      }
      // flash either low white or dim white2 to indicate
      // whether one-click mode has been turned on or off
      Leds::holdAll(isEnabledNow ? RGB_WHITE0 : RGB_WHITE5);
    }
    return;
  }

#ifdef VORTEX_EMBEDDED
  // originally this check was believed to protect against ESD but not so sure
  // anymore, it's clear that it stops the chip from turning on when it initially
  // receives power but ESD tests fail now (put chip in sandwich bag and shake it).
  // This is now where initialization runs right after a fresh firmware flash.
  // The first tick of the engine is 1 not 0 because oops.
  if (now == 1) {
    // This check is for whether a new firmware was just flashed, if a new
    // firmware was flashed then write out a new save header
    if (Modes::getFlag(MODES_FLAG_NEW_FIRMWARE)) {
      // reset the flags back to normal
      Modes::resetFlags();
      // try to load the modes and hope it works, need the num modes to write a
      // new save header and they may have backed up and restored a custom number
      // of modes
      Modes::load();
      // then save the new header with current version and num modes etc
      Modes::saveHeader();
    }
    // then just gracefully go back to sleep to prevent the chip from turning
    // on randomly when it receives power
    enterSleep(false);
    return;
  }
#endif

  if (!Modes::load()) {
    return;
  }

  // finally the user has released the button after initially turning it on,
  // just run the regular main logic of the system

  // re-enter keychain mode if it was never disabled
  if (Modes::keychainModeEnabled() && !Menus::checkInMenu()) {
    // switch to the last mode we were on
    Modes::switchToStartupMode();
    // enter keychain mode menu
    Menus::openMenu(MENU_GLOBAL_BRIGHTNESS, true);
  }

  // first look for the force-sleep and instant on/off toggle
  const uint32_t holdTime = g_pButton->holdDuration();
  // force-sleep check takes precedence above all, but it does not run when keychain mode is enabled
  if (m_forceSleepEnabled && holdTime >= FORCE_SLEEP_THRESHOLD_TICKS) {
    // as long as they hold down past this threshold just turn off
    if (g_pButton->isPressed()) {
      Leds::clearAll();
      return;
    }
    // but as soon as they actually release put the device to sleep
    if (g_pButton->onRelease()) {
      // do not save on force sleep
      enterSleep(false);
    }
    return;
  }

  // run the menus to see if they are open and need to do anything
  if (Menus::run()) {
    // if they return true that means the menus are open and rendering so just return
    return;
  }

  // if the user releases the button after the sleep threshold and
  // we're still in menu state not open, then we can go to sleep
  if (g_pButton->onRelease() && holdTime >= SLEEP_ENTER_THRESHOLD_TICKS) {
    // enter sleep mode
    enterSleep();
    return;
  }

  // if the button is just held beyond the sleep threshold then
  if (g_pButton->isPressed() && holdTime >= SLEEP_ENTER_THRESHOLD_TICKS) {
    // clear all the leds for a short moment
    Leds::clearAll();
    // then oncethe user holds past the sleep window threshold open up the menus
    if (holdTime >= (SLEEP_ENTER_THRESHOLD_TICKS + SLEEP_WINDOW_THRESHOLD_TICKS)) {
      // open the menu selection area
      DEBUG_LOG("Entering ring fill...");
      Menus::openMenuSelection();
    }
    // don't play the modes because the user is going into menus
    return;
  }

  // lastly check if we are locking the device, which can only happen if they click the
  // button 5 times quickly when the device was off, so 4 times in the first x ticks because
  // the first click was used to wake the device and isn't counted in the consecutive clicks
  if (now < (CONSECUTIVE_WINDOW_TICKS * DEVICE_LOCK_CLICKS) && g_pButton->onConsecutivePresses(DEVICE_LOCK_CLICKS - 1)) {
#ifdef VORTEX_LIB
    if (!Vortex::lockEnabled()) {
      return;
    }
#endif
    // lock and just go to sleep, don't need to reset consecutive press counter here
    Modes::setLocked(true);
    enterSleep();
    return;
  }

  // toggle auto cycle mode with many clicks at main modes
  if ((g_pButton->onRelease() && m_autoCycle) || g_pButton->onConsecutivePresses(AUTO_CYCLE_MODES_CLICKS)) {
    m_autoCycle = !m_autoCycle;
    Leds::holdAll(m_autoCycle ? RGB_GREEN : RGB_RED);
  }

  // if auto cycle is enabled and the last switch was more than the delay ago
  if (m_autoCycle && (Modes::lastSwitchTime() + AUTO_RANDOM_DELAY < now)) {
    // then switch to the next mode automatically
    Modes::nextMode();
  }

  // otherwise just play the modes
  Modes::play();
}

bool VortexEngine::serializeVersion(ByteStream &stream)
{
  // serialize the vortex version
  if (!stream.serialize8((uint8_t)VORTEX_VERSION_MAJOR)) {
    return false;
  }
  if (!stream.serialize8((uint8_t)VORTEX_VERSION_MINOR)) {
    return false;
  }
  return true;
}

bool VortexEngine::checkVersion(uint8_t major, uint8_t minor)
{
  if (major != VORTEX_VERSION_MAJOR) {
    return false;
  }
  // minor version doesn't matter
  return true;
}

Mode *VortexEngine::curMode()
{
#ifdef VORTEX_LIB
  return Modes::curMode();
#else
  // don't need this outside vortex lib
  return nullptr;
#endif
}

#ifdef VORTEX_LIB
uint32_t VortexEngine::totalStorageSpace()
{
  return STORAGE_SIZE;
}

uint32_t VortexEngine::savefileSize()
{
  return Storage::lastSaveSize();
}
#endif

void VortexEngine::enterSleep(bool save)
{
  DEBUG_LOG("Sleeping");
  if (save) {
    // update the startup mode when going to sleep
    Modes::setStartupMode(Modes::curModeIndex());
    // save anything that hasn't been saved
    Modes::saveStorage();
  }
  // clear all the leds
  Leds::clearAll();
  Leds::update();
#ifdef VORTEX_EMBEDDED
  // init the output pins to prevent any floating pins
  clearOutputPins();
  // close the mosfet so that power cannot flow to the leds
  enableMOSFET(false);
  // delay for a bit to let the mosfet close and leds turn off
  Time::delayMicroseconds(250);
  // this is an ISR that runs in the timecontrol system to handle
  // micros, it will wake the device up periodically
  TCB0.INTCTRL = 0;
  TCB0.CTRLA = 0;
  // Enable wake on interrupt for the button
  g_pButton->enableWake();
  // Set sleep mode to POWER DOWN mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  // enable the sleep bool right before we enter sleep, this will allow
  // the main loop to break and return
  m_sleeping = true;
  // enter sleep
  sleep_mode();
#else
  // enable the sleep bool
  m_sleeping = true;
#endif
}

void VortexEngine::wakeup(bool reset)
{
  DEBUG_LOG("Waking up");
#ifdef VORTEX_EMBEDDED
  // turn the LED mosfet back on
  enableMOSFET(true);
  if (reset) {
    // just reset
    _PROTECTED_WRITE(RSTCTRL.SWRR, 1);
  }
#else
  // need to fake the reset in vortexlib, lol this works I guess
  m_sleeping = false;
  if (reset) {
    cleanup();
    init();
  }
#endif
}

#ifdef VORTEX_EMBEDDED
// main tick function
ISR(TCB0_INT_vect)
{
  // Clear interrupt flag
  TCB0.INTFLAGS = TCB_CAPT_bm;
  VortexEngine::tick();
}

void VortexEngine::clearOutputPins()
{
  // Set all pins to input with pull-ups
  PORTA.DIRCLR = 0xFF;
  PORTA.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTA.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTA.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTA.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTA.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTA.PIN5CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTA.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTB.DIRCLR = 0xFF;
  PORTB.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTB.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTB.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTB.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTB.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTB.PIN5CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTB.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTB.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTC.DIRCLR = 0xFF;
  PORTC.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTC.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTC.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTC.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTC.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTC.PIN5CTRL = PORT_ISC_INPUT_DISABLE_gc;
}

void VortexEngine::enableMOSFET(bool enabled)
{
  PORTC.DIRSET |= PIN4_bm;
  if (enabled) {
    // Set Mosfet pin (PC4) to output and set it HIGH
    PORTC.OUTSET |= PIN4_bm;
  } else {
    // Set Mosfet pin (PC4) to output and set it LOW
    PORTC.OUTCLR |= PIN4_bm;
  }
}
#endif

#if COMPRESSION_TEST == 1
#include <string.h>
#include <stdio.h>
#include "Colors/Colorset.h"
#include "Memory/Memory.h"
#include "Random/Random.h"
void VortexEngine::compressionTest()
{
  // always use same seed
  Random rand(0xdeadbeef);
  ByteStream stream;
  Modes::clearModes();
  for (uint32_t len = 1; len < 4096; ++len) {
    uint8_t *buf = (uint8_t *)vcalloc(1, len + 1);
    if (!buf) {
      continue;
    }
    ByteStream modeStream;
    Modes::serialize(modeStream);
    while (modeStream.size() < len) {
      Mode tmpMode;
      tmpMode.setPattern((PatternID)(len % PATTERN_COUNT));
      Colorset set;
      set.randomizeColorTheory(rand, 8);
      tmpMode.setColorset(set);
      Modes::addMode(&tmpMode);
      modeStream.clear();
      Modes::serialize(modeStream);
    }
    stream = modeStream;
#if 0
    for (uint32_t i = 0; i < len; ++i) {
      buf[i] = (uint8_t)i&0xFF;
    }
    stream.init(len, buf);
#endif
    stream.compress();
    int complen = stream.size();
    stream.decompress();
    if (memcmp(stream.data(), modeStream.data(), modeStream.size()) != 0) {
      ERROR_LOGF("Buffers not equal: %u", modeStream.size());
      INFO_LOG("\t");
      for (uint32_t i = 0; i < len; ++i) {
        printf("%02x ", modeStream.data()[i]);
        if (i > 0 && ((i + 1) % 32) == 0) {
          printf("\r\n\t");
        }
      }
      return;
    }
    if (modeStream.size() != (size_t)complen) {
      DEBUG_LOGF("Success %u compressed to %u", modeStream.size(), complen);
    }
    free(buf);
  }
  Modes::clearModes();
  DEBUG_LOG("Success testing compression");
}
#endif

#if SERIALIZATION_TEST == 1
#include "Modes/Mode.h"
#include "Colors/Colorset.h"
#include "Patterns/Pattern.h"
#include "Patterns/PatternArgs.h"
#include "Patterns/PatternBuilder.h"
#include "Random/Random.h"
#include <stdio.h>
void VortexEngine::serializationTest()
{
  Colorset bigSet;
  Random rand;
  for (uint32_t i = 0; i < MAX_COLOR_SLOTS; ++i) {
    bigSet.addColorWithValueStyle(rand, i * 31, 255, Colorset::VAL_STYLE_ALTERNATING, MAX_COLOR_SLOTS);
  }
  DEBUG_LOG("== Beginning Serialization Test ==");
  for (PatternID patternID = PATTERN_FIRST; patternID < PATTERN_COUNT; ++patternID) {
    PatternArgs defaults = PatternBuilder::getDefaultArgs(patternID);
    PatternArgs args(0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x11, 0x22, 0x33);
    args.numArgs = defaults.numArgs;
    Mode tmpMode(patternID, &args, &bigSet);
    tmpMode.init();
    ByteStream buffer;
    tmpMode.serialize(buffer);
    if (!buffer.size()) {
      ERROR_LOGF("ERROR!! Buffer empty after serialize on %u", patternID);
      return;
    }
    Mode tmpMode2;
    tmpMode2.unserialize(buffer);
    if (!buffer.unserializerAtEnd()) {
      ERROR_LOGF("ERROR!! Unserializer still has data on %u:", patternID);
      uint8_t byte = 0;
      uint32_t count = 0;
      while (buffer.unserialize(&byte)) {
        INFO_LOGF("%02x ", byte);
        if ((++count % 32) == 0) {
          INFO_LOG("\n");
        }
      }
      INFO_LOG("\n");
      return;
    }
    if (!tmpMode.equals(&tmpMode2)) {
      ERROR_LOGF("ERROR!! Modes are not equal on %u", patternID);
      return;
    }
    PatternArgs pulledArgs;
    tmpMode2.getPattern()->getArgs(pulledArgs);
    if (pulledArgs != args) {
      DEBUG_LOGF("ERROR!! Pattern args are not equal on %u", patternID);
      return;
    }
    DEBUG_LOGF("Success pattern %u serialized cleanly", patternID);
  }
  DEBUG_LOG("All patterns serialized successfully...");
  DEBUG_LOG("Attempting full serialization of all modes");
  Modes::clearModes();
  for (PatternID patternID = PATTERN_FIRST; patternID < PATTERN_COUNT; ++patternID) {
    if (!Modes::addMode(patternID, nullptr, &bigSet)) {
      ERROR_LOGF("ERROR!! Failed to add mode %u", patternID);
      return;
    }
  }
  ByteStream buffer;
  Modes::serialize(buffer);
  if (!buffer.size()) {
    ERROR_LOG("ERROR!! Buffer empty after modes serialize");
    return;
  }
  Modes::clearModes();
  Modes::unserialize(buffer);
  if (!buffer.unserializerAtEnd()) {
    ERROR_LOG("ERROR!! Unserializer still has data after modes unserialize:");
    uint8_t byte = 0;
    uint32_t count = 0;
    while (buffer.unserialize(&byte)) {
      INFO_LOGF("%02x ", byte);
      if ((++count % 32) == 0) {
        printf("\n");
      }
    }
    printf("\n");
    return;
  }
  Modes::clearModes();
  DEBUG_LOG("Success all modes serialized cleanly");
  DEBUG_LOG("== SUCCESS RUNNING SERIALIZATION TEST ==");
}
#endif

#if TIMER_TEST == 1
#include "Time/TimeControl.h"
#include "Time/timer.h"
void VortexEngine::timerTest()
{
  DEBUG_LOG("== BEGINNING TIMER TESTS ==");
  Time::test();
  Timer::test();
  DEBUG_LOG("== SUCCESS TIMER TESTS PASSED ==");
}
#endif
