#include "Pattern.h"

#include "../VortexEngine.h"

#include "../Patterns/PatternBuilder.h"
#include "../Serial/ByteStream.h"
#include "../Time/TimeControl.h"
#include "../Colors/Colorset.h"
#include "../Memory/Memory.h"
#include "../Log/Log.h"

#include "../VortexConfig.h"

Pattern::Pattern(VortexEngine &engine) :
  m_engine(engine),
  m_patternID(PATTERN_FIRST),
  m_patternFlags(0),
  m_colorset(),
  m_ledPos(LED_FIRST),
  m_numArgs(0),
  m_argList()
#ifdef VORTEX_LIB
  ,m_argNameList()
#endif
{
}

Pattern::Pattern(VortexEngine &engine, const PatternArgs &args) :
  Pattern(engine)
{
}

Pattern::~Pattern()
{
}

void Pattern::bind(LedPos pos)
{
  if (pos > LED_COUNT) {
    return;
  }
  m_ledPos = pos;
}

void Pattern::init()
{
  m_colorset.resetIndex();
}

#ifdef VORTEX_LIB
void Pattern::skip(uint32_t ticks)
{
  m_engine.time().startSimulation();
  for (uint32_t i = 0; i < ticks; ++i) {
    play();  // simulate playing the pattern
    m_engine.time().tickSimulation();
  }
  m_engine.time().endSimulation();
}
#endif

// must override the serialize routine to save the pattern
bool Pattern::serialize(ByteStream &buffer) const
{
  if (!buffer.serialize8((uint8_t)m_patternID)) {
    return false;
  }
  if (!m_colorset.serialize(buffer)) {
    return false;
  }
  PatternArgs args;
  getArgs(args);
  PatternArgs defaults = m_engine.patternBuilder().getDefaultArgs(m_patternID);
  // generate a bitmap of which args are defaulted
  uint8_t argmap = ARG_NONE;
  for (uint32_t i = 0; i < MAX_ARGS; ++i) {
    if (args.args[i] != defaults.args[i]) {
      ARGMAP_SET(argmap, i);
    }
  }
  if (!args.serialize(buffer, argmap)) {
    return false;
  }
  return true;
}

// must override unserialize to load patterns
bool Pattern::unserialize(ByteStream &buffer)
{
  // don't unserialize the pattern ID because it is already
  // unserialized by the pattern builder to decide which pattern
  // to instantiate, instead only unserialize the colorset
  if (!m_colorset.unserialize(buffer)) {
    return false;
  }
  // start with the default args for this pattern
  PatternArgs args = m_engine.patternBuilder().getDefaultArgs(m_patternID);
  // then unserialize any different args overtop of the defaults
  args.unserialize(buffer);
  setArgs(args);
  return true;
}

void Pattern::setArg(uint8_t index, uint8_t value)
{
  if (index >= m_numArgs) {
    return;
  }
  *((uint8_t *)this + m_argList[index]) = value;
}

uint8_t Pattern::getArg(uint8_t index) const
{
  if (index >= m_numArgs) {
    return 0;
  }
  return *((uint8_t *)this + m_argList[index]);
}

uint8_t &Pattern::argRef(uint8_t index)
{
  if (index >= m_numArgs) {
    index = 0;
  }
  return *((uint8_t *)this + m_argList[index]);
}

void Pattern::setArgs(const PatternArgs &args)
{
  for (uint32_t i = 0; i < m_numArgs; ++i) {
    *((uint8_t *)this + m_argList[i]) = args.args[i];
  }
}

void Pattern::getArgs(PatternArgs &args) const
{
  args.init();
  for (uint32_t i = 0; i < m_numArgs; ++i) {
    args.addArgs(*((uint8_t *)this + m_argList[i]));
  }
}

bool Pattern::equals(const Pattern *other)
{
  if (!other) {
    return false;
  }
  // compare pattern id
  if (m_patternID != other->m_patternID) {
    return false;
  }
  // then colorset
  if (!m_colorset.equals(&other->m_colorset)) {
    return false;
  }
  // number of args
  if (m_numArgs != other->m_numArgs) {
    return false;
  }
  // compare each arg
  for (uint8_t i = 0; i < m_numArgs; ++i) {
    if (getArg(i) != other->getArg(i)) {
      return false;
    }
  }
  return true;
}

// change the colorset
void Pattern::setColorset(const Colorset &set)
{
  m_colorset = set;
}

void Pattern::clearColorset()
{
  m_colorset.clear();
}

#ifdef VORTEX_LIB
void Pattern::registerArg(const char *name, arg_offset_t argOffset)
#else
void Pattern::registerArg(arg_offset_t argOffset)
#endif
{
  if (m_numArgs >= MAX_PATTERN_ARGS) {
    ERROR_LOG("too many args");
    return;
  }
#ifdef VORTEX_LIB
  m_argNameList[m_numArgs] = name;
#endif
  m_argList[m_numArgs++] = argOffset;
}
