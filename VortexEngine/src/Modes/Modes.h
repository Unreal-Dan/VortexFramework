#ifndef MODES_H
#define MODES_H

#include "../Serial/ByteStream.h"
#include "../Colors/ColorTypes.h"
#include "../Patterns/Patterns.h"
#include "../Leds/Leds.h"

#include "../VortexConfig.h"

#include <inttypes.h>

class PatternArgs;
class Colorset;
class Mode;

// the device is locked and needs to be unlocked to turn on
#define MODES_FLAG_LOCKED     (1 << 0)
// the device will turn on and off with a single click
#define MODES_FLAG_ONE_CLICK  (1 << 1)
// toggles the advanced menus being accessible
#define MODES_FLAG_ADV_MENUS  (1 << 2)
// keychain light mode (behave like a keychain)
#define MODES_FLAG_KEYCHAIN   (1 << 3)
// WARNING!! The upper 4 bits of the flags are taken by the startup mode id,
//           you can only use 4 global flags!

// This is a new special feature used by the Chromadeck when it flashes a new
// firmware to the Duo it will set the global flags to 0xF0 like this:
//
//   0 = button lock enabled
//   0 = one click mode enabled
//   0 = advanced menus enabled
//   0 = keychain mode enabled
//   1111 = Startup Mode Index 15 (impossible)
//
// If this flag is present then the Duo needs to turn on and write out it's
// save header because a new firmware has been flashed and the save header
// will still have the old version number saved inside
#define MODES_FLAG_NEW_FIRMWARE 0xF0

class Modes
{
  // private unimplemented constructor
  Modes();

public:
  // opting for static class here because there should only ever be one
  // Modes control object and I don't like singletons
  static bool init();
  static void cleanup();

  // load modes so they are ready to play
  static bool load();

  // play the current mode
  static void play();

  // full save/load to/from buffer
  static bool saveToBuffer(ByteStream &saveBuffer);
  static bool loadFromBuffer(ByteStream &saveBuffer);

  // save/load the global settings to/from storage
  static bool saveHeader();
  static bool loadHeader();

  // full save/load to/from storage
  static bool saveStorage();
  static bool loadStorage();

  // save load the savefile header from storage
  static bool serializeSaveHeader(ByteStream &saveBuffer);
  static bool unserializeSaveHeader(ByteStream &saveBuffer);

  // saves all modes to a buffer
  static bool serialize(ByteStream &buffer);
  // load all modes from a buffer, optionally specify number of leds
  static bool unserialize(ByteStream &buffer);

  // set default settings (must save after)
  static bool setDefaults();

  // shift the current mode to a different position relative to current position
  // negative values for up, positive values for down, 0 for no move
  static bool shiftCurMode(int32_t offset = 1);

  // add a new mode in various different ways
  static bool addMode(PatternID id, RGBColor c1, RGBColor c2 = RGB_OFF,
    RGBColor c3 = RGB_OFF, RGBColor c4 = RGB_OFF, RGBColor c5 = RGB_OFF,
    RGBColor c6 = RGB_OFF, RGBColor c7 = RGB_OFF, RGBColor c8 = RGB_OFF);
  static bool addMode(PatternID id, const PatternArgs *args, const Colorset *set);
  static bool addMode(const Mode *mode);

  // add a new mode by unserializing raw
  static bool addSerializedMode(ByteStream &serializedMode);
  // add a new mode by loading from a save buffer
  static bool addModeFromBuffer(ByteStream &serializedMode);

  // update the current mode to match the given mode, optionally save
  static bool updateCurMode(const Mode *mode);

  // set the current active mode by index
  static Mode *setCurMode(uint8_t index);

  // get the current mode
  static Mode *curMode();
  // iterate to next mode and return it
  static Mode *nextMode();
  // iterate to previous mode and return it
  static Mode *previousMode();

  // iterate to the next mode but skip empty modes, will not skip mode 0
  // to prevent possibly skipping all modes and ending in an infinite loop
  static Mode *nextModeSkipEmpty();

  // the number of modes
  static uint8_t numModes() { return m_numModes; }
  static uint8_t curModeIndex() { return m_curMode; }

  // the last time the modes switched to a new mode
  static uint32_t lastSwitchTime() { return m_lastSwitchTime; }

  // delete the current mode
  static void deleteCurMode();

  // delete all modes in the list
  static void clearModes();

  // set the startup mode index (which mode will be displayed on startup)
  static void setStartupMode(uint8_t index);
  static uint8_t startupMode();
  static Mode *switchToStartupMode();

  // set or get flags
  static bool setFlag(uint8_t flag, bool enable, bool save = true);
  static bool getFlag(uint8_t flag) { return ((m_globalFlags & flag) == flag); }

  // reset flags to factory default (must save after)
  static void resetFlags() { m_globalFlags = 0; }

  // inline functions to toggle the various flags
  static bool setOneClickMode(bool enable, bool save = true) {
    return setFlag(MODES_FLAG_ONE_CLICK, enable, save);
  }
  static bool oneClickModeEnabled() {
    return getFlag(MODES_FLAG_ONE_CLICK);
  }
  // toggle the locked state
  static bool setLocked(bool locked, bool save = true) {
    return setFlag(MODES_FLAG_LOCKED, locked, save);
  }
  static bool locked() {
    return getFlag(MODES_FLAG_LOCKED);
  }
  // toggle advanced menus
  static bool setAdvancedMenus(bool active, bool save = true) {
    return setFlag(MODES_FLAG_ADV_MENUS, active, save);
  }
  static bool advancedMenusEnabled() {
    return getFlag(MODES_FLAG_ADV_MENUS);
  }
  // toggle the keychain light mode
  static bool setKeychainMode(bool active, bool save = true) {
    return setFlag(MODES_FLAG_KEYCHAIN, active, save);
  }
  static bool keychainModeEnabled() {
    return getFlag(MODES_FLAG_KEYCHAIN);
  }

#if MODES_TEST == 1
  static void test();
#endif

#ifdef VORTEX_LIB
  // get the maximum size a mode can occupy
  static uint32_t maxModeSize();
  // get the maximum size a savefile can occupy
  static uint32_t maxSaveSize();
  // global flags value
  static uint8_t globalFlags();
#endif

private:
  // linked list of internal mode storage
  class ModeLink {
  public:
    // construct a link and optionally instantiate the link
    ModeLink(const Mode *src = nullptr, bool inst = false);
    ModeLink(const ByteStream &src, bool inst = false);
    ~ModeLink();

    // init the link and append another link
    bool init(const Mode *mode = nullptr);
    bool append(const Mode *next);
    bool append(const ByteStream &next);

    // play the instantiated mode inside
    void play();

    // unlink self from the chain, returns link that takes position
    ModeLink *unlinkSelf();

    // link in a node before or after self
    void linkAfter(ModeLink *link);
    void linkBefore(ModeLink *link);

    // instantiate/destroy the mode
    Mode *instantiate();
    void uninstantiate();

    // if the mode is instantiated and the instantiated version
    // has changed at all then save will re-save it to the buffer
    bool save();

    // accessors
    ByteStream &buffer() { return m_storedMode; }
    Mode *mode() { return m_pInstantiatedMode; }
    ModeLink *next() { return m_next; }
    ModeLink *prev() { return m_prev; }

    operator ByteStream &() { return m_storedMode; }
    operator ByteStream() { return m_storedMode; }
    operator Mode *() { return m_pInstantiatedMode; }
  private:
    Mode *m_pInstantiatedMode;
    ByteStream m_storedMode;
    ModeLink *m_next;
    ModeLink *m_prev;
  };

  // fetch a link from the chain by index
  static ModeLink *getModeLink(uint32_t index);

  // initialize current mode from ByteStream, optionally force re-init which
  // will destroy the current instantiated mode and re-load it from serial
  static Mode *initCurMode(bool force = false);
  static bool saveCurMode();

  // whether modes have been loaded
  static bool m_loaded;

  // the current mode we're on
  static uint8_t m_curMode;

  // the number of modes loaded
  static uint8_t m_numModes;

  // the current instantiated mode and it's respective link
  static ModeLink *m_pCurModeLink;

  // list of serialized version of bufers
  static ModeLink *m_storedModes;

  // global flags for all modes
  static uint8_t m_globalFlags;

  // the last switch time of the modes
  static uint32_t m_lastSwitchTime;
};

#endif
