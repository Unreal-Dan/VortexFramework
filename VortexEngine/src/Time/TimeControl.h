#ifndef TIME_CONTROL_H
#define TIME_CONTROL_H

#include <inttypes.h>

#include "../VortexConfig.h"
#include "../Leds/LedTypes.h"

#if VARIABLE_TICKRATE == 1
#define MS_TO_TICKS(ms) Time::millisecondsToTicks(ms)
#define SEC_TO_TICKS(s) Time::secondsToTicks(s)
#else
#define MS_TO_TICKS(ms) (uint32_t)(((uint32_t)(ms) * DEFAULT_TICKRATE) / 1000)
#define SEC_TO_TICKS(s) (uint32_t)((uint32_t)(s) * DEFAULT_TICKRATE)
#endif

#ifdef VORTEX_LIB
#define SIMULATION_TICK getSimulationTick()
#else
#define SIMULATION_TICK 0
#endif

class Time
{
  // private unimplemented constructor
  Time();

public:
  // opting for static class here because there should only ever be one
  // Settings control object and I don't like singletons
  static bool init();
  static void cleanup();

  // tick the clock forward to millis()
  static void tickClock();

  // get the current tick, offset by any active simulation (simulation only exists in vortexlib)
  // Exposing this in the header seems to save on space a non negligible amount, it is used a lot
  // and exposing in the header probably allows the compiler to optimize away repititive calls
  static uint32_t getCurtime() { return m_curTick + SIMULATION_TICK; }

  // this ignore simulation time, it's used by timers to make simulations work
  static uint32_t getRealCurtime();

  // Set tickrate in Ticks Per Second (TPS)
  // The valid range for this is 1 <= x <= 1000000
  //
  // Setting a value of 0 will restore the default
  //
  // NOTE: Patterns will not change when this changes, you
  //       must re-create the pattern for the change to take
  //       effect. This is done by design to allow the test
  //       framework to control the speed of patterns.
  //       See PatternBuilder.cpp for more info.
  static void setTickrate(uint32_t tickrate = 0);

  // The current tickrate
  static uint32_t getTickrate();

  // these apis are only present if the tickrate can change
#if VARIABLE_TICKRATE == 1
  // convert milliseconds to a tickcount based on tickrate
  static uint32_t millisecondsToTicks(uint32_t ms);

  // convert seconds to a tickcount based on tickrate
  static uint32_t secondsToTicks(uint32_t sec) { return millisecondsToTicks(sec * 1000); }
#endif

  // Current microseconds since startup, only use this for things like measuring rapid data transfer timings.
  // If you just need to perform regular time checks for a pattern or some logic then use getCurtime() and measure 
  // time in ticks, use the SEC_TO_TICKS() or MS_TO_TICKS() macros to convert timings to measures of ticks for
  // purpose of comparing against getCurtime()
  static uint32_t microseconds();

  // delay for some number of microseconds or milliseconds, these are bad
  static void delayMicroseconds(uint32_t us);
  static void delayMilliseconds(uint32_t ms);

#ifdef VORTEX_LIB
  // Start a time simulation, while the simulation is active you can
  // increment the 'current time' with tickSimulation() and all calls
  // to Time::getCurtime will reflect the changes, then when you call
  // endSimulation() the currentTime will be restored
  static uint32_t startSimulation();

  // Tick a time simulation forward, returning the next tick
  static uint32_t tickSimulation();

  // Whether running a time simulation
  static bool isSimulation();

  // get the current tick in the simulation
  static uint32_t getSimulationTick();

  // Finish a time simulation
  static uint32_t endSimulation();

  // Toggle instant step
  static void setInstantTimestep(bool instant) { m_instantTimestep = instant; }
  static bool isInstantStepping() { return m_instantTimestep; }
#endif

#if TIMER_TEST == 1
  static void test();
#endif

private:
#ifdef VORTEX_EMBEDDED
  static void initMCUTime();
#endif

#if VARIABLE_TICKRATE == 1
  // the number of ticks per second
  static uint32_t m_tickrate;
#endif

  // global tick counter
  static uint32_t m_curTick;

#ifdef VORTEX_LIB
  // the last frame timestamp
  static uint32_t m_prevTime;

  // the first timestamp
  static uint32_t m_firstTime;

  // the current simulation offset, simulations are
  // used to fastforward patterns and colorsets by
  // simulating tick changes and running pattern logic
  // multiple times in a single frame
  static uint32_t m_simulationTick;

  // whether the timer is running a simulation
  static bool m_isSimulation;

  // whether to disable timestep and instantly tick
  static bool m_instantTimestep;
#endif
};

#endif
