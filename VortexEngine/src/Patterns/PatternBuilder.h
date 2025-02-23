#ifndef PATTERN_BUILDER_H
#define PATTERN_BUILDER_H

#include "Patterns.h"

class Pattern;
class PatternArgs;
class ByteStream;
class MultiLedPattern;
class SingleLedPattern;
class VortexEngine;

class PatternBuilder
{
public:
  PatternBuilder(VortexEngine &engine);
  ~PatternBuilder();

  bool init();
  void cleanup();

  // generic make any pattern
  Pattern *make(PatternID id, const PatternArgs *args = nullptr);

  // make a copy of an existing pattern
  Pattern *dupe(const Pattern *pat);

  // generate a single LED pattern (nullptr if patternid is not single LED)
  SingleLedPattern *makeSingle(PatternID id, const PatternArgs *args = nullptr);

  // generate a multi LED pattern (nullptr if patternid is not multi LED)
  MultiLedPattern *makeMulti(PatternID id, const PatternArgs *args = nullptr);

  // unserialize a buffer into a pattern
  Pattern *unserialize(ByteStream &buffer);

  // This is just the arguments used to construct any given pattern id
  // the numArgs will be incorrect, do not use this to compare!
  PatternArgs getDefaultArgs(PatternID id);
  // this will give you actual amount of default args
  uint8_t numDefaultArgs(PatternID id);

  // fast way to check if some pattern args match the default for an ID
  bool isDefaultArgs(PatternID id, const PatternArgs &args);

private:
  // helper routines
  Pattern *makeInternal(PatternID id, const PatternArgs *args = nullptr);
  Pattern *generate(PatternID id, const PatternArgs *args = nullptr);

  // reference to engine
  VortexEngine &m_engine;
};

#endif
