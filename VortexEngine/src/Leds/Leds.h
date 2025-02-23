#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <inttypes.h>

#include "../Colors/ColorTypes.h"
#include "../VortexConfig.h"

#include <vector>

// drop-in LedPos constant replacements
#define LED_COUNT m_engine.leds().ledCount()
#define LED_LAST m_engine.leds().ledLast()
#define LED_ALL LED_COUNT
#define LED_MULTI m_engine.leds().ledMulti()
#define LED_ALL_SINGLE m_engine.leds().ledAllSingle()
#define LED_ANY m_engine.leds().ledAny()

// check if an led is even or odd
#define isEven(pos) ((pos % 2) == 0)
#define isOdd(pos) ((pos % 2) != 0)

// convert a pair to even or odd led position
#define pairEven(pair) (LedPos)((uint32_t)pair * 2)
#define pairOdd(pair) (LedPos)(((uint32_t)pair * 2) + 1)

// convert an led position to a pair
#define ledToPair(pos) (Pair)((uint32_t)pos / 2)

// various macros for mapping leds to an LedMap
#define MAP_LED(led) (LedMap)((uint64_t)1 << led)
#define MAP_PAIR_EVEN(pair) MAP_LED(pairEven(pair))
#define MAP_PAIR_ODD(pair) MAP_LED(pairOdd(pair))
#define MAP_PAIR(pair) (MAP_PAIR_EVEN(pair) | MAP_PAIR_ODD(pair))

// check if a map is purely just 1 led or not
#define MAP_IS_ONE_LED(map) (map && !(map & (map-1)))

// foreach led macro (only iterates singles)
#define MAP_FOREACH_LED(map) for (LedPos pos = m_engine.leds().ledmapGetFirstLed(map); pos != LED_COUNT; pos = m_engine.leds().ledmapGetNextLed(map, pos))

// bitmap of all pairs (basically LED_COUNT bits)
#define MAP_LED_ALL (LedMap)((2 << (LED_COUNT - 1)) - 1)

// blank map
#define MAP_LED_NONE 0

#define MAP_INVERSE(map) ((~map) & MAP_LED_ALL)

// macro for all evens and odds
#define MAP_PAIR_EVENS (((1 << LED_COUNT) - 1) & 0x55555555)
#define MAP_PAIR_ODDS (((1 << LED_COUNT) - 1) & 0xAAAAAAAA)

// Some preset bitmaps for pair groupings
#define MAP_PAIR_ODD_EVENS (MAP_PAIR_EVEN(PAIR_0) | MAP_PAIR_EVEN(PAIR_2) | MAP_PAIR_EVEN(PAIR_4))
#define MAP_PAIR_ODD_ODDS (MAP_PAIR_ODD(PAIR_0) | MAP_PAIR_ODD(PAIR_2) | MAP_PAIR_ODD(PAIR_4))

#define MAP_PAIR_EVEN_EVENS (MAP_PAIR_EVEN(PAIR_3) | MAP_PAIR_EVEN(PAIR_1))
#define MAP_PAIR_EVEN_ODDS (MAP_PAIR_ODD(PAIR_3) | MAP_PAIR_ODD(PAIR_1))

typedef uint8_t LedPos;
#define LED_FIRST (LedPos)0
#define LED_0 (LedPos)(LED_FIRST + 0)
#define LED_1 (LedPos)(LED_FIRST + 1)
#define LED_2 (LedPos)(LED_FIRST + 2)
#define LED_3 (LedPos)(LED_FIRST + 3)
#define LED_4 (LedPos)(LED_FIRST + 4)
#define LED_5 (LedPos)(LED_FIRST + 5)
#define LED_6 (LedPos)(LED_FIRST + 6)
#define LED_7 (LedPos)(LED_FIRST + 7)
#define LED_8 (LedPos)(LED_FIRST + 8)
#define LED_9 (LedPos)(LED_FIRST + 9)

typedef uint8_t Pair;
#define PAIR_FIRST (Pair)0
#define PAIR_0 (Pair)(PAIR_FIRST + 0)
#define PAIR_1 (Pair)(PAIR_FIRST + 1)
#define PAIR_2 (Pair)(PAIR_FIRST + 2)
#define PAIR_3 (Pair)(PAIR_FIRST + 3)
#define PAIR_4 (Pair)(PAIR_FIRST + 4)
#define PAIR_LAST PAIR_4
#define PAIR_COUNT (Pair)(PAIR_LAST + 1)

// LedMap is a bitmap of leds, used for expressing whether to turn certain leds on
// or off with a single integer
typedef uint64_t LedMap;

class VortexEngine;
class LedStash;

class Leds
{
public:
  Leds(VortexEngine &engine);
  ~Leds();

  // opting for class here because there should only ever be one
  // Led control object and I don't like singletons
  bool init();
  void cleanup();

  // control individual LED, these are appropriate to use in internal pattern logic
  void setIndex(LedPos target, RGBColor col);
  void setRange(LedPos first, LedPos last, RGBColor col);
  void setAll(RGBColor col);

  // Turn off individual LEDs, these are appropriate to use in internal pattern logic
  void clearIndex(LedPos target) { setIndex(target, RGB_OFF); }
  void clearRange(LedPos first, LedPos last) { setRange(first, last, RGB_OFF); }
  void clearAll() { setAll(RGB_OFF); }

  // control two LEDs on a pair, these are appropriate for use in internal pattern logic
  void setPair(Pair pair, RGBColor col);
  void setPairs(Pair first, Pair last, RGBColor col);

  // Turn off both LEDs on a pair, these are appropriate for use in internal pattern logic
  void clearPair(Pair pair) { setPair(pair, RGB_OFF); }
  void clearPairs(Pair first, Pair last) { setPairs(first, last, RGB_OFF); }

  // Controll pair evens
  void setRangeEvens(Pair first, Pair last, RGBColor);
  void setAllEvens(RGBColor col);
  // Controll pair odds
  void setRangeOdds(Pair first, Pair last, RGBColor);
  void setAllOdds(RGBColor col);

  // Turn off tips
  void clearRangeEvens(Pair first, Pair last);
  void clearAllEvens();
  // Turn off tops
  void clearRangeOdds(Pair first, Pair last);
  void clearAllOdds();

  // Turn on/off a mapping of leds with a color
  void setMap(LedMap map, RGBColor col);
  void clearMap(LedMap map);

  // stores Led for later use
  void stashAll(LedStash &stash);

  // restores Leds from stash
  void restoreAll(const LedStash &stash);

  // Dim individual LEDs, these are appropriate to use in internal pattern logic
  void adjustBrightnessIndex(LedPos target, uint8_t fadeBy);
  void adjustBrightnessRange(LedPos first, LedPos last, uint8_t fadeBy);
  void adjustBrightnessAll(uint8_t fadeBy);

  // Blink an led to blank or a color
  //
  // These APIs work by checking if the current time is within the
  // 'on duration' then they apply the color to the target, otherwise they do
  // nothing. The goal of this behaviour is to allow them to be used in any
  // situation to 'blink' an led to either off or some color.
  //
  // However since these APIs modulate current time to check if within the 'on'
  // threshold that makes them unsuitable for internal pattern usage because it
  // is unpredictable whether they will blink on or off first
  void blinkIndexOffset(LedPos target, uint32_t time, uint16_t offMs = 250, uint16_t onMs = 500, RGBColor col = RGB_OFF);
  void blinkRangeOffset(LedPos first, LedPos last, uint32_t time, uint16_t offMs = 250, uint16_t onMs = 500, RGBColor col = RGB_OFF);
  void blinkIndex(LedPos target, uint16_t offMs = 250, uint16_t onMs = 500, RGBColor col = RGB_OFF);
  void blinkRange(LedPos first, LedPos last, uint16_t offMs = 250, uint16_t onMs = 500, RGBColor col = RGB_OFF);
  void blinkMap(LedMap targets, uint16_t offMs = 250, uint16_t onMs = 500, RGBColor col = RGB_OFF);
  void blinkAll(uint16_t offMs = 250, uint16_t onMs = 500, RGBColor col = RGB_OFF);
  // Blink both LEDs on a pair
  void blinkPair(Pair pair, uint16_t offMs = 250, uint16_t onMs = 500, RGBColor col = RGB_OFF);
  void blinkPairs(Pair first, Pair last, uint16_t offMs = 250, uint16_t onMs = 500, RGBColor col = RGB_OFF);

  // breath the hue on an index
  // warning: these use hsv to rgb in realtime!
  void breatheIndex(LedPos target, uint8_t hue, uint32_t variance,
    uint32_t magnitude = 15, uint8_t sat = 255, uint8_t val = 210);
  void breatheRange(LedPos first, LedPos last, uint8_t hue, uint32_t variance,
    uint32_t magnitude = 15, uint8_t sat = 255, uint8_t val = 210);
  void breatheIndexSat(LedPos target, uint8_t hue, uint32_t variance,
    uint32_t magnitude = 15, uint8_t sat = 255, uint8_t val = 210);
  void breatheIndexVal(LedPos target, uint8_t hue, uint32_t variance,
    uint32_t magnitude = 15, uint8_t sat = 255, uint8_t val = 210);

  // a very specialized api to hold all leds on a color for 250ms
  void holdAll(RGBColor col);

  // get the RGBColor of an Led index
  RGBColor getLed(LedPos pos) { return led(pos); }

  // global brightness
  uint8_t getBrightness() { return m_brightness; }
  void setBrightness(uint8_t brightness) { m_brightness = brightness; }

  // actually update the LEDs and show the changes
  void update();

  void setLedCount(uint8_t leds);
  LedPos ledCount() { return (LedPos)m_ledCount; }
  LedPos ledLast() { return (LedPos)(m_ledCount - 1); }
  LedPos ledMulti() { return (LedPos)(m_ledCount + 1); }
  LedPos ledAllSingle() { return (LedPos)(m_ledCount + 2); }
  LedPos ledAny() { return (LedPos)(m_ledCount + 3); }
  RGBColor *ledData() { return m_ledColors.data(); }

  // set a single led
  inline void ledmapSetLed(LedMap &map, LedPos pos)
  {
    if (pos < ledCount()) map |= (1ull << pos);
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

  // convert a map to the first Led position in the map
  inline LedPos ledmapGetFirstLed(LedMap map)
  {
    if (map == MAP_LED(ledMulti())) {
      return ledMulti();
    }
    LedPos pos = LED_FIRST;
    while (map && pos < ledCount()) {
      if (map & 1) {
        return pos;
      }
      map >>= 1;
      pos = (LedPos)(pos + 1);
    }
    return ledCount();
  }

  // given an led map and a position, find the next position in the map
  inline LedPos ledmapGetNextLed(LedMap map, LedPos pos)
  {
    pos = (LedPos)(pos + 1);
    map >>= pos;
    while (map && pos < ledCount()) {
      if (map & 1) {
        return pos;
      }
      map >>= 1;
      pos = (LedPos)(pos + 1);
    }
    return ledCount();
  }

private:
  // accessor for led colors, use this for all access to allow for mapping
  inline RGBColor &led(LedPos pos)
  {
    if (pos >= m_ledColors.size()) {
      pos = LED_0;
    }
    return m_ledColors[pos];
  }

  // reference to engine
  VortexEngine &m_engine;

  // the global brightness
  uint8_t m_brightness;

  // dynamic led count
  uint8_t m_ledCount;
  // array of led color values
  std::vector<RGBColor> m_ledColors;
};

#endif
