#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <inttypes.h>

#include "../Colors/ColorTypes.h"
#include "LedTypes.h"

class LedStash;

class Leds
{
  // private unimplemented constructor
  Leds();

public:
  // opting for static class here because there should only ever be one
  // Led control object and I don't like singletons
  static bool init();
  static void cleanup();

  // control individual LED, these are appropriate to use in internal pattern logic
  static void setIndex(LedPos target, RGBColor col);
  static void setRange(LedPos first, LedPos last, RGBColor col);
  static void setAll(RGBColor col);

  // Turn off individual LEDs, these are appropriate to use in internal pattern logic
  static void clearIndex(LedPos target) { setIndex(target, HSV_OFF); }
  static void clearRange(LedPos first, LedPos last) { setRange(first, last, HSV_OFF); }
  static void clearAll() { setAll(HSV_OFF); }

  // control two LEDs on a pair, these are appropriate for use in internal pattern logic
  static void setPair(Pair pair, RGBColor col);
  static void setPairs(Pair first, Pair last, RGBColor col);

  // Turn off both LEDs on a pair, these are appropriate for use in internal pattern logic
  static void clearPair(Pair pair) { setPair(pair, HSV_OFF); }
  static void clearPairs(Pair first, Pair last) { setPairs(first, last, HSV_OFF); }

  // Controll pair evens
  static void setRangeEvens(Pair first, Pair last, RGBColor);
  static void setAllEvens(RGBColor col);
  // Controll pair odds
  static void setRangeOdds(Pair first, Pair last, RGBColor);
  static void setAllOdds(RGBColor col);

  // Turn off tips
  static void clearRangeEvens(Pair first, Pair last);
  static void clearAllEvens();
  // Turn off tops
  static void clearRangeOdds(Pair first, Pair last);
  static void clearAllOdds();

  // Control pairs of leds
  static void setPair(Pair pair, RGBColor col);
  static void clearPair(Pair pair);

  // Turn on/off a mapping of leds with a color
  static void setMap(LedMap map, RGBColor col);
  static void clearMap(LedMap map);

  // stores Led for later use
  static void stashAll(LedStash &stash);

  // restores Leds from stash
  static void restoreAll(const LedStash &stash);

  // Dim individual LEDs, these are appropriate to use in internal pattern logic
  static void adjustBrightnessIndex(LedPos target, uint8_t fadeBy);
  static void adjustBrightnessRange(LedPos first, LedPos last, uint8_t fadeBy);
  static void adjustBrightnessAll(uint8_t fadeBy);

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
  static void blinkIndex(LedPos target, uint64_t time, uint32_t offMs = 250, uint32_t onMs = 500, RGBColor col = RGB_OFF);
  static void blinkRange(LedPos first, LedPos last, uint64_t time, uint32_t offMs = 250, uint32_t onMs = 500, RGBColor col = RGB_OFF);
  static void blinkAll(uint64_t time, int32_t offMs = 250, uint32_t onMs = 500, RGBColor col = RGB_OFF);
  // Blink both LEDs on a pair
  static void blinkPair(Pair pair, uint64_t time, uint32_t offMs = 250, uint32_t onMs = 500, RGBColor col = RGB_OFF);
  static void blinkPairs(Pair first, Pair last, uint64_t time, uint32_t offMs = 250, uint32_t onMs = 500, RGBColor col = RGB_OFF);

  // breath the hue on an index
  // warning: these use hsv to rgb in realtime!
  static void breathIndex(LedPos target, uint32_t hue, uint32_t variance,
    uint32_t magnitude = 15, uint8_t sat = 255, uint8_t val = 210);
  static void breathIndexSat(LedPos target, uint32_t hue, uint32_t variance,
    uint32_t magnitude = 15, uint8_t sat = 255, uint8_t val = 210);
  static void breathIndexVal(LedPos target, uint32_t hue, uint32_t variance,
    uint32_t magnitude = 15, uint8_t sat = 255, uint8_t val = 210);

  // get the RGBColor of an Led index
  static RGBColor getLed(LedPos pos) { return led(pos); }

  // global brightness
  static uint32_t getBrightness() { return m_brightness; }
  static void setBrightness(uint32_t brightness) { m_brightness = brightness; }

  // actually update the LEDs and show the changes
  static void update();

private:
  // accessor for led colors, use this for all access to allow for mapping
  static inline RGBColor &led(LedPos pos)
  {
    if (pos > LED_LAST) {
      pos = LED_LAST;
    }
    return m_ledColors[pos];
  }

  // the global brightness
  static uint32_t m_brightness;

  // array of led color values
  static RGBColor m_ledColors[LED_COUNT];
};

#endif
