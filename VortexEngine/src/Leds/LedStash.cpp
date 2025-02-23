#include "LedStash.h"

#include "../VortexEngine.h"

LedStash::LedStash(VortexEngine &engine) :
  m_engine(engine),
  m_ledColorsStash()
{
  m_ledColorsStash.resize(LED_COUNT);
}

void LedStash::setIndex(LedPos pos, RGBColor col)
{
  if (pos > LED_LAST) {
    return;
  }
  m_ledColorsStash[pos] = col;
}

void LedStash::clear()
{
  for (LedPos pos = LED_FIRST; pos < LED_LAST; ++pos) {
    m_ledColorsStash[pos].clear();
  }
}

// index operator to access color index
RGBColor &LedStash::operator[](int index)
{
  return m_ledColorsStash[index];
}

LedStash::~LedStash()
{
}
