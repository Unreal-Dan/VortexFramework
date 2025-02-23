#include "Mode.h"

#include "../VortexEngine.h"

#include "../Patterns/Single/SingleLedPattern.h"
#include "../Patterns/Multi/MultiLedPattern.h"
#include "../Patterns/PatternBuilder.h"
#include "../Patterns/Pattern.h"
#include "../Serial/ByteStream.h"
#include "../Time/TimeControl.h"
#include "../Colors/Colorset.h"
#include "../Memory/Memory.h"
#include "../Leds/Leds.h"
#include "../Log/Log.h"

// NOTE!
// Use these to acces led counts from inside mode code, because modes
// have their own led count and don't care about the engine led count
// Also! When using ledmaps always use MAP_LED(MODE_LED_X) do not use
// the mapped engine led constant like MAP_LED_ALL here.
// One exception, LED_FIRST will always be the same so it's okay to use.
#define MODE_LED_COUNT getLedCount()
#define MODE_LED_LAST (MODE_LED_COUNT - 1)
#define MODE_LED_ALL MODE_LED_COUNT
#define MODE_LED_MULTI (MODE_LED_COUNT + 1)
#define MODE_LED_ALL_SINGLE (MODE_LED_COUNT + 2)
#define MODE_LED_ANY (MODE_LED_COUNT + 3)

// MAP_LED(LED_ALL) but for the cur mode
#define MAP_MODE_LED_ALL (LedMap)((2 << (LED_COUNT - 1)) - 1)

// for internal reference to the led count
Mode::Mode(VortexEngine &engine, uint8_t numLeds) :
  m_engine(engine),
#if VORTEX_SLIM == 0
  m_multiPat(nullptr),
#endif
  m_singlePats()
{
  setLedCount(numLeds);
}
Mode::Mode(VortexEngine &engine) :
  Mode(engine, engine.leds().ledCount())
{
}

Mode::Mode(VortexEngine &engine, PatternID id, const Colorset &set) :
  Mode(engine)
{
  setPattern(id, MODE_LED_ANY, nullptr, &set);
}

Mode::Mode(VortexEngine &engine, PatternID id, const PatternArgs &args, const Colorset &set) :
  Mode(engine)
{
  setPattern(id, MODE_LED_ANY, &args, &set);
}

Mode::Mode(VortexEngine &engine, PatternID id, const PatternArgs *args, const Colorset *set) :
  Mode(engine)
{
  setPattern(id, MODE_LED_ANY, args, set);
}

Mode::~Mode()
{
  clearPattern(MODE_LED_ALL);
}

// copy and assignment operators
Mode::Mode(const Mode &other) :
  Mode(other.m_engine)
{
  *this = other;
}

void Mode::operator=(const Mode &other)
{
  setLedCount(other.getLedCount());
  clearPattern(MODE_LED_ALL);
#if VORTEX_SLIM == 0
  if (other.m_multiPat) {
    m_multiPat = m_engine.patternBuilder().dupe(other.m_multiPat);
  }
#endif
  for (LedPos i = LED_FIRST; i < other.getLedCount(); ++i) {
    Pattern *otherPat = other.m_singlePats[i];
    if (!otherPat) {
      continue;
    }
    m_singlePats[i] = m_engine.patternBuilder().dupe(otherPat);
  }
}

// equality operators
bool Mode::operator==(const Mode &other) const
{
  return equals(&other);
}

bool Mode::operator!=(const Mode &other) const
{
  return !equals(&other);
}

void Mode::init()
{
#if VORTEX_SLIM == 0
  // initialize the multi pattern if it's present
  if (m_multiPat) {
    m_multiPat->init();
  }
#endif
  // otherwise regular init
  for (LedPos pos = LED_FIRST; pos < MODE_LED_COUNT; ++pos) {
    // grab the entry for this led and initialize it
    Pattern *entry = m_singlePats[pos];
    if (!entry) {
      continue;
    }
    entry->init();
  }
}

void Mode::play()
{
#if VORTEX_SLIM == 0
  // play multi pattern first so that the singles can override
  if (m_multiPat) {
    m_multiPat->play();
  }
#endif
  // now iterate all singles and play
  for (LedPos pos = LED_FIRST; pos < MODE_LED_COUNT; ++pos) {
    // grab the entry for this led
    Pattern *entry = m_singlePats[pos];
    if (!entry) {
#if VORTEX_SLIM == 0
      // incomplete pattern/set or empty slot
      if (!m_multiPat) {
        // only clear if the multi pattern isn't playing
        m_engine.leds().clearIndex(pos);
      }
#else
      // just clear the index if slim, don't check for multi
      m_engine.leds().clearIndex(pos);
#endif
      continue;
    }
    // play the current pattern with current color set on the current finger
    entry->play();
  }
}

bool Mode::saveToBuffer(ByteStream &modeBuffer, uint8_t numLeds) const
{
  // serialize the engine version into the modes buffer
  m_engine.serializeVersion(modeBuffer);
  // serialize all mode data into the modeBuffer
  serialize(modeBuffer, numLeds);
  DEBUG_LOGF("Serialized mode, uncompressed size: %u", modeBuffer.size());
  return modeBuffer.compress();
}

bool Mode::loadFromBuffer(ByteStream &modeBuffer)
{
  if (!modeBuffer.decompress()) {
    // failed to decompress the mode
    return false;
  }
  // reset the unserializer index before unserializing anything
  modeBuffer.resetUnserializer();
  uint8_t major = 0;
  uint8_t minor = 0;
  // unserialize the vortex version
  if (!modeBuffer.unserialize8(&major)) {
    return false;
  }
  if (!modeBuffer.unserialize8(&minor)) {
    return false;
  }
  // check the version for incompatibility
  if (!m_engine.checkVersion(major, minor)) {
    // incompatible version
    ERROR_LOGF("Incompatible savefile version: %u.%u", major, minor);
    return false;
  }
  // now just unserialize the list of patterns
  if (!unserialize(modeBuffer)) {
    return false;
  }
  // then initialize the mode so that it is ready to play
  init();
  return true;
}

bool Mode::serialize(ByteStream &buffer, uint8_t numLeds) const
{
  if (!numLeds) {
    numLeds = MODE_LED_COUNT;
  }
  // serialize the number of leds
  if (!buffer.serialize8(numLeds)) {
    return false;
  }
  // empty mode?
  if (!numLeds) {
    return true;
  }
  // serialize the flags
  ModeFlags flags = getFlags();
  if (!buffer.serialize8(flags)) {
    return false;
  }
#if VORTEX_SLIM == 0
  // serialiaze the multi led?
  if ((flags & MODE_FLAG_MULTI_LED) && m_multiPat) {
    // serialize the multi led
    if (!m_multiPat->serialize(buffer)) {
      return false;
    }
  }
#endif
  // if no single leds then just stop here
  if (!(flags & MODE_FLAG_SINGLE_LED)) {
    return true;
  }
  // if there are any sparse singles (spaces) then we need to
  // serialize an led map of which singles are set
  if (flags & MODE_FLAG_SPARSE_SINGLES) {
    if (!buffer.serialize32((uint32_t)getSingleLedMap())) {
      return false;
    }
  }
  // then iterate each single led and serialize it
  for (LedPos pos = LED_FIRST; pos < numLeds; ++pos) {
    const Pattern *entry = m_singlePats[pos];
    if (!entry) {
      continue;
    }
    // just serialize the pattern then colorset
    if (!entry->serialize(buffer)) {
      return false;
    }
    // if they are all same single then only serialize one
    if (flags & MODE_FLAG_ALL_SAME_SINGLE) {
      break;
    }
  }
  return true;
}

// this is a hairy function, but a bit of a necessary complexity
bool Mode::unserialize(ByteStream &buffer)
{
  clearPattern(MODE_LED_ALL);
  uint8_t ledCount = MODE_LED_COUNT;
  // unserialize the number of leds
  if (!buffer.unserialize8(&ledCount)) {
    return false;
  }
  // adjust the internal led count of the mode if the incoming stream is different
  // generally no embedded device would ever used an unfixed led count, this is purely
  // for vortexlib to allow things like the editor to load modes of any size leds
  if (ledCount != MODE_LED_COUNT) {
    //setLedCount(ledCount);
  }
  if (!ledCount) {
    // empty mode?
    return true;
  }
  // unserialize the flags value
  ModeFlags flags = 0;
  if (!buffer.unserialize8(&flags)) {
    return false;
  }
  Pattern *firstPat = nullptr;
  // if there is a multi led pattern then unserialize it
  if (flags & MODE_FLAG_MULTI_LED) {
#if VORTEX_SLIM == 1
    // unserialize the multi pattern
    Pattern *multiPat = m_engine.patternBuilder().unserialize(buffer);
    // if there are no single leds then discard the firstpat
    if ((flags & MODE_FLAG_SINGLE_LED) != 0 && multiPat) {
      // discard the multi pattern
      delete multiPat;
    } else {
      // otherwise turn on the all same single flag to use the multi as a single
      flags = MODE_FLAG_SINGLE_LED | MODE_FLAG_ALL_SAME_SINGLE;
      firstPat = multiPat;
    }
#else
    // otherwise in normal build actually unserialize it
    m_multiPat = m_engine.patternBuilder().unserialize(buffer);
    if (!m_multiPat) {
      return false;
    }
    m_multiPat->init();
#endif
  }
  // if there is no single led patterns just stop here
  if (!(flags & MODE_FLAG_SINGLE_LED)) {
    return true;
  }
  // is there an led map to unserialize? if not default to all
  LedMap map = (1 << ledCount) - 1;
  if (flags & MODE_FLAG_SPARSE_SINGLES) {
    if (!buffer.unserialize32((uint32_t *)&map)) {
      return false;
    }
  }
  // unserialize all singleled patterns into their positions
  MAP_FOREACH_LED(map) {
    if (pos >= MODE_LED_COUNT) {
      // in case the map encodes led positions this device doesn't support
      break;
    }
    if (!firstPat) {
      // save the first pattern so that it can be duped if this is 'all same'
      m_singlePats[pos] = firstPat = m_engine.patternBuilder().unserialize(buffer);
    } else if (flags & MODE_FLAG_ALL_SAME_SINGLE) {
      // if all same then just dupe first
      m_singlePats[pos] = m_engine.patternBuilder().dupe(firstPat);
    } else {
      // otherwise unserialize the pattern like normal
      m_singlePats[pos] = m_engine.patternBuilder().unserialize(buffer);
    }
    if (!m_singlePats[pos]) {
      clearPattern(MODE_LED_ALL);
      return false;
    }
    if (!m_singlePats[pos]) {
      clearPattern(MODE_LED_ALL);
      return false;
    }
    m_singlePats[pos]->bind(pos);
  }
  // there is a few different possibilities here:
  //    1. The provided ledCount is less than our current MODE_LED_COUNT
  //      -> if this happens we need to repeat the first ledCount leds
  //         into the remaining MODE_LED_COUNT - ledCount
  //    2. The provided ledCount is more than our current MODE_LED_COUNT
  //      -> if this happens then we can just chop the leds and continue
  //    3. The provided ledCount is the same as our current MODE_LED_COUNT
  //      -> if this happens all is good we can continue
  if (ledCount >= MODE_LED_COUNT) {
    // in this case we either chopped some off or have the exact amount
    return true;
  }
  // in this case we have to repeat them so we loop MODE_LED_COUNT - ledCount
  // times and walk the first ledCount that are already set and copy them
  LedPos src = LED_FIRST;
  // start from ledCount (the first index we didn't load) and loop till
  // MODE_LED_COUNT and dupe the pattern in the src position, but wrap the src
  // around at ledCount so that we repeat the first ledCount over again
  for (LedPos pos = (LedPos)ledCount; pos < MODE_LED_COUNT; ++pos) {
    m_singlePats[pos] = m_engine.patternBuilder().dupe(m_singlePats[src]);
    m_singlePats[pos]->bind(pos);
    src = (LedPos)((src + 1) % ledCount);
  }
  return true;
}

bool Mode::equals(const Mode *other) const
{
  if (!other) {
    return false;
  }
  // compare the led count
  if (other->getLedCount() != getLedCount()) {
    return false;
  }
#if VORTEX_SLIM == 0
  // compare the multi pattern, either:
  //   have a local pat and it's not equal to other (other could be null)
  //   or have no local pat and other has one
  if ((m_multiPat && !m_multiPat->equals(other->m_multiPat)) ||
      (!m_multiPat && other->m_multiPat)) {
    return false;
  }
#endif
  // compare all the singles
  for (LedPos pos = LED_FIRST; pos < MODE_LED_COUNT; ++pos) {
    // if entry is valid, do a comparison
    if (m_singlePats[pos]) {
      // checks if other is not null and equal
      if (!m_singlePats[pos]->equals(other->m_singlePats[pos])) {
        return false;
      }
      continue;
    }
    // current is null, check if other is valid
    if (other->m_singlePats[pos]) {
      return false;
    }
    // both are null
  }
  return true;
}

// change the internal pattern count in the mode object
bool Mode::setLedCount(uint8_t numLeds)
{
  ByteStream data;
  if (!saveToBuffer(data)) {
    return false;
  }
  m_singlePats.clear();
  m_singlePats.resize(numLeds);
  if (!m_singlePats.size()) {
    ERROR_OUT_OF_MEMORY();
    return false;
  }
  if (!loadFromBuffer(data)) {
    return false;
  }
  return true;
}

uint8_t Mode::getLedCount() const
{
  return (uint8_t)m_singlePats.size();
}

const Pattern *Mode::getPattern(LedPos pos) const
{
  return ((Mode *)this)->getPattern(pos);
}

Pattern *Mode::getPattern(LedPos pos)
{
  // equivalent to case MODE_LED_ALL
  if (pos == MODE_LED_ALL) {
    // makes no sense
    return nullptr;
  }

  // equivalent to case MODE_LED_ANY and MODE_LED_MULTI
  else if (pos == MODE_LED_ANY || pos == MODE_LED_MULTI) {
    if (m_multiPat) {
      return m_multiPat;
    }
    // Don't fall through if actually multi, it's possible
    // we got here by falling through from MODE_LED_ANY
    else if (pos == MODE_LED_MULTI) {
      return nullptr;
    }
    // For MODE_LED_ANY, try to fall through to MODE_LED_ALL_SINGLE
    else if (pos == MODE_LED_ANY) {
      if (!m_singlePats.size()) {
        return nullptr;
      }
      for (LedPos p = LED_FIRST; p < MODE_LED_COUNT; ++p) {
        if (m_singlePats[p]) {
          return m_singlePats[p];
        }
      }
    }
  }

  // equivalent to case MODE_LED_ALL_SINGLE
  else if (pos == MODE_LED_ALL_SINGLE) {
    if (!m_singlePats.size()) {
      return nullptr;
    }
    for (LedPos p = LED_FIRST; p < MODE_LED_COUNT; ++p) {
      if (m_singlePats[p]) {
        return m_singlePats[p];
      }
    }
  }

  // equivalent to default case (covers any other pos)
  else {
    if (!m_singlePats.size()) {
      return nullptr;
    }
    if (pos < MODE_LED_COUNT && m_singlePats[pos]) {
      return m_singlePats[pos];
    }
  }

  // in case nothing matches, return nullptr
  return nullptr;
}


const Colorset Mode::getColorset(LedPos pos) const
{
  return ((Mode *)this)->getColorset(pos);
}

Colorset Mode::getColorset(LedPos pos)
{
  Pattern *pat = getPattern(pos);
  if (!pat) {
    return Colorset();
  }
  return pat->getColorset();
}

PatternID Mode::getPatternID(LedPos pos) const
{
  const Pattern *pat = getPattern(pos);
  if (!pat) {
    return PATTERN_NONE;
  }
  return pat->getPatternID();
}

bool Mode::setPattern(PatternID pat, LedPos pos, const PatternArgs *args, const Colorset *set)
{
  // Use provided colorset, or colorset from pos if valid, otherwise use effective colorset
  Colorset newSet = set ? *set : getColorset(((pos < MODE_LED_COUNT) && m_singlePats[pos]) ? pos : MODE_LED_ANY);

  // Equivalent to cases MODE_LED_ANY, MODE_LED_ALL and MODE_LED_MULTI
  if (pos == MODE_LED_ANY || pos == MODE_LED_ALL || pos == MODE_LED_MULTI) {
#if VORTEX_SLIM == 0
    if (m_multiPat) {
      delete m_multiPat;
      m_multiPat = nullptr;
    }
    if (isMultiLedPatternID(pat)) {
      m_multiPat = m_engine.patternBuilder().makeMulti(pat, args);
      if (m_multiPat) {
        // they could set PATTERN_NONE to clear
        m_multiPat->setColorset(newSet);
      }
      // TODO: don't clear single leds if we're setting multi
      // but for now it's easier to just clear them
      clearPattern(MODE_LED_ALL_SINGLE);
      return true;
    }
    if (pos == MODE_LED_MULTI) {
      // don't fallthrough if actually multi, it's possible
      // we got here by falling through from MODE_LED_ALL
      return false;
    }
    // fall through if MODE_LED_ALL and delete the single leds
    else if (pos == MODE_LED_ANY || pos == MODE_LED_ALL) {
      if (!m_singlePats.size()) {
        return false;
      }
      for (LedPos p = LED_FIRST; p < MODE_LED_COUNT; ++p) {
        if (!setPattern(pat, p, args, &newSet)) {
          return false;
        }
      }
      return true;  // actually break here
    }
#endif
  }

  // equivalent to case MODE_LED_ALL_SINGLE
  else if (pos == MODE_LED_ALL_SINGLE) {
    if (!m_singlePats.size()) {
      return false;
    }
    for (LedPos p = LED_FIRST; p < MODE_LED_COUNT; ++p) {
      if (!setPattern(pat, p, args, &newSet)) {
        return false;
      }
    }
    return true;
  }

  // equivalent to default case (covers any other pos)
  else {
    if (!m_singlePats.size()) {
      return false;
    }
    if (pos >= MODE_LED_COUNT) {
      return false;
    }
    if (m_singlePats[pos]) {
      delete m_singlePats[pos];
    }
    m_singlePats[pos] = m_engine.patternBuilder().makeSingle(pat, args);
    // they could set PATTERN_NONE to clear
    if (m_singlePats[pos]) {
      m_singlePats[pos]->setColorset(newSet);
      m_singlePats[pos]->bind(pos);
    }
    return true;
  }

  return false;
}

bool Mode::setPatternMap(LedMap map, PatternID pat, const PatternArgs *args, const Colorset *set)
{
  MAP_FOREACH_LED(map) {
    if (!setPattern(pat, pos, args, set)) {
      return false;
    }
  }
  return true;
}

// set colorset at a specific position
bool Mode::setColorset(const Colorset &set, LedPos pos)
{
  // Equivalent to cases MODE_LED_ANY, MODE_LED_ALL and MODE_LED_MULTI
  if (pos == MODE_LED_ANY || pos == MODE_LED_ALL || pos == MODE_LED_MULTI) {
#if VORTEX_SLIM == 0
    if (m_multiPat) {
      m_multiPat->setColorset(set);
    }
    if (pos == MODE_LED_MULTI) {
      // don't fallthrough if actually multi, it's possible
      // we got here by falling through from MODE_LED_ALL
      return true;
    }
    // fall through if MODE_LED_ALL and delete the single leds
    else if (pos == MODE_LED_ANY || pos == MODE_LED_ALL) {
      if (!m_singlePats.size()) {
        return false;
      }
      for (LedPos p = LED_FIRST; p < MODE_LED_COUNT; ++p) {
        if (m_singlePats[p]) {
          m_singlePats[p]->setColorset(set);
        }
      }
      return true;  // actually break here
    }
#endif
  }

  // equivalent to case MODE_LED_ALL_SINGLE
  else if (pos == MODE_LED_ALL_SINGLE) {
    if (!m_singlePats.size()) {
      return false;
    }
    for (LedPos p = LED_FIRST; p < MODE_LED_COUNT; ++p) {
      if (m_singlePats[p]) {
        m_singlePats[p]->setColorset(set);
      }
    }
    return true;
  }

  // equivalent to default case (covers any other pos)
  else {
    if (!m_singlePats.size()) {
      return false;
    }
    if (pos < MODE_LED_COUNT && m_singlePats[pos]) {
      m_singlePats[pos]->setColorset(set);
      return true;
    }
  }

  return false;
}

// set colorset at each position in a map
bool Mode::setColorsetMap(LedMap map, const Colorset &set)
{
  MAP_FOREACH_LED(map) {
    if (!setColorset(set, pos)) {
      return false;
    }
  }
  return true;
}

void Mode::clearPattern(LedPos pos)
{
  setPattern(PATTERN_NONE, pos);
}

void Mode::clearPatternMap(LedMap map)
{
  MAP_FOREACH_LED(map) {
    clearPattern(pos);
  }
}

void Mode::clearColorset(LedPos pos)
{
  Colorset empty;
  setColorset(empty, pos);
}

void Mode::clearColorsetMap(LedMap map)
{
  MAP_FOREACH_LED(map) {
    clearColorset(pos);
  }
}

void Mode::setArg(uint8_t param, uint8_t value, LedMap map)
{
  MAP_FOREACH_LED(map) {
    Pattern *pat = getPattern(pos);
    if (!pat) {
      continue;
    }
    pat->setArg(param, value);
  }
}

uint8_t Mode::getArg(uint8_t index, LedPos pos)
{
  Pattern *pat = getPattern(pos);
  if (!pat) {
    return 0;
  }
  return pat->getArg(index);
}

ModeFlags Mode::getFlags() const
{
  ModeFlags flags = 0;
  if (hasMultiLed()) flags |= MODE_FLAG_MULTI_LED;
  if (hasSingleLed()) flags |= MODE_FLAG_SINGLE_LED;
  if (hasSameSingleLed()) flags |= MODE_FLAG_ALL_SAME_SINGLE;
  if (hasSparseSingleLed()) flags |= MODE_FLAG_SPARSE_SINGLES;
  return flags;
}

// whether a multi-led pattern is present in the mode
bool Mode::hasMultiLed() const
{
#if VORTEX_SLIM == 0
  return (m_multiPat != nullptr);
#else
  return false;
#endif
}

// whether at least one single-led pattern is present in the mode
bool Mode::hasSingleLed() const
{
  for (LedPos i = LED_FIRST; i < MODE_LED_COUNT; ++i) {
    if (m_singlePats[i]) {
      return true;
    }
  }
  return false;
}

// are all the single led patterns and colorsets equal?
bool Mode::hasSameSingleLed() const
{
  Pattern *firstPat = nullptr;
  for (LedPos i = LED_FIRST; i < MODE_LED_COUNT; ++i) {
    if (!m_singlePats[i]) {
      continue;
    }
    if (!firstPat) {
      firstPat = m_singlePats[i];
      continue;
    }
    if (!firstPat->equals(m_singlePats[i])) {
      return false;
    }
  }
  // all the same if at least one pattern was found
  return (firstPat != nullptr);
}

bool Mode::hasSparseSingleLed() const
{
  LedMap map = getSingleLedMap();
  if (map == MAP_MODE_LED_ALL || map == 0) {
    return false;
  }
  // if anything else is set it's sparse
  return true;
}

bool Mode::isEmpty() const
{
#if VORTEX_SLIM == 0
  if (m_multiPat && m_multiPat->getPatternID() != PATTERN_NONE) {
    return false;
  }
#endif
  for (LedPos i = LED_FIRST; i < MODE_LED_COUNT; ++i) {
    // if any patterns are non-null and not PATTERN_NONE
    if (m_singlePats[i] && m_singlePats[i]->getPatternID() != PATTERN_NONE) {
      // then this mode is not empty
      return false;
    }
  }
  return true;
}

LedMap Mode::getSingleLedMap() const
{
  LedMap map = 0;
  for (LedPos pos = LED_FIRST; pos < MODE_LED_COUNT; pos++) {
    if (m_singlePats[pos]) {
      m_engine.leds().ledmapSetLed(map, pos);
    }
  }
  return map;
}

// is this a multi-led pattern in the mode?
bool Mode::isMultiLed() const
{
#if VORTEX_SLIM == 0
  return hasMultiLed() && !hasSingleLed();
#else
  return false;
#endif
}

#if MODES_TEST == 1
#include <stdio.h>
#include <assert.h>

// Mode::Test function
void Mode::test()
{
  INFO_LOG("== Beginning Mode Tests ==\n");

  INFO_LOG("= Testing Mode::setPattern =\n");

  Mode modeTest;

  assert(modeTest.getMultiPatID() == PATTERN_NONE);
  assert(modeTest.getMultiColorset() == nullptr);
  assert(modeTest.getMultiPat() == nullptr);

  // Check if default constructor initializes Mode object correctly
  for (LedPos pos = LED_FIRST; pos < MODE_LED_COUNT; ++pos) {
    assert(modeTest.getPatternIDAt(pos) == PATTERN_NONE);
    assert(modeTest.getColorsetAt(pos) == nullptr);
    assert(modeTest.getPatternAt(pos) == nullptr);
  }

  INFO_LOG("Test Mode default constructor passed.\n");

  // Test setPattern
  PatternID testPatternID = PATTERN_SOLID;
  PatternArgs testPatternArgs = { 100, 255, 0, 10 };
  Colorset testColorset(0, 0, 255);

  modeTest.setPattern(testPatternID, &testPatternArgs, &testColorset);

  // Check if the pattern and colorset were set correctly for all LEDs
  for (LedPos pos = LED_FIRST; pos < MODE_LED_COUNT; ++pos) {
    assert(modeTest.getPatternIDAt(pos) == testPatternID);
    assert(modeTest.getColorsetAt(pos)->equals(&testColorset));
  }
  INFO_LOG("Test setPattern passed.\n");

  // Test setPattern with multi-led pattern
  Mode modeMultiLedTest;
  PatternID multiPatternID = PATTERN_MULTI_FIRST;
  PatternArgs multiPatternArgs = { 200, 200, 200, 5 };
  Colorset multiColorset(255, 0, 0);

  modeMultiLedTest.setPattern(multiPatternID, &multiPatternArgs, &multiColorset);

  // Check if the multi-led pattern and colorset were set correctly
  assert(modeMultiLedTest.getPatternID() == multiPatternID);
  assert(modeMultiLedTest.getColorset()->equals(&multiColorset));
  INFO_LOG("Test setPattern with multi-led pattern passed.\n");

  // Test setPattern with single-led pattern and multi-led pattern simultaneously
  Mode modeBothTest;
  PatternID singlePatternID = PATTERN_SOLID;
  PatternArgs singlePatternArgs = { 100, 255, 0, 10 };
  Colorset singleColorset(0, 0, 255);

  modeBothTest.setPattern(singlePatternID, &singlePatternArgs, &singleColorset);
  modeBothTest.setPattern(multiPatternID, &multiPatternArgs, &multiColorset);

  // Check if the single-led and multi-led patterns coexist
  assert(modeBothTest.hasMultiLed() && modeBothTest.hasSingleLed());
  INFO_LOG("Test setPattern with single-led and multi-led patterns simultaneously passed.\n");

  // Test setPatternAt
  Mode modeSetPatternAtTest;
  LedPos setPatternAtPosition = LED_5;
  modeSetPatternAtTest.setPatternAt(setPatternAtPosition, testPatternID, &testPatternArgs, &testColorset);

  // Check if the pattern and colorset were set correctly at the specified position
  for (LedPos pos = LED_FIRST; pos < MODE_LED_COUNT; ++pos) {
    if (pos == setPatternAtPosition) {
      assert(modeSetPatternAtTest.getPatternIDAt(pos) == testPatternID);
      assert(modeSetPatternAtTest.getColorsetAt(pos)->equals(&testColorset));
    } else {
      assert(modeSetPatternAtTest.getPatternIDAt(pos) == PATTERN_NONE);
    }
  }
  INFO_LOG("Test setPatternAt passed.\n");

  // Test setColorsetAt
  Colorset newColorset(RGB_YELLOW, RGB_ORANGE, RGB_GREEN);
  LedPos setColorsetPosition = LED_3;
  modeTest.setColorsetAt(setColorsetPosition, &newColorset);

  // Check if the colorset was set correctly at the specified position
  for (LedPos pos = LED_FIRST; pos < MODE_LED_COUNT; ++pos) {
    if (pos == setColorsetPosition) {
      assert(modeTest.getColorsetAt(pos)->equals(&newColorset));
    } else {
      assert(modeTest.getColorsetAt(pos)->equals(&testColorset));
    }
  }
  INFO_LOG("Test setColorsetAt passed.\n");

  // Test clearPattern with single-led pattern
  Mode modeClearSingleLedTest;
  modeClearSingleLedTest.setPattern(testPatternID, &testPatternArgs, &testColorset);
  modeClearSingleLedTest.clearPatterns();

  for (LedPos pos = LED_FIRST; pos < MODE_LED_COUNT; ++pos) {
    assert(modeClearSingleLedTest.getPatternIDAt(pos) == PATTERN_NONE);
  }
  INFO_LOG("Test clearPattern with single-led pattern passed.\n");

  // Test clearPattern with multi-led pattern
  Mode modeClearMultiLedTest;
  modeClearMultiLedTest.setPattern(multiPatternID, &multiPatternArgs, &multiColorset);
  modeClearMultiLedTest.clearPatterns();

  assert(modeClearMultiLedTest.getPatternID() == PATTERN_NONE);
  INFO_LOG("Test clearPattern with multi-led pattern passed.\n");

  // Test clearPattern with both single-led and multi-led patterns
  modeBothTest.clearPatterns();

  assert(modeBothTest.getPatternID() == PATTERN_NONE);
  for (LedPos pos = LED_FIRST; pos < MODE_LED_COUNT; ++pos) {
    assert(modeBothTest.getPatternIDAt(pos) == PATTERN_NONE);
  }
  INFO_LOG("Test clearPattern with both single-led and multi-led patterns passed.\n");

  // Test clearColorset
  Mode modeClearColorsetTest;
  modeClearColorsetTest.setPattern(testPatternID, &testPatternArgs, &testColorset);
  modeClearColorsetTest.clearColorsets();

  for (LedPos pos = LED_FIRST; pos < MODE_LED_COUNT; ++pos) {
    assert(modeClearColorsetTest.getColorsetAt(pos)->numColors() == 0);
  }
  INFO_LOG("Test clearColorset passed.\n");

  // Test clearColorsetAt for a specific position
  Mode modeClearColorsetAtTest;
  modeClearColorsetAtTest.setPattern(testPatternID, &testPatternArgs, &testColorset);
  LedPos clearColorsetAtPosition = LED_3;
  modeClearColorsetAtTest.clearColorsetAt(clearColorsetAtPosition);

  for (LedPos pos = LED_FIRST; pos < MODE_LED_COUNT; ++pos) {
    if (pos == clearColorsetAtPosition) {
      assert(modeClearColorsetAtTest.getColorsetAt(pos)->numColors() == 0);
    } else {
      assert(modeClearColorsetAtTest.getColorsetAt(pos)->numColors() == testColorset.numColors());
    }
  }
  INFO_LOG("Test clearColorsetAt passed.\n");

  // Test clearColorsets with multi-led pattern
  Mode modeClearMultiColorsetsTest;
  modeClearMultiColorsetsTest.setPattern(multiPatternID, &multiPatternArgs, &multiColorset);
  modeClearMultiColorsetsTest.clearColorsets();

  assert(modeClearMultiColorsetsTest.getColorset()->numColors() == 0);
  INFO_LOG("Test clearColorsets with multi-led pattern passed.\n");

  // Test clearColorsets with both single-led and multi-led patterns
  modeBothTest.setPattern(PATTERN_BASIC);
  modeBothTest.clearColorsets();

  assert(modeBothTest.getColorset()->numColors() == 0);
  for (LedPos pos = LED_FIRST; pos < MODE_LED_COUNT; ++pos) {
    assert(modeBothTest.getColorsetAt(pos)->numColors() == 0);
  }
  INFO_LOG("Test clearColorsets with both single-led and multi-led patterns passed.\n");

  // Test hasSparseSingleLed with sparse single-led patterns
  Mode modeSparseTest;
  modeSparseTest.setPatternAt(LED_3, testPatternID, &testPatternArgs, &testColorset);
  modeSparseTest.setPatternAt(LED_5, testPatternID, &testPatternArgs, &testColorset);
  modeSparseTest.setPatternAt(LED_7, testPatternID, &testPatternArgs, &testColorset);

  assert(modeSparseTest.hasSparseSingleLed());
  INFO_LOG("Test hasSparseSingleLed with sparse single-led patterns passed.\n");

  // Test getSingleLedMap with no single-led patterns
  Mode modeNoSingleLedTest;
  assert(modeNoSingleLedTest.getSingleLedMap() == 0);
  INFO_LOG("Test getSingleLedMap with no single-led patterns passed.\n");

  // Test getSingleLedMap with all single-led patterns
  Mode modeAllSingleLedTest;
  modeAllSingleLedTest.setPattern(testPatternID, &testPatternArgs, &testColorset);
  assert(modeAllSingleLedTest.getSingleLedMap() == MAP_MODE_LED_ALL);
  INFO_LOG("Test getSingleLedMap with all single-led patterns passed.\n");

  // Test getSingleLedMap with sparse single-led patterns
  assert(modeSparseTest.getSingleLedMap() == (1 << LED_3 | 1 << LED_5 | 1 << LED_7));
  INFO_LOG("Test getSingleLedMap with sparse single-led patterns passed.\n");

  // Test isMultiLed with only multi-led pattern
  Mode modeOnlyMultiTest;
  modeOnlyMultiTest.setPattern(multiPatternID, &multiPatternArgs, &multiColorset);
  assert(modeOnlyMultiTest.isMultiLed());
  INFO_LOG("Test isMultiLed with only multi-led pattern passed.\n");

  // Test isMultiLed with both single-led and multi-led patterns
  assert(!modeBothTest.isMultiLed());
  INFO_LOG("Test isMultiLed with both single-led and multi-led patterns passed.\n");

  // Test setPatternAt for specific position and clearPatternAt for a specific position
  Mode modeSetClearAtTest;
  modeSetClearAtTest.setPatternAt(LED_5, testPatternID, &testPatternArgs, &testColorset);
  modeSetClearAtTest.clearPatternAt(LED_5);

  assert(modeSetClearAtTest.getPatternIDAt(LED_5) == PATTERN_NONE);
  assert(modeSetClearAtTest.getColorsetAt(LED_5) == nullptr);
  INFO_LOG("Test setPatternAt and clearPatternAt for specific position passed.\n");

  // Test setColorsetAt for specific position and clearColorsetAt for a specific position
  Mode modeSetClearColorsetAtTest;
  modeSetClearColorsetAtTest.setPatternAt(LED_5, testPatternID, &testPatternArgs, &testColorset);
  modeSetClearColorsetAtTest.clearColorsetAt(LED_5);

  assert(modeSetClearColorsetAtTest.getColorsetAt(LED_5)->numColors() == 0);
  INFO_LOG("Test setColorsetAt and clearColorsetAt for specific position passed.\n");

  // Test clearMultiColorset with multi-led pattern
  Mode modeMultiClearTest;
  modeMultiClearTest.setPattern(multiPatternID, &multiPatternArgs, &multiColorset);
  modeMultiClearTest.clearMultiColorset();

  assert(modeMultiClearTest.getPatternID() == multiPatternID);
  assert(modeMultiClearTest.getColorset()->numColors() == 0);
  INFO_LOG("Test clearMultiColorset with multi-led pattern passed.\n");

  PatternID invalidID = (PatternID)(PATTERN_COUNT + 1);

  // Test setPattern with an invalid pattern ID
  Mode modeInvalidPatternIDTest;
  bool success = modeInvalidPatternIDTest.setPattern(invalidID, &testPatternArgs, &testColorset);
  assert(!success);
  INFO_LOG("Test setPattern with invalid pattern ID passed.\n");

  // Test setPatternAt with an invalid pattern ID
  Mode modeInvalidPatternIDAtTest;
  success = modeInvalidPatternIDAtTest.setPatternAt(LED_3, invalidID, &testPatternArgs, &testColorset);
  assert(!success);
  INFO_LOG("Test setPatternAt with invalid pattern ID passed.\n");

  // Test setMultiPat with an invalid pattern ID
  Mode modeInvalidMultiPatternIDTest;
  success = modeInvalidMultiPatternIDTest.setMultiPat(PATTERN_SINGLE_LAST, &testPatternArgs, &testColorset);
  assert(!success);
  INFO_LOG("Test setMultiPat with invalid pattern ID passed.\n");

  LedPos invalidPos = (LedPos)(MODE_LED_COUNT + 1);

  // Test setPatternAt
  Mode modeInvalidLEDPositionTest;
  success = modeInvalidLEDPositionTest.setPatternAt(invalidPos, testPatternID, &testPatternArgs, &testColorset);
  assert(success);
  INFO_LOG("Test setPattern with invalid LED position passed.\n");

  // Test setColorsetAt with an invalid LED position
  Mode modeInvalidLEDPositionSetColorsetTest;
  success = modeInvalidLEDPositionSetColorsetTest.setColorsetAt(invalidPos, &testColorset);
  assert(success);
  INFO_LOG("Test setColorsetAt with invalid LED position passed.\n");

  // Test clearPatternAt with an invalid LED position
  Mode modeInvalidLEDPositionClearPatternTest;
  modeInvalidLEDPositionClearPatternTest.setPattern(PATTERN_BLENDSTROBE);
  modeInvalidLEDPositionClearPatternTest.clearPatternAt(invalidPos);
  for (LedPos pos = LED_FIRST; pos < MODE_LED_COUNT; ++pos) {
    assert(modeInvalidLEDPositionClearPatternTest.getPatternAt(pos) != nullptr);
  }
  INFO_LOG("Test clearPatternAt with invalid LED position passed.\n");

  // Test clearColorsetAt with an invalid LED position
  Mode modeInvalidLEDPositionClearColorsetTest;
  modeInvalidLEDPositionClearColorsetTest.setPattern(PATTERN_BLEND);
  modeInvalidLEDPositionClearColorsetTest.clearColorsetAt(invalidPos);
  for (LedPos pos = LED_FIRST; pos < MODE_LED_COUNT; ++pos) {
    assert(modeInvalidLEDPositionClearColorsetTest.getColorsetAt(pos) != nullptr);
  }
  INFO_LOG("Test clearColorsetAt with invalid LED position passed.\n");

  // Test setPatternAt with PATTERN_NONE
  Mode modePatternNoneTest;
  modePatternNoneTest.setPatternAt(LED_2, PATTERN_NONE, &testPatternArgs, &testColorset);
  assert(modePatternNoneTest.getPatternIDAt(LED_2) == PATTERN_NONE);
  INFO_LOG("Test setPatternAt with PATTERN_NONE passed.\n");

  // Test setPattern with PATTERN_NONE
  Mode modeSetPatternNoneTest;
  modeSetPatternNoneTest.setPattern(PATTERN_NONE, &testPatternArgs, &testColorset);
  for (LedPos pos = LED_FIRST; pos < MODE_LED_COUNT; ++pos) {
    assert(modeSetPatternNoneTest.getPatternIDAt(pos) == PATTERN_NONE);
  }
  INFO_LOG("Test setPattern with PATTERN_NONE passed.\n");

  // Test setPattern with PATTERN_MULTI_LAST
  Mode modeSetPatternMultiLastTest;
  modeSetPatternMultiLastTest.setPattern(PATTERN_MULTI_LAST, &testPatternArgs, &testColorset);
  assert(modeSetPatternMultiLastTest.getPatternID() == PATTERN_MULTI_LAST);
  assert(modeSetPatternMultiLastTest.getMultiPatID() == PATTERN_MULTI_LAST);
  for (LedPos pos = LED_FIRST; pos < MODE_LED_COUNT; ++pos) {
    assert(modeSetPatternNoneTest.getPatternIDAt(pos) == PATTERN_NONE);
  }
  INFO_LOG("Test setPattern with PATTERN_MULTI_LAST passed.\n");

  // Test setMultiPat with PATTERN_MULTI_FIRST
  Mode modeSetMultiPatMultiFirstTest;
  modeSetMultiPatMultiFirstTest.setMultiPat(PATTERN_MULTI_FIRST, &testPatternArgs, &testColorset);
  assert(modeSetMultiPatMultiFirstTest.getPatternID() == PATTERN_MULTI_FIRST);
  INFO_LOG("Test setMultiPat with PATTERN_MULTI_FIRST passed.\n");

  // Test setMultiPat with PATTERN_MULTI_LAST
  Mode modeSetMultiPatMultiLastTest;
  modeSetMultiPatMultiLastTest.setMultiPat(PATTERN_MULTI_LAST, &testPatternArgs, &testColorset);
  assert(modeSetMultiPatMultiLastTest.getPatternID() == PATTERN_MULTI_LAST);
  INFO_LOG("Test setMultiPat with PATTERN_MULTI_LAST passed.\n");

  // Test setMultiPat with PATTERN_SINGLE_FIRST
  Mode modeSetMultiPatSingleFirstTest;
  bool setMultiPatSingleFirstSuccess = modeSetMultiPatSingleFirstTest.setMultiPat(PATTERN_SINGLE_FIRST, &testPatternArgs, &testColorset);
  assert(!setMultiPatSingleFirstSuccess);
  INFO_LOG("Test setMultiPat with PATTERN_SINGLE_FIRST passed.\n");

  // Test setMultiPat with PATTERN_SINGLE_LAST
  Mode modeSetMultiPatSingleLastTest;
  bool setMultiPatSingleLastSuccess = modeSetMultiPatSingleLastTest.setMultiPat(PATTERN_SINGLE_LAST, &testPatternArgs, &testColorset);
  assert(!setMultiPatSingleLastSuccess);
  INFO_LOG("Test setMultiPat with PATTERN_SINGLE_LAST passed.\n");
}

#endif
