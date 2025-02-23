#ifndef COLORSET_H
#define COLORSET_H

#include "ColorTypes.h"

#include "../VortexConfig.h"

class ByteStream;
class Random;

class Colorset
{
public:
  // empty colorset
  Colorset();
  // constructor for 1-8 color slots
  Colorset(RGBColor c1, RGBColor c2 = RGB_OFF, RGBColor c3 = RGB_OFF,
    RGBColor c4 = RGB_OFF, RGBColor c5 = RGB_OFF, RGBColor c6 = RGB_OFF,
    RGBColor c7 = RGB_OFF, RGBColor c8 = RGB_OFF);
  Colorset(uint8_t numCols, const uint32_t *cols);
  ~Colorset();

  // move constructor
  Colorset(Colorset &&other) noexcept;

  // copy and assignment operators
  Colorset(const Colorset &other);
  void operator=(const Colorset &other);

  // equality operators
  bool operator==(const Colorset &other) const;
  bool operator!=(const Colorset &other) const;

  // initialize the colorset
  void init(RGBColor c1 = RGB_OFF, RGBColor c2 = RGB_OFF, RGBColor c3 = RGB_OFF,
    RGBColor c4 = RGB_OFF, RGBColor c5 = RGB_OFF, RGBColor c6 = RGB_OFF,
    RGBColor c7 = RGB_OFF, RGBColor c8 = RGB_OFF);

  // clear the colorset
  void clear();

  // pointer comparison
  bool equals(const Colorset &set) const;
  bool equals(const Colorset *set) const;

  // index operator to access color index
  RGBColor operator[](int index) const;

  enum ValueStyle : uint8_t
  {
    // Random values
    VAL_STYLE_RANDOM = 0,
    // First color low value, the rest are random
    VAL_STYLE_LOW_FIRST_COLOR,
    // First color high value, the rest are low
    VAL_STYLE_HIGH_FIRST_COLOR,
    // Alternat between high and low value
    VAL_STYLE_ALTERNATING,
    // Ascending values from low to high
    VAL_STYLE_ASCENDING,
    // Descending values from high to low
    VAL_STYLE_DESCENDING,
    // Constant value
    VAL_STYLE_CONSTANT,
    // Total number of value styles
    VAL_STYLE_COUNT
  };

  // add a single color
  bool addColor(RGBColor col);
  bool addColorHSV(uint8_t hue, uint8_t sat, uint8_t val);
  void addColorWithValueStyle(Random &ctx, uint8_t hue, uint8_t sat,
    ValueStyle valStyle, uint8_t numColors, uint8_t colorPos);
  void removeColor(uint8_t index);

  // randomize a colorset with a specific number of colors with
  // various different randomization techniques
  void randomize(Random &ctx, uint8_t numColors = 0);

  // function to randomize the colors with various different modes of randomization
  enum ColorMode {
    THEORY,
    MONOCHROMATIC,
    EVENLY_SPACED
  };
  void randomizeColors(Random &ctx, uint8_t numColors, ColorMode mode);

  // similar function but with some different modes
  enum ColorMode2 {
    DOUBLE_SPLIT_COMPLIMENTARY,
    TETRADIC
  };
  void randomizeColors2(Random &ctx, ColorMode2 mode);

  // wrappers for various spacings
  void randomizeSolid(Random &ctx) { randomizeColors(ctx, 1, Colorset::ColorMode::EVENLY_SPACED); }
  void randomizeComplimentary(Random &ctx) { randomizeColors(ctx, 2, Colorset::ColorMode::EVENLY_SPACED); }
  void randomizeTriadic(Random &ctx) { randomizeColors(ctx, 3, Colorset::ColorMode::EVENLY_SPACED); }
  void randomizeSquare(Random &ctx) { randomizeColors(ctx, 4, Colorset::ColorMode::EVENLY_SPACED); }
  void randomizePentadic(Random &ctx) { randomizeColors(ctx, 5, Colorset::ColorMode::EVENLY_SPACED); }
  void randomizeRainbow(Random &ctx) { randomizeColors(ctx, 8, Colorset::ColorMode::EVENLY_SPACED); }

  // fade all of the colors in the set
  void adjustBrightness(uint8_t fadeby);

  // get a color from the colorset
  RGBColor get(uint8_t index = 0) const;

  // set an rgb color in a slot, or add a new color if you specify
  // a slot higher than the number of colors in the colorset
  void set(uint8_t index, RGBColor col);

  // skip some amount of colors
  void skip(int32_t amount = 1);

  // get current color in cycle
  RGBColor cur();

  // set the current index of the colorset
  void setCurIndex(uint8_t index);
  void resetIndex();

  // the current index
  uint8_t curIndex() const { return m_curIndex; }

  // get the prev color in cycle
  RGBColor getPrev();

  // get the next color in cycle
  RGBColor getNext();

  // peek at the color indexes from current but don't iterate
  RGBColor peek(int32_t offset) const;

  // better wording for peek 1 ahead
  RGBColor peekNext() const { return peek(1); }

  // the number of colors in the palette
  uint8_t numColors() const { return m_numColors; }

  // shift an index to a new destination, dest index before move
  void shift(uint8_t idx, uint8_t dest);

  // whether the colorset is currently on the first color or last color
  bool onStart() const;
  bool onEnd() const;

  // serialize the colorset to save/load
  bool serialize(ByteStream &buffer) const;
  bool unserialize(ByteStream &buffer);

private:
  // pre-allocate the palette
  bool initPalette(uint8_t numColors);

  // palette of colors
  RGBColor *m_palette;
  // the current index, starts at UINT8_MAX so that
  // the very first call to getNext will iterate to 0
  uint8_t m_curIndex;
  // the actual number of colors in the set
  uint8_t m_numColors;
};

#endif
