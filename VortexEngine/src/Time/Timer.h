#ifndef TIMER_H
#define TIMER_H

#include <inttypes.h>

#include "../Leds/LedTypes.h"

#define ALARM_NONE -1

// an alarm id is just an int
typedef int8_t AlarmID;

#define TIMER_FLAGS_NONE  0

// the number of alarms
#define TIMER_1_ALARM     1
#define TIMER_2_ALARMS    2
#define TIMER_3_ALARMS    3
#define TIMER_4_ALARMS    4

// bitmask for all alarms
#define TIMER_ALARM_MASK  ( 1 | 2 | 3 | 4 )

// start the alarm
#define TIMER_START       (1 << 7)

class Timer
{
public:
  Timer(VortexEngine &engine);
  ~Timer();

  // init a timer with a number of alarms and optionally start it
  void init(uint8_t flags = TIMER_FLAGS_NONE, uint8_t alarm1 = 0,
    uint8_t alarm2 = 0, uint8_t alarm3 = 0, uint8_t alarm4 = 0);

  // add alarms to the timer and returns it's ID
  // Alarm IDs start at 0 and count upward
  AlarmID addAlarm(uint32_t interval);

  // restart the timer entirely
  void restart(uint32_t offset = 0);

  // start the timer but don't change current alarm, this shifts
  // the timer startTime but does not reset it's alarm state
  void start(uint32_t offset = 0);

  // delete all alarms from the timer and reset
  void reset();

  // returns true when the current timer is starting, used for synchronization
  bool onStart() const;
  // returns true when the current timer is ending, used for synchronization
  bool onEnd() const;

  // Will return the alarmID of the alarm that triggered
  // If no alarm hits it will return ALARM_NONE.
  // Outputs an integer indicating the number of times the alarm has triggered
  // since the start time or the last detection of that alarm ID
  AlarmID alarm();

  // the number of alarms
  uint32_t numAlarms() const { return m_numAlarms; }
  uint32_t curAlarm() const { return m_curAlarm; }

  // the start time of the timer
  uint32_t startTime() const { return m_startTime; }

#if TIMER_TEST == 1
  static void test();
#endif

private:
  // reference to engine
  VortexEngine &m_engine;

  // helpers to set/get start time
  uint32_t getStartTime() const;
  void setStartTime(uint32_t);

  // the list of alarms and number of alarms
  uint32_t *m_alarms;
  uint8_t m_numAlarms;
  // the current alarm being checked
  AlarmID m_curAlarm;
  // start time in microseconds
  uint32_t m_startTime;

  // only need time simulations for test framework pattern strip generation
#ifdef VORTEX_LIB
  // the simulation start time
  uint32_t m_simStartTime;
#endif
};

#endif