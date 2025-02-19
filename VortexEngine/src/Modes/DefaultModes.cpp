#include "DefaultModes.h"

#include "../Colors/ColorTypes.h"

// all the different colorsets used in defaults defined separately from the default list
// so we can save space instead of using fixed sized arrays in the DefaultModeEntry for colors
const uint32_t rgbCols[] = { RGB_RED, RGB_GREEN, RGB_BLUE, };
const uint32_t pat2TipCols = 0x97709F;
const uint32_t pat2TopCols = 0x4D00B2;
const uint32_t pat4TipCols[] = { 0xC4003B, 0x700000, 0x000000, 0x0017E9, 0x22004E, 0x000000, };
const uint32_t pat4TopCols[] = { 0xC4003B, 0x4D00B2, 0x0017E9 };
const uint32_t pat5TipCols[] = { 0xFFC600, 0x235500, 0x00FF66, 0x004355, 0x0600FF, 0x480055, 0xFF0057 };
const uint32_t pat5TopCols[] = { 0xFFC600, 0x235500, 0x00FF66, 0x004355, 0x0600FF, 0x480055, 0xFF0057 };
const uint32_t pat6Cols[] = { 0x9FFF00, 0x00FF66, 0x009FFF, 0x5A00FF, 0xFF009F };
const uint32_t pat7Cols[] = { 0x00AA90, 0x00B1FF, 0x300055, 0xFF002D, 0x541B00 };
const uint32_t pat8TipCols[] = { 0x0000FF, 0x400055, 0x54000B, 0x525400, 0x26AA00 };
const uint32_t pat8TopCols[] = { 0xFFC600, 0x235500, 0x00FF66, 0x004355, 0x0600FF, 0x480055, 0xFF0057 };
const uint32_t pat9Cols[] = { 0xFFF600, 0x000880 };

// Here is the array of 'default modes' that are assigned to
// the gloveset upon factory reset
const DefaultModeEntry defaultModes[MAX_MODES] = {
  // mode 1
  {{
    { PATTERN_STROBEGAP, 3, rgbCols }, // tip
    { PATTERN_STROBEGAP, 3, rgbCols }  // top
  }},
  // mode 2
  {{
    { PATTERN_COMPLEMENTARY_BLEND, 3, rgbCols },
    { PATTERN_BLEND, 3, rgbCols }
  }},
  // mode 3
  {{
    { PATTERN_STROBE, 1, &pat2TipCols },
    { PATTERN_HYPERGAP, 1, &pat2TopCols }
  }},
  // mode 4
  {{
    { PATTERN_STROBE, 6, pat4TipCols },
    { PATTERN_STROBIE, 3, pat4TopCols }
  }},
  // mode 5
  {{
    { PATTERN_DOUBLEDOPS, 7, pat5TipCols },
    { PATTERN_DOUBLEDOPS, 7, pat5TopCols }
  }},
  // mode 6
  {{
    { PATTERN_GHOSTCRUSH, 5, pat6Cols },
    { PATTERN_GHOSTCRUSH, 5, pat6Cols }
  }},
  // mode 7
  {{
    { PATTERN_ULTRADOPS, 5, pat7Cols },
    { PATTERN_ULTRADOPS, 5, pat7Cols }
  }},
  // mode 8
  {{
    { PATTERN_PICOGAP, 5, pat8TipCols },
    { PATTERN_PICOGAP, 7, pat8TopCols }
  }},
  // mode 9
  {{
    { PATTERN_DOPS, 2, pat9Cols },
    { PATTERN_HYPERSTROBE, 2, pat9Cols }
  }},
};
