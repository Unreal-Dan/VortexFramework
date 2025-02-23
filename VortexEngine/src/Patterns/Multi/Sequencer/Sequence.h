#ifndef SEQUENCER_H
#define SEQUENCER_H

#include "../../Patterns.h"
#include "../../../Leds/LedTypes.h"
#include "../../../Colors/Colorset.h"
#include "../../../Time/Timer.h"

#include <vector>

class SequencedPattern;
class SingleLedPattern;
class ByteStream;

// a map of leds to pattern ids
class PatternMap
{
public:
  PatternMap(VortexEngine &engine);
  PatternMap(VortexEngine &engine, PatternID pattern, LedMap positions);
  PatternMap(VortexEngine &engine, const PatternMap &other);

  void operator=(const PatternMap &other);
  bool operator==(const PatternMap &other) const;
  bool operator!=(const PatternMap &other) const;

  // set a pattern at each position in the LedMap
  void setPatternAt(PatternID pattern, LedMap positions);
  PatternID operator[](LedPos index) const;

  // serialize and unserialize a pattern map
  bool serialize(ByteStream &buffer) const;
  bool unserialize(ByteStream &buffer);

  // engine reference
  VortexEngine &m_engine;

  // public list of pattern IDs for each led
  std::vector<PatternID> m_patterns;
};

// A map of leds to colorsets
class ColorsetMap
{
public:
  ColorsetMap(VortexEngine &engine);
  ColorsetMap(VortexEngine &engine, const Colorset &colorset, LedMap positions);
  ColorsetMap(VortexEngine &engine, const ColorsetMap &other);

  void operator=(const ColorsetMap &other);
  bool operator==(const ColorsetMap &other) const;
  bool operator!=(const ColorsetMap &other) const;

  // set a pattern at each position in the LedMap
  void setColorsetAt(const Colorset &colorset, LedMap positions);
  const Colorset &operator[](LedPos index) const;

  // serialize and unserialize a colorset map
  bool serialize(ByteStream &buffer) const;
  bool unserialize(ByteStream &buffer);

  // engine reference
  VortexEngine &m_engine;

  // public list of pattern IDs for each led
  std::vector<Colorset> m_colorsets;
};

// A single step in a sequence
class SequenceStep
{
public:
  SequenceStep(VortexEngine &engine);
  SequenceStep(VortexEngine &engine, uint16_t duration, const PatternMap &patternMap, const ColorsetMap &colorsetMap);
  SequenceStep(VortexEngine &engine, const SequenceStep &other);

  void operator=(const SequenceStep &other);
  bool operator==(const SequenceStep &other) const;
  bool operator!=(const SequenceStep &other) const;

  // serialize and unserialize a step in the sequencer
  bool serialize(ByteStream &buffer) const;
  bool unserialize(ByteStream &buffer);

  // engine reference
  VortexEngine &m_engine;

  // public members to allow for easy initialization of an array of SequenceSteps
  uint16_t m_duration;
  PatternMap m_patternMap;
  ColorsetMap m_colorsetMap;
};

// A sequence which can contain multiple SequenceSteps
class Sequence
{
public:
  Sequence(VortexEngine &engine);
  ~Sequence();

  Sequence(const Sequence &other);
  Sequence(Sequence &&other) noexcept;

  void operator=(const Sequence &other);
  bool operator==(const Sequence &other) const;
  bool operator!=(const Sequence &other) const;

  void initSteps(uint8_t numSteps);
  uint8_t addStep(const SequenceStep &step);
  uint8_t addStep(uint16_t duration, const PatternMap &patternMap, const ColorsetMap &colorsetMap);
  void clear();

  bool serialize(ByteStream &buffer) const;
  bool unserialize(ByteStream &buffer);

  uint8_t numSteps() const;
  const SequenceStep &operator[](uint8_t index) const;

private:
  // engine reference
  VortexEngine &m_engine;

  std::vector<SequenceStep> m_sequenceSteps;
};

#endif
