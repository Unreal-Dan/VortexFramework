#ifndef MODES_H
#define MODES_H

#include "../Serial/ByteStream.h"
#include "../Colors/ColorTypes.h"
#include "../Patterns/Patterns.h"
#include "../Leds/Leds.h"

#include "../VortexConfig.h"

#include <inttypes.h>

class VortexEngine;
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

class Modes
{
public:
  Modes(VortexEngine &engine);
  ~Modes();

  // opting for class here because there should only ever be one
  // Modes control object and I don't like singletons
  bool init();
  void cleanup();

  // load modes so they are ready to play
  bool load();

  // play the current mode
  void play();

  // full save/load to/from buffer
  bool saveToBuffer(ByteStream &saveBuffer);
  bool loadFromBuffer(ByteStream &saveBuffer);

  // save the header to storage
  bool saveHeader();
  bool loadHeader();

  // full save/load to/from storage
  bool saveStorage();
  bool loadStorage();

  // save load the savefile header from storage
  bool serializeSaveHeader(ByteStream &saveBuffer);
  bool unserializeSaveHeader(ByteStream &saveBuffer);

  // saves all modes to a buffer
  bool serialize(ByteStream &buffer);
  // load all modes from a buffer, optionally specify number of leds
  bool unserialize(ByteStream &buffer);

  // set default settings (must save after)
  bool setDefaults();

  // shift the current mode to a different position relative to current position
  // negative values for up, positive values for down, 0 for no move
  bool shiftCurMode(int32_t offset = 1);

  // add a new mode in various different ways
  bool addMode(PatternID id, RGBColor c1, RGBColor c2 = RGB_OFF,
    RGBColor c3 = RGB_OFF, RGBColor c4 = RGB_OFF, RGBColor c5 = RGB_OFF,
    RGBColor c6 = RGB_OFF, RGBColor c7 = RGB_OFF, RGBColor c8 = RGB_OFF);
  bool addMode(PatternID id, const PatternArgs *args, const Colorset *set);
  bool addMode(const Mode *mode);

  // add a new mode by unserializing raw
  bool addSerializedMode(ByteStream &serializedMode);
  // add a new mode by loading from a save buffer
  bool addModeFromBuffer(ByteStream &serializedMode);

  // update the current mode to match the given mode, optionally save
  bool updateCurMode(const Mode *mode);

  // set the current active mode by index
  Mode *setCurMode(uint8_t index);

  // get the current mode
  Mode *curMode();
  // iterate to next mode and return it
  Mode *nextMode();
  // iterate to previous mode and return it
  Mode *previousMode();

  // whether the current mode has been loaded out of storage this is
  // more for internal use, it was added to help vortexlib detect
  // whether a mode has been loaded or not when led count changes
  bool curModeInstantiated() { return (m_pCurModeLink && m_pCurModeLink->mode() != nullptr); }

  // iterate to the next mode but skip empty modes, will not skip mode 0
  // to prevent possibly skipping all modes and ending in an infinite loop
  Mode *nextModeSkipEmpty();

  // the number of modes
  uint8_t numModes() { return m_numModes; }
  uint8_t curModeIndex() { return m_curMode; }

  // the last time the modes switched to a new mode
  uint32_t lastSwitchTime() { return m_lastSwitchTime; }

  // delete the current mode
  void deleteCurMode();

  // delete all modes in the list
  void clearModes();

  // set the startup mode index (which mode will be displayed on startup)
  void setStartupMode(uint8_t index);
  uint8_t startupMode();
  Mode *switchToStartupMode();

  // initialize current mode from ByteStream, optionally force re-init which
  // will destroy the current instantiated mode and re-load it from serial
  Mode *initCurMode(bool force = false);
  bool saveCurMode();

  // set or get flags
  bool setFlag(uint8_t flag, bool enable, bool save = true);
  bool getFlag(uint8_t flag) { return ((m_globalFlags & flag) != 0); }
  // reset flags to factory default (must save after)
  void resetFlags() { m_globalFlags = 0; }

  // inline functions to toggle the various flags
  bool setOneClickMode(bool enable, bool save = true) {
    return setFlag(MODES_FLAG_ONE_CLICK, enable, save);
  }
  bool oneClickModeEnabled() {
    return getFlag(MODES_FLAG_ONE_CLICK);
  }
  // toggle the locked state
  bool setLocked(bool locked, bool save = true) {
    return setFlag(MODES_FLAG_LOCKED, locked, save);
  }
  bool locked() {
    return getFlag(MODES_FLAG_LOCKED);
  }
  // toggle advanced menus
  bool setAdvancedMenus(bool active, bool save = true) {
    return setFlag(MODES_FLAG_ADV_MENUS, active, save);
  }
  bool advancedMenusEnabled() {
    return getFlag(MODES_FLAG_ADV_MENUS);
  }
  // toggle the keychain light mode
  bool setKeychainMode(bool active, bool save = true) {
    return setFlag(MODES_FLAG_KEYCHAIN, active, save);
  }
  bool keychainModeEnabled() {
    return getFlag(MODES_FLAG_KEYCHAIN);
  }

#if MODES_TEST == 1
  void test();
#endif

#ifdef VORTEX_LIB
  // get the maximum size a mode can occupy
  uint32_t maxModeSize();
  // get the maximum size a savefile can occupy
  uint32_t maxSaveSize();
  // global flags value
  uint8_t globalFlags();
#endif

private:
  // linked list of internal mode storage
  class ModeLink {
  public:
    // construct a link and optionally instantiate the link
    ModeLink(VortexEngine &engine, const Mode *src = nullptr, bool inst = false);
    ModeLink(VortexEngine &engine, const ByteStream &src, bool inst = false);
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
    VortexEngine &m_engine;
    Mode *m_pInstantiatedMode;
    ByteStream m_storedMode;
    ModeLink *m_next;
    ModeLink *m_prev;
  };

  // reference to engine
  VortexEngine &m_engine;

  // fetch a link from the chain by index
  ModeLink *getModeLink(uint32_t index);

  // whether modes have been loaded
  bool m_loaded;

  // the current mode we're on
  uint8_t m_curMode;

  // the number of modes loaded
  uint8_t m_numModes;

  // the current instantiated mode and it's respective link
  ModeLink *m_pCurModeLink;

  // list of serialized version of bufers
  ModeLink *m_storedModes;

  // global flags for all modes
  uint8_t m_globalFlags;

  // the last switch time of the modes
  uint32_t m_lastSwitchTime;
};

#endif
