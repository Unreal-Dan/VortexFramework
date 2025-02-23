#ifndef SEQUENCED_PATTERN_H
#define SEQUENCED_PATTERN_H

#include "../CompoundPattern.h"

#include "../../../Leds/LedTypes.h"
#include "../../../Time/Timer.h"

#include "Sequence.h"

class ByteStream;

class SequencedPattern : public CompoundPattern
{
protected:
  // SequencedPattern is an abstract class it cannot be directly
  SequencedPattern(VortexEngine &engine, const PatternArgs &args);
  SequencedPattern(VortexEngine &engine, const PatternArgs &args, const Sequence &sequence);

public:
  // initialize a sequence pattern with a list of pointers to sequencesteps and the
  // number of entries in that list
  virtual ~SequencedPattern();

  // init the pattern to initial state
  virtual void init() override;

  // pure virtual must override the play function
  virtual void play() override;

  // change the sequence of the pattern
  void bindSequence(const Sequence &sequence);

protected:
  void playSequenceStep(const SequenceStep &step);

  // static data
  Sequence m_sequence;

  // runtime data
  uint8_t m_curSequence;
  Timer m_timer;
};

#endif
