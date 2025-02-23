#ifndef VORTEX_ENGINE_H
#define VORTEX_ENGINE_H

#include <inttypes.h>
#include "VortexConfig.h"

class ByteStream;
class Mode;

class VortexEngine
{
  // private unimplemented constructor
  VortexEngine();

public:
  // setup and initialization function
  static bool init();
  // cleanup
  static void cleanup();

  // tick function for each loop
  static void tick();

  // run the main logic of the engine
  static void runMainLogic();

  // serialize the version number to a byte stream
  static bool serializeVersion(ByteStream &stream);
  static bool checkVersion(uint8_t major, uint8_t minor);

  // get the current mode
  static Mode *curMode();

#ifdef VORTEX_LIB
  // the total available storage space
  static uint32_t totalStorageSpace();
  // the size of the savefile
  static uint32_t savefileSize();
  // whether sleeping or not
  static bool isSleeping() { return m_sleeping; }
#endif

  // enter/leave sleep mode
  static void enterSleep(bool save = true);
  static void wakeup(bool reset = true);

  // toggle force sleep from working
  static void toggleForceSleep(bool enabled) { m_forceSleepEnabled = enabled; }
  // toggle auto cycle enabled
  static void setAutoCycle(bool enabled) { m_autoCycle = enabled; }

#ifdef VORTEX_EMBEDDED
  // clear output pins
  static void clearOutputPins();
  // enable/disable the mosfet
  static void enableMOSFET(bool enabled);
#endif

private:
  // bool in vortexlib to simulate sleeping
  static volatile bool m_sleeping;
  static bool m_forceSleepEnabled;

  // whether auto cycle modes is turned on
  static bool m_autoCycle;

#if COMPRESSION_TEST == 1
  static void compressionTest();
#endif
#if SERIALIZATION_TEST == 1
  static void serializationTest();
#endif
#if TIMER_TEST == 1
  static void timerTest();
#endif
};

#endif
