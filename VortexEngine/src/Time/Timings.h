#ifndef TIMINGS_H
#define TIMINGS_H

// so that anything which uses a timing can have access to msToTicks
#include "TimeControl.h"
#include "../VortexConfig.h"

// local definition which converts the global configuration that
// is defined in milliseconds into a time in ticks that can be
// used for comparisons in the menu
#define MENU_DURATION_TICKS           Time::msToTicks(MENU_DURATION)
#define MENU_TRIGGER_THRESHOLD_TICKS  Time::msToTicks(MENU_TRIGGER_THRESHOLD)
#define SHORT_CLICK_THRESHOLD_TICKS   Time::msToTicks(SHORT_CLICK_THRESHOLD)

// Strobe Timings
//
// Below are timings for all different kinds of standard
// strobes as defined by the gloving community.  All times 
// are in milliseconds.

// Basic Strobe
#define STROBE_ON_DURATION          5
#define STROBE_OFF_DURATION         8

// Hyperstrobe
#define HYPERSTROBE_ON_DURATION     25
#define HYPERSTROBE_OFF_DURATION    25

// Dops
#define DOPS_ON_DURATION            2
#define DOPS_OFF_DURATION           13

// Dopish
#define DOPISH_ON_DURATION          2
#define DOPISH_OFF_DURATION         7

// Ultradops
#define ULTRADOPS_ON_DURATION       1
#define ULTRADOPS_OFF_DURATION      3

// Strobie
#define STROBIE_ON_DURATION         3
#define STROBIE_OFF_DURATION        22

// Ribbon
#define RIBBON_DURATION             20

#endif
