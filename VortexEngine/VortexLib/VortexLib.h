#pragma once

#if defined(_MSC_VER)
#include <Windows.h>
#endif

#include "VortexEngine.h"
#include "Patterns/Patterns.h"
#include "Random/Random.h"
#include "Leds/LedTypes.h"
#include "json.hpp"

#include <inttypes.h>

#include <vector>
#include <string>
#include <deque>
#include <deque>

class Vortex;

// ============================================================================
//  Vortex Engine Wrapper
//
// This file is the interface for the Vortex Engine.
//
// The things in Vortex that you can hook into include:
//
//  - The serial input/output callbacks (editor connection)
//  - The Wireless IR/VL input/output callbacks (mode sharing)
//  - The digital pin read callbacks (buttons press)
//  - The leds init, brightness, and display (render the lights)
//
// Then on top of being able to hook into the various actions of the engine
// the Vortex class provides all kinds of APIs for controlling and manipulating
// the engine while it is running.
//
class VortexCallbacks
{
public:
  VortexCallbacks(Vortex &vortex) : m_vortex(vortex) {}
  virtual ~VortexCallbacks() {}
  // called when engine reads digital pins, use this to feed button presses to the engine
  virtual long checkPinHook(uint32_t pin);
  // called when engine writes to ir, use this to read data from the vortex engine
  // the data received will be in timings of milliseconds
  // NOTE: to send data to IR use Vortex::IRDeliver at any time
  virtual void infraredWrite(bool mark, uint32_t amount) { }
  // called when engine checks for Serial, use this to indicate serial is connected
  virtual bool serialCheck() { return false; }
  // called when engine begins serial, use this to do any initialization of the connection
  virtual void serialBegin(uint32_t baud) { }
  // called when engine checks for data on serial, use this to tell the engine data is ready
  virtual int32_t serialAvail() { return 0; }
  // called when engine reads from serial, use this to deliver data to the vortex engine
  // TODO: buffer this in libengine and add a deliver api like IR
  virtual size_t serialRead(char *buf, size_t amt) { return 0; }
  // called when engine writes to serial, use this to read data from the vortex engine
  virtual uint32_t serialWrite(const uint8_t *buf, size_t amt) { return 0; }
  // called when the LED strip is initialized
  virtual void ledsInit(void *cl, int count) { }
  // called when the brightness is changed
  virtual void ledsBrightness(int brightness) { }
  // called when the leds are shown
  virtual void ledsShow() { }

protected:
  // reference to the vortexlib this is initialized for
  Vortex &m_vortex;
};

// forward decls for various classes
class PatternArgs;
class ByteStream;
class Colorset;
class Pattern;
class Button;
class Mode;

// json type is just nlohmann::json
using json = nlohmann::json;

// Vortex Engine wrapper class, use this to interface with
// the vortex engine as much as possible
class Vortex
{
  // internal initializer
  bool init(VortexCallbacks *callbacks);

public:
  Vortex();
  ~Vortex();

  // simple initialization nothing special
  void init() { initEx<VortexCallbacks>(); }

  // extended initialization, provide a callbacks class to receive events
  template <typename T>
  T *initEx()
  {
    if (!std::is_base_of<VortexCallbacks, T>()) {
      return nullptr;
    }
    T *callbacks = new T(*this);
    if (!callbacks) {
      return nullptr;
    }
    if (!init(callbacks)) {
      return nullptr;
    }
    return callbacks;
  }
  void cleanup();

  // tick the engine forward, return false if engine exits
  bool tick();

  // install a callback for digital reads (button press)
  void installCallbacks(VortexCallbacks *callbacks);

  // control whether the engine will tick instantly or not
  void setInstantTimestep(bool timestep);

  // select the button to send clicks to by default (0 = first button, 1 = 2nd, etc)
  void selectButton(uint8_t buttonIndex);

  // send various clicks to the selected button, unless the button
  // index is provided as an argument then whichever button is selected
  // will receive the event. So if selectButton(1) is called then all of
  // these will by default target the 2nd button, unless they are given a
  // non-zero button index to target then they will target that one instead
  void shortClick(uint8_t buttonIndex = 0);
  void longClick(uint8_t buttonIndex = 0);
  void menuEnterClick(uint8_t buttonIndex = 0);
  void advMenuEnterClick(uint8_t buttonIndex = 0);
  void deleteColClick(uint8_t buttonIndex = 0);
  void sleepClick(uint8_t buttonIndex = 0);
  void forceSleepClick(uint8_t buttonIndex = 0);
  void pressButton(uint8_t buttonIndex = 0);
  void releaseButton(uint8_t buttonIndex = 0);
  bool isButtonPressed(uint8_t buttonIndex = 0);

  // send a wait event, will let the engine run a tick if running in lockstep
  // for example when running the testing system
  void sendWait(uint32_t amount = 0);

  // deliver some number of rapid clicks (only availbe for button0... sorry)
  void rapidClick(uint32_t amount = 3);

  // get the current menu demo mode
  Mode *getMenuDemoMode();
  bool setMenuDemoMode(const Mode *mode);

  // special 'click' that quits the engine
  void quitClick();

  // deliver IR timing, the system expects mark first, but it doesn't matter
  // because the system will reset on bad data and then it can interpret any
  // timing either way
  void IRDeliver(uint32_t timing);
  void VLDeliver(uint32_t timing);

  // get total/used storage space
  void getStorageStats(uint32_t *outTotal, uint32_t *outUsed);
  void loadStorage();

  // open various menus on the core (if they exist!)
  void openRandomizer(bool advanced = false);
  void openColorSelect(bool advanced = false);
  void openPatternSelect(bool advanced = false);
  void openGlobalBrightness(bool advanced = false);
  void openFactoryReset(bool advanced = false);
  void openModeSharing(bool advanced = false);
  void openEditorConnection(bool advanced = false);

  // set the target leds for the open menu
  void clearMenuTargetLeds();
  void setMenuTargetLeds(LedMap targetLeds);
  void addMenuTargetLeds(LedPos pos);

  // convert modes to/from a bytestream
  bool getModes(ByteStream &outStream);
  bool setModes(ByteStream &stream, bool save = true);
  void clearModes();

  // match the ledcount of the savefile in the stream, vtxMode = true
  // to indicate it is a .vtxmode file or not
  bool matchLedCount(ByteStream &stream, bool vtxMode);
  // TODO: do we need this?
  bool checkLedCount();
  uint8_t setLedCount(uint8_t ledCount);
  uint8_t getLedCount();

  // functions to operate on the current mode selection
  uint32_t curModeIndex();
  uint32_t numModes();
  uint32_t numLedsInMode();
  bool addMode(const Mode *mode, bool save = true);
  bool addNewMode(bool save = true);
  bool addNewMode(ByteStream &stream, bool save = true);
  bool addNewModeRaw(ByteStream &stream, bool save = true);
  bool getCurMode(ByteStream &stream);
  bool getCurModeRaw(ByteStream &stream);
  bool setCurMode(uint32_t index, bool save = true);
  bool nextMode(bool save = true);
  bool delCurMode(bool save = true);
  bool shiftCurMode(int8_t offset, bool save = true);

  // functions to operate on the current Mode
  bool setPattern(PatternID id, const PatternArgs *args = nullptr,
    const Colorset *set = nullptr, bool save = true);
  PatternID getPatternID(LedPos pos);
  std::string getPatternName(LedPos pos);
  std::string getModeName();
  bool setPatternAt(LedPos pos, PatternID id,
    const PatternArgs *args = nullptr, const Colorset *set = nullptr,
    bool save = true);
  bool getColorset(LedPos pos, Colorset &set);
  bool setColorset(LedPos pos, const Colorset &set, bool save = true);
  bool getPatternArgs(LedPos pos, PatternArgs &args);
  bool setPatternArgs(LedPos pos, PatternArgs &args, bool save = true);

  // whether the current mode is a multi-led pattern
  bool isCurModeMulti();

  // Helpers for converting pattern id and led id to string
  std::string patternToString(PatternID id = PATTERN_NONE);
  PatternID stringToPattern(const std::string &pattern);
  std::string ledToString(LedPos pos);
  uint32_t numCustomParams(PatternID id);
  std::vector<std::string> getCustomParams(PatternID id);

  // undo redo
  void setUndoBufferLimit(uint32_t limit);
  bool addUndoBuffer();
  bool undo();
  bool redo();

  // change tickrate of the engine if the engine was configured to support it
  void setTickrate(uint32_t tickrate);
  uint32_t getTickrate();

  // enable/disable undo
  void enableUndo(bool enabled) { m_undoEnabled = enabled; }

  // access stored callbacks
  VortexCallbacks *vcallbacks() { return m_storedCallbacks; }

  // printing to log system
  void printlog(const char *file, const char *func, int line, const char *msg, va_list list);

  // injects a command into the engine, the engine will parse one command
  // per tick so multiple commands will be queued up
  void doCommand(char c);

  // whether the engine has sleep enabled, if disabled it will always be awake
  void setSleepEnabled(bool enable);
  bool sleepEnabled();

  // whether the engine is sleeping, and/or to enter sleep
  void enterSleep(bool save);
  bool isSleeping();

  // enable, fetch and clear the internal command log
  void enableCommandLog(bool enable) { m_commandLogEnabled = enable; }
  const std::string &getCommandLog() { return m_commandLog; }
  void clearCommandLog() { m_commandLog.clear(); }

  // enable/disable lockstep mode (one tick per input)
  bool enableLockstep(bool enable) { return m_lockstepEnabled = enable; }
  bool isLockstep() { return m_lockstepEnabled; }

  // enable disable storage
  bool enableStorage(bool enable) { return m_storageEnabled = enable; }
  bool storageEnabled() { return m_storageEnabled; }

  // control the storage
  void setStorageFilename(const std::string &name);
  std::string getStorageFilename();

  // enable or disable the 'lock'
  void setLockEnabled(bool enable) { m_lockEnabled = enable; }
  bool lockEnabled() { return m_lockEnabled; }

  // get the engine version as a string
  std::string getVersion() const;
  // get the version as separated integers
  uint8_t getVersionMajor() const;
  uint8_t getVersionMinor() const;
  uint8_t getVersionBuild() const;

  // convert a mode to/from a json object
  json modeToJson(const Mode *mode);
  Mode *modeFromJson(const json &modeJson);
  // convert a pattern to/from a json object
  json patternToJson(const Pattern *pattern);
  Pattern *patternFromJson(const json &patternJson);
  // save current mode to json or load a mode by json
  json saveModeToJson();
  bool loadModeFromJson(const json &modeJson);
  // save/load the engine storage to/from raw json object
  json saveToJson();
  bool loadFromJson(const json &json);

  // print/parse the current mode json
  std::string printModeJson(bool pretty = false);
  bool parseModeJson(const std::string &json);
  // print/parse a pattern of the current mode json
  std::string printPatternJson(LedPos pos, bool pretty = false);
  bool parsePatternJson(LedPos pos, const std::string &json);
  // print/parse the json from a string
  std::string printJson(bool pretty = false);
  bool parseJson(const std::string &json);
  // print/parse the json from a string in a file
  bool printJsonToFile(const std::string &filename, bool pretty = false);
  bool parseJsonFromFile(const std::string &filename);

  // save and add undo buffer
  bool doSave();
  bool applyUndo();

  // reference to engine
  VortexEngine &engine() { return m_engine; }

#ifdef WASM
  // pointer to the led array and led count in the engine
  RGBColor *leds() { return m_leds; }
  int ledCount() { return m_led_count; }
  // initialize
  void initWasm(int led_count, RGBColor *leds) { m_led_count = led_count; m_leds = leds; }
#endif

private:
  // internal function to handle numeric values in commands
  void handleNumber(char c);

  // so that the buttons class can call handleInputQueue
  friend class Buttons;

  // called by the engine right after all buttons are checked, this will process
  // the input deque that is fed by the apis like shortClick() above and translate
  // those messages into actual button events by overwriting button data that tick
  void handleInputQueue(Button *buttons, uint32_t numButtons);
  uint32_t getNumInputs();

  // The various different button events that can be injected into vortex
  enum VortexButtonEventType
  {
    // no event
    EVENT_NONE,

    // a short click
    EVENT_SHORT_CLICK,
    // a long click
    EVENT_LONG_CLICK,
    // a press that is just long enough to open the ring menu
    EVENT_MENU_ENTER_CLICK,
    // a press that is just long enough to enter adv menus
    EVENT_ADV_MENU_ENTER_CLICK,
    // a press that is long enough to delete a color from col select
    EVENT_DELETE_COL,
    // a press just long enough to put the device to sleep from main modes
    EVENT_SLEEP_CLICK,
    // a press very long so that the chip triggers it's force sleep
    EVENT_FORCE_SLEEP_CLICK,
    // just wait around a tick (mainly used for testing)
    EVENT_WAIT,
    // toggle the button (press or unpress it)
    EVENT_TOGGLE_CLICK,
    // rapid click the button x times
    EVENT_RAPID_CLICK,
    // quit the engine (not really a 'click')
    EVENT_QUIT_CLICK,

    // this is an automated event that will reset the button click
    // after the ring menu hold-to-enter sequence
    EVENT_RESET_CLICK,
  };

  class VortexButtonEvent
  {
  public:
    VortexButtonEvent(uint32_t target, VortexButtonEventType type) :
      target(target), type(type)
    {
    }
    // target button index
    uint32_t target;
    // the event to trigger
    VortexButtonEventType type;
  };

  // the instance of the engine
  VortexEngine m_engine;
  // undo buffer
  std::deque<ByteStream> m_undoBuffer;
  // the undo limit
  uint32_t m_undoLimit;
  // undo position in buffer
  uint32_t m_undoIndex;
  // whether undo buffer is disabled recording
  bool m_undoEnabled;
  // stored callbacks
  VortexCallbacks *m_storedCallbacks;
  // handle to the console and logfile
  FILE *m_consoleHandle;
#if LOG_TO_FILE == 1
  FILE *m_logHandle;
#endif
#ifdef WASM
  // pointer to the led array and led count in the engine
  RGBColor *m_leds;
  int m_led_count;
#endif
  // queue of button events, deque so can push to front and back
  std::deque<VortexButtonEvent> m_buttonEventQueue;
  // whether initialized
  bool m_initialized;
  // whether each button is pressed (bitflags) so technically this only
  // supports 32 buttons but idc whoever adds 33 buttons can fix this
  uint32_t m_buttonsPressed;
  // the selected button to target for input events
  uint8_t m_selectedButton;
  // keeps a log of all the commands issued
  std::string m_commandLog;
  // whether to record commands
  bool m_commandLogEnabled;
  // whether to run in lockstep mode (one input per step)
  bool m_lockstepEnabled;
  // whether storage is enabled
  bool m_storageEnabled;
  // whether sleep is enabled
  bool m_sleepEnabled;
  // whether lock is enabled
  bool m_lockEnabled;
  // the last command to have been executed
  char m_lastCommand;
  // internal random ctx for stuff
  Random m_randCtx;
};
