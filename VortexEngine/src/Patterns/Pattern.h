#ifndef PATTERN_H
#define PATTERN_H

#include "../Leds/LedTypes.h"
#include "../Colors/Colorset.h"

#include "Patterns.h"
#include "PatternArgs.h"

#define MAX_PATTERN_ARGS 8

// The heirarchy of pattern currently looks like this:
/*
 *                                pattern*
 *                              /        \
 *                single led pat*           multi led pat*
 *                /           \                /
 *           basic pat       tracer pat       hybrid pat*
 *           /                               /           \
 *      Advanced pattern                 Rabbit Pattern   sequenced pattern
 *
 *     * = abstract class that cannot be instantiated
 */

#define PATTERN_FLAGS_NONE  0

// the pattern is a multi-pattern
#define PATTERN_FLAG_MULTI  (1<<0)

// and arg_offset_t is the distance from the base of the object
// to the desired argument, in slim vortex we only need a single byte
// but in multi-led patterns they can be a bit further in
#if VORTEX_SLIM == 1
typedef uint8_t arg_offset_t;
#else
typedef uint16_t arg_offset_t;
#endif

// macro to register args of a pattern
#ifdef VORTEX_LIB
#define REGISTER_ARG(arg) \
  registerArg(#arg, (arg_offset_t)(((uintptr_t)&arg - (uintptr_t)this)));
#else
#define REGISTER_ARG(arg) \
  registerArg((arg_offset_t)(((uintptr_t)&arg - (uintptr_t)this)));
#endif

class VortexEngine;
class ByteStream;

class Pattern
{
  // PatternBuilder can access the Pattern internals
  friend class PatternBuilder;

protected:
  // Pattern is an abstract class
  Pattern(VortexEngine &engine);
  Pattern(VortexEngine &engine, const PatternArgs &args);

public:
  virtual ~Pattern();

  // bind a colorset and position to the pattern
  virtual void bind(LedPos pos);

  // init the pattern to initial state
  virtual void init();

  // pure virtual must override the play function
  virtual void play() = 0;

#ifdef VORTEX_LIB
  // skip the pattern ahead some ticks
  void skip(uint32_t ticks);
#endif

  // serialize and unserialize a pattern to a bytestream
  bool serialize(ByteStream &buffer) const;
  bool unserialize(ByteStream &buffer);

  // get or set a single arg
  void setArg(uint8_t index, uint8_t value);
  uint8_t getArg(uint8_t index) const;

  uint8_t &argRef(uint8_t index);

#ifdef VORTEX_LIB
  // get the name of an arg
  const char *getArgName(uint8_t index) const { return index >= m_numArgs ? "" : m_argNameList[index]; }
#endif

  // get or set the entire list of pattern args
  void setArgs(const PatternArgs &args);
  void getArgs(PatternArgs &args) const;

  // number of args the pattern has
  uint8_t getNumArgs() const { return m_numArgs; }

  // comparison to other pattern
  // NOTE: That may cause problems because the parameter is still a Pattern *
  //       which means comparison would need to cast the other upwards first
  // NOTE2: Removing virtual because this probably shouldn't be overridden
  bool equals(const Pattern *other);

  // change the colorset
  const Colorset getColorset() const { return m_colorset; }
  Colorset getColorset() { return m_colorset; }
  void setColorset(const Colorset &set);
  void clearColorset();

  // change the led position
  void setLedPos(LedPos pos) { m_ledPos = pos; }

  // get/set the ID of the pattern (set by mode builder)
  PatternID getPatternID() const { return m_patternID; }

  // get a pointer to the colorset that is bound to the pattern
  LedPos getLedPos() const { return m_ledPos; }

  // get the pattern flags
  uint32_t getFlags() const { return m_patternFlags; }
  bool hasFlags(uint32_t flags) const { return (m_patternFlags & flags) != 0; }

protected:
  // reference to engine
  VortexEngine &m_engine;
  // the ID of this pattern (set by pattern builder)
  PatternID m_patternID;
  // any flags the pattern has
  uint8_t m_patternFlags;
  // a copy of the colorset that this pattern is initialized with
  Colorset m_colorset;
  // the Led the pattern is running on
  LedPos m_ledPos;

  uint8_t m_numArgs;
  arg_offset_t m_argList[MAX_PATTERN_ARGS];

#ifdef VORTEX_LIB
  void registerArg(const char *name, arg_offset_t argOffset);
  const char *m_argNameList[MAX_PATTERN_ARGS];
#else
  void registerArg(arg_offset_t argOffset);
#endif
};

#endif
