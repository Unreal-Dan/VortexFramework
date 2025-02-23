#include "TimeControl.h"

#include <math.h>

#include "../VortexConfig.h"

#include "../Memory/Memory.h"
#include "../Log/Log.h"

#include "Timings.h"

#include "../Leds/Leds.h"

#if !defined(_WIN32) || defined(WASM)
#include <unistd.h>
#include <time.h>
uint64_t start = 0;
// convert seconds and nanoseconds to microseconds
#define SEC_TO_US(sec) ((sec)*1000000)
#define NS_TO_US(ns) ((ns)/1000)
#else
#include <Windows.h>
static LARGE_INTEGER tps;
static LARGE_INTEGER start;
#endif

#if VORTEX_EMBEDDED == 1
#include <Arduino.h>
#endif

// static members
#if VARIABLE_TICKRATE == 1
uint32_t Time::m_tickrate = DEFAULT_TICKRATE;
#endif
uint32_t Time::m_curTick = 0;
uint32_t Time::m_prevTime = 0;
uint32_t Time::m_firstTime = 0;
#ifdef VORTEX_LIB
uint32_t Time::m_simulationTick = 0;
bool Time::m_isSimulation = false;
bool Time::m_instantTimestep = false;
#endif

// Within this file TICKRATE may refer to the variable member
// or the default tickrate constant based on the configuration
#if VARIABLE_TICKRATE == 1
#define TICKRATE m_tickrate
#else
#define TICKRATE DEFAULT_TICKRATE
#endif

bool Time::init()
{
  m_firstTime = m_prevTime = microseconds();
  m_curTick = 0;
#if VARIABLE_TICKRATE == 1
  m_tickrate = DEFAULT_TICKRATE;
#endif
#ifdef VORTEX_LIB
  m_simulationTick = 0;
  m_isSimulation = false;
  m_instantTimestep = false;
#endif
#if !defined(_WIN32) || defined(WASM)
  start = microseconds();
#else
  QueryPerformanceFrequency(&tps);
  QueryPerformanceCounter(&start);
#endif
  return true;
}

void Time::cleanup()
{
}

void Time::tickClock()
{
  // tick clock forward
  m_curTick++;

#if DEBUG_ALLOCATIONS == 1
  if ((m_curTick % MS_TO_TICKS(1000)) == 0) {
    DEBUG_LOGF("Cur Memory: %u (%u)", cur_memory_usage(), cur_memory_usage_background());
  }
#endif

#ifdef VORTEX_LIB
  if (m_instantTimestep) {
    return;
  }
#endif

  // perform timestep
  uint32_t elapsed_us;
  uint32_t us;
  do {
    us = microseconds();
    // detect rollover of microsecond counter
    if (us < m_prevTime) {
      // calculate wrapped around difference
      elapsed_us = (uint32_t)((UINT32_MAX - m_prevTime) + us);
    } else {
      // otherwise calculate regular difference
      elapsed_us = (uint32_t)(us - m_prevTime);
    }
    // if building anywhere except visual studio then we can run alternate sleep code
    // because in visual studio + windows it's better to just spin and check the high
    // resolution clock instead of trying to sleep for microseconds.
#if !defined(_WIN32) && defined(VORTEX_LIB)
    uint32_t required = (1000000 / TICKRATE);
    uint32_t sleepTime = 0;
    if (required > elapsed_us) {
      // in vortex lib on linux we can just sleep instead of spinning
      // but on embedded we must spin and on windows it actually ends
      // up being more accurate to poll QPF + QPC via microseconds()
      sleepTime = required - elapsed_us;
    }
    Time::delayMicroseconds(sleepTime);
    break;
#endif
    // 1000us per ms, divided by tickrate gives
    // the number of microseconds per tick
  } while (elapsed_us < (1000000 / TICKRATE));

  // store current time
  m_prevTime = microseconds();
}

// the real current time, bypass simulations, used by timers
uint32_t Time::getRealCurtime()
{
  return m_curTick;
}

uint32_t Time::getTickrate()
{
  return TICKRATE;
}

// Set tickrate in Ticks Per Second (TPS)
// The valid range for this is 1 <= x <= 1000000
void Time::setTickrate(uint32_t tickrate)
{
#if VARIABLE_TICKRATE == 1
  if (!tickrate) {
    // can't set 0 tickrate, so 0 sets default
    tickrate = DEFAULT_TICKRATE;
  } else if (tickrate > 1000000) {
    // more than 1 million ticks per second won't work anyway
    tickrate = 1000000;
  }
  // update the tickrate
  m_tickrate = tickrate;
#endif
}

#if VARIABLE_TICKRATE == 1
uint32_t Time::millisecondsToTicks(uint32_t ms)
{
  // 0ms = 0 ticks
  if (!ms) {
    return 0;
  }
  // but anything > 0 ms must be no less than 1 tick
  // otherwise short durations will disappear at low
  // tickrates
  uint32_t ticks = (ms * TICKRATE) / 1000;
  if (!ticks) {
    return 1;
  }
  return ticks;
}
#endif

uint32_t Time::microseconds()
{
#ifndef VORTEX_LIB // Embedded avr devices
  // arduino micros, or whatever micro implementation you have chosen for the embedded device
  return micros();
#elif defined(_WIN32) // windows
  LARGE_INTEGER now;
  QueryPerformanceCounter(&now);
  if (!tps.QuadPart) {
    return 0;
  }
  // yes, this will overflow, that's how arduino microseconds() works *shrug*
  return (unsigned long)((now.QuadPart - start.QuadPart) * 1000000 / tps.QuadPart);
#else // linux/wasm/etc
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  uint64_t us = SEC_TO_US((uint64_t)ts.tv_sec) + NS_TO_US((uint64_t)ts.tv_nsec);
  return (unsigned long)us;
#endif
}

void Time::delayMicroseconds(uint32_t us)
{
#if VORTEX_EMBEDDED == 1 || defined(_WIN32)
  uint32_t newtime = microseconds() + us;
  while (microseconds() < newtime) {
    // busy loop
  }
#else
  usleep(us);
#endif
}

void Time::delayMilliseconds(uint32_t ms)
{
#if VORTEX_EMBEDDED == 1
  delay(ms);
#elif defined(_WIN32)
  Sleep(ms);
#else
  usleep(ms * 1000);
#endif
}

#ifdef VORTEX_LIB

// Start a time simulation, while the simulation is active you can
// increment the 'current time' with tickSimulation() then when you
// call endSimulation() the currentTime will be restored
uint32_t Time::startSimulation()
{
  m_simulationTick = 0;
  m_isSimulation = true;
  return (uint32_t)getCurtime();
}

// Tick a time simulation forward, returning the next tick
uint32_t Time::tickSimulation()
{
  return ++m_simulationTick;
}

// whether running time simulation
bool Time::isSimulation()
{
  return m_isSimulation;
}

// get the current tick in the simulation
uint32_t Time::getSimulationTick()
{
  return m_simulationTick;
}

// Finish a time simulation
uint32_t Time::endSimulation()
{
  uint32_t endTick = (uint32_t)getCurtime();
  m_simulationTick = 0;
  m_isSimulation = false;
  return endTick;
}

#endif

#if TIMER_TEST == 1
#include <assert.h>

void Time::test()
{
  DEBUG_LOG("Starting Time class tests...");

  // just return immediately when testing the time system,
  // this prevents the actual delay from occurring and
  // allows us to tick forward as fast as we want
  setInstantTimestep(true);

  // Test init function
  assert(init());
  DEBUG_LOG("Init test passed");
  cleanup();

  // Test tickClock function
  uint32_t initialTick = m_curTick;
  tickClock();
  assert(m_curTick == initialTick + 1);
  DEBUG_LOG("tickClock test passed");

  // Test msToTicks function
  assert(msToTicks(1000) == TICKRATE);
  assert(msToTicks(500) == TICKRATE / 2);
  assert(msToTicks(0) == 0);
  DEBUG_LOG("msToTicks test passed");

  // Test getRealCurtime function
  assert(getRealCurtime() == m_curTick);
  DEBUG_LOG("getRealCurtime test passed");

  // Test getTickrate function
  assert(getTickrate() == TICKRATE);
  DEBUG_LOG("getTickrate test passed");

  // Test setTickrate function (only when VARIABLE_TICKRATE is enabled)
#if VARIABLE_TICKRATE == 1
  uint32_t newTickrate = TICKRATE * 2;
  setTickrate(newTickrate);
  assert(getTickrate() == newTickrate);
  DEBUG_LOG("setTickrate test passed");
#endif

#ifdef VORTEX_LIB
  // Test simulation functions
  uint32_t simulationStartTick = startSimulation();
  assert(isSimulation());
  assert(getSimulationTick() == 0);
  DEBUG_LOG("startSimulation test passed");

  uint32_t simulationTick = tickSimulation();
  assert(getSimulationTick() == 1);
  DEBUG_LOGF("tickSimulation test passed, simulationTick: %u", simulationTick);

  uint32_t simulationEndTick = endSimulation();
  assert(!isSimulation());
  assert(simulationEndTick == simulationStartTick + 1);
  DEBUG_LOGF("endSimulation test passed, simulationEndTick: %u", simulationEndTick);
#endif

  DEBUG_LOG("Time class tests completed successfully.");
}
#endif
