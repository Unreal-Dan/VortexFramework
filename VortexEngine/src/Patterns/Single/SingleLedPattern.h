#ifndef SINGLE_LED_PATTERN_H
#define SINGLE_LED_PATTERN_H

#include "../Pattern.h"

class SingleLedPattern : public Pattern
{
protected:
  // SingleLedPattern is an abstract class
  SingleLedPattern(VortexEngine &engine, const PatternArgs &args);

public:
  virtual ~SingleLedPattern();

  virtual void init() override;

private:

};

#endif
