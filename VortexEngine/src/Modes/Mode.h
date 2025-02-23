#ifndef MODE_H
#define MODE_H

#include "../Leds/LedTypes.h"
#include "../Patterns/Patterns.h"
#include "../VortexConfig.h"

class SingleLedPattern;
class MultiLedPattern;
class PatternArgs;
class ByteStream;
class Colorset;
class Pattern;

typedef uint8_t ModeFlags;

// Bitflags for saving modes
#define MODE_FLAG_NONE              0
// the mode is utilizing a multi-led pattern
#define MODE_FLAG_MULTI_LED         (1 << 0)
// the mode is utilizing at least one single-led pattern
#define MODE_FLAG_SINGLE_LED        (1 << 1)
// the mode is utilizing the same single-led pattern on each finger
// optimization will be done when saving and only one led is saved
#define MODE_FLAG_ALL_SAME_SINGLE   (1 << 2)
// the mode is missing leds on some single slots, an led map is
// present in savefile in order to convey which leds are missing
#define MODE_FLAG_SPARSE_SINGLES    (1 << 3)

// A mode is the container for instances of patterns. A pattern
// must be bound to a colorset and led position with bind() and
// the Mode class allows that to be done in one step. Other things
// can be done in the Mode class such as accelerometer reaction,
// microphone reaction, timed colorset or pattern switches, or any
// other kind of logic that would involve manipulating the pattern
// or colorset as a whole
class Mode
{
public:
  Mode(VortexEngine &engine, uint8_t numLeds);
  Mode(VortexEngine &engine);

  Mode(VortexEngine &engine, PatternID id, const Colorset &set);
  Mode(VortexEngine &engine, PatternID id, const PatternArgs &args, const Colorset &set);
  Mode(VortexEngine &engine, PatternID id, const PatternArgs *args, const Colorset *set);
  ~Mode();

  // copy and assignment operators
  Mode(const Mode &other);
  void operator=(const Mode &other);

  // equality operators
  bool operator==(const Mode &other) const;
  bool operator!=(const Mode &other) const;

  // initialize the mode to initial state
  void init();

  // Play the mode
  void play();

  // save/load from a buffer for individual mode sharing
  bool saveToBuffer(ByteStream &saveBuffer, uint8_t numLeds = 0) const;
  bool loadFromBuffer(ByteStream &saveBuffer);

  // save the mode to serial
  bool serialize(ByteStream &buffer, uint8_t numLeds = 0) const;
  // load the mode from serial (optional led count)
  bool unserialize(ByteStream &buffer);

  // mode comparison
  bool equals(const Mode *other) const;

  // change the internal pattern count in the mode object
  bool setLedCount(uint8_t numLeds);
  uint8_t getLedCount() const;

  // get the pattern at a position
  const Pattern *getPattern(LedPos pos) const;
  Pattern *getPattern(LedPos pos);

  // get the colorset at a position
  const Colorset getColorset(LedPos pos) const;
  Colorset getColorset(LedPos pos);

  // get the pattern ID of the given pattern
  PatternID getPatternID(LedPos pos) const;

  // change the pattern on a mode (NOTE: you may need to call init() after!)
  bool setPattern(PatternID pat, LedPos pos, const PatternArgs *args = nullptr, const Colorset *set = nullptr);
  bool setPatternMap(LedMap pos, PatternID pat, const PatternArgs *args = nullptr, const Colorset *set = nullptr);

  // set colorset at a specific position
  bool setColorset(const Colorset &set, LedPos pos);
  // set colorset at each position in a map
  bool setColorsetMap(LedMap map, const Colorset &set);

  // clear stored patterns in various ways
  void clearPattern(LedPos pos);
  void clearPatternMap(LedMap map);

  // clear colorset in various ways
  void clearColorset(LedPos pos);
  void clearColorsetMap(LedMap map);

  // set/get a single argument on various positions
  void setArg(uint8_t index, uint8_t value, LedMap map);
  uint8_t getArg(uint8_t index, LedPos pos);

  // get the flags associated with this mode
  ModeFlags getFlags() const;

  // whether a multi-led pattern is present in the mode
  bool hasMultiLed() const;
  // whether at least one single-led pattern is present in the mode
  bool hasSingleLed() const;
  // whether this mode has all same single-led patterns
  bool hasSameSingleLed() const;
  // whether this mode has sparse single-led patterns (missing some slots)
  bool hasSparseSingleLed() const;

  // whether the mode contains no patterns, or all patterns are PATTERN_NONE
  bool isEmpty() const;

  // get the led map of which singles are set
  LedMap getSingleLedMap() const;

  // whether this mode purely a multi-led pattern with no singles
  bool isMultiLed() const;

#if MODES_TEST == 1
  static void test();
#endif

private:
  // reference to engine
  VortexEngine &m_engine;

#if VORTEX_SLIM == 0
  // the multi-led pattern slot is only present in non-slim builds
  Pattern *m_multiPat;
#endif
  // list of pointers to Patterns, one for each led
  std::vector<Pattern *> m_singlePats;
};

#endif
