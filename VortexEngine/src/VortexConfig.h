#ifndef VORTEX_CONFIG_H
#define VORTEX_CONFIG_H

// ===================================================================
//  Version Configurations

// The engine major version indicates the state of the save file,
// if any changes to the save format occur then the major version
// must increment so that the savefiles will not be loaded
#define VORTEX_VERSION_MAJOR  1

// A minor version simply indicates a bugfix or minor change that
// will not effect the save files produces by the engine. This means
// a savefile produced by 1.1 should be loadable by an engine on 1.2
// and vice versa. But an engine on 2.0 cannot share savefiles with
// either of the engines on version 1.1 or 1.2
#define VORTEX_VERSION_MINOR  0

// produces a number like 1.0
#define VORTEX_VERSION_NUMBER VORTEX_VERSION_MAJOR.VORTEX_VERSION_MINOR

// produces a string like "1.0"
#define ADD_QUOTES(str) #str
#define EXPAND_AND_QUOTE(str) ADD_QUOTES(str)
#define VORTEX_VERSION EXPAND_AND_QUOTE(VORTEX_VERSION_NUMBER)

// the engine flavour, this should change for each device/flavour
// of the engine that branches off from the main indefinitely
#define VORTEX_NAME "Igneous"

// the full name of this build for ex:
//    Vortex Engine v1.0 'Igneous' (built Tue Jan 31 19:03:55 2023)
#define VORTEX_FULL_NAME "Vortex Engine v" VORTEX_VERSION " '" VORTEX_NAME "' ( built " __TIMESTAMP__ ")"

// Vortex Slim
//
// Turn on this flag to enable the 'slim' version of the engine
#define VORTEX_SLIM           0

// ===================================================================
//  Numeric Configurations

// Menu Trigger Threshold (in milliseconds)
//
// How long the button must be held to trigger ring menu and begin
// filling the first menu color
#define MENU_TRIGGER_TIME     1000

// Menu Duration (in milliseconds)
//
// How long each ring menu takes to fill, after which the next
// menu will start filling
#define MENU_FILL_TIME        1000

// Short Click Threshold (in milliseconds)
//
// If click held for <= this value then the click will be registered
// as a 'short click' otherwise if held longer than this threshold
// it will be registered as a 'long click'
#define CLICK_THRESHOLD       250

// Startup Ignore Button Time (in milliseconds)
//
// The amount of time the engine will ignore button presses after
// it has started up, this is to prevent accidental button presses
#define IGNORE_BUTTON_TIME    150

// Color delete threshold (in milliseconds)
//
// How long you must hold down on a color in the color select menu to
// trigger the delete option to start flashing on the tip
#define COL_DELETE_THRESHOLD  2000

// Color delete cycle time (in milliseconds)
//
// How long the color delete light takes to cycle on/off when holding
// down the button on a color in the color select menu
#define COL_DELETE_CYCLE      2000

// Factory Reset Threshold (in milliseconds)
//
// This is how long the user must hold down the button on the factory
// reset menu to confirm the reset and restore factory settings
#define RESET_HOLD_TIME       5000

// Serial check time (in milliseconds)
//
// This is how quickly the serial class can check for serial connections
// when the editor menu is open
#define SERIAL_CHECK_TIME     500

// Max Color Slots
//
// The max number of colors in a colorset, this was never tested with
// anything other than 8, you have been warned
#define MAX_COLOR_SLOTS       8

// Default Global Brightness
//
// The starting default global brightness if there is no savefile
// present The maximum value is 255
#define DEFAULT_BRIGHTNESS    20

// Max Modes
//
// The maximum number of modes that can be stored on the device.
// This should reflect the available RAM of the device.
//
// In our tests even 45 fully loaded modes only took up 15kb
//
// Set this to 0 for no limit on the number of modes
//
#define MAX_MODES             0

// Default Tickrate in Ticks Per Second (TPS)
//
// The valid range for this is 1 <= x <= 1000000 (default 1000)
//
// Any value near or above 10000 will most likely be too fast for
// the processor to handle.
//
// Any value less than 100 and you risk a single tick taking longer
// than some pattern timings which results in unexpected behaviour
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
#define IR_RECEIVER_TIMEOUT_DURATION 1000

// IR Sender Wait Duration (ms)
//
// This is the amount of time in ms for the IR sender to wait
// between IR sends. This duration allows the user to give input
// as it is not possible to give input during a send.
#define IR_SENDER_WAIT_DURATION 1000

// ===================================================================
//  Boolean Configurations (0 or 1)

// Fill From Thumb
//
// The ring menu will fill from the thumb if this is present, otherwise
// it will fill from the pinkie.
//
// The logic is cleaner for fill from pinkie but fill from thumb is preferred
#define FILL_FROM_THUMB       0

// Use Palm Lights
//
// Adjust the engine to account for palm lights
#define USE_PALM_LIGHTS       0

// Demo All Patterns
//
// The default modes that are set on the gloveset will be dynamically
// generated with one mode per pattern in the patterns list
//
// This can be used to quickly demo all possible patterns, mostly useful
// for testing and development
#define DEMO_ALL_PATTERNS     0

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
// are saved in the savefile. This should be enabled for arduino or vortex
// device builds because they cannot change their number of LEDs. However
// other tools like the editor or vortex emulator may be able to make use
// of this to dynamically adjust the number of leds that a mode can handle
//
// NOTE: This does not touch the 'leds' class itself it only adjusts whether
//       a mode will stretch it's list of patterns to match the number of leds
#define FIXED_LED_COUNT       1

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

// ===================================================================
//  Editor Verbs
//
//  These are the commands used in the protocol with the editor.
//  They are defined here so the editor can access them easily,
//  also so you can configure them at your own free will.

// the initial hello from the gloveset to the editor
// is the full name of this build of vortex
#define EDITOR_VERB_GREETING_PREFIX   "== "
#define EDITOR_VERB_GREETING_POSTFIX  " =="
#define EDITOR_VERB_GREETING          EDITOR_VERB_GREETING_PREFIX VORTEX_FULL_NAME EDITOR_VERB_GREETING_POSTFIX

// the hello from the editor to the gloves
#define EDITOR_VERB_HELLO             "a"

// the response from the gloveset when it's ready to receive something
// after the editor has given it a command to do something the gloveset
// will respond with this then once it's done doing the action it will
// send a different finished response for each action
#define EDITOR_VERB_READY             "b"

// the command from the editor to send modes over
#define EDITOR_VERB_PULL_MODES        "c"
// the response from the editor once modes are received
#define EDITOR_VERB_PULL_MODES_DONE   "d"
// the response from the gloves once it acknowledges the editor got the modes
#define EDITOR_VERB_PULL_MODES_ACK    "e"

// the command from the editor to send modes over
#define EDITOR_VERB_PUSH_MODES        "f"
// the response from the gloveset when it received the mode
#define EDITOR_VERB_PUSH_MODES_ACK    "g"

// the command from the editor to tell the gloveset to demo a mode
#define EDITOR_VERB_DEMO_MODE         "h"
// the response from the gloveset when it's received the mode to demo
#define EDITOR_VERB_DEMO_MODE_ACK     "i"

// the command from the editor to tell the gloveset to clear the demo
#define EDITOR_VERB_CLEAR_DEMO        "j"
// the response from the gloveset when it's received disabled the demo
#define EDITOR_VERB_CLEAR_DEMO_ACK    "k"

// when the gloveset is leaving the menu and needs to tell the editor
// that it's no longer listening
#define EDITOR_VERB_GOODBYE           "l"

// ===================================================================
//  Manually Configured Sizes
//
//  These are the various storage space constants of the vortex device

// maximum storage space in bytes
#define MAX_STORAGE_SPACE 262144

// the size of the compiled engine
#define ENGINE_SIZE 88776

// the raw amount of available space
#define RAW_AVAILABLE_SPACE (MAX_STORAGE_SPACE - ENGINE_SIZE)

// usable flash space is one eighth of what we have left idk why I
// just kept picking numbers till it worked
#define USABLE_SPACE (RAW_AVAILABLE_SPACE / 8)

// the space available for storing modes is the usable space rounded
// down to nearest 4096
#define STORAGE_SIZE (USABLE_SPACE - (USABLE_SPACE % 4096))

// ===================================================================
//  Test Framework configurations
//
//   * Unless you are using the test framework, don't touch these! *

// These defines come from the project settings for preprocessor, an
// entry for $(SolutionName) produces preprocessor definitions that
// match the solution that is compiling the engine
#if !defined(PROJECT_NAME_VortexTestingFramework) && !defined(PROJECT_NAME_VortexEditor) && !defined(VORTEX_LIB)
#define VORTEX_ARDUINO 1
#endif

// if building for editor or test framework then we're building vortex lib, it should
// be defined in the project settings but sometimes it's not for example if a cpp file
// from the test framework includes a header from the engine it might not have VORTEX_LIB
// this will ensure that anything which includes this config file will have VORTEX_LIB
#if defined(PROJECT_NAME_VortexTestingFramework) || defined(PROJECT_NAME_VortexEditor)
#undef VORTEX_LIB
#define VORTEX_LIB
#endif

// This will be defined if the project is being built inside the test framework
#ifdef PROJECT_NAME_VortexTestingFramework

// In the test framework variable tickrate must be enabled to allow
// the tickrate slider to function, also the test framework never runs
// at full tickrate, maximum is 500 tps
#undef VARIABLE_TICKRATE
#define VARIABLE_TICKRATE 1

#undef DEFAULT_BRIGHTNESS
#define DEFAULT_BRIGHTNESS 255

#undef FIXED_LED_COUNT
#define FIXED_LED_COUNT 0

// force logging to 3 on linux build
#ifndef _MSC_VER
#undef LOGGING_LEVEL
#define LOGGING_LEVEL 3
#endif

#endif // VortexTestingFramework

// This will be defined if the project is being built inside the editor
#ifdef VortexEditor

#undef FIXED_LED_COUNT
#define FIXED_LED_COUNT 0

#endif // VortexEditor

// When running in the test framework with demo all patterns enabled
// we should change the max patterns to the total pattern count because
// the test framework can handle the memory usage and we can't demo
// all the patterns without the increased limit
#if DEMO_ALL_PATTERNS == 1 || SERIALIZATION_TEST == 1 || COMPRESSION_TEST == 1
  #undef MAX_MODES
  #include "Patterns/Patterns.h"
  #define MAX_MODES           0
  #undef LOGGING_LEVEL
  #define LOGGING_LEVEL         3
#endif

#endif // VORTEX_CONFIG_H
