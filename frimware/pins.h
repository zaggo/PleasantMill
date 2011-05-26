/************
  * "Pleasant Mill" Firmware
 * Copyright (c) 2011 Eberhard Rensch, Pleasant Software, Offenburg
 * All rights reserved.
 * http://pleasantsoftware.com/developer/3d/pleasant-mill/
 *
 * Parts of this code are based on the RepRap machine firmware by Adrian Bowyer
 * See http://www.reprap.org for more information
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


/* Seeeduino Mega configuration */

#ifndef PINS_H
#define PINS_H

#define DEBUG_PIN        13

#define X_STEP_PIN (byte)48
#define X_DIR_PIN (byte)47
#define X_MIN_PIN (byte)45
#define X_MAX_PIN (byte)53
#define X_ENABLE_PIN (byte)46

#define Y_STEP_PIN (byte)43
#define Y_DIR_PIN (byte)42
#define Y_MIN_PIN (byte)44
#define Y_MAX_PIN (byte)52
#define Y_ENABLE_PIN (byte)40

#define Z_STEP_PIN (byte)35
#define Z_DIR_PIN (byte)34
#define Z_MIN_PIN (byte)41
#define Z_MAX_PIN (byte)37
#define Z_ENABLE_PIN (byte)33

// Heated bed

#define BED_HEATER_PIN (byte)3
#define BED_TEMPERATURE_PIN (byte)1 


//extruder pins

#define A_STEP_PIN (byte)30
#define A_DIR_PIN (byte)15
#define A_ENABLE_PIN (byte)14
#define EXTRUDER_0_HEATER_PIN (byte)2
#define EXTRUDER_0_TEMPERATURE_PIN (byte)0 

#define B_STEP_PIN (byte)4
#define B_DIR_PIN (byte)5
#define B_ENABLE_PIN (byte)6
#define EXTRUDER_1_HEATER_PIN (byte)13
#define EXTRUDER_1_TEMPERATURE_PIN (byte)2 


// UI Pins
#define JOYSTICK_P (byte)25
#define JOYSTICK_A (byte)23
#define JOYSTICK_B (byte)24
#define JOYSTICK_C (byte)22
#define JOYSTICK_D (byte)26
#define EMERGENCY_STOP (byte)27
#endif
