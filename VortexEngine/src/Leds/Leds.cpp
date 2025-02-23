#include <math.h>

#include "LedStash.h"
#include "Leds.h"

#include "../Time/TimeControl.h"
#include "../Modes/Modes.h"

#include "../VortexConfig.h"

#ifdef VORTEX_LIB
#include "../../VortexLib/VortexLib.h"
#endif

#ifdef VORTEX_EMBEDDED
#include <SPI.h>
#define ONBOARD_LED_SCK 8
#define ONBOARD_LED_MOSI 7
static void transfer(uint8_t byte)
{
  uint8_t startbit = 0x80;
  bool lastmosi = !(byte & startbit);
  for (uint8_t b = startbit; b != 0; b = b >> 1) {
    delayMicroseconds(4);
    bool towrite = byte & b;
    if (lastmosi != towrite) {
      digitalWrite(ONBOARD_LED_MOSI, towrite);
      lastmosi = towrite;
    }
    digitalWrite(ONBOARD_LED_SCK, HIGH);
    delayMicroseconds(4);
    digitalWrite(ONBOARD_LED_SCK, LOW);
  }
}
static void turnOffOnboardLED()
{
  // spi device begin
  pinMode(ONBOARD_LED_SCK, OUTPUT);
  digitalWrite(ONBOARD_LED_SCK, LOW);
  pinMode(ONBOARD_LED_MOSI, OUTPUT);
  digitalWrite(ONBOARD_LED_MOSI, HIGH);

  // Begin transaction, setting SPI frequency
  static const SPISettings mySPISettings(8000000, MSBFIRST, SPI_MODE0);
  SPI.beginTransaction(mySPISettings);
  for (uint8_t i = 0; i < 4; i++) {
    transfer(0x00); // begin frame
  }
  transfer(0xFF); //  Pixel start
  for (uint8_t i = 0; i < 3; i++) {
    transfer(0x00); // R,G,B
  }
  transfer(0xFF); // end frame
  SPI.endTransaction();
}
#endif

// global brightness
uint8_t Leds::m_brightness = DEFAULT_BRIGHTNESS;
// array of led color values
RGBColor Leds::m_ledColors[LED_COUNT] = { RGB_OFF };

bool Leds::init()
{
#ifdef VORTEX_EMBEDDED
  turnOffOnboardLED();
  SPI.begin();
#endif
#ifdef VORTEX_LIB
  Vortex::vcallbacks()->ledsInit(m_ledColors, LED_COUNT);
#endif
  return true;
}

void Leds::cleanup()
{
#ifdef VORTEX_EMBEDDED
  SPI.end();
#endif
  for (uint8_t i = 0; i < LED_COUNT; ++i) {
    m_ledColors[i].clear();
  }
}

void Leds::setIndex(LedPos target, RGBColor col)
{
  if (target >= LED_COUNT) {
    setAll(col);
    return;
  }
  led(target) = col;
}

void Leds::setRange(LedPos first, LedPos last, RGBColor col)
{
  for (LedPos pos = first; pos <= last; pos++) {
    setIndex(pos, col);
  }
}

void Leds::setAll(RGBColor col)
{
  setRange(LED_FIRST, LED_LAST, col);
}

void Leds::setPair(Pair pair, RGBColor col)
{
  setIndex(pairTop(pair), col);
  setIndex(pairBot(pair), col);
}

void Leds::setPairs(Pair first, Pair last, RGBColor col)
{
  // start from tip and go to top
  for (Pair p = first; p < last; ++p) {
    setPair(p, col);
  }
}

void Leds::setAllEvens(RGBColor col)
{
  for (LedPos i = LED_FIRST; i < LED_COUNT; ++i) {
    if ((i % 2) == 0) {
      setIndex(i, col);
    }
  }
}

void Leds::setAllOdds(RGBColor col)
{
  for (LedPos i = LED_FIRST; i < LED_COUNT; ++i) {
    if ((i % 2) != 0) {
      setIndex(i, col);
    }
  }
}

void Leds::clearAllEvens()
{
  for (LedPos i = LED_FIRST; i < LED_COUNT; ++i) {
    if ((i % 2) == 0) {
      clearIndex(i);
    }
  }
}

void Leds::clearAllOdds()
{
  for (LedPos i = LED_FIRST; i < LED_COUNT; ++i) {
    if ((i % 2) != 0) {
      clearIndex(i);
    }
  }
}

void Leds::setQuadrant(Quadrant quadrant, RGBColor col)
{
  if (quadrant == QUADRANT_5) {
    led(LED_3) = col;
    led(LED_10) = col;
    led(LED_17) = col;
    led(LED_24) = col;
    return;
  }
  // start from tip and go to top
  setRange(quadrantFirstLed(quadrant), quadrantLastLed(quadrant), col);
}

void Leds::setQuadrants(Quadrant first, Quadrant last, RGBColor col)
{
  for (Quadrant quad = first; quad <= last; ++quad) {
    setQuadrant(quad, col);
  }
}

void Leds::setRing(Ring ring, RGBColor col)
{
  for (Pair i = PAIR_FIRST; i < 4; ++i) {
    setPair((Pair)(ring + (4 * i)), col);
  }
}

void Leds::setRings(Ring first, Ring last, RGBColor col)
{
  for (Ring i = first; i <= last; ++i) {
    setRing(i, col);
  }
}

void Leds::clearRing(Ring ring)
{
  for (Pair i = PAIR_FIRST; i < 4; ++i) {
    clearPair((Pair)(ring + (4 * i)));
  }
}

void Leds::clearRings(Ring first, Ring last)
{
  for (Ring i = first; i <= last; ++i) {
    clearRing(i);
  }
}

void Leds::setMap(LedMap map, RGBColor col)
{
  for (LedPos pos = LED_FIRST; pos <= LED_LAST; pos++) {
    if (ledmapCheckLed(map, pos)) {
      setIndex(pos, col);
    }
  }
}

void Leds::clearMap(LedMap map)
{
  for (LedPos pos = LED_FIRST; pos <= LED_LAST; pos++) {
    if (ledmapCheckLed(map, pos)) {
      clearIndex(pos);
    }
  }
}

void Leds::stashAll(LedStash &stash)
{
  for (LedPos pos = LED_FIRST; pos <= LED_LAST; pos++) {
    stash.m_ledColorsStash[pos] = led(pos);
  }
}

void Leds::restoreAll(const LedStash &stash)
{
  for (LedPos pos = LED_FIRST; pos <= LED_LAST; pos++) {
    led(pos) = stash.m_ledColorsStash[pos];
  }
}

void Leds::adjustBrightnessIndex(LedPos target, uint8_t fadeBy)
{
  led(target).adjustBrightness(fadeBy);
}

void Leds::adjustBrightnessRange(LedPos first, LedPos last, uint8_t fadeBy)
{
  for (LedPos pos = first; pos <= last; pos++) {
    adjustBrightnessIndex(pos, fadeBy);
  }
}

void Leds::adjustBrightnessAll(uint8_t fadeBy)
{
  adjustBrightnessRange(LED_FIRST, LED_LAST, fadeBy);
}

// blinkIndex with the additional optional time offset
void Leds::blinkIndexOffset(LedPos target, uint32_t time, uint16_t offMs, uint16_t onMs, RGBColor col)
{
  if ((time % MS_TO_TICKS(offMs + onMs)) < MS_TO_TICKS(onMs)) {
    setIndex(target, col);
  }
}

void Leds::blinkRangeOffset(LedPos first, LedPos last, uint32_t time, uint16_t offMs, uint16_t onMs, RGBColor col)
{
  if ((time % MS_TO_TICKS(offMs + onMs)) < MS_TO_TICKS(onMs)) {
    setRange(first, last, col);
  }
}

void Leds::blinkIndex(LedPos target, uint16_t offMs, uint16_t onMs, RGBColor col)
{
  if ((Time::getCurtime() % MS_TO_TICKS(offMs + onMs)) < MS_TO_TICKS(onMs)) {
    setIndex(target, col);
  }
}

void Leds::blinkRange(LedPos first, LedPos last, uint16_t offMs, uint16_t onMs, RGBColor col)
{
  if ((Time::getCurtime() % MS_TO_TICKS(offMs + onMs)) < MS_TO_TICKS(onMs)) {
    setRange(first, last, col);
  }
}

void Leds::blinkQuadrantOffset(Quadrant target, uint32_t time, uint16_t offMs, uint16_t onMs, RGBColor col)
{
  if ((time % MS_TO_TICKS(offMs + onMs)) < MS_TO_TICKS(onMs)) {
    setQuadrant(target, col);
  }
}

void Leds::blinkQuadrant(Quadrant target, uint16_t offMs, uint16_t onMs, RGBColor col)
{
  if ((Time::getCurtime() % MS_TO_TICKS(offMs + onMs)) < MS_TO_TICKS(onMs)) {
    setQuadrant(target, col);
  }
}

void Leds::blinkMap(LedMap targets, uint16_t offMs, uint16_t onMs, RGBColor col)
{
  if ((Time::getCurtime() % MS_TO_TICKS(offMs + onMs)) < MS_TO_TICKS(onMs)) {
    for (LedPos pos = LED_FIRST; pos < LED_COUNT; pos++) {
      if (ledmapCheckLed(targets, pos)) {
        setIndex(pos, col);
      }
    }
  }
}

void Leds::blinkAll(uint16_t offMs, uint16_t onMs, RGBColor col)
{
  if ((Time::getCurtime() % MS_TO_TICKS(offMs + onMs)) < MS_TO_TICKS(onMs)) {
    setRange(LED_FIRST, LED_LAST, col);
  }
}

void Leds::blinkPair(Pair pair, uint16_t offMs, uint16_t onMs, RGBColor col)
{
  if ((Time::getCurtime() % MS_TO_TICKS(offMs + onMs)) < MS_TO_TICKS(onMs)) {
    setRange(pairEven(pair), pairOdd(pair), col);
  }
}

void Leds::blinkPairs(Pair first, Pair last, uint16_t offMs, uint16_t onMs, RGBColor col)
{
  if ((Time::getCurtime() % MS_TO_TICKS(offMs + onMs)) < MS_TO_TICKS(onMs)) {
    setRange(pairEven(first), pairOdd(last), col);
  }
}

void Leds::breatheIndex(LedPos target, uint8_t hue, uint32_t variance, uint32_t magnitude, uint8_t sat, uint8_t val)
{
  setIndex(target, HSVColor((uint8_t)(hue + ((sin(variance * 0.0174533) + 1) * magnitude)), sat, val));
}

void Leds::breatheRange(LedPos first, LedPos last, uint8_t hue, uint32_t variance, uint32_t magnitude, uint8_t sat, uint8_t val)
{
  setRange(first, last, HSVColor((uint8_t)(hue + ((sin(variance * 0.0174533) + 1) * magnitude)), sat, val));
}

void Leds::breatheIndexSat(LedPos target, uint8_t hue, uint32_t variance, uint32_t magnitude, uint8_t sat, uint8_t val)
{
  setIndex(target, HSVColor(hue, 255 - (uint8_t)(sat + 128 + ((sin(variance * 0.0174533) + 1) * magnitude)), val));
}

void Leds::breatheIndexVal(LedPos target, uint8_t hue, uint32_t variance, uint32_t magnitude, uint8_t sat, uint8_t val)
{
  setIndex(target, HSVColor(hue, sat, 255 - (uint8_t)(val + 128 + ((sin(variance * 0.0174533) + 1) * magnitude))));
}

void Leds::breatheQuadrant(Quadrant target, uint32_t hue, uint32_t variance, uint32_t magnitude, uint8_t sat, uint8_t val)
{
  for (uint8_t pos = 0; pos < 7; ++pos) {
    setQuadrant(target, HSVColor((uint8_t)(hue + ((sin(variance * 0.0174533) + 1) * magnitude)), sat, val));
  }
}

void Leds::holdAll(RGBColor col)
{
  setAll(col);
  update();
  Time::delayMilliseconds(250);
}

void Leds::update()
{
#ifdef VORTEX_EMBEDDED
  // the transaction prevents this from interfering with other communications
  // on the pins that are used for SPI, for example IR is on pin 2
  static const SPISettings mySPISettings(12000000, MSBFIRST, SPI_MODE0);
  SPI.beginTransaction(mySPISettings);
  // Double start frame, normally 4, idk why it's double
  for (uint8_t i = 0; i < 8; i++) {
    SPI.transfer(0);
  }
  // Adjust brightness to 5 bits
  uint8_t adjustedBrightness = 0b11100000 | ((m_brightness >> 3) & 0b00011111);
  // LED frames
  for (LedPos pos = LED_FIRST; pos < LED_COUNT; pos++) {
    SPI.transfer(adjustedBrightness);     // brightness
    SPI.transfer(m_ledColors[pos].blue);  // blue
    SPI.transfer(m_ledColors[pos].green); // green
    SPI.transfer(m_ledColors[pos].red);   // red
  }
  // don't need to end the SPI frame apparently, just end transaction
  SPI.endTransaction();
#endif
#ifdef VORTEX_LIB
  Vortex::vcallbacks()->ledsShow();
#endif
}
