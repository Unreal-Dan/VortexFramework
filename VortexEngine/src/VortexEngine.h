#ifndef VORTEX_ENGINE_H
#define VORTEX_ENGINE_H

#include <inttypes.h>
#include "VortexConfig.h"

#include "Patterns/PatternBuilder.h"
#include "Wireless/IRReceiver.h"
#include "Wireless/IRSender.h"
#include "Wireless/VLReceiver.h"
#include "Wireless/VLSender.h"
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

class ByteStream;
class Mode;
class Vortex;

class VortexEngine
{
public:
  VortexEngine(Vortex &vortexLib);
  ~VortexEngine();

  // setup and initialization function
  bool init();
  // cleanup
  void cleanup();

  // tick function for each loop
  void tick();

  // run the main logic of the engine
  void runMainLogic();

  // serialize the version number to a byte stream
  bool serializeVersion(ByteStream &stream);
  bool checkVersion(uint8_t major, uint8_t minor);

  // get the current mode
  Mode *curMode();

#ifdef VORTEX_LIB
  // the total available storage space
  uint32_t totalStorageSpace();
  // the size of the savefile
  uint32_t savefileSize();
  // whether sleeping or not
  bool isSleeping() { return m_sleeping; }
#endif

  // enter/leave sleep mode
  void enterSleep(bool save = true);
  void wakeup(bool reset = true);

  // toggle auto cycle enabled
  void setAutoCycle(bool enabled) { m_autoCycle = enabled; }

  // reference to the VortexLib that created this instance
  Vortex &vortexLib() { return m_vortexLib; }

  // references to various managers
  SerialComs &serial() { return m_serial; }
  Time &time() { return m_time; }
  Storage &storage() { return m_storage; }
  IRReceiver &irReceiver() { return m_irReceiver; }
  IRSender &irSender() { return m_irSender; }
  VLReceiver &vlReceiver() { return m_vlReceiver; }
  VLSender &vlSender() { return m_vlSender; }
  Leds &leds() { return m_leds; }
  Buttons &buttons() { return m_buttons; }
  Button &button(uint8_t index = 0) { return m_buttons.button(index); }
  Menus &menus() { return m_menus; }
  Modes &modes() { return m_modes; }
  PatternBuilder &patternBuilder() { return m_patternBuilder; }

private:
  // reference to the parent vortexlib that created this instance
  Vortex &m_vortexLib;

  // reference to self to make some macros work that use m_engine
  VortexEngine &m_engine;

  // instances of main managers
  SerialComs m_serial;
  Time m_time;
  Storage m_storage;
  IRReceiver m_irReceiver;
  IRSender m_irSender;
  VLReceiver m_vlReceiver;
  VLSender m_vlSender;
  Leds m_leds;
  Buttons m_buttons;
  Menus m_menus;
  Modes m_modes;
  PatternBuilder m_patternBuilder;

  // bool in vortexlib to simulate sleeping
  volatile bool m_sleeping;

  // whether auto cycle modes is turned on
  bool m_autoCycle;

#if COMPRESSION_TEST == 1
  void compressionTest();
#endif
#if SERIALIZATION_TEST == 1
  void serializationTest();
#endif
#if TIMER_TEST == 1
  void timerTest();
#endif
};

#endif
