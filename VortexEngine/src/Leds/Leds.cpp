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
#include <avr/io.h>
#include <string.h>
#endif

// swap two variables in place
#define SWAP(x, y) x ^= y; y ^= x; x ^= y;

#define LED_DATA_PIN  7

// global brightness
uint8_t Leds::m_brightness = DEFAULT_BRIGHTNESS;
// array of led color values
RGBColor Leds::m_ledColors[LED_COUNT] = { RGB_OFF };

// Output PORT register
volatile uint8_t *Leds::m_port = nullptr;
// Output PORT bitmask
uint8_t Leds::m_pinMask = 0;

bool Leds::init()
{
#ifdef VORTEX_EMBEDDED
  // clear the onboard led so it displays nothing
  // tiny neo pixels
  PORTB.DIRSET |= PIN4_bm;
  // register ouput port
  m_port = &VPORTB.OUT;
  // create a pin mask to use later
  m_pinMask = PIN4_bm;
#endif
#ifdef VORTEX_LIB
  Vortex::vcallbacks()->ledsInit(m_ledColors, LED_COUNT);
#endif
  return true;
}

void Leds::cleanup()
{
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
  // start from tip and go to top
  setRange(pairEven(pair), pairOdd(pair), col);
}

void Leds::setPairs(Pair first, Pair last, RGBColor col)
{
  // start from tip and go to top
  setRange(pairEven(first), pairOdd(last), col);
}

void Leds::setRangeEvens(Pair first, Pair last, RGBColor col)
{
  for (Pair pos = first; pos <= last; pos++) {
    setIndex(pairEven(pos), col);
  }
}

void Leds::setAllEvens(RGBColor col)
{
  for (Pair pos = PAIR_FIRST; pos <= PAIR_LAST; pos++) {
    setIndex(pairEven(pos), col);
  }
}

void Leds::setRangeOdds(Pair first, Pair last, RGBColor col)
{
  for (Pair pos = first; pos <= last; pos++) {
    setIndex(pairOdd(pos), col);
  }
}

void Leds::setAllOdds(RGBColor col)
{
  for (Pair pos = PAIR_FIRST; pos <= PAIR_LAST; pos++) {
    setIndex(pairOdd(pos), col);
  }
}

void Leds::clearRangeEvens(Pair first, Pair last)
{
  for (Pair pos = first; pos <= last; pos++) {
    clearIndex(pairEven(pos));
  }
}

void Leds::clearAllEvens()
{
  for (Pair pos = PAIR_FIRST; pos <= PAIR_LAST; pos++) {
    clearIndex(pairEven(pos));
  }
}

void Leds::clearRangeOdds(Pair first, Pair last)
{
  for (Pair pos = first; pos <= last; pos++) {
    clearIndex(pairOdd(pos));
  }
}

void Leds::clearAllOdds()
{
  for (Pair pos = PAIR_FIRST; pos <= PAIR_LAST; pos++) {
    clearIndex(pairOdd(pos));
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

void Leds::holdAll(RGBColor col)
{
  setAll(col);
  update();
  Time::delayMilliseconds(250);
}

void Leds::update()
{
#ifdef VORTEX_EMBEDDED
  RGBColor ledbackups[LED_COUNT];
  memcpy(ledbackups, m_ledColors, sizeof(m_ledColors));
  for (int c = 0; c < LED_COUNT; ++c) {
#define SCALE8(i, scale)  (((uint16_t)i * (uint16_t)(scale)) >> 8)
    m_ledColors[c].red = SCALE8(m_ledColors[c].red, m_brightness);
    m_ledColors[c].green = SCALE8(m_ledColors[c].green, m_brightness);
    m_ledColors[c].blue = SCALE8(m_ledColors[c].blue, m_brightness);
  }
  // swap the red and green channels for the 2nd led on the microlight,
  // they will be swapped back at the end of this function
  SWAP(m_ledColors[LED_1].red, m_ledColors[LED_1].green);
#endif

#ifdef VORTEX_LIB
  Vortex::vcallbacks()->ledsShow();
#endif

  // Thanks to TinyNeoPixel for this code
#ifdef VORTEX_EMBEDDED
  volatile uint16_t
    i = LED_COUNT * sizeof(RGBColor); // Loop counter
  volatile uint8_t
    *ptr = (volatile uint8_t *)m_ledColors,   // Pointer to next byte
    b = *ptr++,   // Current byte value
    hi,             // PORT w/output bit set high
    lo;             // PORT w/output bit set low

  // AVRxt MCUs --  tinyAVR 0/1/2, megaAVR 0, AVR Dx ----------------------
  // with extended maximum speeds to supm_port vigorously overclocked
  // Dx-series parts. This is by no means intended to imply that they will
  // run at those speeds, only that - if they do - you can control WS2812s
  // with them.

  // Hand-tuned assembly code issues data to the LED drivers at a specific
  // rate.  There's separate code for different CPU speeds (8, 12, 16 MHz)
  // for both the WS2811 (400 KHz) and WS2812 (800 KHz) drivers.  The
  // datastream timing for the LED drivers allows a little wiggle room each
  // way (listed in the datasheets), so the conditions for compiling each
  // case are set up for a range of frequencies rather than just the exact
  // 8, 12 or 16 MHz values, permitting use with some close-but-not-spot-on
  // devices (e.g. 16.5 MHz DigiSpark).  The ranges were arrived at based
  // on the datasheet figures and have not been extensively tested outside
  // the canonical 8/12/16 MHz speeds; there's no guarantee these will work
  // close to the extremes (or possibly they could be pushed further).
  // Keep in mind only one CPU speed case actually gets compiled; the
  // resulting program isn't as massive as it might look from source here.

  #if (F_CPU >= 9500000UL) && (F_CPU <= 11100000UL)
    /*
    volatile uint8_t n1, n2 = 0;  // First, next bits out

    */
    // 14 instruction clocks per bit: HHHHxxxxLLLLL
    // ST instructions:               ^   ^   ^   (T=0,4,7)
    volatile uint8_t next;

    hi   = *m_port |  m_pinMask;
    lo   = *m_port & ~m_pinMask;
    next = lo;
    if (b & 0x80) {
      next = hi;
    }

    // Don't "optimize" the OUT calls into the bitTime subroutine;
    // we're exploiting the RCALL and RET as 3- and 4-cycle NOPs!
    asm volatile(
      "_head10:"                  "\n\t" //        (T =  0)
      "st   %a[port], %[hi]"      "\n\t" //        (T =  1)
      "rcall _bitTime10"          "\n\t" // Bit 7  (T = 14)
      "st   %a[port], %[hi]"      "\n\t"
      "rcall _bitTime10"          "\n\t" // Bit 6
      "st   %a[port], %[hi]"      "\n\t"
      "rcall _bitTime10"          "\n\t" // Bit 5
      "st   %a[port], %[hi]"      "\n\t"
      "rcall _bitTime10"          "\n\t" // Bit 4
      "st   %a[port], %[hi]"      "\n\t"
      "rcall _bitTime10"          "\n\t" // Bit 3
      "st   %a[port], %[hi]"      "\n\t"
      "rcall _bitTime10"          "\n\t" // Bit 2
      "st   %a[port], %[hi]"      "\n\t"
      "rcall _bitTime10"          "\n\t" // Bit 1
      // Bit 0:
      "st   %a[port], %[hi]"      "\n\t" // 1    PORT = hi    (T =  1)
      "ld   %[byte] , %a[ptr]+"   "\n\t" // 2    b = *ptr++   (T =  4)
      "st   %a[port], %[next]"    "\n\t" // 1    PORT = next  (T =  5)
      "nop"                       "\n\t" // 1    nop          (T =  6)
      "mov  %[next] , %[lo]"      "\n\t" // 1    next = lo    (T =  7)
      "sbrc %[byte] , 7"          "\n\t" // 1-2  if (b & 0x80) (T =  8)
       "mov %[next] , %[hi]"      "\n\t" // 0-1    next = hi  (T =  9)
      "st   %a[port], %[lo]"      "\n\t" // 1    PORT = lo    (T = 10)
      "sbiw %[count], 1"          "\n\t" // 2    i--          (T = 12)
      "brne _head10"              "\n\t" // 2    if (i != 0) -> (next byte) (T = 14)
       "rjmp _done10"             "\n\t"
      "_bitTime10:"               "\n\t" //      nop nop nop     (T =  4)
       "st   %a[port], %[next]"   "\n\t" // 1    PORT = next     (T =  5)
       "mov  %[next], %[lo]"      "\n\t" // 1    next = lo       (T =  6)
       "lsl  %[byte]"             "\n\t" // 1    b <<= 1         (T =  7)
       "sbrc %[byte], 7"          "\n\t" // 1-2  if (b & 0x80)    (T =  8)
        "mov %[next], %[hi]"      "\n\t" // 0-1   next = hi      (T =  9)
       "st   %a[port], %[lo]"     "\n\t" // 1    PORT = lo       (T = 10)
       "ret"                      "\n\t" // 4    return to above where we called from
       "_done10:"                 "\n"
    : [ptr]   "+e" (ptr),
      [byte]  "+r" (b),
      [next]  "+r" (next),
      [count] "+w" (i)
    : [port]   "e" (m_port),
      [hi]     "r" (hi),
      [lo]     "r" (lo));

// 20 MHz(ish) AVRxt ------------------------------------------------------
#elif (F_CPU >= 19000000UL) && (F_CPU <= 22000000L)


    // 25 inst. clocks per bit: HHHHHHHxxxxxxxxLLLLLLLLLL
    // ST instructions:         ^      ^       ^       (T=0,7,15)

    volatile uint8_t next, bit;

    hi   = *m_port |  m_pinMask;
    lo   = *m_port & ~m_pinMask;
    next = lo;
    bit  = 8;

    asm volatile(
     "head20:"                   "\n\t" // Clk  Pseudocode    (T =  0)
      "st   %a[port],  %[hi]"    "\n\t" // 1    PORT = hi     (T =  1)
      "sbrc %[byte],  7"         "\n\t" // 1-2  if (b & 128)
       "mov  %[next], %[hi]"     "\n\t" // 0-1   next = hi    (T =  3)
      "dec  %[bit]"              "\n\t" // 1    bit--         (T =  4)
      "nop"                      "\n\t" // 1    nop           (T =  5)
      "rjmp .+0"                 "\n\t" // 2    nop nop       (T =  7)
      "st   %a[port],  %[next]"  "\n\t" // 1    PORT = next   (T =  8)
      "mov  %[next] ,  %[lo]"    "\n\t" // 1    next = lo     (T =  9)
      "breq nextbyte20"          "\n\t" // 1-2  if (bit == 0) (from dec above)
      "rol  %[byte]"             "\n\t" // 1    b <<= 1       (T = 11)
      "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 13)
      "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 15)
      "st   %a[port],  %[lo]"    "\n\t" // 1    PORT = lo     (T = 16)
      "nop"                      "\n\t" // 1    nop           (T = 17)
      "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 19)
      "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 21)
      "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 23)
      "rjmp head20"              "\n\t" // 2    -> head20 (next bit out)
     "nextbyte20:"               "\n\t" //                    (T = 11)
      "ldi  %[bit]  ,  8"        "\n\t" // 1    bit = 8       (T = 12)
      "ld   %[byte] ,  %a[ptr]+" "\n\t" // 2    b = *ptr++    (T = 14)
      "nop"                      "\n\t" // 1    nop           (T = 15)
      "st   %a[port], %[lo]"     "\n\t" // 1    PORT = lo     (T = 16)
      "nop"                      "\n\t" // 1    nop           (T = 17)
      "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 19)
      "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 21)
      "sbiw %[count], 1"         "\n\t" // 2    i--           (T = 23)
       "brne head20"             "\n"   // 2    if (i != 0) -> (next byte)  ()
    : [ptr]   "+e" (ptr),
      [byte]  "+r" (b),
      [bit]   "+d" (bit),
      [next]  "+r" (next),
      [count] "+w" (i)
    : [port]   "e" (m_port),
      [hi]     "r" (hi),
      [lo]     "r" (lo));

  #else
    #error "CPU SPEED NOT SUPPORTED"
  #endif
  // END AVR ----------------------------------------------------------------
#endif

#ifdef VORTEX_EMBEDDED
  // swap red and green channels back so all algorithms continue working
  SWAP(m_ledColors[LED_1].red, m_ledColors[LED_1].green);
  // restore the led colors from the backup (brightness mod)
  memcpy(m_ledColors, ledbackups, sizeof(m_ledColors));
#endif
}
