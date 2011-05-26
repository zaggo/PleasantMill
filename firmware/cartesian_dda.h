/************
 * cartesian_dda
 * 
 * Based on code from Adrian Bowyer 9 May 2009
 * This class controls the movement of the machine.
 * It implements a DDA in five dimensions, so the length of extruded 
 * filament is treated as a variable, just like X, Y, and Z.  
 * Optional: Speed is also a variable, making accelleration and 
 * deceleration automatic.
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
 

#ifndef CARTESIAN_DDA_H
#define CARTESIAN_DDA_H
#include "MachineModel.h"
#include "pins.h"

// Main class for moving the RepRap machine about

class cartesian_dda
{
private:
  FloatPoint target_position;  // Where it's going
  FloatPoint delta_position;   // The difference between the two
  float distance;              // How long the path is
  
  LongPoint current_steps;     // Similar information as above in steps rather than units
  LongPoint target_steps;
  LongPoint delta_steps;
  LongPoint dda_counter;       // DDA error-accumulation variables
  long t_scale;                // When doing lots of t steps, scale them so the DDA doesn't spend for ever on them
  
  volatile bool x_direction;            // Am I going in the + or - direction?
  volatile bool y_direction;
  volatile bool z_direction;
  volatile bool a_direction;
  volatile bool b_direction;
  volatile bool f_direction;

  volatile bool x_can_step;             // Am I not at an endstop?  Have I not reached the target? etc.
  volatile bool y_can_step;
  volatile bool z_can_step;
  volatile bool a_can_step;
  volatile bool b_can_step;
  volatile bool f_can_step;

// Variables for acceleration calculations

  volatile long total_steps;            // The number of steps to take along the longest movement axis
  volatile long stepsMade;
  
  long slowSteps;
  long easeOutTrigger;
  
  long timestep;               // microseconds
  bool nullmove;               // this move is zero length
  volatile bool real_move;     // Flag to know if we've changed something physical
  volatile bool feed_change;   // Flag to know if feedrate has changed
  volatile bool live;          // Flag for when we're plotting a line

// Internal functions that need not concern the user

  // Take a single step

  void do_x_step();               
  void do_y_step();
  void do_z_step();
  void do_a_step();
  void do_b_step();
  
  // Can this axis step?
  
  bool xCanStep(long current, long target, bool dir);
  bool yCanStep(long current, long target, bool dir);
  bool zCanStep(long current, long target, bool dir);
  bool aCanStep(long current, long target, bool dir);
  bool bCanStep(long current, long target, bool dir);
  bool fCanStep(long current, long target, bool dir);
  
  // Read a limit switch
  
  //bool read_switch(byte pin, bool inv);
  
  // Work out the number of microseconds between steps
  
  long calculate_feedrate_delay(const float& feedrate);
  
  // Switch the steppers on and off
  
  void enable_steppers();
  void disable_steppers();
  
  
public:

  cartesian_dda();
  
  // Set where I'm going
  
  void set_target(const FloatPoint& p);
  
  // Start the DDA
  
  void dda_start();
  
  // Do one step of the DDA
  
  void dda_step();
  
  // Are we running at the moment?
  
  bool active();
  
  // Are we extruding at the moment?
  
  //bool extruding();
  
  // Kill - stop all activity and turn off steppers
  void shutdown();
};

// Short functions inline to save memory; particularly useful in the Arduino

inline bool cartesian_dda::active()
{
  return live;
}

//inline bool cartesian_dda::extruding()
//{
//  return live && (current_steps.e != target_steps.e);
//}


inline void cartesian_dda::do_x_step()
{
	digitalWrite(X_STEP_PIN, HIGH);
	digitalWrite(X_STEP_PIN, LOW);
}

inline void cartesian_dda::do_y_step()
{
	digitalWrite(Y_STEP_PIN, HIGH);
	digitalWrite(Y_STEP_PIN, LOW);
}

inline void cartesian_dda::do_z_step()
{
	digitalWrite(Z_STEP_PIN, HIGH);
	digitalWrite(Z_STEP_PIN, LOW);
}

inline void cartesian_dda::do_a_step()
{
	digitalWrite(A_STEP_PIN, HIGH);
	digitalWrite(A_STEP_PIN, LOW);
}

inline void cartesian_dda::do_b_step()
{
	digitalWrite(B_STEP_PIN, HIGH);
	digitalWrite(B_STEP_PIN, LOW);
}

inline long cartesian_dda::calculate_feedrate_delay(const float& feedrate)
{  
        
	// Calculate delay between steps in microseconds.  Here it is in English:
        // (feedrate is in mm/minute, distance is in mm)
	// 60000000.0*distance/feedrate  = move duration in microseconds
	// move duration/total_steps = time between steps for master axis.

	return round( (distance*60000000.0) / (feedrate*(float)total_steps) );	
}

inline bool cartesian_dda::aCanStep(long current, long target, bool dir)
{
	//stop us if we're on target
	return !(target == current);
}

inline bool cartesian_dda::bCanStep(long current, long target, bool dir)
{
	//stop us if we're on target
	return !(target == current);
}

inline bool cartesian_dda::fCanStep(long current, long target, bool dir)
{
	//stop us if we're on target
	return !(target == current);
}

#endif
