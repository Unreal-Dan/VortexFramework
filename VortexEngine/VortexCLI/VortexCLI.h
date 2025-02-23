#pragma once

#include "VortexLib.h"

#include "Patterns/Patterns.h"
#include "Patterns/Pattern.h"
#include "Colors/ColorTypes.h"
#include "Colors/Colorset.h"
#include "Leds/LedTypes.h"

class VortexCLI
{
public:
  VortexCLI();
  ~VortexCLI();

  // initialize the test framework
  bool init(int argc, char *argv[]);

  // run the test framework
  void run();
  void cleanup();

  // render the cli vortex tool
  void show();

  // whether the button is pressed
  bool isButtonPressed();

  // whether the test framework is still running
  bool stillRunning() const;

  // setup the array of leds
  void installLeds(void *leds, uint32_t count);

  static void printlog(const char *file, const char *func, int line, const char *msg, va_list list);

  void setColoredOutput(bool output) { m_outputType = OUTPUT_TYPE_COLOR; }
  void setHexOutput(bool output) { m_outputType = OUTPUT_TYPE_HEX; }
  void setNoTimestep(bool timestep) { m_noTimestep = timestep; }
  void setInPlace(bool inplace) { m_inPlace = inplace; }

private:
  class VortexCLICallbacks : public VortexCallbacks
  {
  public:
    VortexCLICallbacks(Vortex &vortex) : VortexCallbacks(vortex) {}
    virtual ~VortexCLICallbacks() {}
    virtual long checkPinHook(uint32_t pin) override;
    virtual void ledsInit(void *cl, int count) override;
    virtual void ledsShow() override;
  private:
    // receive a message from client
  };

  // internal helper for updating terminal size
  void get_terminal_size();

  // vortex lib
  Vortex m_vortex;
  // engine reference so that LED_ macros work
  VortexEngine &m_engine;

  // these are in no particular order
  RGBColor *m_ledList;
  uint32_t m_numLeds; // filled by callback from engine

  uint8_t m_ledCount; // CLI controls this
  bool m_initialized;
  bool m_buttonPressed;
  bool m_keepGoing;
  volatile bool m_isPaused;
  PatternID m_curPattern;
  Colorset m_curColorset;
  enum OutputType {
    OUTPUT_TYPE_NONE,
    OUTPUT_TYPE_HEX,
    OUTPUT_TYPE_COLOR,
    OUTPUT_TYPE_SILENT,
  };
  OutputType m_outputType;
  enum JsonMode : uint32_t {
    JSON_MODE_NONE = 0,

    // technically two these are mutually exclusive... but this is easier
    JSON_MODE_READ_STDIN    = (1<<0),
    JSON_MODE_READ_FILE     = (1<<1),

    // also these too, mutually exclusive but this is easier
    JSON_MODE_WRITE_STDOUT  = (1<<2),
    JSON_MODE_WRITE_FILE    = (1<<3),
  };
  JsonMode m_jsonMode;
  bool m_noTimestep;
  bool m_lockstep;
  bool m_inPlace;
  bool m_record;
  bool m_storage;
  bool m_sleepEnabled;
  bool m_lockEnabled;
  bool m_jsonPretty;
  bool m_quickExit;
  bool m_displayVersion;
  int m_verbosity;
  std::string m_storageFile;
  std::string m_writeSaveFile;
  std::string m_writeModeFile;
  std::string m_loadSaveFile;
  std::string m_jsonInFile;
  std::string m_jsonOutFile;
  std::string m_patternIDStr;
  std::string m_colorsetStr;
  std::string m_argumentsStr;
  // to pipe stuff into the engine
  int m_pipe_fd[2];
  int m_saved_stdin;
  std::string m_inputBuffer;
};

extern VortexCLI *g_pVortexCLI;
