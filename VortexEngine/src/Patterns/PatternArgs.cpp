#include "PatternArgs.h"

#include "../Serial/ByteStream.h"

PatternArgs::PatternArgs() :
  arg1(0), arg2(0), arg3(0), arg4(0), arg5(0), arg6(0), arg7(0), arg8(0), numArgs(0)
{
}
PatternArgs::PatternArgs(uint8_t a1) :
  arg1(a1), arg2(0), arg3(0), arg4(0), arg5(0), arg6(0), arg7(0), arg8(0), numArgs(1)
{
}
PatternArgs::PatternArgs(uint8_t a1, uint8_t a2) :
  arg1(a1), arg2(a2), arg3(0), arg4(0), arg5(0), arg6(0), arg7(0), arg8(0), numArgs(2)
{
}
PatternArgs::PatternArgs(uint8_t a1, uint8_t a2, uint8_t a3) :
  arg1(a1), arg2(a2), arg3(a3), arg4(0), arg5(0), arg6(0), arg7(0), arg8(0), numArgs(3)
{
}
PatternArgs::PatternArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4) :
  arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(0), arg6(0), arg7(0), arg8(0), numArgs(4)
{
}
PatternArgs::PatternArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5) :
  arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5), arg6(0), arg7(0), arg8(0), numArgs(5)
{
}
PatternArgs::PatternArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6) :
  arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5), arg6(a6), arg7(0), arg8(0), numArgs(6)
{
}
PatternArgs::PatternArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7) :
  arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5), arg6(a6), arg7(a7), arg8(0), numArgs(7)
{
}
PatternArgs::PatternArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8) :
  arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5), arg6(a6), arg7(a7), arg8(a8), numArgs(8)
{
}

void PatternArgs::init()
{
  *this = PatternArgs();
}
void PatternArgs::init(uint8_t a1)
{
  *this = PatternArgs(a1);
}
void PatternArgs::init(uint8_t a1, uint8_t a2)
{
  *this = PatternArgs(a1, a2);
}
void PatternArgs::init(uint8_t a1, uint8_t a2, uint8_t a3)
{
  *this = PatternArgs(a1, a2, a3);
}
void PatternArgs::init(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4)
{
  *this = PatternArgs(a1, a2, a3, a4);
}
void PatternArgs::init(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5)
{
  *this = PatternArgs(a1, a2, a3, a4, a5);
}
void PatternArgs::init(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6)
{
  *this = PatternArgs(a1, a2, a3, a4, a5, a6);
}
void PatternArgs::init(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7)
{
  *this = PatternArgs(a1, a2, a3, a4, a5, a6, a7);
}
void PatternArgs::init(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8)
{
  *this = PatternArgs(a1, a2, a3, a4, a5, a6, a7, a8);
}

void PatternArgs::addArgs(uint8_t a1)
{
  args[numArgs++] = a1;
}

void PatternArgs::addArgs(uint8_t a1, uint8_t a2)
{
  args[numArgs++] = a1;
  args[numArgs++] = a2;
}

void PatternArgs::addArgs(uint8_t a1, uint8_t a2, uint8_t a3)
{
  args[numArgs++] = a1;
  args[numArgs++] = a2;
  args[numArgs++] = a3;
}

void PatternArgs::addArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4)
{
  args[numArgs++] = a1;
  args[numArgs++] = a2;
  args[numArgs++] = a3;
  args[numArgs++] = a4;
}

void PatternArgs::addArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5)
{
  args[numArgs++] = a1;
  args[numArgs++] = a2;
  args[numArgs++] = a3;
  args[numArgs++] = a4;
  args[numArgs++] = a5;
}

void PatternArgs::addArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6)
{
  args[numArgs++] = a1;
  args[numArgs++] = a2;
  args[numArgs++] = a3;
  args[numArgs++] = a4;
  args[numArgs++] = a5;
  args[numArgs++] = a6;
}

void PatternArgs::addArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7)
{
  args[numArgs++] = a1;
  args[numArgs++] = a2;
  args[numArgs++] = a3;
  args[numArgs++] = a4;
  args[numArgs++] = a5;
  args[numArgs++] = a6;
  args[numArgs++] = a7;
}

void PatternArgs::addArgs(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8)
{
  args[numArgs++] = a1;
  args[numArgs++] = a2;
  args[numArgs++] = a3;
  args[numArgs++] = a4;
  args[numArgs++] = a5;
  args[numArgs++] = a6;
  args[numArgs++] = a7;
  args[numArgs++] = a7;
}

bool PatternArgs::operator!=(const PatternArgs &rhs) const
{
  return !(*this == rhs);
}

bool PatternArgs::operator==(const PatternArgs &rhs) const
{
  // just check number of args first
  if (rhs.numArgs != numArgs) {
    return false;
  }
  // compare num args
  for (uint8_t i = 0; i < numArgs; ++i) {
    if (args[i] != rhs.args[i]) {
      return false;
    }
  }
  return true;
}

uint8_t PatternArgs::operator[](int index) const
{
  if (index > 7) {
    return 0;
  }
  return args[index];
}

bool PatternArgs::serialize(ByteStream &buffer, ArgMap argmap) const
{
  if (!buffer.serialize8(argmap)) {
    return false;
  }
  for (uint8_t i = 0; i < MAX_ARGS; ++i) {
    if (ARGMAP_ISSET(argmap, i)) {
      if (!buffer.serialize8(args[i])) {
        return false;
      }
    }
  }
  return true;
}

ArgMap PatternArgs::unserialize(ByteStream &buffer)
{
  ArgMap argmap = ARG_NONE;
  if (!buffer.unserialize8(&argmap)) {
    return ARG_NONE;
  }
  for (uint8_t i = 0; i < MAX_ARGS; ++i) {
    if (ARGMAP_ISSET(argmap, i)) {
      if (!buffer.unserialize8(args + i)) {
        break;
      }
    }
  }
  return argmap;
}
