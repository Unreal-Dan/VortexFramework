#ifndef PATTERN_ARGS_H
#define PATTERN_ARGS_H

#include <inttypes.h>

// maximum number of args, this isn't really configurable so I'm
// not putting it in VortexConfig.h
#define MAX_ARGS 8

class ByteStream;

// a bit map of arguments, this is used to indicate which arguments
// are present in a pattern when saved to storage
typedef uint8_t ArgMap;

// some macros to access each arg in the argmap
#define ARG_NONE  0
#define ARG(x)    (1 << x)
#define ARG1      (1 << 0)
#define ARG2      (1 << 1)
#define ARG3      (1 << 2)
#define ARG4      (1 << 3)
#define ARG5      (1 << 4)
#define ARG6      (1 << 5)
#define ARG7      (1 << 6)
#define ARG8      (1 << 7)
#define ARG_ALL   0xFF

// some helpers to set/clear/check
#define ARGMAP_SET(map, arg)    (map |= ARG(arg))
#define ARGMAP_CLEAR(map, arg)  (map &= ~ARG(arg))
#define ARGMAP_ISSET(map, arg)  ((map & ARG(arg)) != 0)

// a structured method for passing arguments to patterns, there's definitely more dynamic
// ways to approach this but I'm aiming for simple and lightweight here
class PatternArgs
{
public:
  // constructor for each number of arguments, automatically fills the 'numArgs' field accordingly
  PatternArgs();
  PatternArgs(uint8_t a1);
  PatternArgs(uint8_t a1, uint8_t a2);
  PatternArgs(uint8_t a1, uint8_t a2, uint8_t a3);
  PatternArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4);
  PatternArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5);
  PatternArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6);
  PatternArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7);
  PatternArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8);

  void init();
  void init(uint8_t a1);
  void init(uint8_t a1, uint8_t a2);
  void init(uint8_t a1, uint8_t a2, uint8_t a3);
  void init(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4);
  void init(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5);
  void init(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6);
  void init(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7);
  void init(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8);

  void addArgs(uint8_t a1);
  void addArgs(uint8_t a1, uint8_t a2);
  void addArgs(uint8_t a1, uint8_t a2, uint8_t a3);
  void addArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4);
  void addArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5);
  void addArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6);
  void addArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7);
  void addArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8);

  bool operator!=(const PatternArgs &rhs) const;
  bool operator==(const PatternArgs &rhs) const;
  uint8_t operator[](int index) const;

  // serialize the pattern args with a specific mapping of which args to store
  bool serialize(ByteStream &buffer, ArgMap argmap = ARG_ALL) const;
  // unserialize the pattern args and return the argmap of which args were loaded
  // NOTE: You should start with an instance of the default args before you unserialize
  ArgMap unserialize(ByteStream &buffer);

  // public access to args
  union {
    uint8_t args[8];
    struct {
      uint8_t arg1;
      uint8_t arg2;
      uint8_t arg3;
      uint8_t arg4;
      uint8_t arg5;
      uint8_t arg6;
      uint8_t arg7;
      uint8_t arg8;
    };
    // arg access for basic pattern
    struct {
      uint8_t onDuration;
      uint8_t offDuration;
      uint8_t gapDuration;
      uint8_t dashDuration;
      uint8_t groupSize;
    } basic;
    // arg access for blend
    struct {
      uint8_t onDuration;
      uint8_t offDuration;
      uint8_t gapDuration;
      uint8_t dashDuration;
      uint8_t groupSize;
      uint8_t blendSpeed;
      uint8_t numFlips;
    } blend;
    // arg access for solid
    struct {
      uint8_t onDuration;
      uint8_t offDuration;
      uint8_t gapDuration;
      uint8_t dashDuration;
      uint8_t groupSize;
      uint8_t colorIndex;
    } solid;
  };
  // indicates how many of the above args are filled
  uint8_t numArgs;
};

#endif
