#ifndef RANDOMIZER_H
#define RANDOMIZER_H

#include "../Menu.h"

#include "../../Random/Random.h"
#include "../../Modes/Mode.h"

class Mode;

class Randomizer : public Menu
{
public:
  Randomizer(const RGBColor &col, bool advanced);
  ~Randomizer();

  bool init() override;
  MenuAction run() override;

  // handlers for clicks
  void onShortClick() override;
  void onLongClick() override;

  // re-roll a new randomization with a given context on an led
  bool reRoll();

private:
  // random context for each single led
  Random m_singlesRandCtx[LED_COUNT];
#if VORTEX_SLIM == 0
  // random context for the multi led position
  Random m_multiRandCtx;
#endif

  // the time of the last randomization
  uint32_t m_lastRandomization;

  enum RandomizeFlags : uint8_t {
    // this isn't a valid randomization state, if the flags are
    // on this state then the user will be prompted to pick
    RANDOMIZE_NONE = 0,

    // the two main kinds of randomization
    RANDOMIZE_COLORSET = (1 << 0),
    RANDOMIZE_PATTERN = (1 << 1),

    // compound flags both colorset and pattern randomization
    RANDOMIZE_BOTH = (RANDOMIZE_COLORSET | RANDOMIZE_PATTERN)
  };

  // the randomization flags above
  uint8_t m_flags;

  // auxilliary variable to display cycling hue at menus
  uint8_t m_displayHue;

  // whether still need to select a randomization type
  bool m_needToSelect;
  // whether auto cycling
  bool m_autoCycle;

  // show the randomization type selection
  void showRandomizationSelect();

  // main reRoll functions
#if VORTEX_SLIM == 0
  bool reRollMulti();
  PatternID rollMultiLedPatternID(Random &ctx);
  bool splitMultiRandomize();
  bool rollSinglesLedMap(Random &ctx, LedMap map);
#endif
  bool reRollSingles();
  PatternID rollSingleLedPatternID(Random &ctx);

  // generate a random colorset with a random context
  Colorset rollColorset(Random &ctx);

  // roll a custom pattern by generating random arguments
  bool rollCustomPattern(Random &ctx, Mode *pMode, LedPos pos);

  // more specific random pattern generators that just generate patternargs
  void traditionalPattern(Random &ctx, PatternArgs &outArgs);
  void gapPattern(Random &ctx, PatternArgs &outArgs);
  void dashPattern(Random &ctx, PatternArgs &outArgs);
  void crushPattern(Random &ctx, PatternArgs &outArgs);
};

#endif
