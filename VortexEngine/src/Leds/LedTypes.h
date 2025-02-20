#ifndef LED_CONFIG_H
#define LED_CONFIG_H

#include <inttypes.h>
#include <stdarg.h>

#include "../VortexConfig.h"

// Defined the LED positions, their order, and index
enum LedPos : uint8_t
{
  // this should always be first
  LED_FIRST = 0,

  // LED constants for each led
  LED_0 = LED_FIRST,
  LED_1,
  LED_2,
  LED_3,
  LED_4,
  LED_5,
  LED_6,
  LED_7,
  LED_8,
  LED_9,

  // inner circle of leds
  LED_10,
  LED_11,
  LED_12,
  LED_13,
  LED_14,
  LED_15,
  LED_16,
  LED_17,
  LED_18,
  LED_19,

  // the number of entries above
  LED_COUNT,

  // the last LED index
  LED_LAST = (LED_COUNT - 1),

  // target all leds (multi and single)
  // When fetching this the same as LED_ANY
  // When setting this will set all of the leds
  LED_ALL = LED_COUNT,

  // target the multi led slot
  //
  // When fetching this will return the multi led slot
  // When setting this will set the multi led slot
  LED_MULTI = (LED_COUNT + 1),

  // target all single led slots
  //
  // When fetching this will return the first single led slot
  // When setting this will set all single led slots
  LED_ALL_SINGLE = (LED_COUNT + 2),

  // Target the 'effective' led slot (any slot)
  //
  // When fetching this will:
  //    1. return the multi led slot if it exists
  //    2. otherwise the first single led slot
  // 
  // When setting this will:
  //    1. if setting single led pattern will set all
  //    2. if setting multi led pattern will set multi
  LED_ANY = (LED_COUNT + 3),

  // other customs?
  // LED_EVENS = (LED_COUNT + 2),
  // LED_ODDS = (LED_COUNT + 3),
};

enum Pair : uint8_t
{
  PAIR_FIRST = 0,

  // one pair for each pair of leds, adjust this to be 2x the LED_COUNT
  PAIR_0 = PAIR_FIRST,
  PAIR_1,
  PAIR_2,
  PAIR_3,
  PAIR_4,

  PAIR_5,
  PAIR_6,
  PAIR_7,
  PAIR_8,
  PAIR_9,

  PAIR_COUNT,
  PAIR_LAST = (PAIR_COUNT - 1),
};

// Compile-time check on the number of pairs and leds
static_assert(LED_COUNT == (PAIR_COUNT * 2), "Incorrect number of Pairs for Leds! Adjust the Led enum or Pair enum to match");

enum Radial : uint8_t
{
  RADIAL_FIRST = 0,

  RADIAL_0 = RADIAL_FIRST,
  RADIAL_1,
  RADIAL_2,
  RADIAL_3,
  RADIAL_4,
  RADIAL_5,
  RADIAL_6,
  RADIAL_7,
  RADIAL_8,
  RADIAL_9,

  RADIAL_COUNT,
  RADIAL_LAST = (RADIAL_COUNT - 1),
};

static_assert(RADIAL_COUNT == (LED_COUNT / 2), "Incorrect number of Radials for Leds! Adjust the Led enum or Radial enum to match");

#define radialInner(radial) (LedPos)((uint32_t)LED_10 + (uint32_t)radial)
#define radialOuter(radial) (LedPos)((uint32_t)LED_0 + (uint32_t)radial)

// check if an led is even or odd
#define isEven(pos) ((pos % 2) == 0)
#define isOdd(pos) ((pos % 2) != 0)

// convert a pair to even or odd led position
#define pairEven(pair) (LedPos)((uint32_t)pair * 2)
#define pairOdd(pair) (LedPos)(((uint32_t)pair * 2) + 1)

// convert an led position to a pair
#define ledToPair(pos) (Pair)((uint32_t)pos / 2)

// LedMap is a bitmap of leds, used for expressing whether to turn certain leds on
// or off with a single integer
typedef uint64_t LedMap;

// various macros for mapping leds to an LedMap
#define MAP_LED(led) (LedMap)((uint64_t)1 << led)
#define MAP_PAIR_EVEN(pair) MAP_LED(pairEven(pair))
#define MAP_PAIR_ODD(pair) MAP_LED(pairOdd(pair))
#define MAP_PAIR(pair) (MAP_PAIR_EVEN(pair) | MAP_PAIR_ODD(pair))

// check if a map is purely just 1 led or not
#define MAP_IS_ONE_LED(map) (map && !(map & (map-1)))

// foreach led macro (only iterates singles)
#define MAP_FOREACH_LED(map) for (LedPos pos = ledmapGetFirstLed(map); pos != LED_COUNT; pos = ledmapGetNextLed(map, pos))

// convert a map to the first Led position in the map
inline LedPos ledmapGetFirstLed(LedMap map)
{
  if (map == MAP_LED(LED_MULTI)) {
    return LED_MULTI;
  }
  LedPos pos = LED_FIRST;
  while (map && pos < LED_COUNT) {
    if (map & 1) {
      return pos;
    }
    map >>= 1;
    pos = (LedPos)(pos + 1);
  }
  return LED_COUNT;
}

// given an led map and a position, find the next position in the map
inline LedPos ledmapGetNextLed(LedMap map, LedPos pos)
{
  pos = (LedPos)(pos + 1);
  map >>= pos;
  while (map && pos < LED_COUNT) {
    if (map & 1) {
      return pos;
    }
    map >>= 1;
    pos = (LedPos)(pos + 1);
  }
  return LED_COUNT;
}

// bitmap of all pairs (basically LED_COUNT bits)
#define MAP_LED_ALL ((2 << (LED_COUNT - 1)) - 1)

// blank map
#define MAP_LED_NONE 0

#define MAP_INVERSE(map) ((~map) & MAP_LED_ALL)

// macro for all evens and odds
#define MAP_PAIR_EVENS (((1 << LED_COUNT) - 1) & 0x55555555)
#define MAP_PAIR_ODDS (((1 << LED_COUNT) - 1) & 0xAAAAAAAA)

#define MAP_OUTER_RING ((((1 << LED_COUNT) - 1) >> (LED_COUNT / 2)) << (LED_COUNT / 2))
#define MAP_INNER_RING ((((1 << LED_COUNT) - 1) << (LED_COUNT / 2)) >> (LED_COUNT / 2))

// Some preset bitmaps for pair groupings
#define MAP_PAIR_ODD_EVENS (MAP_PAIR_EVEN(PAIR_0) | MAP_PAIR_EVEN(PAIR_2) | MAP_PAIR_EVEN(PAIR_4))
#define MAP_PAIR_ODD_ODDS (MAP_PAIR_ODD(PAIR_0) | MAP_PAIR_ODD(PAIR_2) | MAP_PAIR_ODD(PAIR_4))

#define MAP_PAIR_EVEN_EVENS (MAP_PAIR_EVEN(PAIR_3) | MAP_PAIR_EVEN(PAIR_1))
#define MAP_PAIR_EVEN_ODDS (MAP_PAIR_ODD(PAIR_3) | MAP_PAIR_ODD(PAIR_1))

#define MAP_RING_INNER  (MAP_LED(LED_0) | MAP_LED(LED_1) | MAP_LED(LED_2) | MAP_LED(LED_3) | \
                         MAP_LED(LED_4) | MAP_LED(LED_5) | MAP_LED(LED_6) | MAP_LED(LED_7) | \
                         MAP_LED(LED_8) | MAP_LED(LED_9))
#define MAP_RING_OUTER  (MAP_LED(LED_10) | MAP_LED(LED_11) | MAP_LED(LED_12) | MAP_LED(LED_13) | \
                         MAP_LED(LED_14) | MAP_LED(LED_15) | MAP_LED(LED_16) | MAP_LED(LED_17) | \
                         MAP_LED(LED_18) | MAP_LED(LED_19))

#define MAP_RING_INNER_EVEN  (MAP_RING_INNER & 0xAAAAAAAA)
#define MAP_RING_INNER_ODD   (MAP_RING_INNER & 0x55555555)
#define MAP_RING_OUTER_EVEN  (MAP_RING_OUTER & 0xAAAAAAAA)
#define MAP_RING_OUTER_ODD   (MAP_RING_OUTER & 0x55555555)

#define MAP_LINE_1      (MAP_LED(LED_0) | MAP_LED(LED_10) | MAP_LED(LED_15) | MAP_LED(LED_5))
#define MAP_LINE_2      (MAP_LED(LED_1) | MAP_LED(LED_11) | MAP_LED(LED_16) | MAP_LED(LED_6))
#define MAP_LINE_3      (MAP_LED(LED_2) | MAP_LED(LED_12) | MAP_LED(LED_17) | MAP_LED(LED_7))
#define MAP_LINE_4      (MAP_LED(LED_3) | MAP_LED(LED_13) | MAP_LED(LED_18) | MAP_LED(LED_8))
#define MAP_LINE_5      (MAP_LED(LED_4) | MAP_LED(LED_14) | MAP_LED(LED_19) | MAP_LED(LED_9))

//Chromadeck bitmap
#define MAP_OPPOSITES_1 (MAP_LED(LED_0) | MAP_LED(LED_5) | MAP_LED(LED_10) | MAP_LED(LED_15))
#define MAP_OPPOSITES_2 (MAP_LED(LED_1) | MAP_LED(LED_6) | MAP_LED(LED_11) | MAP_LED(LED_16))
#define MAP_OPPOSITES_3 (MAP_LED(LED_2) | MAP_LED(LED_7) | MAP_LED(LED_12) | MAP_LED(LED_17))
#define MAP_OPPOSITES_4 (MAP_LED(LED_3) | MAP_LED(LED_8) | MAP_LED(LED_13) | MAP_LED(LED_18))
#define MAP_OPPOSITES_5 (MAP_LED(LED_4) | MAP_LED(LED_9) | MAP_LED(LED_14) | MAP_LED(LED_19))

// set a single led
inline void ledmapSetLed(LedMap &map, LedPos pos)
{
  if (pos < LED_COUNT) map |= (1ull << pos);
}
// set a single pair
inline void ledmapSetPair(LedMap &map, Pair pair)
{
  ledmapSetLed(map, pairEven(pair));
  ledmapSetLed(map, pairOdd(pair));
}

// check if an led is set in the map
inline bool ledmapCheckLed(LedMap map, LedPos pos)
{
  return ((map & (1ull << pos)) != 0);
}
// check if a pair is set in the map (both leds)
inline bool ledmapCheckPair(LedMap map, Pair pair)
{
  return ledmapCheckLed(map, pairEven(pair)) && ledmapCheckLed(map, pairOdd(pair));
}

// LedPos operators
inline LedPos &operator++(LedPos &c)
{
  c = LedPos(((uint32_t)c) + 1);
  return c;
}
inline LedPos operator++(LedPos &c, int)
{
  LedPos temp = c;
  ++c;
  return temp;
}
inline LedPos operator+(LedPos &c, int b)
{
  return (LedPos)((uint32_t)c + b);
}
inline LedPos &operator+=(LedPos &c, int b)
{
  c = LedPos(((uint32_t)c) + b);
  return c;
}
inline LedPos operator-(LedPos &c, int b)
{
  return (LedPos)((uint32_t)c - b);
}
inline LedPos &operator-=(LedPos &c, int b)
{
  c = LedPos(((uint32_t)c) - b);
  return c;
}

// pair operators
inline Pair &operator++(Pair &c)
{
  c = Pair(((uint32_t)c) + 1);
  return c;
}
inline Pair operator++(Pair &c, int)
{
  Pair temp = c;
  ++c;
  return temp;
}
inline Pair operator+(Pair &c, int b)
{
  return (Pair)((uint32_t)c + b);
}
inline Pair operator-(Pair &c, int b)
{
  return (Pair)((uint32_t)c - b);
}

// Radial operators
inline Radial &operator++(Radial &c)
{
  c = Radial(((uint32_t)c) + 1);
  return c;
}
inline Radial operator++(Radial &c, int)
{
  Radial temp = c;
  ++c;
  return temp;
}
inline Radial operator+(Radial &c, int b)
{
  return (Radial)((uint32_t)c + b);
}
inline Radial operator-(Radial &c, int b)
{
  return (Radial)((uint32_t)c - b);
}

#endif
