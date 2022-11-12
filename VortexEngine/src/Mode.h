#ifndef MODE_H
#define MODE_H

#include "LedTypes.h"
#include "Patterns.h"

// minimal stl usage
#include <vector>
using namespace std;

class MultiLedPattern;
class SingleLedPattern;
class SerialBuffer;
class Pattern;
class Colorset;

// Bitflags for saving modes
#define MODE_FLAG_NONE              0
// the mode is utilizing a multi-led pattern
#define MODE_FLAG_MULTI_LED         (1 << 0)
// the mode is utilizing the same single-led pattern on each finger
#define MODE_FLAG_ALL_SAME_SINGLE   (1 << 1)

// the keyword 'ALL_SLOTS' can be used to refer to all of the
// mode slots at once when using changePattern or changeColorset
#define ALL_SLOTS LED_COUNT

class Mode
{
public:
  Mode();
  ~Mode();

  // initialize the mode to initial state
  void init();

  // Play the mode
  void play();

  // save the mode to serial
  void serialize(SerialBuffer &buffer) const;
  // load the mode from serial
  void unserialize(SerialBuffer &buffer);

#ifdef TEST_FRAMEWORK
  // save the data template
  void saveTemplate(int level = 0) const;
#endif

  // bind either a multi-led pattern o
  bool bind(PatternID id, const Colorset *set);
  // bind a pattern and colorset to individual LED
  bool bindSingle(PatternID id, const Colorset *set, LedPos pos = LED_FIRST);
  // bind a multi led pattern and colorset to all of the LEDs
  bool bindMulti(PatternID id, const Colorset *set);

  // unbind a single pattern and colorset from the mode
  void unbindSingle(LedPos pos = LED_FIRST);
  // unbind a multi pattern and colorset from the mode
  void unbindMulti();
  // clear and delete all patterns and colorsets from the mode
  void unbindAll();

  // Get pointer to an individual pattern/colorset
  const Pattern *getPattern(LedPos pos = LED_FIRST) const;
  // get a pointer to a colorset
  const Colorset *getColorset(LedPos pos = LED_FIRST) const;
  // get the pattern ID of the given pattern
  PatternID getPatternID(LedPos pos = LED_FIRST) const;

  // mode comparison
  bool equals(const Mode *other) const;

  // set the pattern/colorset of the mode, if a multi-led pattern is provided then the pos
  // is ignored. If a single led pattern is provided then it will be applied to all LEDS
  // unless a specific LED is provided
  bool setPattern(PatternID pat);
  bool setColorset(const Colorset *set);

  // get the flags associated with this mode
  uint32_t getFlags() const;

  // is this a multi-led pattern in the mode?
  bool isMultiLed() const;
  // are all the single led patterns and colorsets equal?
  bool isSameSingleLed() const;

private:
  bool setSinglePat(PatternID pat, LedPos pos);
  bool setMultiPat(PatternID pat);

  // erase any stored patterns or colorsets
  void clearPatterns();
  void clearPattern(LedPos pos);
  void clearColorsets();

  // A mode simply contains a list of patterns for each LED, these can either
  // be each SingleLedPatterns up to LED_COUNT of them -- or the first entry
  // can be a MultiLedPattern, just one.
  union {
    // map of led positions => pattern entries
    Pattern *m_ledEntries[LED_COUNT];
    // accessors for single leds
    SingleLedPattern *m_singleLedEntries[LED_COUNT];
    // or the first one is also a multi led pat
    MultiLedPattern *m_multiPat;
  };
};

#endif
