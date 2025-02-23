#ifndef TIME_CONTROL_H
#define TIME_CONTROL_H

#include <inttypes.h>

#include "../VortexConfig.h"
#include "../Leds/LedTypes.h"

#if defined(_WIN32) && !defined(WASM)
#include <Windows.h>
#endif

class VortexEngine;

#if VARIABLE_TICKRATE == 1
#define MS_TO_TICKS(ms) m_engine.time().millisecondsToTicks(ms)
#define SEC_TO_TICKS(s) m_engine.time().secondsToTicks(s)
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
public:
  // private unimplemented constructor
  Time(VortexEngine &engine);
  ~Time();

  // opting for class here because there should only ever be one
  // Settings control object and I don't like singletons
  bool init();
  void cleanup();

  // tick the clock forward to millis()
  void tickClock();

  // get the current tick, offset by any active simulation (simulation only exists in vortexlib)
  // Exposing this in the header seems to save on space a non negligible amount, it is used a lot
  // and exposing in the header probably allows the compiler to optimize away repititive calls
  uint32_t getCurtime() { return m_curTick + SIMULATION_TICK; }

  // this ignore simulation time, it's used by timers to make simulations work
  uint32_t getRealCurtime();

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
  void setTickrate(uint32_t tickrate = 0);

  // The current tickrate
  uint32_t getTickrate();

  // these apis are only present if the tickrate can change
#if VARIABLE_TICKRATE == 1
  // convert milliseconds to a tickcount based on tickrate
  uint32_t millisecondsToTicks(uint32_t ms);

  // convert seconds to a tickcount based on tickrate
  uint32_t secondsToTicks(uint32_t sec) { return millisecondsToTicks(sec * 1000); }
#endif

  // Current microseconds since startup, only use this for things like measuring rapid data transfer timings.
  // If you just need to perform regular time checks for a pattern or some logic then use getCurtime() and measure 
  // time in ticks, use the SEC_TO_TICKS() or MS_TO_TICKS() macros to convert timings to measures of ticks for
  // purpose of comparing against getCurtime()
  uint32_t microseconds();

  // delay for some number of microseconds or milliseconds, these are bad
  void delayMicroseconds(uint32_t us);
  void delayMilliseconds(uint32_t ms);

#ifdef VORTEX_LIB
  // Start a time simulation, while the simulation is active you can
  // increment the 'current time' with tickSimulation() and all calls
  // to Time::getCurtime will reflect the changes, then when you call
  // endSimulation() the currentTime will be restored
  uint32_t startSimulation();

  // Tick a time simulation forward, returning the next tick
  uint32_t tickSimulation();

  // Whether running a time simulation
  bool isSimulation();

  // get the current tick in the simulation
  uint32_t getSimulationTick();

  // Finish a time simulation
  uint32_t endSimulation();

  // Toggle instant step
  void setInstantTimestep(bool instant) { m_instantTimestep = instant; }
  bool isInstantStepping() { return m_instantTimestep; }
#endif

#if TIMER_TEST == 1
  void test();
#endif

private:
  // reference to engine
  VortexEngine &m_engine;

#if !defined(_WIN32) || defined(WASM)
  uint64_t start = 0;
#else
  LARGE_INTEGER tps;
  LARGE_INTEGER start;
#endif

#if VARIABLE_TICKRATE == 1
  // the number of ticks per second
  uint32_t m_tickrate;
#endif

  // global tick counter
  uint32_t m_curTick;

  // the last frame timestamp
  uint32_t m_prevTime;

  // the first timestamp
  uint32_t m_firstTime;

#ifdef VORTEX_LIB
  // the current simulation offset, simulations are
  // used to fastforward patterns and colorsets by
  // simulating tick changes and running pattern logic
  // multiple times in a single frame
  uint32_t m_simulationTick;

  // whether the timer is running a simulation
  bool m_isSimulation;

  // whether to disable timestep and instantly tick
  bool m_instantTimestep;
#endif
};

#endif
