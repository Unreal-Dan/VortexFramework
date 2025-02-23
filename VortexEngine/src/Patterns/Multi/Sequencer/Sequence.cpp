#include "Sequence.h"

#include "../../../VortexEngine.h"

#include "../../../Serial/ByteStream.h"
#include "../../../Memory/Memory.h"
#include "../../../Leds/Leds.h"
#include "../../../Log/Log.h"

#include <string.h>

// some arbitrary number
#define MAX_SEQUENCE_STEPS 64

PatternMap::PatternMap(VortexEngine &engine) :
  m_engine(engine),
  m_patterns()
{
  m_patterns.resize(LED_COUNT);
  for (LedPos i = LED_FIRST; i < LED_COUNT; ++i) {
    m_patterns[i] = PATTERN_NONE;
  }
}

PatternMap::PatternMap(VortexEngine &engine, PatternID pattern, LedMap positions) :
  PatternMap(engine)
{
  setPatternAt(pattern, positions);
}

PatternMap::PatternMap(VortexEngine &engine, const PatternMap &other) :
  PatternMap(engine)
{
  *this = other;
}

void PatternMap::operator=(const PatternMap &other)
{
  m_patterns = other.m_patterns;
}

bool PatternMap::operator==(const PatternMap &other) const
{
  return m_patterns == other.m_patterns;
}

bool PatternMap::operator!=(const PatternMap &other) const
{
  return !(*this == other);
}

// set a pattern at each position in the LedMap
void PatternMap::setPatternAt(PatternID pattern, LedMap positions)
{
  for (LedPos pos = LED_FIRST; pos < LED_COUNT; ++pos) {
    if (m_engine.leds().ledmapCheckLed(positions, pos)) {
      m_patterns[pos] = pattern;
    }
  }
}

PatternID PatternMap::operator[](LedPos index) const
{
  return m_patterns[index];
}

bool PatternMap::serialize(ByteStream &buffer) const
{
  for (LedPos i = LED_FIRST; i < LED_COUNT; ++i) {
    // ensure the PatternID is interpreted as uint8_t
    if (!buffer.serialize8((uint8_t)m_patterns[i])) {
      return false;
    }
  }
  return true;
}

bool PatternMap::unserialize(ByteStream &buffer)
{
  for (LedPos i = LED_FIRST; i < LED_COUNT; ++i) {
    if (!buffer.unserialize8((uint8_t *)m_patterns.data() + i)) {
      return false;
    }
  }
  return true;
}

ColorsetMap::ColorsetMap(VortexEngine &engine) :
  m_engine(engine),
  m_colorsets()
{
}

ColorsetMap::ColorsetMap(VortexEngine &engine, const Colorset &colorset, LedMap positions) :
  ColorsetMap(engine)
{
  setColorsetAt(colorset, positions);
}

ColorsetMap::ColorsetMap(VortexEngine &engine, const ColorsetMap &other) :
  ColorsetMap(engine)
{
  *this = other;
}

void ColorsetMap::operator=(const ColorsetMap &other)
{
  m_colorsets = other.m_colorsets;
}

bool ColorsetMap::operator==(const ColorsetMap &other) const
{
  return m_colorsets == other.m_colorsets;
}

bool ColorsetMap::operator!=(const ColorsetMap &other) const
{
  return !(*this == other);
}

// set a pattern at each position in the LedMap
void ColorsetMap::setColorsetAt(const Colorset &colorset, LedMap positions)
{
  for (LedPos pos = LED_FIRST; pos < LED_COUNT; ++pos) {
    if (m_engine.leds().ledmapCheckLed(positions, pos)) {
      m_colorsets[pos] = colorset;
    }
  }
}
const Colorset &ColorsetMap::operator[](LedPos index) const
{
  return m_colorsets[index];
}

bool ColorsetMap::serialize(ByteStream &buffer) const
{
  for (LedPos i = LED_FIRST; i < LED_COUNT; ++i) {
    if (!m_colorsets[i].serialize(buffer)) {
      return false;
    }
  }
  return true;
}

bool ColorsetMap::unserialize(ByteStream &buffer)
{
  for (LedPos i = LED_FIRST; i < LED_COUNT; ++i) {
    if (!m_colorsets[i].unserialize(buffer)) {
      return false;
    }
  }
  return true;
}

// Make an array of sequence steps to create a sequenced pattern
SequenceStep::SequenceStep(VortexEngine &engine) :
  m_engine(engine), m_duration(0), m_patternMap(engine), m_colorsetMap(engine)
{
}

SequenceStep::SequenceStep(VortexEngine &engine, uint16_t duration, const PatternMap &patternMap, const ColorsetMap &colorsetMap) :
  m_engine(engine), m_duration(duration), m_patternMap(engine, patternMap), m_colorsetMap(engine, colorsetMap)
{
}

SequenceStep::SequenceStep(VortexEngine &engine, const SequenceStep &other) :
  SequenceStep(engine)
{
  *this = other;
}

void SequenceStep::operator=(const SequenceStep &other)
{
  this->m_duration = other.m_duration;
  this->m_patternMap = other.m_patternMap;
  this->m_colorsetMap = other.m_colorsetMap;
}

bool SequenceStep::operator==(const SequenceStep &other) const
{
  return (this->m_duration == other.m_duration &&
          this->m_patternMap == other.m_patternMap &&
          this->m_colorsetMap == other.m_colorsetMap);
}

bool SequenceStep::operator!=(const SequenceStep &other) const
{
  return !(*this == other);
}

bool SequenceStep::serialize(ByteStream &buffer) const
{
  if (!buffer.serialize16(m_duration)) {
    return false;
  }
  if (!m_patternMap.serialize(buffer)) {
    return false;
  }
  if (!m_colorsetMap.serialize(buffer)) {
    return false;
  }
  return true;
}

bool SequenceStep::unserialize(ByteStream &buffer)
{
  if (!buffer.unserialize16(&m_duration)) {
    return false;
  }
  if (!m_patternMap.unserialize(buffer)) {
    return false;
  }
  if (!m_colorsetMap.unserialize(buffer)) {
    return false;
  }
  return true;
}

Sequence::Sequence(VortexEngine &engine) :
  m_engine(engine),
  m_sequenceSteps()
{
}

Sequence::~Sequence()
{
  clear();
}

Sequence::Sequence(const Sequence &other) :
  Sequence(other.m_engine)
{
  // invoke = operator
  *this = other;
}

Sequence::Sequence(Sequence &&other) noexcept :
  m_engine(other.m_engine),
  m_sequenceSteps(other.m_sequenceSteps)
{
  other.m_sequenceSteps.clear();
}

void Sequence::operator=(const Sequence &other)
{
  m_sequenceSteps = other.m_sequenceSteps;
}

bool Sequence::operator==(const Sequence &other) const
{
  return m_sequenceSteps == other.m_sequenceSteps;
}

bool Sequence::operator!=(const Sequence &other) const
{
  return !operator==(other);
}

void Sequence::initSteps(uint8_t numSteps)
{
  m_sequenceSteps.clear();
  m_sequenceSteps.reserve(numSteps);
}

uint8_t Sequence::addStep(const SequenceStep &step)
{
  if (m_sequenceSteps.size() >= MAX_SEQUENCE_STEPS) {
    return false;
  }
  m_sequenceSteps.push_back(step);
  return true;
}

uint8_t Sequence::addStep(uint16_t duration, const PatternMap &patternMap, const ColorsetMap &colorsetMap)
{
  return addStep(SequenceStep(m_engine, duration, patternMap, colorsetMap));
}

void Sequence::clear()
{
  m_sequenceSteps.clear();
}

bool Sequence::serialize(ByteStream &buffer) const
{
  uint8_t numSteps = (uint8_t)m_sequenceSteps.size();
  if (!buffer.serialize8(numSteps)) {
    return false;
  }
  for (uint8_t i = 0; i < numSteps; ++i) {
    if (!m_sequenceSteps[i].serialize(buffer)) {
      return false;
    }
  }
  return true;
}

bool Sequence::unserialize(ByteStream &buffer)
{
  uint8_t numSteps = 0;
  if (!buffer.unserialize8(&numSteps)) {
    return false;
  }
  m_sequenceSteps.clear();
  m_sequenceSteps.reserve(numSteps);
  for (uint8_t i = 0; i < m_sequenceSteps.size(); ++i) {
    if (!m_sequenceSteps[i].unserialize(buffer)) {
      return false;
    }
  }
  return true;
}

uint8_t Sequence::numSteps() const
{
  return (uint8_t)m_sequenceSteps.size();
}

const SequenceStep &Sequence::operator[](uint8_t index) const
{
  return m_sequenceSteps[index];
}
