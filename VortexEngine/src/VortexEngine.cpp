#include "VortexEngine.h"

#include "Infrared/IRReceiver.h"
#include "Infrared/IRSender.h"
#include "Storage/Storage.h"
#include "Buttons/Buttons.h"
#include "Time/TimeControl.h"
#include "Time/Timings.h"
#include "Serial/Serial.h"
#include "Modes/Modes.h"
#include "Menus/Menus.h"
#include "Modes/Mode.h"
#include "Leds/Leds.h"
#include "Log/Log.h"

#include <Arduino.h>

bool VortexEngine::init()
{
  // pull up all the pins to prevent floating pins from triggering interrupts
  //for (int p = 0; p < 21; ++p) {
  //  pinMode(p, INPUT_PULLUP);
  //}

  // turn on the mosfet to enable power to the leds
  //pinMode(1, OUTPUT);
  //digitalWrite(1, HIGH);

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
  if (!IRReceiver::init()) {
    DEBUG_LOG("Infrared receiver failed to initialize");
    return false;
  }
  if (!IRSender::init()) {
    DEBUG_LOG("Infrared sender failed to initialize");
    return false;
  }
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
  return true;
}

void VortexEngine::cleanup()
{
  // cleanup in reverse order
  // NOTE: the arduino doesn't actually cleanup,
  //       but the test frameworks do
  Modes::cleanup();
  Menus::cleanup();
  Buttons::cleanup();
  Leds::cleanup();
  IRSender::cleanup();
  IRReceiver::cleanup();
  Storage::cleanup();
  Time::cleanup();
  SerialComs::cleanup();
}

void VortexEngine::tick()
{
  // tick the current time counter forward
  Time::tickClock();

  // don't poll the button till some cycles have passed, this prevents
  // the wakeup from cycling to the next mode
  if (Time::getCurtime() > IGNORE_BUTTON_TICKS) {
    Buttons::check();
  }

  // run the main logic for the engine
  runMainLogic();

  // update the leds
  Leds::update();
}

void VortexEngine::runMainLogic()
{
  // if the menus are open and running then just return
  if (Menus::run()) {
    return;
  }
  // otherwise if the button is not pressed just run the modes
  if (!g_pButton->isPressed() || g_pButton->holdDuration() < MENU_TRIGGER_THRESHOLD_TICKS) {
    Modes::play();
  } else {
    DEBUG_LOG("Entering ring fill...");
    Menus::openRingMenu();
  }
}

void VortexEngine::serializeVersion(ByteStream &stream)
{
  // serialize the vortex version
  stream.serialize((uint8_t)VORTEX_VERSION_MAJOR);
  stream.serialize((uint8_t)VORTEX_VERSION_MINOR);
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
  return Modes::curMode();
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

#if COMPRESSION_TEST == 1
#include <stdio.h>
#include "Memory/Memory.h"
void VortexEngine::compressionTest()
{
  ByteStream stream;
  for (uint32_t len = 1; len < 4096; ++len) {
    uint8_t *buf = (uint8_t *)vcalloc(1, len + 1);
    if (!buf) {
      continue;
    }
    for (uint32_t i = 0; i < len; ++i) {
      buf[i] = (uint8_t)i&0xFF;
    }
    stream.init(len, buf);
    stream.compress();
    stream.decompress();
    if (memcmp(stream.data(), buf, len) != 0) {
      ERROR_LOGF("Buffers not equal: %u", len);
      printf("\t");
      for (uint32_t i = 0; i < len; ++i) {
        printf("%02x ", buf[i]);
        if (i > 0 && ((i + 1) % 32) == 0) {
          printf("\r\n\t");
        }
      }
      return;
    }
    DEBUG_LOGF("Success %u compressed", len);
    free(buf);
  }
  DEBUG_LOG("Success testing compression");
}
#endif

#if SERIALIZATION_TEST == 1
#include "Modes/Mode.h"
#include "Colors/Colorset.h"
#include "Patterns/Pattern.h"
#include "Patterns/PatternArgs.h"
#include "Patterns/PatternBuilder.h"
#include <stdio.h>
void VortexEngine::serializationTest()
{
  Colorset bigSet;
  for (uint32_t i = 0; i < MAX_COLOR_SLOTS; ++i) {
    bigSet.addColorByHue(i * 31);
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
        printf("%02x ", byte);
        if ((++count % 32) == 0) {
          printf("\n");
        }
      }
      printf("\n");
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
      printf("%02x ", byte);
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

