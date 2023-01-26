#ifndef VORTEX_ENGINE_H
#define VORTEX_ENGINE_H

#include <inttypes.h>

// TODO STILL:
/*
 *  Everything is working!
 *
 *  But there is still more to work on:
 *   - eliminate library code to reduce size
 *   - palm light?
 *   - Code documentation (DOXYGEN???)
 *   - menu 'blink' api is slightly buggy
 *   - randomizer auto random
 *   - randomizer individual randomizers (colorset, pattern, full, game?)
 *   - update color or pattern used to display send/receive
 * 
 *   - Editor!
 *   - color wheel/picker!
 *   - color 'mode' (rainbow/generic)
 *   - global brightness slider in editor? - tools > brightness
 *   - refactor message pump for editor to solve issue with editor 
 *     parsing handshake properly, need to split messages, need delimiter
 *
 *   - options!
 *   -  load default modes
 *   -  ?
 *   - timers for auto demo? auto random?
 *   - 
 *
 *   - test framework
 *    - could it be a demo gloveset?
 *    - with a moving pattern strip to demo the pattern
 *    - and a glove background picture
 *
 *   - Major version 2.0 notes:
 *      - dynamically save params, allows for loading of params with different amount prevents errors
 *      - but doesn't really make sense because same pattern should have same params in each place
 *
 *      - readout for how big a mode is vs total storage used (progress bar?)
 */

#include "VortexConfig.h"

class ByteStream;

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

  // serialize the version number to a byte stream
  static void serializeVersion(ByteStream &stream);
  static bool checkVersion(uint8_t major, uint8_t minor);

private:
#if COMPRESSION_TEST == 1
  static void compressionTest();
#endif
#if SERIALIZATION_TEST == 1
  static void serializationTest();
#endif
};

#endif
