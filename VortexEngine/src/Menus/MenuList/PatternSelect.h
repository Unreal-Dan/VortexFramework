#ifndef PATTERN_SELECT_H
#define PATTERN_SELECT_H

#include "../Menu.h"

#include "../../Colors/Colorset.h"
#include "../../Modes/Mode.h"

class PatternSelect : public Menu
{
public:
  PatternSelect(VortexEngine &engine, const RGBColor &col, bool advanced);
  ~PatternSelect();

  bool init() override;
  MenuAction run() override;

  // callback after the user selects the target led
  void onLedSelected() override;

  // handlers for clicks
  void onShortClick() override;
  void onLongClick() override;

private:
  void nextPatternID();
  void nextPattern();

  // the patternid of the current demo
  PatternID m_newPatternID;

  // helpful member
  LedPos m_srcLed;

  // the pat select starts by showing the current pattern
  // then the first click begin cycling the list of pats
  bool m_started;
};

#endif
