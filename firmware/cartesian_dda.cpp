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
 
#include <stdio.h>
#include "configuration.h"
#include "pins.h"
#include "vectors.h"
#include "cartesian_dda.h"
#include "interruptHandling.h"


cartesian_dda::cartesian_dda()
{
	live = false;
	nullmove = false;
        
	// Default is going forward
	x_direction = true;
	y_direction = true;
	z_direction = true;
	a_direction = true;
	b_direction = true;
	f_direction = true;
        
// Default to the origin and not going anywhere
  
	target_position.x = 0.0;
	target_position.y = 0.0;
	target_position.z = 0.0;
	target_position.a = 0.0;
	target_position.b = 0.0;
	target_position.f = SLOW_FEEDRATE;

// Set up the pin directions
	pinMode(X_STEP_PIN, OUTPUT);
	pinMode(X_DIR_PIN, OUTPUT);

	pinMode(Y_STEP_PIN, OUTPUT);
	pinMode(Y_DIR_PIN, OUTPUT);

	pinMode(Z_STEP_PIN, OUTPUT);
	pinMode(Z_DIR_PIN, OUTPUT);

#ifdef X_ENABLE_PIN
	pinMode(X_ENABLE_PIN, OUTPUT);
#endif
#ifdef Y_ENABLE_PIN
	pinMode(Y_ENABLE_PIN, OUTPUT);
#endif
#ifdef Z_ENABLE_PIN
	pinMode(Z_ENABLE_PIN, OUTPUT);
#endif

	//turn the motors off at the start.
	disable_steppers();

#if ENDSTOPS_MIN_ENABLED == 1
	pinMode(X_MIN_PIN, INPUT);
	pinMode(Y_MIN_PIN, INPUT);
	pinMode(Z_MIN_PIN, INPUT);

// pullup resistors:
#if OPTO_PULLUPS_INTERNAL == 1
        digitalWrite(X_MIN_PIN,HIGH);
        digitalWrite(Y_MIN_PIN,HIGH);
        digitalWrite(Z_MIN_PIN,HIGH);
#endif
#endif

#if ENDSTOPS_MAX_ENABLED == 1
	pinMode(X_MAX_PIN, INPUT);
	pinMode(Y_MAX_PIN, INPUT);
	pinMode(Z_MAX_PIN, INPUT);
// pullup resistors:
#if OPTO_PULLUPS_INTERNAL == 1
        digitalWrite(X_MAX_PIN,HIGH);
        digitalWrite(Y_MAX_PIN,HIGH);
        digitalWrite(Z_MAX_PIN,HIGH);
#endif
#endif
	
}

void cartesian_dda::set_target(const FloatPoint& p)
{
	stepsMade = 0;
	target_position = p;
	nullmove = false;
        
    FloatPoint locPos = sharedMachineModel.localPosition;
    
	//figure our deltas.
	delta_position = fabsv(target_position - locPos);
        
	// The feedrate values refer to distance in (X, Y, Z) space, so ignore e and f
	// values unless they're the only thing there.
	FloatPoint squares = delta_position*delta_position;
	distance = squares.x + squares.y + squares.z;
	
	// If we are 0, only thing changing is a
	if(distance < SMALL_DISTANCE2)
		distance = squares.a;
	// If we are 0, only thing changing is b
	if(distance < SMALL_DISTANCE2)
		distance = squares.b;
	// If we are still 0, only thing changing is f
	if(distance < SMALL_DISTANCE2)
		distance = squares.f;
	distance = sqrt(distance);          
                                                                                   			
	//set our steps current, target, and delta
	FloatPoint units = sharedMachineModel.returnUnits();
				
	current_steps = to_steps(units, locPos+sharedMachineModel.localZeroOffset); // Calculate Steps always absolute, this enables us to determine virtual endstop hits
	target_steps = to_steps(units, target_position+sharedMachineModel.localZeroOffset);
	delta_steps = absv(target_steps - current_steps);

//	Serial.print("locX:");
//	Serial.print(locPos.x);
//	Serial.print(" localZeroOffsetY:");
//	Serial.print(sharedMachineModel.localZeroOffset.x);
//	Serial.print(" tarX:");
//	Serial.print(target_position.x);
//	Serial.print(" current_steps:");
//	Serial.print(current_steps.x);
//	Serial.print(" target_steps:");
//	Serial.print(target_steps.x);
//	Serial.print(" delta_steps:");
//	Serial.print(delta_steps.x);
//	FloatPoint absolute = from_steps(units, current_steps);
//	Serial.print(" current:");
//	Serial.print(absolute.x-sharedMachineModel.localZeroOffset.x);
//	absolute = from_steps(units, target_steps);
//	Serial.print(" target:");
//	Serial.println(absolute.x-sharedMachineModel.localZeroOffset.x);

	// find the dominant axis.
	// NB we ignore the f values here, as it takes no time to take a step in time :-)

	total_steps = max(delta_steps.x, delta_steps.y);
	total_steps = max(total_steps, delta_steps.z);
	total_steps = max(total_steps, delta_steps.a);
	total_steps = max(total_steps, delta_steps.b);
  
	// If we're not going anywhere, flag the fact
	if(total_steps == 0)
	{
		nullmove = true;
		sharedMachineModel.localPosition=p;
	}    
	else
	{
#if EASEINOUT
		slowSteps = SLOW_FEEDRATE*units.f;
		if(target_steps.f<=slowSteps)
		{
       		current_steps.f = target_steps.f;
 			easeOutTrigger=total_steps;
			delta_steps.f=total_steps/EASE_INTERLEAF;
			t_scale = 1;
		}
		else
		{
			current_steps.f = slowSteps;
			delta_steps.f = abs(target_steps.f - current_steps.f)*EASE_INTERLEAF;
			if(total_steps<delta_steps.f)
				easeOutTrigger = total_steps/2;
			else
				easeOutTrigger = total_steps-delta_steps.f;
			
			t_scale = 1;
			delta_steps.f=total_steps/EASE_INTERLEAF;
		}
			
	//		Serial.print("TotalSteps ");
	//		Serial.println(total_steps);
	//		Serial.print("easeOutTrigger ");
	//		Serial.println(easeOutTrigger);
	//		Serial.print("current_steps ");
	//		Serial.println(current_steps.f);
#else

#if	ACCELERATION != ACCELERATION_ON
        current_steps.f = target_steps.f;
#endif

        delta_steps.f = abs(target_steps.f - current_steps.f);
        
        // Rescale the feedrate so it doesn't take lots of steps to do
        t_scale = 1;
        if(delta_steps.f > total_steps)
        {
            t_scale = delta_steps.f/total_steps;
            if(t_scale >= 3)
            {
              target_steps.f = target_steps.f/t_scale;
              current_steps.f = current_steps.f/t_scale;
              delta_steps.f = abs(target_steps.f - current_steps.f);
              if(delta_steps.f > total_steps)
                total_steps =  delta_steps.f;
            } 
            else
            {
              t_scale = 1;
              total_steps =  delta_steps.f;
            }
        } 
#endif
		//what is our direction?
        
		x_direction = (target_position.x >= locPos.x);
		y_direction = (target_position.y >= locPos.y);
		z_direction = (target_position.z >= locPos.z);
        a_direction = (target_position.a >= locPos.a);
        b_direction = (target_position.b >= locPos.b);
		f_direction = (target_position.f >= locPos.f);


		dda_counter.x = -total_steps/2;
		dda_counter.y = dda_counter.x;
		dda_counter.z = dda_counter.x;
        dda_counter.a = dda_counter.x;
        dda_counter.b = dda_counter.x;
        dda_counter.f = dda_counter.x;
  
        sharedMachineModel.localPosition=p;
	}
}

// This function is called by an interrupt.  Consequently interrupts are off for the duration
// of its execution.  Consequently it has to be as optimised and as fast as possible.


void cartesian_dda::dda_step()
{  
	if(live)
	{
		do
		{
			x_can_step = xCanStep(current_steps.x, target_steps.x, x_direction);
			y_can_step = yCanStep(current_steps.y, target_steps.y, y_direction);
			z_can_step = zCanStep(current_steps.z, target_steps.z, z_direction);
			a_can_step = aCanStep(current_steps.a, target_steps.a, a_direction);
			b_can_step = bCanStep(current_steps.b, target_steps.b, b_direction);
			
			if(stepsMade>easeOutTrigger)
				f_can_step = fCanStep(current_steps.f, slowSteps, f_direction);
            else
				f_can_step = fCanStep(current_steps.f, target_steps.f, f_direction);

			real_move = false;
			if (x_can_step)
			{
				dda_counter.x += delta_steps.x;
				if (dda_counter.x > 0)
				{
					do_x_step();
					real_move = true;
					dda_counter.x -= total_steps;
				
					if (x_direction)
						current_steps.x++;
					else
						current_steps.x--;
				}
			}

			if (y_can_step)
			{
				dda_counter.y += delta_steps.y;
				if (dda_counter.y > 0)
				{
					do_y_step();
					real_move = true;
					dda_counter.y -= total_steps;

					if (y_direction)
						current_steps.y++;
					else
						current_steps.y--;
				}
			}
		
			if (z_can_step)
			{
				dda_counter.z += delta_steps.z;
				if (dda_counter.z > 0)
				{
					do_z_step();
					real_move = true;
					dda_counter.z -= total_steps;

					if (z_direction)
						current_steps.z++;
					else
						current_steps.z--;
				}
			}

			if (a_can_step)
			{
				dda_counter.a += delta_steps.a;
				if (dda_counter.a > 0)
				{
					do_a_step();
					real_move = true;
					dda_counter.a -= total_steps;
					
					if (a_direction)
						current_steps.a++;
					else
						current_steps.a--;
				}
			}

			if (b_can_step)
			{
				dda_counter.b += delta_steps.b;
				if (dda_counter.b > 0)
				{
					do_b_step();
					real_move = true;
					dda_counter.b -= total_steps;
					
					if (b_direction)
						current_steps.b++;
					else
						current_steps.b--;
				}
			}

#if EASEINOUT
			if(stepsMade>easeOutTrigger)
			{
				dda_counter.f += delta_steps.f;
				if (dda_counter.f > 0)
				{
					dda_counter.f -= total_steps;
					if(current_steps.f>SLOW_FEEDRATE)
						current_steps.f--;
					feed_change = true;
				}
			}
			else if(current_steps.f<target_steps.f)
			{
				dda_counter.f += delta_steps.f;
				if (dda_counter.f > 0)
				{
					dda_counter.f -= total_steps;
					current_steps.f++;
					feed_change = true;
				}
			}
//			Serial.print("stepsMade ");
//			Serial.print(stepsMade);
//			Serial.print(" current:steps ");
//			Serial.println(current_steps.f);
#else
			if (f_can_step)
			{
				dda_counter.f += delta_steps.f;
				if (dda_counter.f > 0)
				{
					dda_counter.f -= total_steps;
					if (f_direction)
						current_steps.f++;
					else
						current_steps.f--;
					feed_change = true;
				} else
					feed_change = false;
			}
#endif		

			stepsMade++;
				
			// wait for next step.
			// Use milli- or micro-seconds, as appropriate
			// If the only thing that changed was f keep looping
			if(real_move && feed_change)
			{
				timestep = t_scale*current_steps.f;
				timestep = calculate_feedrate_delay((float) timestep);
				setTimer(timestep);
			}
			feed_change = false;
		} while (!real_move && f_can_step);
  
		live = (x_can_step || y_can_step || z_can_step  || a_can_step || b_can_step || f_can_step);

		// Wrap up at the end of a line
		if(!live)
		{
//			Serial.print("Steps Made ");
//			Serial.println(stepsMade);
			
			disable_steppers();
			setTimer(DEFAULT_TICK);
		}    
	}
}


// Run the DDA
void cartesian_dda::dda_start()
{    
	// Set up the DDA
	if(!nullmove)
	{
		//set our direction pins as well
		byte d = 1;
#if INVERT_X_DIR == 1
		if(x_direction)
			d = 0;
#else
		if(!x_direction)
			d = 0;	
#endif
		digitalWrite(X_DIR_PIN, d);
			
		d = 1;
#if INVERT_Y_DIR == 1
		if(y_direction)
			d = 0;
#else
		if(!y_direction)
			d = 0;	
#endif
		digitalWrite(Y_DIR_PIN, d);
			
		d = 1;
#if INVERT_Z_DIR == 1
		if(z_direction)
			d = 0;
#else
		if(!z_direction)
			d = 0;	
#endif
		digitalWrite(Z_DIR_PIN, d);
	
		d = 1;
#if INVERT_A_DIR == 1
		if(a_direction)
			d = 0;
#else
		if(!a_direction)
			d = 0;	
#endif
	  digitalWrite(A_DIR_PIN, d);
	  
		d = 1;
#if INVERT_B_DIR == 1
		if(b_direction)
			d = 0;
#else
		if(!b_direction)
			d = 0;	
#endif
		digitalWrite(B_DIR_PIN, d);
	  
		//turn on steppers to start moving =)
		enable_steppers();
	
		setTimer(DEFAULT_TICK);
		live = true;
		feed_change = true; // force timer setting on the first call to dda_step()
	}
}

void cartesian_dda::enable_steppers()
{
#ifdef X_ENABLE_PIN 
  if(delta_steps.x)
    digitalWrite(X_ENABLE_PIN, ENABLE_ON);
#endif
#ifdef Y_ENABLE_PIN
  if(delta_steps.y)    
    digitalWrite(Y_ENABLE_PIN, ENABLE_ON);
#endif
#ifdef Z_ENABLE_PIN
  if(delta_steps.z)
    digitalWrite(Z_ENABLE_PIN, ENABLE_ON);
#endif
#ifdef A_ENABLE_PIN
  if(delta_steps.a)
    digitalWrite(A_ENABLE_PIN, ENABLE_ON);
#endif  
#ifdef B_ENABLE_PIN
  if(delta_steps.b)
    digitalWrite(B_ENABLE_PIN, ENABLE_ON);
#endif  
}



void cartesian_dda::disable_steppers()
{
	//disable our steppers
#if DISABLE_X
	digitalWrite(X_ENABLE_PIN, !ENABLE_ON);
#endif
#if DISABLE_Y
	digitalWrite(Y_ENABLE_PIN, !ENABLE_ON);
#endif
#if DISABLE_Z
	digitalWrite(Z_ENABLE_PIN, !ENABLE_ON);
#endif
#if DISABLE_A
	digitalWrite(A_ENABLE_PIN, !ENABLE_ON);
#endif
#if DISABLE_B
	digitalWrite(B_ENABLE_PIN, !ENABLE_ON);
#endif        
}

void cartesian_dda::shutdown()
{
  live = false;
  nullmove = false;
  target_steps = current_steps;
  disable_steppers();
}

bool cartesian_dda::xCanStep(long current, long target, bool dir)
{
	bool canStep = true;
    bool endstopHit = false;

#if ENDSTOPS_MIN_ENABLED == 1
	//stop us if we're home and still going lower
	#if X_ENDSTOP_INVERTING
	endstopHit = (!dir && !digitalRead(X_MIN_PIN));
	#else
	endstopHit = (!dir && digitalRead(X_MIN_PIN));
	#endif
	canStep = sharedMachineModel.checkEndstops(X_LOW_HIT, endstopHit, current, dir);
#endif

#if ENDSTOPS_MAX_ENABLED == 1
	//stop us if we're at max and still going higher
	if(canStep)
	{
		#if X_ENDSTOP_INVERTING
		endstopHit = (dir && !digitalRead(X_MAX_PIN) );
		#else
		endstopHit = (dir && digitalRead(X_MAX_PIN) );
		#endif
		canStep = sharedMachineModel.checkEndstops(X_HIGH_HIT, endstopHit, current, dir);
	}
#endif
        
	//stop us if we're on target
	if (canStep && target == current)
		canStep = false;
   
   	return canStep;
}

bool cartesian_dda::yCanStep(long current, long target, bool dir)
{
	bool canStep = true;
    bool endstopHit = false;

#if ENDSTOPS_MIN_ENABLED == 1
	//stop us if we're home and still going lower
	#if Y_ENDSTOP_INVERTING
	endstopHit = (!dir && !digitalRead(Y_MIN_PIN));
	#else
	endstopHit = (!dir && digitalRead(Y_MIN_PIN));
	#endif
	canStep = sharedMachineModel.checkEndstops(Y_LOW_HIT, endstopHit, current, dir);
#endif

#if ENDSTOPS_MAX_ENABLED == 1
	//stop us if we're at max and still going higher
	if(canStep)
	{
		#if Y_ENDSTOP_INVERTING
		endstopHit = (dir && !digitalRead(Y_MAX_PIN) );
		#else
		endstopHit = (dir && digitalRead(Y_MAX_PIN) );
		#endif
		canStep = sharedMachineModel.checkEndstops(Y_HIGH_HIT, endstopHit, current, dir);
	}
#endif
        
	//stop us if we're on target
	if (canStep && target == current)
		canStep = false;
   
   	return canStep;
}

bool cartesian_dda::zCanStep(long current, long target, bool dir)
{
	bool canStep = true;
    bool endstopHit = false;

#if ENDSTOPS_MIN_ENABLED == 1
	//stop us if we're home and still going lower
	#if Y_ENDSTOP_INVERTING
	endstopHit = (!dir && !digitalRead(Z_MIN_PIN));
	#else
	endstopHit = (!dir && digitalRead(Z_MIN_PIN));
	#endif
	canStep = sharedMachineModel.checkEndstops(Z_LOW_HIT, endstopHit, current, dir);
#endif

#if ENDSTOPS_MAX_ENABLED == 1
	//stop us if we're at max and still going higher
	if(canStep)
	{
		#if Y_ENDSTOP_INVERTING
		endstopHit = (dir && !digitalRead(Z_MAX_PIN) );
		#else
		endstopHit = (dir && digitalRead(Z_MAX_PIN) );
		#endif
		canStep = sharedMachineModel.checkEndstops(Z_HIGH_HIT, endstopHit, current, dir);
	}
#endif
        
	//stop us if we're on target
	if (canStep && target == current)
		canStep = false;
   
   	return canStep;
}
