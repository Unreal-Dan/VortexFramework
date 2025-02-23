#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <ctime>
#include <map>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#include "VortexCLI.h"

#include "Log/Log.h"

#include "VortexLib.h"

#include "Patterns/PatternBuilder.h"
#include "Time/TimeControl.h"
#include "Colors/ColorTypes.h"
#include "Colors/Colorset.h"
#include "Buttons/Button.h"
#include "Time/Timings.h"
#include "Menus/Menus.h"
#include "Modes/Modes.h"
#include "Modes/Mode.h"

#include "Patterns/Pattern.h"
#include "Patterns/Single/SingleLedPattern.h"

#define RECORD_FILE "recorded_input.txt"

VortexCLI *g_pVortexCLI = nullptr;

using namespace std;

// the usage for the input strings
const char *input_usage[] = {
  "\n   c         standard short click",
  "\n   l         standard long click",
  "\n   m         open menus length click",
  "\n   a         enter adv menu length click (enter adv menu from menus)",
  "\n   d         delete length click (delete color in col select)",
  "\n   s         enter sleep length click (enter sleep at main modes)",
  "\n   f         force sleep length click (force sleep anywhere)",
  "\n   t         toggle button pressed (only way to wake after sleep)",
  "\n   r         rapid button click (ex: r15)",
  "\n   w         wait 1 tick",
  "\n   <digits>  repeat command n times (only single digits in -i mode)",
  "\n   b<index>  select button index (ex: b0 for first button or b1 for second button)",
  "\n   q         quit",
};

#define NUM_USAGE (sizeof(input_usage) / sizeof(input_usage[0]))

// the usage for the input strings but in a brief format
const char *input_usage_brief[NUM_USAGE] = {
  "\n   c         short",
  "\n   l         long",
  "\n   m         menus",
  "\n   a         adv menu",
  "\n   d         delete col",
  "\n   s         sleep",
  "\n   f         force sleep",
  "\n   t         toggle",
  "\n   r         rapid",
  "\n   w         wait",
  "\n   <digits>  repeat",
  "\n   b<index>  button idx",
  "\n   q         quit",
};


// 1000 spaces, used for efficiently printing any number of spaces up to 1000 by
// offsetting from the start of this string with the SPACES() macro below
#define SPACESTR "                                                                                                                                                     " \
                 "                                                                                                                                                     " \
                 "                                                                                                                                                     " \
                 "                                                                                                                                                     " \
                 "                                                                                                                                                     " \
                 "                                                                                                                                                     " \
                 "                                                                                                                                                     " \
                 "                                                                                                                                                     " \
                 "                                                                                                                                                     " \
                 "                                                                                                                                                     "

// 1000 hyphens for a long line
#define LINESTR  "-----------------------------------------------------------------------------------------------------------------------------------------------------" \
                 "-----------------------------------------------------------------------------------------------------------------------------------------------------" \
                 "-----------------------------------------------------------------------------------------------------------------------------------------------------" \
                 "-----------------------------------------------------------------------------------------------------------------------------------------------------" \
                 "-----------------------------------------------------------------------------------------------------------------------------------------------------" \
                 "-----------------------------------------------------------------------------------------------------------------------------------------------------" \
                 "-----------------------------------------------------------------------------------------------------------------------------------------------------" \
                 "-----------------------------------------------------------------------------------------------------------------------------------------------------" \
                 "-----------------------------------------------------------------------------------------------------------------------------------------------------"

// some amount of spaces, if you pass in an amount greater than 1000 you will die
#define SPACES(amt) SPACESTR + (sizeof(SPACESTR) - (amt))
// some length of line, if you pass in an amount greater than 1000 you will die
#define LINE(amt) LINESTR + (sizeof(LINESTR) - (amt))

#ifdef WASM // Web assembly glue
#include <emscripten/html5.h>
#include <emscripten.h>

static EM_BOOL key_callback(int eventType, const EmscriptenKeyboardEvent *e, void *userData)
{
  if (e->key[0] == ' ') {
    if (eventType == EMSCRIPTEN_EVENT_KEYDOWN) {
      m_vortex.pressButton();
    } else if (eventType == EMSCRIPTEN_EVENT_KEYUP) {
      m_vortex.releaseButton();
    }
  }
  return 0;
}

static void do_run()
{
  g_pVortexCLI->run();
}

static void wasm_init()
{
  emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, key_callback);
  emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, key_callback);
  emscripten_set_main_loop(do_run, 0, true);
  // turn colored output off in the wasm version
  //g_pVortexCLI->setColoredOuptut(false);
}
#endif // ifdef WASM

VortexCLI::VortexCLI() :
  m_vortex(),
  m_engine(m_vortex.engine()),
  m_ledList(),
  m_numLeds(0),
  m_ledCount(0),
  m_initialized(false),
  m_buttonPressed(false),
  m_keepGoing(true),
  m_isPaused(true),
  m_curPattern(PATTERN_FIRST),
  m_curColorset(),
  m_outputType(OUTPUT_TYPE_NONE),
  m_jsonMode(JSON_MODE_NONE),
  m_noTimestep(false),
  m_lockstep(false),
  m_inPlace(false),
  m_record(false),
  m_storage(false),
  m_sleepEnabled(true),
  m_lockEnabled(true),
  m_jsonPretty(false),
  m_quickExit(false),
  m_displayVersion(false),
  m_verbosity(0),
  m_storageFile("FlashStorage.flash"),
  m_patternIDStr(),
  m_colorsetStr(),
  m_argumentsStr(),
  m_pipe_fd{-1, -1},
  m_saved_stdin(),
  m_inputBuffer()
{
}

VortexCLI::~VortexCLI()
{
}

static struct option long_options[] = {
  {"hex", no_argument, nullptr, 'x'},
  {"color", no_argument, nullptr, 'c'},
  {"silent", no_argument, nullptr, 's'},
  {"no-timestep", no_argument, nullptr, 't'},
  {"lockstep", no_argument, nullptr, 'l'},
  {"in-place", no_argument, nullptr, 'i'},
  {"record", no_argument, nullptr, 'r'},
  {"autowake", no_argument, nullptr, 'a'},
  {"nolock", no_argument, nullptr, 'n'},
  {"quick", no_argument, nullptr, 'q'},
  {"led-count", required_argument, nullptr, 'u'},
  {"storage", optional_argument, nullptr, 'S'},
  {"write-save", required_argument, nullptr, 'W'},
  {"write-mode", required_argument, nullptr, 'M'},
  {"load-save", required_argument, nullptr, 'L'},
  {"json-in", optional_argument, nullptr, 'I'},
  {"json-out", optional_argument, nullptr, 'O'},
  {"json-human-read", no_argument, nullptr, 'H'},
  {"pattern", required_argument, nullptr, 'P'},
  {"colorset", required_argument, nullptr, 'C'},
  {"arguments", required_argument, nullptr, 'A'},
  {"version", no_argument, nullptr, 'V'},
  {"verbose", no_argument, nullptr, 'v'},
  {"help", no_argument, nullptr, 'h'},
  {nullptr, 0, nullptr, 0}
};

// all the available colors that can be used to make colorsets
std::map<std::string, int> color_map = {
    {"black",   0x000000},
    {"white",   0xFFFFFF},
    {"red",     0xFF0000},
    {"lime",    0x00FF00},
    {"blue",    0x0000FF},
    {"yellow",  0xFFFF00},
    {"cyan",    0x00FFFF},
    {"magenta", 0xFF00FF},
    {"silver",  0xC0C0C0},
    {"gray",    0x808080},
    {"maroon",  0x800000},
    {"olive",   0x808000},
    {"green",   0x008000},
    {"purple",  0x800080},
    {"teal",    0x008080},
    {"navy",    0x000080},
    {"orange",  0xFFA500},
    {"pink",    0xFFC0CB},
    {"brown",   0xA52A2A}
    //...add more as needed
};

static void print_usage(const char* program_name) 
{
  fprintf(stderr, VORTEX_FULL_NAME "\n");
  fprintf(stderr, "Usage: %s [options] < input commands\n", program_name);
  fprintf(stderr, "Output Selection (at least one required):\n");
  fprintf(stderr, "  -x, --hex                Use hex values to represent led colors\n");
  fprintf(stderr, "  -c, --color              Use console color codes to represent led colors\n");
  fprintf(stderr, "  -s, --silent             Do not print any output while running\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Engine Control Flags (optional):\n");
  fprintf(stderr, "  -t, --no-timestep        Bypass the timestep and run as fast as possible\n");
  fprintf(stderr, "  -l, --lockstep           Only step once each time an input is received\n");
  fprintf(stderr, "  -i, --in-place           Print the output in-place (interactive mode)\n");
  fprintf(stderr, "  -r, --record             Record the inputs and dump to a file after (" RECORD_FILE ")\n");
  fprintf(stderr, "  -a, --autowake           Automatically and instantly wake on sleep (disable sleep)\n");
  fprintf(stderr, "  -n, --nolock             Automatically unlock upon locking the chip (disable lock)\n");
  fprintf(stderr, "  -q, --quick              Exit immediately after initialization (useful to convert data)\n");
  fprintf(stderr, "  -u, --led-count          Set the initial led count of the engine (default 1)\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Storage and Data Conversion (optional):\n");
  fprintf(stderr, "  -S, --storage [file]     Persistent storage to file (default file: FlashStorage.flash)\n");
  fprintf(stderr, "  -W, --write-save <file>  Write a full .vortex savefile at exit\n");
  fprintf(stderr, "  -M, --write-mode <file>  At exit write the current mode to a .vtxmode file\n");
  fprintf(stderr, "  -L, --load-save <file>   Load a vortex savefile (either .vortex or .vtxmode)\n");
  fprintf(stderr, "  -I, --json-in [file]     Load json from file at init (if no file: read from stdin)\n");
  fprintf(stderr, "  -O, --json-out [file]    Dump json to file at exit (if no file: write to stdout)\n");
  fprintf(stderr, "  -H, --json-human-read    Dump human readable json, instead of condensed json\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Initial Pattern Options (optional):\n");
  fprintf(stderr, "  -P, --pattern <id>       Preset the pattern ID on the first mode (numeric ID or pattern name ex: blend)\n");
  fprintf(stderr, "  -C, --colorset c1,c2...  Preset the colorset on the first mode (csv list of hex codes or color names)\n");
  fprintf(stderr, "  -A, --arguments a1,a2... Preset the arguments on the first mode (csv list of arguments)\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Other Options:\n");
  fprintf(stderr, "  -V, --version            Display the engine version number\n");
  fprintf(stderr, "  -v, --verbose            Make this utility more noisy\n");
  fprintf(stderr, "  -h, --help               Display this help message\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Input Commands (pass to stdin):");
  for (uint32_t i = 0; i < NUM_USAGE; ++i) {
    fprintf(stderr, "%s", input_usage[i]);
  }
  fprintf(stderr, "\n\n");
  fprintf(stderr, "Example Usage:\n");
  fprintf(stderr, "   ./vortex --color\n");
  fprintf(stderr, "   ./vortex -ci -P42 -Ccyan,purple\n");
  fprintf(stderr, "   ./vortex -ct -Pblend -Ccyan,yellow,magenta <<< w100q\n");
  fprintf(stderr, "   ./vortex -sq -L mysave.vortex -O output_data.json\n");
  fprintf(stderr, "   ./vortex -sq -L mymode.vtxmode -O output_data.json\n");
}

#ifdef _WIN32
static HANDLE hStdin;
static DWORD mode;

static void restore_terminal()
{
  SetConsoleMode(hStdin, mode);
}
void set_terminal_nonblocking()
{
  hStdin = GetStdHandle(STD_INPUT_HANDLE);
  GetConsoleMode(hStdin, &mode);
  SetConsoleMode(hStdin, mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));
  atexit(restore_terminal);
}
#else
static struct termios orig_term_attr = {0};
static void restore_terminal()
{
  tcsetattr(STDIN_FILENO, TCSANOW, &orig_term_attr);
}

void set_terminal_nonblocking()
{
  struct termios term_attr = {0};

  // Get the current terminal attributes and store them
  tcgetattr(STDIN_FILENO, &term_attr);
  orig_term_attr = term_attr;

  // Set the terminal in non-canonical mode (raw mode)
  term_attr.c_lflag &= ~(ICANON | ECHO);

  // Set the minimum number of input bytes read at a time to 1
  term_attr.c_cc[VMIN] = 1;

  // Set the timeout for read to 0 (no waiting)
  term_attr.c_cc[VTIME] = 0;

  // Apply the new terminal attributes
  tcsetattr(STDIN_FILENO, TCSANOW, &term_attr);

  // Set the terminal to non-blocking mode
  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

  // Register the restore_terminal function to be called at exit
  atexit(restore_terminal);
}
#endif

uint32_t term_columns = 0;
void VortexCLI::get_terminal_size()
{
#ifdef _WIN32
  // Windows-specific code
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
  term_columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
  // Linux-specific code
  struct winsize new_terminal_size = { 0 };
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &new_terminal_size);
  if (term_columns != new_terminal_size.ws_col) {
    term_columns = new_terminal_size.ws_col;
    if (m_inPlace && !system("clear")) {
      printf("Failed to clear\n");
    }
  }
#endif
}

bool VortexCLI::init(int argc, char *argv[])
{
  if (g_pVortexCLI) {
    return false;
  }
  g_pVortexCLI = this;

  if (argc == 1) {
    print_usage(argv[0]);
    exit(1);
  }

  int opt = -1;
  int option_index = 0;
  while ((opt = getopt_long(argc, argv, "xcstliranquS::W:M:L:I::O::HP:C:A:Vvh", long_options, &option_index)) != -1) {
    switch (opt) {
    case 'x':
      // if the user wants pretty colors or hex codes
      m_outputType = OUTPUT_TYPE_HEX;
      break;
    case 'c':
      // if the user wants pretty colors
      m_outputType = OUTPUT_TYPE_COLOR;
      break;
    case 's':
      // turn off output
      m_outputType = OUTPUT_TYPE_SILENT;
      break;
    case 't':
      // if the user wants to bypass timestep
      m_noTimestep = true;
      break;
    case 'l':
      // if the user wants to step in lockstep with the engine
      m_lockstep = true;
      break;
    case 'i':
      // if the user wants to print in-place (on one line)
      m_inPlace = true;
      break;
    case 'r':
      // record the inputs and dump them to a file after
      m_record = true;
      break;
    case 'a':
      // autuowake prevents sleep
      m_sleepEnabled = false;
      break;
    case 'n':
      // disable the lock
      m_lockEnabled = false;
      break;
    case 'q':
      // quick exit
      m_quickExit = true;
      break;
    case 'u':
      if (optarg == NULL && optind < argc && argv[optind][0] != '-') {
        optarg = argv[optind++];
      }
      if (optarg) {
        m_ledCount = (uint8_t)strtoul(optarg, NULL, 10);
      }
      break;
    case 'S':
      // enable persistent storage to file
      m_storage = true;
      if (optarg == NULL && optind < argc && argv[optind][0] != '-') {
        optarg = argv[optind++];
      }
      if (optarg) {
        m_storageFile = optarg;
      }
      break;
    case 'W':
      m_writeSaveFile = optarg;
      break;
    case 'M':
      m_writeModeFile = optarg;
      break;
    case 'L':
      m_loadSaveFile = optarg;
      break;
    case 'I':
      // read json from stdin or file
      if (optarg == NULL && optind < argc && argv[optind][0] != '-') {
        optarg = argv[optind++];
      }
      if (optarg) {
        m_jsonMode = (JsonMode)(m_jsonMode | JSON_MODE_READ_FILE);
        m_jsonInFile = optarg;
      } else {
        m_jsonMode = (JsonMode)(m_jsonMode | JSON_MODE_READ_STDIN);
      }
      break;
    case 'O':
      // write json to output or file
      if (optarg == NULL && optind < argc && argv[optind][0] != '-') {
        optarg = argv[optind++];
      }
      if (optarg) {
        m_jsonMode = (JsonMode)(m_jsonMode | JSON_MODE_WRITE_FILE);
        m_jsonOutFile = optarg;
      } else {
        m_jsonMode = (JsonMode)(m_jsonMode | JSON_MODE_WRITE_STDOUT);
      }
      break;
    case 'H':
      // json human readable
      m_jsonPretty = true;
      break;
    case 'P':
      // preset the pattern ID on the first mode
      m_patternIDStr = optarg;
      break;
    case 'C':
      // preset the colorset on the first mode
      m_colorsetStr = optarg;
      break;
    case 'A':
      // preset the arguments on the first mode
      m_argumentsStr = optarg;
      break;
    case 'V':
      m_displayVersion = true;
      break;
    case 'v':
      // TODO: make this do something
      m_verbosity++;
      break;
    case 'h':
      // print usage and exit
      print_usage(argv[0]);
      exit(EXIT_SUCCESS);
    default: // '?' for unrecognized options
      printf("Unknown arg: -%c\n", opt);
      exit(EXIT_FAILURE);
    }
  }

  if (m_displayVersion) {
    printf(VORTEX_FULL_NAME "\n");
    exit(0);
  }

  switch (m_outputType) {
  case OUTPUT_TYPE_NONE:
    print_usage(argv[0]);
    exit(EXIT_SUCCESS);
    break;
  case OUTPUT_TYPE_COLOR:
    setColoredOutput(true);
    break;
  case OUTPUT_TYPE_HEX:
    setHexOutput(true);
    break;
  case OUTPUT_TYPE_SILENT:
    break;
  }

  // do the vortex init/setup
  m_vortex.initEx<VortexCLICallbacks>();

  // configure the vortex engine as the parameters dictate
  m_vortex.setInstantTimestep(m_noTimestep);
  m_vortex.enableCommandLog(m_record);
  m_vortex.enableLockstep(m_lockstep);
  m_vortex.enableStorage(m_storage);
  if (m_ledCount > 0 && m_ledCount < 256) {
    m_vortex.setLedCount(m_ledCount);
  }
  if (m_storage) {
    m_vortex.setStorageFilename(m_storageFile);
    if (access(m_storageFile.c_str(), F_OK) == 0) {
      // load storage if the file exists
      m_vortex.loadStorage();
    }
  }
  m_vortex.setSleepEnabled(m_sleepEnabled);
  m_vortex.setLockEnabled(m_lockEnabled);

  if (m_jsonMode & JSON_MODE_READ_STDIN) {
    // todo: read js from stdin
    printf("Reading json from stdin\n");
    std::string inputString;
    // Read from stdin until EOF
    inputString.assign((std::istreambuf_iterator<char>(std::cin)),
                        std::istreambuf_iterator<char>());
    // read from m_jsonInFile;
    m_vortex.parseJson(inputString);
  }

  if (m_jsonMode & JSON_MODE_READ_FILE) {
    printf("Reading json from %s\n", m_jsonInFile.c_str());
    // read from m_jsonInFile;
    m_vortex.parseJsonFromFile(m_jsonInFile);
  }

  // load a .vortex or .vtxmode savefile
  if (m_loadSaveFile.size() > 0) {
    ByteStream stream(4096);
    // Open the file in read mode
    FILE *inputFile = fopen(m_loadSaveFile.c_str(), "r");
    if (!inputFile) {
      printf("Failed to open: [%s] (%s)\n", m_loadSaveFile.c_str(), strerror(errno));
      exit(2);
    }
    if (!fread((void *)stream.rawData(), 1, stream.capacity(), inputFile)) {
      // error
      printf("Failed to read [%s]\n", m_loadSaveFile.c_str());
      exit(2);
    }
    fclose(inputFile);
    // clear existing modes
    m_vortex.engine().modes().clearModes();
    // check if the load from savefile was provided, this is kinda ugly but whatever
    if (m_loadSaveFile.size() >= 8 && m_loadSaveFile.rfind(".vtxmode") == m_loadSaveFile.size() - 8) {
      // match the led count of the savefile, a vtxmode
      m_vortex.matchLedCount(stream, true);
      // ends with .vtxmode, load just a single mode
      m_vortex.addNewMode(stream);
    } else if (m_loadSaveFile.size() >= 7 && m_loadSaveFile.rfind(".vortex") == m_loadSaveFile.size() - 7) {
      // match the led count of the savefile, a vortex file
      m_vortex.matchLedCount(stream, false);
      // ends with .vortex, load the entire save
      m_vortex.setModes(stream);
    } else {
      // wasn't a valid savefile name
      printf("Savefile name must end in .vortex or .vtxmode: [%s]\n", m_loadSaveFile.c_str());
      exit(2);
    }
  }

  if (m_patternIDStr.length() > 0) {
    // convert both numeric and string to see which one seems more correct
    PatternID id = (PatternID)strtoul(m_patternIDStr.c_str(), nullptr, 10);
    PatternID strID = m_vortex.stringToPattern(m_patternIDStr);
    if (id == PATTERN_FIRST && strID != PATTERN_NONE) {
      // use the str ID if the numeric ID didn't convert and the string did
      id = strID;
    }
    // TODO: add arg for the led position
    m_vortex.setPatternAt(LED_ALL, id);
  }
  if (m_colorsetStr.length() > 0) {
    stringstream ss(m_colorsetStr);
    string color;
    Colorset set;
    while (getline(ss, color, ',')) {
      // iterate letters and lowercase them
      transform(color.begin(), color.end(), color.begin(), [](unsigned char c){ return tolower(c); });
      if (color_map.count(color) > 0) {
        set.addColor(color_map[color]);
      } else {
        set.addColor(strtoul(color.c_str(), nullptr, 16));
      }
    }
    // TODO: add arg for the led position
    m_vortex.setColorset(LED_ALL, set);
  }
  if (m_argumentsStr.length() > 0) {
    stringstream ss(m_argumentsStr);
    string arg;
    PatternArgs args;
    while (getline(ss, arg, ',')) {
      args.args[args.numArgs++] = strtoul(arg.c_str(), nullptr, 10);
    }
    // TODO: add arg for the led position
    m_vortex.setPatternArgs(LED_ALL, args);
  }
  if (m_inPlace && !system("clear")) {
    printf("Failed to clear\n");
  }

  set_terminal_nonblocking();
  get_terminal_size();

  m_initialized = true;

#ifndef WASM
#else
  // NOTE: This call does not return and will instead automatically 
  // call the VortexCLI::run() in a loop
  wasm_init();
#endif

  return true;
}

void VortexCLI::run()
{
  if (!stillRunning()) {
    return;
  }
  if (!m_vortex.tick() || m_quickExit) {
    cleanup();
  }
}

void VortexCLI::cleanup()
{
  DEBUG_LOG("Quitting...");
  if (m_inPlace) {
    printf("\n");
  }
  if (m_record) {
    // Open the file in write mode
    FILE *outputFile = fopen(RECORD_FILE, "w");
    if (!outputFile) {
      printf("Failed to open: [%s] (%s)", RECORD_FILE, strerror(errno));
      exit(2);
    }
    // Print the recorded input to the file
    fprintf(outputFile, "%s", m_vortex.getCommandLog().c_str());
    // Close the output file
    fclose(outputFile);
    printf("Wrote recorded input to " RECORD_FILE "\n");
  }
  if (m_jsonMode & JSON_MODE_WRITE_STDOUT) {
    // dump the current save in json format
    std::cout << m_vortex.printJson(m_jsonPretty);
  }
  if (m_jsonMode & JSON_MODE_WRITE_FILE) {
    m_vortex.printJsonToFile(m_jsonOutFile.c_str(), m_jsonPretty);
    printf("Wrote JSON to file [%s]\n", m_jsonOutFile.c_str());
  }
  if (m_writeSaveFile.length() > 0) {
    ByteStream stream;
    m_vortex.getModes(stream);
    FILE *outputFile = fopen(m_writeSaveFile.c_str(), "w");
    if (!outputFile) {
      printf("Failed to open: [%s] (%s)", m_writeSaveFile.c_str(), strerror(errno));
      exit(2);
    }
    // Print the recorded input to the file
    fwrite(stream.rawData(), 1, stream.rawSize(), outputFile);
    // Close the output file
    fclose(outputFile);
    printf("Wrote vortex save to [%s]\n", m_writeSaveFile.c_str());
  }
  if (m_writeModeFile.length() > 0) {
    ByteStream stream;
    m_vortex.getCurMode(stream);
    FILE *outputFile = fopen(m_writeModeFile.c_str(), "w");
    if (!outputFile) {
      printf("Failed to open: [%s] (%s)", m_writeModeFile.c_str(), strerror(errno));
      exit(2);
    }
    // Print the recorded input to the file
    fwrite(stream.rawData(), 1, stream.rawSize(), outputFile);
    // Close the output file
    fclose(outputFile);
    printf("Wrote vtxmode save to [%s]\n", m_writeModeFile.c_str());
  }
  m_keepGoing = false;
  m_isPaused = false;
  if (m_storage) {
    m_vortex.doSave();
  }
  m_vortex.cleanup();
#ifdef WASM
  emscripten_force_exit(0);
#endif
}

// when the glove framework calls 'FastLED.show'
void VortexCLI::show()
{
  if (!m_initialized) {
    return;
  }
  if (m_outputType == OUTPUT_TYPE_SILENT) {
    return;
  }
  string out;
  get_terminal_size();
  uint32_t wid = term_columns;
  uint32_t odd = (wid) % 2;
  uint32_t halfwid = wid / 2;
  uint32_t midWid = (halfwid - ((2 + (m_outputType == OUTPUT_TYPE_HEX)) * LED_COUNT)) - 1;
  if (m_inPlace) {
    // this resets the cursor back to the beginning of the line and moves it up 12 lines
    out += "\33[2K\033[17A\r";
    // this is the top border line
    out += "+";
    out += LINE((wid + odd) - 2);
    out += "+\n|";
    // this is the left inner line
    out += LINE(midWid + odd);
    out += "=";
  }
  if (m_outputType == OUTPUT_TYPE_COLOR) {
    // the color strip itself
    for (uint32_t i = 0; i < m_numLeds; ++i) {
      out += "\x1B[0m["; // opening |
      out += "\x1B[48;2;"; // colorcode start
      out += to_string(m_ledList[i].red) + ";"; // col red
      out += to_string(m_ledList[i].green) + ";"; // col green
      out += to_string(m_ledList[i].blue) + "m"; // col blue
      out += "  "; // colored space
      out += "\x1B[0m]"; // ending |
    }
  } else if (m_outputType == OUTPUT_TYPE_HEX) {
    // otherwise this just prints out the raw hex code if not in color mode
    for (uint32_t i = 0; i < m_numLeds; ++i) {
      char buf[128] = { 0 };
      snprintf(buf, sizeof(buf), "%06X", m_ledList[i].raw());
      out += buf;
    }
  }
  if (!m_inPlace) {
    out += "\n";
  } else {
    // the right inner line
    out += "=";
    out += LINE((midWid - 1) + odd);
    // the end of middle line, fold and start of 3rd line
    out += "|\n+";
    out += LINE((wid + odd) - 2);
    // space line after the box and before usage
    out += "+\n";
    out += SPACES(wid + odd);
    // the usage message
    for (uint32_t i = 0; i < NUM_USAGE; ++i) {
      const char *brief = (wid < 70) ? input_usage_brief[i] : input_usage[i];
      out += brief;
      out += SPACES((wid + odd + 1) - strlen(brief));
    }
  }
  printf("%s", out.c_str());
  fflush(stdout);
}

bool VortexCLI::isButtonPressed()
{
  return m_vortex.isButtonPressed();
}

bool VortexCLI::stillRunning() const
{
  return (m_initialized && m_keepGoing);
}

void VortexCLI::installLeds(void *leds, uint32_t count)
{
  m_ledList = (RGBColor *)leds;
  m_numLeds = count;
}

long VortexCLI::VortexCLICallbacks::checkPinHook(uint32_t pin)
{
  if (pin == 20) {
    // orbit button 2
    return m_vortex.isButtonPressed(1) ? 0 : 1;
  }
  return m_vortex.isButtonPressed(0) ? 0 : 1;
}

void VortexCLI::VortexCLICallbacks::ledsInit(void *cl, int count)
{
  g_pVortexCLI->installLeds(cl, count);
}

void VortexCLI::VortexCLICallbacks::ledsShow()
{
  g_pVortexCLI->show();
}

// main function to run the CLI
int main(int argc, char *argv[])
{
  VortexCLI cli;
  cli.init(argc, argv);
#ifndef WASM
  while (cli.stillRunning()) {
    cli.run();
  }
#endif
  return 0;
}
