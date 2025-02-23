#include "VortexEngine.h"

#ifdef VORTEX_LIB
#include "VortexLib.h"
#endif

// bool in vortexlib to simulate sleeping
VortexEngine::VortexEngine(Vortex &vortexLib) :
  m_vortexLib(vortexLib),
  m_engine(*this),
  m_serial(m_engine),
  m_time(m_engine),
  m_storage(m_engine),
  m_irReceiver(m_engine),
  m_irSender(m_engine),
  m_vlReceiver(m_engine),
  m_vlSender(m_engine),
  m_leds(m_engine),
  m_buttons(m_engine),
  m_menus(m_engine),
  m_modes(m_engine),
  m_patternBuilder(m_engine),
  m_sleeping(false),
  m_autoCycle(false)
{
}

VortexEngine::~VortexEngine()
{
}

bool VortexEngine::init()
{
  // all of the global controllers
  if (!m_serial.init()) {
    DEBUG_LOG("Serial failed to initialize");
    return false;
  }
  if (!m_time.init()) {
    DEBUG_LOG("Time failed to initialize");
    return false;
  }
  if (!m_storage.init()) {
    DEBUG_LOG("Storage failed to initialize");
    return false;
  }
#if IR_ENABLE_RECEIVER == 1
  if (!m_irReceiver.init()) {
    DEBUG_LOG("IRReceiver failed to initialize");
    return false;
  }
#endif
#if IR_ENABLE_SENDER == 1
  if (!m_irSender.init()) {
    DEBUG_LOG("IRSender failed to initialize");
    return false;
  }
#endif
#if VL_ENABLE_RECEIVER == 1
  if (!m_vlReceiver.init()) {
    DEBUG_LOG("VLReceiver failed to initialize");
    return false;
  }
#endif
#if VL_ENABLE_SENDER == 1
  if (!m_vlSender.init()) {
    DEBUG_LOG("VLSender failed to initialize");
    return false;
  }
#endif
  if (!m_leds.init()) {
    DEBUG_LOG("Leds failed to initialize");
    return false;
  }
  if (!m_buttons.init()) {
    DEBUG_LOG("Buttons failed to initialize");
    return false;
  }
  if (!m_menus.init()) {
    DEBUG_LOG("Menus failed to initialize");
    return false;
  }
  if (!m_modes.init()) {
    DEBUG_LOG("Settings failed to initialize");
    return false;
  }
  if (!m_patternBuilder.init()) {
    DEBUG_LOG("Pattern builder failed to initialize");
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

  return true;
}

void VortexEngine::cleanup()
{
  // cleanup in reverse order
  // NOTE: the embedded doesn't actually cleanup,
  //       but the test frameworks do
#ifdef VORTEX_LIB
  m_patternBuilder.cleanup();
  m_modes.cleanup();
  m_menus.cleanup();
  m_buttons.cleanup();
  m_leds.cleanup();
#if VL_ENABLE_SENDER == 1
  m_vlSender.cleanup();
#endif
#if VL_ENABLE_RECEIVER == 1
  m_vlReceiver.cleanup();
#if IR_ENABLE_SENDER == 1
  m_irSender.cleanup();
#endif
#endif
#if IR_ENABLE_RECEIVER == 1
  m_irReceiver.cleanup();
#endif
  m_storage.cleanup();
  m_time.cleanup();
  m_serial.cleanup();
#endif
}

void VortexEngine::tick()
{
#ifdef VORTEX_LIB
  if (m_sleeping) {
    // update the buttons to check for wake
    m_buttons.update();
    // several fast clicks will unlock the device
    if (m_modes.locked() && m_engine.button().onConsecutivePresses(DEVICE_LOCK_CLICKS - 1)) {
      // turn off the lock flag and save it to disk
      m_modes.setLocked(false);
    }
    // check for any kind of press to wakeup
    if (m_engine.button().check() || m_engine.button().onRelease() || !m_vortexLib.sleepEnabled()) {
      wakeup();
    }
    return;
  }
#endif

  // tick the current time counter forward
  m_time.tickClock();

  // poll the button(s) and update the button object states
  m_buttons.update();

  // run the main logic for the engine
  runMainLogic();

  // update the leds
  m_leds.update();
}

void VortexEngine::runMainLogic()
{
  // the current tick
  uint32_t now = m_time.getCurtime();

  // load modes if necessary
  if (!m_engine.modes().load()) {
    // don't do anything if modes couldn't load
    return;
  }

  // if the menus are open and running then just return
  if (m_menus.run()) {
    return;
  }

  // check if we should enter the menu
  if (m_engine.button().isPressed() && m_engine.button().holdDuration() > MENU_TRIGGER_THRESHOLD_TICKS) {
    DEBUG_LOG("Entering Menu Selection...");
    m_menus.openMenuSelection();
    return;
  }

  // toggle auto cycle mode with many clicks at main modes
  if ((m_engine.button().onRelease() && m_autoCycle) || m_engine.button().onConsecutivePresses(AUTO_CYCLE_MODES_CLICKS)) {
    m_autoCycle = !m_autoCycle;
    m_leds.holdAll(m_autoCycle ? RGB_GREEN : RGB_RED);
  }

  // if auto cycle is enabled and the last switch was more than the delay ago
  if (m_autoCycle && (m_modes.lastSwitchTime() + AUTO_RANDOM_DELAY < now)) {
    // then switch to the next mode automatically
    m_modes.nextMode();
  }

  // otherwise just play the modes
  m_modes.play();
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
  return m_modes.curMode();
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
  return m_storage.lastSaveSize();
}
#endif

void VortexEngine::enterSleep(bool save)
{
  DEBUG_LOG("Sleeping");
  if (save) {
    // update the startup mode when going to sleep
    m_modes.setStartupMode(m_modes.curModeIndex());
    // save anything that hasn't been saved
    m_modes.saveStorage();
  }
  // clear all the leds
  m_leds.clearAll();
  m_leds.update();
  // enable the sleep bool
  m_sleeping = true;
}

void VortexEngine::wakeup(bool reset)
{
  DEBUG_LOG("Waking up");
  m_sleeping = false;
  // need to fake the reset in vortexlib, lol this works I guess
  if (reset) {
    cleanup();
    init();
  }
}

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
  m_modes.clearModes();
  for (uint32_t len = 1; len < 4096; ++len) {
    uint8_t *buf = (uint8_t *)vcalloc(1, len + 1);
    if (!buf) {
      continue;
    }
    ByteStream modeStream;
    m_modes.serialize(modeStream);
    while (modeStream.size() < len) {
      Mode tmpMode;
      tmpMode.setPattern((PatternID)(len % PATTERN_COUNT));
      Colorset set;
      set.randomizeColorTheory(rand, 8);
      tmpMode.setColorset(set);
      m_modes.addMode(&tmpMode);
      modeStream.clear();
      m_modes.serialize(modeStream);
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
  m_modes.clearModes();
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
  m_modes.clearModes();
  for (PatternID patternID = PATTERN_FIRST; patternID < PATTERN_COUNT; ++patternID) {
    if (!m_modes.addMode(patternID, nullptr, &bigSet)) {
      ERROR_LOGF("ERROR!! Failed to add mode %u", patternID);
      return;
    }
  }
  ByteStream buffer;
  m_modes.serialize(buffer);
  if (!buffer.size()) {
    ERROR_LOG("ERROR!! Buffer empty after modes serialize");
    return;
  }
  m_modes.clearModes();
  m_modes.unserialize(buffer);
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
  m_modes.clearModes();
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
  m_time.test();
  Timer::test();
  DEBUG_LOG("== SUCCESS TIMER TESTS PASSED ==");
}
#endif
