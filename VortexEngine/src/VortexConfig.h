#ifndef VORTEX_CONFIG_H
#define VORTEX_CONFIG_H

// ===================================================================
//  Version Configurations

// The engine major version indicates the state of the save file,
// if any changes to the save format occur then the major version
// must increment so that the savefiles will not be loaded
#ifndef VORTEX_VERSION_MAJOR
#define VORTEX_VERSION_MAJOR  1
#endif

// A minor version simply indicates a bugfix or minor change that
// will not effect the save files produces by the engine. This means
// a savefile produced by 1.1 should be loadable by an engine on 1.2
// and vice versa. But an engine on 2.0 cannot share savefiles with
// either of the engines on version 1.1 or 1.2
#ifndef VORTEX_VERSION_MINOR
#define VORTEX_VERSION_MINOR  3
#endif

// The build or patch number based on the major.minor version, this is
// set by the build system using the number of commits since last version
#ifndef VORTEX_BUILD_NUMBER
#define VORTEX_BUILD_NUMBER 0
#endif

// produces a number like 1.0.0
#ifndef VORTEX_VERSION_NUMBER
#define VORTEX_VERSION_NUMBER VORTEX_VERSION_MAJOR.VORTEX_VERSION_MINOR.VORTEX_BUILD_NUMBER
#endif

// produces a string like "1.1.0"
#define ADD_QUOTES(str) #str
#define EXPAND_AND_QUOTE(str) ADD_QUOTES(str)
#define VORTEX_VERSION EXPAND_AND_QUOTE(VORTEX_VERSION_NUMBER)

// the engine flavour, this should change for each device/flavour
// of the engine that branches off from the main indefinitely
#define VORTEX_NAME "Gloves"

// the full name of this build for ex:
//    Vortex Engine v1.0 'Igneous' (built Tue Jan 31 19:03:55 2023)
#define VORTEX_FULL_NAME "Vortex Engine v" VORTEX_VERSION " '" VORTEX_NAME "' (built " __TIMESTAMP__ ")"

// Vortex Slim
//
// Turn on this flag to enable the 'slim' version of the engine.
// This disabled multi-led patterns and compression because the
// multi-led patterns take up too much space and the compression
// uses too much stack space to run on smaller devices
#define VORTEX_SLIM           0

// ===================================================================
//  Numeric Configurations

// Menu Trigger Threshold (in milliseconds)
//
// How long the button must be held to trigger menu selection and
// begin blinking the first menu color
#define MENU_TRIGGER_TIME     1000

// Short Click Threshold (in milliseconds)
//
// If click held for <= this value then the click will be registered
// as a 'short click' otherwise if held longer than this threshold
// it will be registered as a 'long click'
#define CLICK_THRESHOLD       250

// Device Lock Clicks
//
// How many rapid clicks the user must perform to lock/unlock the device.
// From sleep, this many rapid clicks will locks the device.
// From locked, the first click will turn it on into 'twilight mode' and then the user
// has UNLOCK_WAKE_WINDOW time to press the button this many times to wake the device.
#define DEVICE_LOCK_CLICKS    5

// Advanced Menu Clicks
//
// The number of rapid clicks required in the menu section to enable or disable
// the advanced menu access
#define ADVANCED_MENU_CLICKS  ((MENU_COUNT >= 5) ? (MENU_COUNT * 2) : 10)

// Rapid Press Window (in milliseconds)
//
// How long the user has after releasing to short click the button
// again and be counted as a 'consecutive press'
#define CONSECUTIVE_WINDOW    250

// Auto Randomization Delay (in milliseconds)
//
// How long the randomizer will wait before it triggers another randomization
// when it's been set to auto mode.
//
// Most electronic music often falls within a range of approximately 120-130 BPM
// To convert BPM to a measure of time in milliseconds (ms) we can do:
// Milliseconds per beat at 128 BPM = 1/128 * 60,000 = approximately 468.75 ms
// So, a beat at 128 BPM lasts about 468.75 ms, so 4 beats would be about 1875.
//
// This will make the randomizer wait approximately 4 edm beats between switches
#define AUTO_RANDOM_DELAY     1875

// Auto Cycle Modes Clicks
//
// The number of consecutive clicks required to toggle the auto-cycle modes feature
// on the main modes list. This will auto cycle to the next mode at the same time
// as the auto random delay. This number is intentionally high because we really
// don't want it activated automatically but it's there for demo purposes and as
// a fun little easter egg to anybody that might come across it
#define AUTO_CYCLE_MODES_CLICKS 10

// Randomizer Auto Cycle Modes Clicks
//
// The number of consecutive clicks required to toggle the auto-cycle modes feature
// on the randomizer.
#define AUTO_CYCLE_RANDOMIZER_CLICKS 3

// Leave Advanced Color Select Clicks
//
// The number of consecutive clicks required to exit the advanced color select menu
#define LEAVE_ADV_COL_SELECT_CLICKS 5

// Color delete threshold (in milliseconds)
//
// How long you must hold down on a color in the color select menu to
// trigger the delete option to start flashing
#define COL_DELETE_THRESHOLD  500

// Color delete cycle time (in milliseconds)
//
// How long the color delete light takes to cycle on/off when holding
// down the button on a color in the color select menu
#define COL_DELETE_CYCLE      2000

// Factory Reset Threshold (in milliseconds)
//
// This is how long the user must hold down the button on the factory
// reset menu to confirm the reset and restore factory settings
#define RESET_HOLD_TIME       2500

// Serial check time (in milliseconds)
//
// This is how quickly the serial class can check for serial connections
// when the editor menu is open, if you lower this then the editor will
// connect quicker but the vortex device will have to do more work
#define SERIAL_CHECK_TIME     500

// Max Color Slots
//
// The max number of colors in a colorset, this was never tested or
// designed to be anything other than 8, you have been warned. If you
// want to increase this or change this then it is suggested you review
// all locations where it or colorset code is being used
#define MAX_COLOR_SLOTS       8

// Default Global Brightness
//
// The starting default global brightness if there is no savefile
// present The maximum value is 255
#define DEFAULT_BRIGHTNESS    185

// Max Modes
//
// The maximum number of modes that can be stored on the device.
// This should reflect the available RAM of the device.
//
// In our tests even 45 fully loaded modes only took up 15kb,
// however this is heavily dependent on the state of compression.
//
// Smaller devices aren't able to comrpess as easily due to the lack
// of stack space for compression algorithms. This means they have
// two fold the issues with space because they are already limited
// and the lack of compression makes their save files bigger
//
// This should not be set to 0, it should be a specific maximum for
// each separate device
//
#define MAX_MODES             15

// Default Tickrate in Ticks Per Second (TPS)
//
// The valid range for this is 1 <= x <= 1000000 (default 1000)
//
// Any value near or above 10000 will most likely be too fast for
// the processor to handle.
//
// Any value less than 100 and you risk a single tick taking longer
// than some pattern timings which results in very weird behaviour
//
// It's probably best that you leave this at 1000
//
// WARNING:
//
//  The timer system was not designed to handle ticks that are not
//  exactly 1 ms. This means that if you change the tickrate to any
//  value other than 1000 the timer system will miss alarms, patterns
//  will not look correct and there is no fix at the moment.
//
#define DEFAULT_TICKRATE      1000

// Pair time offset in ticks
//
// This changes how many ticks out of sync each finger will run.
// So 33 means each finger runs 33 ticks out of sync with the
// previous finger.
//
// This was an early feature that fell into disrepair, I don't
// think it works anymore and really hasn't proven to be useful
#define DEFAULT_TICK_OFFSET   0

// Max Memory Usage
//
// The maximum memory usage allowed by the memory tracker.
// The memory tracker isn't present in final builds, only debug
// so this number doesn't actually do anything in production.
// Mostly for catching leaks or high memory usage in development.
#define MAX_MEMORY            8000

// Log Level
//
// Set the logging level to control info/error/debug logs accordingly
// The available logging levels are:
//
//  0     Off     All logging is disabled
//  1     Info    Only info logs are present
//  2     Errors  Info and error logs are present
//  3     Debug   All logs are present, info, error, and debug
//
#define LOGGING_LEVEL         0

// Log to Console
//
// Enable logging to console, still need to change LOGGING_LEVEL
// this only enables the console output connection
#define LOG_TO_CONSOLE        0

// Log to File
//
// Enable this configuration to enable logging to the file
#define LOG_TO_FILE           0

// Log Name
//
// The name of the file on disk that will receive the log info
#define VORTEX_LOG_NAME       "vortexlog"

// HSV to RGB Conversion Algorithm
//
// Here you can choose the HSV to RGB conversion algorithm, this will
// control the overall appearance of all colours produced with HSV.
// The available options are:
//
//    1     FastLED 'hsv to rgb rainbow'
//    2     FastLED 'hsv to rgb raw C'
//    3     generic hsv to rgb 'pastel'
//
// Option 1 is the default and legacy choice, also looks best because
// it puts even weight into every color of the rainbow which involves
// stretching some segments like yellow to take up more hue space.
//
// Note you can still call the other routines from your pattern code,
// for example blend and complementary blend use hsv to rgb 'pastel'
// because it looks better than hsv to rgb rainbow
#define HSV_TO_RGB_ALGORITHM   3

// IR Receiver Time-out Duration (ms)
//
// This is the amount of time in ms for the IR receiver to wait
// before reseting itself in the case that communication gets
// interrupted.
#define IR_RECEIVER_TIMEOUT_DURATION 2000

// IR Sender Wait Duration (ms)
//
// This is the amount of time in ms for the IR sender to wait
// between IR sends. This duration allows the user to give input
// as it is not possible to give input during a send.
#define IR_SENDER_WAIT_DURATION 2000

// Enter Advanced Menus Click Duration (ms)
//
// How long the user must long click on the menu item to enter the
// 'advanced' version of the menu
#define ADVANCED_MENU_ENTER_DURATION 1500

// Brightness Options
//
// These are the four options available in the global brightness menu
// There is only four options, be careful not to go too low
#define BRIGHTNESS_OPTION_1         40
#define BRIGHTNESS_OPTION_2         120
#define BRIGHTNESS_OPTION_3         185
#define BRIGHTNESS_OPTION_4         255

// Saturation Options
//
// These are the four saturations available in the color selection menu
// Any color can be picked with any of these 4 saturations
#define SAT_OPTION_1                0
#define SAT_OPTION_2                85
#define SAT_OPTION_3                170
#define SAT_OPTION_4                255

// Value/Luminance Options
//
// These are the four values/luminance options in the color selection menu
// Any color can be picked with any of these 4 values
#define VAL_OPTION_1                0
#define VAL_OPTION_2                85
#define VAL_OPTION_3                170
#define VAL_OPTION_4                255

// Serial Baud Rate
//
// The serial connection baud rate for the editor and anything else serial
#define SERIAL_BAUD_RATE            9600

// ===================================================================
//  Boolean Configurations (0 or 1)

// Debug Allocations
//
// Tracks all memory allocations and logs them, useful for finding leaks
//
// Note tracking allocations while using the test framework will be inaccurate
// because the test framework allocations will be caught as well.
//
// When the test framework does things like display a color for the first time
// it will allocate space permanently for the new brush and never free it
#define DEBUG_ALLOCATIONS     0

// Variable Tickrate
//
// This controls whether the setTickrate function is available and
// whether changing the tickrate is allowed. This goes hand-in-hand
// with the Default Tickrate configuration above
//
// The tickrate should always be fixed in final builds because this
// functionality induces extra performance costs and the intended
// tickrate for the final build should already be known.
//
// However there may be some clever uses for variable tickrate in
// the final build? I'm not sure.
#define VARIABLE_TICKRATE     0

// Fixed LED Count
//
// Do not allow the Mode loader to dynamically load however many modes
// are saved in the savefile. This should be enabled for embedded or vortex
// device builds because they cannot change their number of LEDs. However
// other tools like the editor or vortex emulator may be able to make use
// of this to dynamically adjust the number of leds that a mode can handle
//
// NOTE: This does not touch the 'leds' class itself it only adjusts whether
//       a mode will stretch it's list of patterns to match the number of leds
#define FIXED_LED_COUNT       1

// Enable Editor Connection
//
// Turn on the editor connection, some devices are capable of connecting to
// the pc-based editor, this controls whether the engine exposes the purple
// editor connection menu or not
#define ENABLE_EDITOR_CONNECTION 1

// Compression Test
//
// Run the built-in compression test that will find any faults
// in the compressor or decompressor. This is useful if you install
// a new compressor or want to test any changes to the compressor
#define COMPRESSION_TEST      0

// Serialization Test
//
// Run the serializer/unserializer test which will find any objects
// which don't serialize and unserialize cleanly
#define SERIALIZATION_TEST    0

// Modes Test
//
// Run unit tests on the Modes class which manages the list of Modes.
// If any patterns fail to instantiate or perform basic operations
// it should show up in the modes test. Also if the Modes api has been
// updated then this will test for any issues
#define MODES_TEST            0

// Timer Test
//
// Tests the timers, time control, and timestep system to ensure they
// are all working as expected and behaving properly
#define TIMER_TEST            0


// ===================================================================
//  Menu Colors
//
//  These are the colors for the respective menus, each color should
//  be unique and distinct from the others. It should also be bright
//  enough to be seen on the lowest brightness setting of the device
//
//  See the below header for all of the available color constants
#include "Colors/ColorConstants.h"

// Randomizer Menu Color
#define RGB_MENU_RANDOMIZER         RGB_WHITE1

// Mode Sharing Menu Color
#define RGB_MENU_MODE_SHARING       RGB_CYAN1

// Editor Connection Menu Color
#define RGB_MENU_EDITOR_CONNECTION  RGB_MAGENTA1

// Color Select Menu Color
#define RGB_MENU_COLOR_SELECT       RGB_GREEN1

// Pattern Select Menu Color
#define RGB_MENU_PATTERN_SELECT     RGB_BLUE1

// Global Brightness Menu Color
#define RGB_MENU_BRIGHTNESS_SELECT  RGB_YELLOW1

// Factory Reset Menu Color
#define RGB_MENU_FACTORY_RESET      RGB_RED1


// ===================================================================
//  Editor Verbs
//
//  These are the commands used in the protocol with the editor.
//  They are defined here so the editor can access them easily,
//  also so you can configure them at your own free will.

// the initial hello from the device to the editor
// is the full name of this build of vortex
#define EDITOR_VERB_GREETING_PREFIX       "== "
#define EDITOR_VERB_GREETING_POSTFIX      " =="
#define EDITOR_VERB_GREETING              EDITOR_VERB_GREETING_PREFIX VORTEX_FULL_NAME EDITOR_VERB_GREETING_POSTFIX

// the hello from the editor to the device
#define EDITOR_VERB_HELLO                 "a"

// the response from the device when it's ready to receive something
// after the editor has given it a command to do something the device
// will respond with this then once it's done doing the action it will
// send a different finished response for each action
#define EDITOR_VERB_READY                 "b"

// ===============================================================================
// TODO: remove the below commands once they are no longer used
//       these commands can sometimes cause crashes if the modes list is too big
// the command from the editor to send modes over
#define EDITOR_VERB_PULL_MODES            "c"
// the response from the editor once modes are received
#define EDITOR_VERB_PULL_MODES_DONE       "d"
// the response from the device once it acknowledges the editor got the modes
#define EDITOR_VERB_PULL_MODES_ACK        "e"
// the command from the editor to send modes over
#define EDITOR_VERB_PUSH_MODES            "f"
// the response from the device when it received the mode
#define EDITOR_VERB_PUSH_MODES_ACK        "g"
// ===============================================================================

// the command from the editor to tell the device to demo a mode
#define EDITOR_VERB_DEMO_MODE             "h"
// the response from the device when it's received the mode to demo
#define EDITOR_VERB_DEMO_MODE_ACK         "i"

// the command from the editor to tell the device to clear the demo
#define EDITOR_VERB_CLEAR_DEMO            "j"
// the response from the device when it's received disabled the demo
#define EDITOR_VERB_CLEAR_DEMO_ACK        "k"

// when the device is leaving the menu and needs to tell the editor
// that it's no longer listening
#define EDITOR_VERB_GOODBYE               "l"

// when the computer wants to send a mode to the duo it tells the device
// to transmit over VL and send the current preview mode to the duo
#define EDITOR_VERB_TRANSMIT_VL           "m"
// the response from the device when it's done transmitting the mode
#define EDITOR_VERB_TRANSMIT_VL_ACK       "n"

// when the pc wants the chromadeck to listen for a mode from the duos
#define EDITOR_VERB_LISTEN_VL             "o"
// and the response for when it's done fetching a duo mode
#define EDITOR_VERB_LISTEN_VL_ACK         "p"

// pull the duo saveheader via the chromalink
#define EDITOR_VERB_PULL_CHROMA_HDR       "q"
// and the response for when it's done fetching the modes
#define EDITOR_VERB_PULL_CHROMA_HDR_ACK   "r"

// push the duo save header via the chromalink
#define EDITOR_VERB_PUSH_CHROMA_HDR       "s"
// and the response for when it's done pushing the modes
#define EDITOR_VERB_PUSH_CHROMA_HDR_ACK   "t"

// pull a duo mode via the chromalink
#define EDITOR_VERB_PULL_CHROMA_MODE      "u"
// and the response for when it's done fetching the modes
#define EDITOR_VERB_PULL_CHROMA_MODE_ACK  "v"

// push a duo mode via the chromalink
#define EDITOR_VERB_PUSH_CHROMA_MODE      "w"
// and the response for when it's done pushing the modes
#define EDITOR_VERB_PUSH_CHROMA_MODE_ACK  "x"

// pull a single mode
#define EDITOR_VERB_PULL_SINGLE_MODE      "y"
// and the response for when it's done
#define EDITOR_VERB_PULL_SINGLE_MODE_ACK  "z"

// push a single mode
#define EDITOR_VERB_PUSH_SINGLE_MODE      "A"
// and the response for when it's done
#define EDITOR_VERB_PUSH_SINGLE_MODE_ACK  "B"

// the command from the editor to send modes over
#define EDITOR_VERB_PULL_EACH_MODE        "C"
// the response from the device when it acknowledges a command
#define EDITOR_VERB_PULL_EACH_MODE_ACK    "D"
// the done message from the editor once modes are received
#define EDITOR_VERB_PULL_EACH_MODE_DONE   "E"

// the command from the editor to get ready to receive modes
#define EDITOR_VERB_PUSH_EACH_MODE        "F"
// the response from the device when it received the command
#define EDITOR_VERB_PUSH_EACH_MODE_ACK    "G"
// the done message from the device when it done the command
#define EDITOR_VERB_PUSH_EACH_MODE_DONE   "H"

// flash firmware
#define EDITOR_VERB_FLASH_FIRMWARE        "I"
// the response to each one
#define EDITOR_VERB_FLASH_FIRMWARE_ACK    "J"
// done flashing firmware
#define EDITOR_VERB_FLASH_FIRMWARE_DONE   "K"

// set the global brightness of the device
#define EDITOR_VERB_SET_GLOBAL_BRIGHTNESS "L"
// get the global brightness of the device
#define EDITOR_VERB_GET_GLOBAL_BRIGHTNESS "M"

// ===================================================================
//  Manually Configured Sizes
//
//  These are the various storage space constants of the vortex device

// maximum size of a mode here
#define MAX_MODE_SIZE 512

// the number of storage slots for modes, add 1 for the header
#define NUM_MODE_SLOTS (MAX_MODES + 1)

// the space available for storing modes, we can't make this too big
// otherwise we will have trouble loading it into memory
#define STORAGE_SIZE (MAX_MODE_SIZE * NUM_MODE_SLOTS)

// ===================================================================
//  Test Framework configurations
//
//   * Unless you are using the test framework, don't touch these! *

// These defines come from the project settings for preprocessor, an
// entry for $(SolutionName) produces preprocessor definitions that
// match the solution that is compiling the engine
#if !defined(PROJECT_NAME_VortexTestingFramework) && !defined(PROJECT_NAME_VortexEditor) && !defined(VORTEX_LIB)
#define VORTEX_EMBEDDED 1
#endif

// if building for editor or test framework then we're building vortex lib, it should
// be defined in the project settings but sometimes it's not for example if a cpp file
// from the test framework includes a header from the engine it might not have VORTEX_LIB
// this will ensure that anything which includes this config file will have VORTEX_LIB
#if defined(PROJECT_NAME_VortexTestingFramework) || defined(PROJECT_NAME_VortexEditor)
#undef VORTEX_LIB
#define VORTEX_LIB
#endif

#ifdef VORTEX_LIB
#undef VORTEX_SLIM
#define VORTEX_SLIM 0

// The test framework needs brighter menu colors can't really see them on the screen
#undef RGB_MENU_RANDOMIZER
#undef RGB_MENU_MODE_SHARING
#undef RGB_MENU_EDITOR_CONNECTION
#undef RGB_MENU_COLOR_SELECT
#undef RGB_MENU_PATTERN_SELECT
#undef RGB_MENU_BRIGHTNESS_SELECT
#undef RGB_MENU_FACTORY_RESET
#define RGB_MENU_RANDOMIZER         RGB_WHITE4
#define RGB_MENU_MODE_SHARING       RGB_CYAN4
#define RGB_MENU_EDITOR_CONNECTION  RGB_MAGENTA4
#define RGB_MENU_COLOR_SELECT       RGB_GREEN4
#define RGB_MENU_PATTERN_SELECT     RGB_BLUE4
#define RGB_MENU_BRIGHTNESS_SELECT  RGB_YELLOW4
#define RGB_MENU_FACTORY_RESET      RGB_RED4

#endif // ifdef VORTEX_LIB

// This will be defined if the project is being built inside the test framework
#ifdef PROJECT_NAME_VortexTestingFramework

// In the test framework variable tickrate must be enabled to allow
// the tickrate slider to function, also the test framework never runs
// at full tickrate, maximum is 500 tps
#undef VARIABLE_TICKRATE
#define VARIABLE_TICKRATE 1

#undef DEFAULT_BRIGHTNESS
#define DEFAULT_BRIGHTNESS 255

// test framework needs more time to click idk
#undef CONSECUTIVE_WINDOW
#define CONSECUTIVE_WINDOW 300

// force logging to 3 on linux build
#ifndef _WIN32
#undef LOGGING_LEVEL
#define LOGGING_LEVEL 3
#endif

#endif // VortexTestingFramework

// This will be defined if the project is being built inside the editor
#ifdef PROJECT_NAME_VortexEditor

// The editor needs an unfixed led count in order to load any mode
#undef FIXED_LED_COUNT
#define FIXED_LED_COUNT 0

#endif // VortexEditor

// When running various tests lift the max mode limit and enable logging
#if SERIALIZATION_TEST == 1 || COMPRESSION_TEST == 1
  #undef MAX_MODES
  #include "Patterns/Patterns.h"
  #define MAX_MODES           0
  #undef LOGGING_LEVEL
  #define LOGGING_LEVEL         3
#endif

#endif // VORTEX_CONFIG_H
