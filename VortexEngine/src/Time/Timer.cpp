#include "Timer.h"

#include "../VortexEngine.h"

#include "TimeControl.h"

#include "../Memory/Memory.h"
#include "../Log/Log.h"

Timer::Timer(VortexEngine &engine) :
  m_engine(engine),
  m_alarms(nullptr),
  m_numAlarms(0),
  m_curAlarm(0),
  m_startTime(0)
#ifdef VORTEX_LIB
  , m_simStartTime(0)
#endif
{
}

Timer::~Timer()
{
  if (m_alarms) {
    vfree(m_alarms);
    m_alarms = nullptr;
  }
}

void Timer::init(uint8_t flags, uint8_t alarm1, uint8_t alarm2, uint8_t alarm3, uint8_t alarm4)
{
  reset();
  if ((flags & TIMER_ALARM_MASK) >= TIMER_1_ALARM) addAlarm(alarm1);
  if ((flags & TIMER_ALARM_MASK) >= TIMER_2_ALARMS) addAlarm(alarm2);
  if ((flags & TIMER_ALARM_MASK) >= TIMER_3_ALARMS) addAlarm(alarm3);
  if ((flags & TIMER_ALARM_MASK) >= TIMER_4_ALARMS) addAlarm(alarm4);
  if ((flags & TIMER_START) != 0) start();
}

AlarmID Timer::addAlarm(uint32_t interval)
{
  void *temp = vrealloc(m_alarms, sizeof(uint32_t) * (m_numAlarms + 1));
  if (!temp) {
    ERROR_OUT_OF_MEMORY();
    return false;
  }
  m_alarms = (uint32_t *)temp;
  m_alarms[m_numAlarms] = interval;
  return m_numAlarms++;
}

void Timer::restart(uint32_t offset)
{
  start(offset);
  // reset the current alarm
  m_curAlarm = 0;
}

void Timer::start(uint32_t offset)
{
  // reset the start time
  m_startTime = m_engine.time().getCurtime() + offset;
#ifdef VORTEX_LIB
  m_simStartTime = m_startTime;
#endif
}

void Timer::reset()
{
  vfree(m_alarms);
  m_alarms = nullptr;
  m_numAlarms = 0;
  m_curAlarm = 0;
  m_startTime = 0;
#ifdef VORTEX_LIB
  m_simStartTime = 0;
#endif
}

bool Timer::onStart() const
{
  if (m_engine.time().getCurtime() == getStartTime() && m_curAlarm == 0) {
    return true;
  }
  return false;
}

bool Timer::onEnd() const
{
  if (!m_alarms || !m_numAlarms) {
    return false;
  }
  // the tick for when this alarm triggers
  uint32_t alarmTime = m_alarms[m_curAlarm];
  if (alarmTime <= 1) {
    return true;
  }
  // grab curTime
  uint32_t now = m_engine.time().getCurtime();
  // get the start time of the timer
  uint32_t startTime = getStartTime();
  // time since start (forward or backwards)
  int32_t timeDiff = (int32_t)(int64_t)(now - startTime);
  // if no time since start then this definitely isn't the end
  if (!timeDiff) {
    return false;
  }
  // check if timediff is multiple of last tick in alarm
  return ((timeDiff % (alarmTime - 1)) == 0);
}

AlarmID Timer::alarm()
{
  if (!m_numAlarms || !m_alarms || m_curAlarm == ALARM_NONE) {
    return ALARM_NONE;
  }
  uint32_t now = m_engine.time().getCurtime();
  // time since start (forward or backwards)
  int32_t timeDiff = (int32_t)(int64_t)(now - getStartTime());
  if (timeDiff < 0) {
    return ALARM_NONE;
  }
  // if no time passed it's first alarm that is starting
  if (timeDiff == 0) {
    return 0;
  }
  uint32_t alarmTime = m_alarms[m_curAlarm];
  // if the current alarm duration is not a multiple of the current tick
  if (alarmTime && (timeDiff % alarmTime) != 0) {
    // then the alarm was not hit
    return ALARM_NONE;
  }
  // update the start time of the timer
  setStartTime(now);
  // increment current alarm then return that id
  m_curAlarm = (AlarmID)((m_curAlarm + 1) % m_numAlarms);
  return m_curAlarm;
}

uint32_t Timer::getStartTime() const
{
#ifdef VORTEX_LIB
  // timers use a different 'start time' tracker in simulations so that
  // the startTime remains unchanged when the simulation ends
  if (m_engine.time().isSimulation() && m_simStartTime) {
    return m_simStartTime;
  }
#endif
  return m_startTime;
}

void Timer::setStartTime(uint32_t tick)
{
#ifdef VORTEX_LIB
  // if this timer is running in a simulation then don't actually update
  // the starttime, instead update the simulation start time so that when
  // the simulation ends the startTime will not have changed at all
  if (m_engine.time().isSimulation()) {
    // move the sim start time
    m_simStartTime = tick;
    return;
  }
#endif
  // move the start time forward
  m_startTime = tick;
}

#if TIMER_TEST == 1
#include <assert.h>

void Timer::test()
{
  DEBUG_LOG("Starting Timer class tests...");
  Timer newTimer;

  // Test: Add alarms and check if alarms are added correctly
  newTimer.init();
  AlarmID alarm0 = newTimer.addAlarm(5);
  AlarmID alarm1 = newTimer.addAlarm(10);
  AlarmID alarm2 = newTimer.addAlarm(20);
  assert(alarm0 == 0);
  assert(alarm1 == 1);
  assert(alarm2 == 2);
  DEBUG_LOG("Alarm addition tests passed.");

  // Test: Start timer and check for alarms
  newTimer.start();
  // alarm1 should trigger immediately because it is starting
  assert(newTimer.alarm() == alarm0);
  DEBUG_LOG("Alarm start test passed.");

  // Test: Advance time and check for alarms
  for (uint32_t t = 0; t <= 35; ++t) {
    AlarmID alarm = newTimer.alarm();
    if (t == 0) { // 0th tick timer 0 fires as it begins
      assert(alarm == alarm0);
      DEBUG_LOG("Alarm 1 triggered as expected.");
    } else if (t == 5) { // 6th tick timer 1 fires after 5 ticks of timer 0
      assert(alarm == alarm1);
      DEBUG_LOG("Alarm 2 triggered as expected.");
    } else if (t == 15) { // 16th tick timer 2 fires after 15 ticks of timer 0 and 1
      assert(alarm == alarm2);
      DEBUG_LOG("Alarm 3 triggered as expected.");
    } else if (t == 35) { // 36th tick timer 0 fires against after 35 ticks of 0 1 2
      assert(alarm == alarm0);
      DEBUG_LOG("Alarm 1 triggered as expected.");
    } else {
      assert(alarm == ALARM_NONE);
    }
    m_engine.time().tickClock();
  }

#ifdef VORTEX_LIB
  // Test time simulation APIs in vortex lib builds
  newTimer.init();

  // Add alarms
  AlarmID simAlarm0 = newTimer.addAlarm(5);
  AlarmID simAlarm1 = newTimer.addAlarm(10);
  AlarmID simAlarm2 = newTimer.addAlarm(20);

  newTimer.start();
  assert(newTimer.alarm() == simAlarm0);

  m_engine.time().startSimulation();
  for (uint32_t t = 0; t <= 35; ++t) {
    AlarmID alarm = newTimer.alarm();
    if (t == 0) { // 0th tick timer 0 fires as it begins
      assert(alarm == simAlarm0);
      DEBUG_LOG("Alarm 1 triggered as expected.");
    } else if (t == 5) { // 6th tick timer 1 fires after 5 ticks of timer 0
      assert(alarm == simAlarm1);
      DEBUG_LOG("Alarm 2 triggered as expected.");
    } else if (t == 15) { // 16th tick timer 2 fires after 15 ticks of timer 0 and 1
      assert(alarm == simAlarm2);
      DEBUG_LOG("Alarm 3 triggered as expected.");
    } else if (t == 35) { // 36th tick timer 0 fires against after 35 ticks of 0 1 2
      assert(alarm == simAlarm0);
      DEBUG_LOG("Alarm 1 triggered as expected.");
    } else {
      assert(alarm == ALARM_NONE);
    }
    m_engine.time().tickSimulation();
  }
  m_engine.time().endSimulation();
  DEBUG_LOG("Time simulation tests passed.");
#endif

  DEBUG_LOG("Timer class tests completed successfully.");
}
#endif
