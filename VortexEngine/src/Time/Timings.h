#ifndef TIMINGS_H
#define TIMINGS_H

// so that anything which uses a timing can have access to msToTicks
#include "TimeControl.h"
#include "../VortexConfig.h"

// these definitions pass the global configs defined in milliseconds from vortexconfig 
// through the MS_TO_TICKS macro which calculates the corresponding number of ticks for
// the given duration at the current tickrate, on embedded this is 1 to 1 but in vortexlib
// the tickrate can be adjusted so these will expand out to function calls to millisecondsToTicks()
#define MENU_TRIGGER_THRESHOLD_TICKS  MS_TO_TICKS(MENU_TRIGGER_TIME)
#define SHORT_CLICK_THRESHOLD_TICKS   MS_TO_TICKS(CLICK_THRESHOLD)
#define CONSECUTIVE_WINDOW_TICKS      MS_TO_TICKS(CONSECUTIVE_WINDOW)
#define AUTO_RANDOM_DELAY_TICKS       MS_TO_TICKS(AUTO_RANDOM_DELAY)
#define UNLOCK_WAKE_WINDOW_TICKS      MS_TO_TICKS(UNLOCK_WAKE_WINDOW)
#define SLEEP_ENTER_THRESHOLD_TICKS   MS_TO_TICKS(SLEEP_TRIGGER_TIME)
#define SLEEP_WINDOW_THRESHOLD_TICKS  MS_TO_TICKS(SLEEP_WINDOW_TIME)
#define FORCE_SLEEP_THRESHOLD_TICKS   MS_TO_TICKS(FORCE_SLEEP_TIME)
#define ONE_CLICK_THRESHOLD_TICKS     MS_TO_TICKS(ONE_CLICK_MODE_TRHESHOLD)
#define DELETE_THRESHOLD_TICKS        MS_TO_TICKS(COL_DELETE_THRESHOLD)
#define DELETE_CYCLE_TICKS            MS_TO_TICKS(COL_DELETE_CYCLE)
#define FACTORY_RESET_THRESHOLD_TICKS MS_TO_TICKS(RESET_HOLD_TIME)
#define MAX_SERIAL_CHECK_INTERVAL     MS_TO_TICKS(SERIAL_CHECK_TIME)
#define MAX_TIMEOUT_DURATION          MS_TO_TICKS(IR_RECEIVER_TIMEOUT_DURATION)
#define MAX_WAIT_DURATION             MS_TO_TICKS(IR_SENDER_WAIT_DURATION)
#define ADV_MENU_DURATION_TICKS       MS_TO_TICKS(ADVANCED_MENU_ENTER_DURATION)

// Strobe Timings
//
// Below are timings for all different kinds of standard
// strobes as defined by the gloving community.  All times
// are in milliseconds.

// Basic Strobe
#define STROBE_ON_DURATION          6
#define STROBE_OFF_DURATION         6

// Hyperstrobe
#define HYPERSTROBE_ON_DURATION     16
#define HYPERSTROBE_OFF_DURATION    20

// Dops
#define PICOSTROBE_ON_DURATION      6
#define PICOSTROBE_OFF_DURATION     40

// Dopy
#define DOPS_ON_DURATION            1
#define DOPS_OFF_DURATION           10

// Ultradops
#define ULTRADOPS_ON_DURATION       1
#define ULTRADOPS_OFF_DURATION      3

// Strobie
#define STROBIE_ON_DURATION         2
#define STROBIE_OFF_DURATION        28

// Signal
#define SIGNAL_ON_DURATION          10
#define SIGNAL_OFF_DURATION         296

// A blink speed good for blends
#define BLEND_ON_DURATION           2
#define BLEND_OFF_DURATION          13

// Ribbon
#define RIBBON_DURATION             6

// Blink Timings
//
// Below are blink timings used for various menus and indication blinks

// Bulb selection blink speed
#define BULB_SELECT_ON_MS           150
#define BULB_SELECT_OFF_MS          50

// Exit Menu blink speed
#define EXIT_MENU_ON_MS             300
#define EXIT_MENU_OFF_MS            150

#endif
