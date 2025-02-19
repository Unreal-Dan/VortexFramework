#include "Colorset.h"

#include "../Serial/ByteStream.h"
#include "../Random/Random.h"
#include "../Memory/Memory.h"

#include "../Log/Log.h"

#include <string.h>

// when no color is selected in the colorset the index is this
// then when you call getNext() for the first time it returns
// the 0th color in the colorset and after the index will be 0
#define INDEX_NONE UINT8_MAX

Colorset::Colorset() :
  m_palette(),
  m_curIndex(INDEX_NONE),
  m_numColors(0)
{
  init();
}

Colorset::Colorset(RGBColor c1, RGBColor c2, RGBColor c3, RGBColor c4,
  RGBColor c5, RGBColor c6, RGBColor c7, RGBColor c8) :
  Colorset()
{
  init(c1, c2, c3, c4, c5, c6, c7, c8);
}

Colorset::Colorset(uint8_t numCols, const uint32_t *cols) :
  Colorset()
{
  if (numCols > MAX_COLOR_SLOTS) {
    numCols = MAX_COLOR_SLOTS;
  }
  for (uint8_t i = 0; i < numCols; ++i) {
    addColor(RGBColor(cols[i]));
  }
}

Colorset::Colorset(const Colorset &other) :
  Colorset()
{
  // invoke = operator
  *this = other;
}

Colorset::~Colorset()
{
  clear();
}

Colorset::Colorset(Colorset &&other) noexcept :
  m_palette(),
  m_curIndex(INDEX_NONE),
  m_numColors(other.m_numColors)
{
  memcpy((void *)m_palette, (void *)other.m_palette, sizeof(m_palette));
  memset((void *)other.m_palette, 0, sizeof(m_palette));
  other.m_numColors = 0;
  other.m_curIndex = INDEX_NONE;
}

void Colorset::operator=(const Colorset &other)
{
  initPalette(other.m_numColors);
  for (uint8_t i = 0; i < other.m_numColors; ++i) {
    m_palette[i] = other.m_palette[i];
  }
  resetIndex();
}

bool Colorset::operator==(const Colorset &other) const
{
  // only compare the palettes for equality
  return (m_numColors == other.m_numColors) &&
         (memcmp(m_palette, other.m_palette, m_numColors * sizeof(RGBColor)) == 0);
}

bool Colorset::operator!=(const Colorset &other) const
{
  return !operator==(other);
}

void Colorset::init(RGBColor c1, RGBColor c2, RGBColor c3, RGBColor c4,
  RGBColor c5, RGBColor c6, RGBColor c7, RGBColor c8)
{
  // clear any existing colors
  clear();
  // would be nice if we could do this another way
  if (!c1.empty()) addColor(c1);
  if (!c2.empty()) addColor(c2);
  if (!c3.empty()) addColor(c3);
  if (!c4.empty()) addColor(c4);
  if (!c5.empty()) addColor(c5);
  if (!c6.empty()) addColor(c6);
  if (!c7.empty()) addColor(c7);
  if (!c8.empty()) addColor(c8);
}

void Colorset::clear()
{
  memset((void *)m_palette, 0, sizeof(m_palette));
  m_numColors = 0;
  resetIndex();
}

bool Colorset::equals(const Colorset &set) const
{
  return operator==(set);
}

bool Colorset::equals(const Colorset *set) const
{
  if (!set) {
    return false;
  }
  return operator==(*set);
}

RGBColor Colorset::operator[](int index) const
{
  return get(index);
}

// add a single color
bool Colorset::addColor(RGBColor col)
{
  if (m_numColors >= MAX_COLOR_SLOTS) {
    return false;
  }
  // insert new color and increment number of colors
  m_palette[m_numColors] = col;
  m_numColors++;
  return true;
}

bool Colorset::addColorHSV(uint8_t hue, uint8_t sat, uint8_t val)
{
  return addColor(HSVColor(hue, sat, val));
}

void Colorset::addColorWithValueStyle(Random &ctx, uint8_t hue, uint8_t sat, ValueStyle valStyle, uint8_t numColors, uint8_t colorPos)
{
  if (numColors == 1) {
    addColorHSV(hue, sat, ctx.next8(16, 255));
    return;
  }
  switch (valStyle) {
  default:
  case VAL_STYLE_RANDOM:
    addColorHSV(hue, sat, 85 * ctx.next8(1, 4));
    break;
  case VAL_STYLE_LOW_FIRST_COLOR:
    if (m_numColors == 0) {
      addColorHSV(hue, sat, ctx.next8(0, 86));
    } else {
      addColorHSV(hue, sat, 85 * ctx.next8(1, 4));
    }
    break;
  case VAL_STYLE_HIGH_FIRST_COLOR:
    if (m_numColors == 0) {
      addColorHSV(hue, sat, 255);
    } else {
      addColorHSV(hue, sat, ctx.next8(0, 86));
    }
    break;
  case VAL_STYLE_ALTERNATING:
    if (m_numColors % 2 == 0) {
      addColorHSV(hue, sat, 255);
    } else {
      addColorHSV(hue, sat, 85);
    }
    break;
  case VAL_STYLE_ASCENDING:
    addColorHSV(hue, sat, (colorPos + 1) * (255 / numColors));
    break;
  case VAL_STYLE_DESCENDING:
    addColorHSV(hue, sat, 255 - (colorPos * (255 / numColors)));
    break;
  case VAL_STYLE_CONSTANT:
    addColorHSV(hue, sat, 255);
  }
}

void Colorset::removeColor(uint8_t index)
{
  if (index >= m_numColors) {
    return;
  }
  for (uint8_t i = index; i < (m_numColors - 1); ++i) {
    m_palette[i] = m_palette[i + 1];
  }
  m_palette[--m_numColors].clear();
}

// create a set of truely random colors
void Colorset::randomize(Random &ctx, uint8_t numColors)
{
  clear();
  if (!numColors) {
    numColors = ctx.next8(2, 9);
  }
  ValueStyle valStyle = (ValueStyle)ctx.next8(0, VAL_STYLE_COUNT);

  for (uint8_t i = 0; i < numColors; ++i) {
    // call next8 explicitly in this order because the order they
    // are called is undefined when called as parameters to another function.
    // ex: f(a,b,c) may call in the order a,b,c or c,b,a depending on compiler.
    // So different compilers may produce different results, 
    // but like this it is explicit
    uint8_t sat = ctx.next8();
    uint8_t hue = ctx.next8();
    addColorWithValueStyle(ctx, hue, sat, valStyle, numColors, i);
  }
}

void Colorset::randomizeColors(Random &ctx, uint8_t numColors, ColorMode mode)
{
  clear();
  if (!numColors) {
    numColors = ctx.next8(mode == MONOCHROMATIC ? 2 : 1, 9);
  }
  uint8_t randomizedHue = ctx.next8();
  uint8_t colorGap = 0;
  if (mode == THEORY && numColors > 1) {
    colorGap = ctx.next8(16, 256 / (numColors - 1));
  }
  ValueStyle valStyle = (ValueStyle)ctx.next8(0, VAL_STYLE_COUNT);
  // the doubleStyle decides if some colors are added to the set twice
  uint8_t doubleStyle = 0;
  if (numColors <= 7) {
    doubleStyle = (ctx.next8(0, 1));
  }
  if (numColors <= 4) {
    doubleStyle = (ctx.next8(0, 2));
  }
  for (uint8_t i = 0; i < numColors; i++) {
    uint8_t hueToUse;
    uint8_t valueToUse = 255;
    if (mode == THEORY) {
      hueToUse = (randomizedHue + (i * colorGap));
    } else if (mode == MONOCHROMATIC) {
      hueToUse = randomizedHue;
      valueToUse = 255 - (i * (256 / numColors));
    } else { // EVENLY_SPACED
      hueToUse = (randomizedHue + (256 / numColors) * i);
    }
    addColorWithValueStyle(ctx, hueToUse, valueToUse, valStyle, numColors, i);
    // double all colors or only first color
    if (doubleStyle == 2 || (doubleStyle == 1 && !i)) {
      addColorWithValueStyle(ctx, hueToUse, valueToUse, valStyle, numColors, i);
    }
  }
}

void Colorset::randomizeColors2(Random &ctx, ColorMode2 mode)
{
  clear();
  uint8_t primaryHue = ctx.next8();
  if (mode == DOUBLE_SPLIT_COMPLIMENTARY) {
    uint8_t splitGap = ctx.next8(1, 64);
    ValueStyle valStyle = (ValueStyle)ctx.next8(0, VAL_STYLE_COUNT);
    addColorWithValueStyle(ctx, (primaryHue + splitGap + 128), 255, valStyle, 5, 0);
    addColorWithValueStyle(ctx, (primaryHue - splitGap), 255, valStyle, 5, 1);
    addColorWithValueStyle(ctx, primaryHue, 255, valStyle, 5, 2);
    addColorWithValueStyle(ctx, (primaryHue + splitGap), 255, valStyle, 5, 3);
    addColorWithValueStyle(ctx, (primaryHue - splitGap + 128), 255, valStyle, 5, 4);
  } else if (mode == TETRADIC) {
    uint8_t secondaryHue = ctx.next8();
    ValueStyle valStyle = (ValueStyle)ctx.next8(0, VAL_STYLE_COUNT);
    addColorWithValueStyle(ctx, primaryHue, 255, valStyle, 4, 0);
    addColorWithValueStyle(ctx, secondaryHue, 255, valStyle, 4, 1);
    addColorWithValueStyle(ctx, (primaryHue + 128), 255, valStyle, 4, 2);
    addColorWithValueStyle(ctx, (secondaryHue + 128), 255, valStyle, 4, 3);
  }
}

void Colorset::adjustBrightness(uint8_t fadeby)
{
  for (uint8_t i = 0; i < m_numColors; ++i) {
    m_palette[i].adjustBrightness(fadeby);
  }
}

// get a color from the colorset
RGBColor Colorset::get(uint8_t index) const
{
  if (index >= m_numColors) {
    return RGBColor(0, 0, 0);
  }
  return m_palette[index];
}

// set an rgb color in a slot, or add a new color if you specify
// a slot higher than the number of colors in the colorset
void Colorset::set(uint8_t index, RGBColor col)
{
  // special case for 'setting' a color at the edge of the palette,
  // ie adding a new color when you set an index higher than the max
  if (index >= m_numColors) {
    if (!addColor(col)) {
      ERROR_LOGF("Failed to add new color at index %u", index);
    }
    return;
  }
  m_palette[index] = col;
}

// skip some amount of colors
void Colorset::skip(int32_t amount)
{
  if (!m_numColors) {
    return;
  }
  // if the colorset hasn't started yet
  if (m_curIndex == INDEX_NONE) {
    m_curIndex = 0;
  }

  // first modulate the amount to skip to be within +/- the number of colors
  amount %= (int32_t)m_numColors;

  // max = 3
  // m_curIndex = 2
  // amount = -10
  m_curIndex = ((int32_t)m_curIndex + (int32_t)amount) % (int32_t)m_numColors;
  if (m_curIndex > m_numColors) { // must have wrapped
    // simply wrap it back
    m_curIndex += m_numColors;
  }
}

RGBColor Colorset::cur()
{
  if (m_curIndex >= m_numColors) {
    return RGBColor(0, 0, 0);
  }
  if (m_curIndex == INDEX_NONE) {
    return m_palette[0];
  }
  return m_palette[m_curIndex];
}

void Colorset::setCurIndex(uint8_t index)
{
  if (!m_numColors) {
    return;
  }
  if (index > (m_numColors - 1)) {
    return;
  }
  m_curIndex = index;
}

void Colorset::resetIndex()
{
  m_curIndex = INDEX_NONE;
}

RGBColor Colorset::getPrev()
{
  if (!m_numColors) {
    return RGB_OFF;
  }
  // handle wrapping at 0
  if (m_curIndex == 0 || m_curIndex == INDEX_NONE) {
    m_curIndex = numColors() - 1;
  } else {
    m_curIndex--;
  }
  // return the color
  return m_palette[m_curIndex];
}

RGBColor Colorset::getNext()
{
  if (!m_numColors) {
    return RGB_OFF;
  }
  // iterate current index, let it wrap at max uint8
  m_curIndex++;
  // then modulate the result within max colors
  m_curIndex %= numColors();
  // return the color
  return m_palette[m_curIndex];
}

// peek at the next color but don't iterate
RGBColor Colorset::peek(int32_t offset) const
{
  if (!m_numColors) {
    return RGB_OFF;
  }
  uint8_t nextIndex = 0;
  // get index of the next color
  if (offset >= 0) {
    nextIndex = (m_curIndex + offset) % numColors();
  } else {
    if (offset < -1 * (int32_t)(numColors())) {
      return RGB_OFF;
    }
    nextIndex = ((m_curIndex + numColors()) + (int)offset) % numColors();
  }
  // return the color
  return m_palette[nextIndex];
}

bool Colorset::onStart() const
{
  return (m_curIndex == 0);
}

bool Colorset::onEnd() const
{
  if (!m_numColors) {
    return false;
  }
  return (m_curIndex == m_numColors - 1);
}

bool Colorset::serialize(ByteStream &buffer) const
{
  if (!buffer.serialize8(m_numColors)) {
    return false;
  }
  // write all the reds/greens/blues together to maximize chance of
  // repeated values to improve RLE compression
  for (uint8_t i = 0; i < m_numColors; ++i) {
    if (!buffer.serialize8(m_palette[i].red)) {
      return false;
    }
  }
  for (uint8_t i = 0; i < m_numColors; ++i) {
    if (!buffer.serialize8(m_palette[i].green)) {
      return false;
    }
  }
  for (uint8_t i = 0; i < m_numColors; ++i) {
    if (!buffer.serialize8(m_palette[i].blue)) {
      return false;
    }
  }
  return true;
}

bool Colorset::unserialize(ByteStream &buffer)
{
  if (!buffer.unserialize8(&m_numColors)) {
    return false;
  }
  if (m_numColors > MAX_COLOR_SLOTS) {
    return false;
  }
  if (!initPalette(m_numColors)) {
    return false;
  }
  for (uint8_t i = 0; i < m_numColors; ++i) {
    if (!buffer.unserialize8(&m_palette[i].red)) {
      return false;
    }
  }
  for (uint8_t i = 0; i < m_numColors; ++i) {
    if (!buffer.unserialize8(&m_palette[i].green)) {
      return false;
    }
  }
  for (uint8_t i = 0; i < m_numColors; ++i) {
    if (!buffer.unserialize8(&m_palette[i].blue)) {
      return false;
    }
  }
  return true;
}

bool Colorset::initPalette(uint8_t numColors)
{
  clear();
  m_numColors = numColors;
  return true;
}

