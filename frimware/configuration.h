/************
 * Machine Configuration
 * 
 * "Pleasant Mill" Firmware
 * Copyright (c) 2011 Eberhard Rensch, Pleasant Software, Offenburg
 * All rights reserved.
 * http://pleasantsoftware.com/developer/3d/pleasant-mill/
 *
 *  This program is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free Software 
 *  Foundation; either version 3 of the License, or (at your option) any later 
 *  version.
 * 
 *  This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 *  PARTICULAR PURPOSE. See the GNU General Public License for more details. 
 *
 *  You should have received a copy of the GNU General Public License along with 
 *  this program; if not, see <http://www.gnu.org/licenses>.
 * 
 */
 
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "vectors.h"

#define ENABLE_PIN_STATE_NORMAL 0
#define ENABLE_PIN_STATE_INVERTING 1
#define  HAS_NO_ENABLE_LINES 0 
#define  HAS_ENABLE_LINES 1
#define ACCELERATION_ON 1
#define ACCELERATION_OFF 0

#define FW_VERSION "0.4.1"

// Config

#define ENDSTOPS_MIN_ENABLED 1
#define ENDSTOPS_MAX_ENABLED 1
#define OPTO_PULLUPS_INTERNAL 1
#define X_ENDSTOP_INVERTING 1
#define Y_ENDSTOP_INVERTING 1
#define Z_ENDSTOP_INVERTING 1

#define MICROSTEPPING 8L
#define X_STEPS_PER_MM   MICROSTEPPING*100L
#define Y_STEPS_PER_MM   MICROSTEPPING*100L
#define Z_STEPS_PER_MM   MICROSTEPPING*100L
#define A_STEPS_PER_MM   MICROSTEPPING*100L
#define B_STEPS_PER_MM   MICROSTEPPING*100L

#define INVERT_X_DIR 1
#define INVERT_Y_DIR 1
#define INVERT_Z_DIR 0
#define INVERT_A_DIR 0
#define INVERT_B_DIR 0

#define FAST_XY_FEEDRATE 1100.0
#define FAST_Z_FEEDRATE  1100.0

#define ACCELERATION  ACCELERATION_OFF
#define EASEINOUT 1

#define ENABLE_LINES HAS_ENABLE_LINES  
#define ENABLE_PIN_STATE ENABLE_PIN_STATE_INVERTING

#define DISABLE_X 0
#define DISABLE_Y 0
#define DISABLE_Z 0
#define DISABLE_A 0
#define DISABLE_B 0

#define X_STEPS_PER_INCH (X_STEPS_PER_MM*INCHES_TO_MM) // *RO
#define Y_STEPS_PER_INCH (Y_STEPS_PER_MM*INCHES_TO_MM) // *RO
#define Z_STEPS_PER_INCH (Z_STEPS_PER_MM*INCHES_TO_MM) // *RO
#define A_STEPS_PER_INCH (A_STEPS_PER_MM*INCHES_TO_MM) // *RO
#define B_STEPS_PER_INCH (B_STEPS_PER_MM*INCHES_TO_MM) // *RO

// Data for acceleration calculations - change the ACCELERATION variable above, not here
#if ACCELERATION == ACCELERATION_ON
#define SLOW_FEEDRATE 150.0 // Speed from which to start accelerating
#endif
#if EASEINOUT
#define SLOW_FEEDRATE 500.0
#define EASE_INTERLEAF 2 	// Geater values increase the ease in/out duration
#endif

#if ENABLE_PIN_STATE == ENABLE_PIN_STATE_INVERTING 
#define ENABLE_ON LOW        // *RO
#else                        // *RO
#define ENABLE_ON HIGH       // *RO
#endif                       // *RO

// if we are not using aceleration code, we define these constants just to prevent compile errors on the unused 'f' coordinate of the DDA
#ifndef SLOW_FEEDRATE
#define SLOW_FEEDRATE 0.0 
#endif

//-----------------------------------------------------------------------------------------------
// IMMUTABLE (READONLY) CONSTANTS GO HERE:
//-----------------------------------------------------------------------------------------------
// The width of Henry VIII's thumb (or something).
#define INCHES_TO_MM 25.4 // *RO

// The temperature routines get called each time the main loop
// has gone round this many times
#define SLOW_CLOCK 2000

// The speed at which to talk with the host computer; default is 19200=
#define HOST_BAUD 115200 // *RO

// The number of mm below which distances are insignificant (one tenth the
// resolution of the machine is the default value).
#define SMALL_DISTANCE 0.01 // *RO

// Useful to have its square
#define SMALL_DISTANCE2 (SMALL_DISTANCE*SMALL_DISTANCE) // *RO

//our command string length
#define COMMAND_SIZE 128 // *RO

// Our response string length
#define RESPONSE_SIZE 256 // *RO

// The size of the movement buffer
#define BUFFER_SIZE 4 // *RO

// Number of microseconds between timer interrupts when no movement
// is happening
#define DEFAULT_TICK (long)1000 // *RO

// What delay() value to use when waiting for things to free up in milliseconds
#define WAITING_DELAY 1 // *RO

// Bit flags for hit endstops

#define X_LOW_HIT 1
#define Y_LOW_HIT 2
#define Z_LOW_HIT 4
#define X_HIGH_HIT 8
#define Y_HIGH_HIT 16
#define Z_HIGH_HIT 32

#define MACHINE_MAX_X_MM	180.f
#define MACHINE_MAX_Y_MM	145.f
#define MACHINE_MAX_Z_MM	80.f
#define  MACHINE_MAX_X_STEPS	(X_STEPS_PER_MM*(long)MACHINE_MAX_X_MM)
#define  MACHINE_MAX_Y_STEPS	(Y_STEPS_PER_MM*(long)MACHINE_MAX_Y_MM)
#define  MACHINE_MAX_Z_STEPS	(Z_STEPS_PER_MM*(long)MACHINE_MAX_Z_MM)

#endif

