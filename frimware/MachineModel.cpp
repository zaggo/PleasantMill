/************
 * Machine Model
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
 
#include "MachineModel.h"
#include "cartesian_dda.h"
#include "LCDUI.h"
#include "hostcom.h"
#include "Persistent.h"

extern hostcom talkToHost;

static cartesian_dda cdda0;
static cartesian_dda cdda1;
static cartesian_dda cdda2;
static cartesian_dda cdda3;

static LcdUi  lcdUi;

MachineModel::MachineModel()
{
  cdda[0] = &cdda0;
  cdda[1] = &cdda1;  
  cdda[2] = &cdda2;  
  cdda[3] = &cdda3;
  head = 0;
  tail = 0;
  led = false;
	
  absolutePositionValid=false; // Until the first hardware homing
  endstop_hits = 0;
  localPosition.f=SLOW_FEEDRATE;
  receiving = false;
  
  clearanceIncrement=2.5; // TODO: Systemparameter, should be read from EEPROM
  
  setUnits(true);		// Default units are mm
  setAbsMode(true);		// Default is absolute mode
  setRetractMode(true); // Default is Retract to old Z mode
  setRetractHeight(0.);
  stickyQ = 0.;
  stickyP = 0.;
}

void MachineModel::startup()
{
  lcdUi.startup();
  emergencyStop = false;
}

// This does a hard stop.  It disables interrupts, turns off all the motors 
// (regardless of DISABLE_X etc), and calls extruder.shutdown() for each
// extruder.  It then spins in an endless loop, never returning.  The only
// way out is to press the reset button.

void MachineModel::shutdown()
{
  lcdUi.shutdown();

  // No more stepping or other interrupts
  cli();
  
   
  // Delete everything in the ring buffer
  cancelAndClearQueue();
  
#if ENABLE_LINES == HAS_ENABLE_LINES
// Motors off
// Note - we ignore DISABLE_X etc here; we are
// definitely turning everything off.
  digitalWrite(X_ENABLE_PIN, !ENABLE_ON);
  digitalWrite(Y_ENABLE_PIN, !ENABLE_ON);
  digitalWrite(Z_ENABLE_PIN, !ENABLE_ON);
#endif

  // Stop the extruders
  
//  for(byte i = 0; i < EXTRUDER_COUNT; i++)
//    ex[i]->shutdown();

// If we run the bed, turn it off.

//#if HEATED_BED == HEATED_BED_ON
//  heatedBed.shutdown();
//#endif
  
  // LED off
  digitalWrite(DEBUG_PIN, 0);
  
  // Till the end of time...
  for(;;); 
}

void MachineModel::manage(bool withGUI)
{
//  for(byte i = 0; i < EXTRUDER_COUNT; i++)
//    ex[i]->manage();
//#if HEATED_BED == HEATED_BED_ON   
//  heatedBed.manage();
//#endif
	if(withGUI)  
  		lcdUi.handleUI();
}

void MachineModel::blink()
{
  led = !led;
  if(led)
      digitalWrite(DEBUG_PIN, 1);
  else
      digitalWrite(DEBUG_PIN, 0);
} 

// The move buffer
void MachineModel::cancelAndClearQueue()
{
	cli();
	tail = head;	// clear buffer
	for(int i=0;i<BUFFER_SIZE;i++)
		cdda[i]->shutdown();
	sei();
}

bool MachineModel::qEmpty()
{
   return tail == head && !cdda[tail]->active();
}

bool MachineModel::qFull()
{
  if(tail == 0)
    return head == (BUFFER_SIZE - 1);
  else
    return head == (tail - 1);
}

void MachineModel::waitFor_qEmpty()
{
// while waiting maintain the temperatures
  while(!qEmpty()) {
    manage(true);
  }
}
 
void MachineModel::waitFor_qNotFull()
{
// while waiting maintain the temperatures
  while(qFull()) {
    manage(true);
  }
}

void MachineModel::qMove(const FloatPoint& p)
{
  waitFor_qNotFull();
  byte h = head; 
  h++;
  if(h >= BUFFER_SIZE)
    h = 0;
  cdda[h]->set_target(p);
  head = h;
}

void MachineModel::dQMove()
{
  if(!qEmpty())
  {
    byte t = (tail+1)%BUFFER_SIZE;  
	cdda[t]->dda_start();
	tail = t;
  }
}

// Switch between mm and inches
void MachineModel::setUnits(bool um)
{
	using_mm = um;
    if(using_mm)
    {
      units.x = X_STEPS_PER_MM;
      units.y = Y_STEPS_PER_MM;
      units.z = Z_STEPS_PER_MM;
      units.a = A_STEPS_PER_MM;
      units.b = B_STEPS_PER_MM;
      units.f = 1.0;
    } else
    {
      units.x = X_STEPS_PER_INCH;
      units.y = Y_STEPS_PER_INCH;
      units.z = Z_STEPS_PER_INCH;
      units.a = A_STEPS_PER_INCH;
      units.b = B_STEPS_PER_INCH;
      units.f = 1.0;  
    }
}

void MachineModel::handleInterrupt()
{
  if(cdda[tail]->active())
	  cdda[tail]->dda_step();
  else
	  dQMove();
}

//
//
//

bool MachineModel::checkEndstops(byte flag, bool endstopHit, long current, bool dir)
{
	switch(flag)
	{
		case X_LOW_HIT:
		case X_HIGH_HIT:
			absolutePosition.x = current;
			break;
		case Y_LOW_HIT:
		case Y_HIGH_HIT:
			absolutePosition.y = current;
			break;
		case Z_LOW_HIT:
		case Z_HIGH_HIT:
			absolutePosition.z = current;
			break;
	}
	
	if(endstopHit)
	{
		endstop_hits |= flag;
		switch(flag)
		{
			case X_LOW_HIT:
			case X_HIGH_HIT:
				zeroHit.x = current;
				break;
			case Y_LOW_HIT:
			case Y_HIGH_HIT:
				zeroHit.y = current;
				break;
			case Z_LOW_HIT:
			case Z_HIGH_HIT:
				zeroHit.z = current;
				break;
		}
	}
	else
		endstop_hits &= ~flag;

	if(absolutePositionValid)
	{
		switch(flag)
		{
//			case X_LOW_HIT:
//				if(!dir && current<=0)
//				{
//					endstopHit = true;
//					endstop_hits |= flag;
//					zeroHit.x = current;
//				}
//				break;
			case X_HIGH_HIT:
				if(dir && current>=MACHINE_MAX_X_STEPS)
				{
					endstopHit = true;
					endstop_hits |= flag;
					zeroHit.x = current;
				}
				break;
//			case Y_LOW_HIT:
//				if(!dir && current<=0)
//				{
//					endstopHit = true;
//					endstop_hits |= flag;
//					zeroHit.y = current;
//				}
//				break;
			case Y_HIGH_HIT:
				if(dir && current>=MACHINE_MAX_Y_STEPS)
				{
					endstopHit = true;
					endstop_hits |= flag;
					zeroHit.y = current;
				}
				break;
			case Z_LOW_HIT:
				if(!dir && current<=0)
				{
					endstopHit = true;
					endstop_hits |= flag;
					zeroHit.z = current;
				}
				break;
//			case Z_HIGH_HIT:
//				if(dir && current>=MACHINE_MAX_Z_STEPS)
//				{
//					endstopHit = true;
//					endstop_hits |= flag;
//					zeroHit.z = current;
//				}
//				break;
		}
	}
	
	return !endstopHit;
}


FloatPoint MachineModel::livePosition()
{
	FloatPoint absolute = from_steps(units, absolutePosition);
	absolute.f = localPosition.f;
	return absolute-localZeroOffset;
}

// The following three inline functions are used for things like return to 0

void MachineModel::specialMoveX(const float& x, const float& feed)
{
  FloatPoint sp = localPosition;
  sp.x = x;
  sp.f = feed;
  qMove(sp);
}

void MachineModel::specialMoveY(const float& y, const float& feed)
{
  FloatPoint sp = localPosition;
  sp.y = y;
  sp.f = feed;
  qMove(sp);
}

void MachineModel::specialMoveZ(const float& z, const float& feed)
{
  FloatPoint sp = localPosition;
  sp.z = z; 
  sp.f = feed;
  qMove(sp);
}

void MachineModel::zeroX()
{
	//110% for luck
	specialMoveX(localPosition.x - MACHINE_MAX_X_MM*1.1, FAST_XY_FEEDRATE);
	localPosition.x = 0.f;
	specialMoveX(localPosition.x + 1.f, SLOW_FEEDRATE);
	specialMoveX(localPosition.x - 10.f, SLOW_FEEDRATE);                                
	
	// Wait for movements to finish, then check we hit the stop
 
	waitFor_qEmpty();
	localPosition.x = 0.f;
	localZeroOffset.x = 0.f;
	absolutePosition.x = 0;

	if(!isEndstopHit(X_LOW_HIT))
	{
		sprintf(talkToHost.string(), "X endstop not hit - hard fault.");
		talkToHost.setFatal();
	}  
}

void MachineModel::zeroY()
{
  //110% for luck
  specialMoveY(localPosition.y - MACHINE_MAX_Y_MM*1.1, FAST_XY_FEEDRATE);
  localPosition.y = 0.f;
  specialMoveY(localPosition.y + 1.f, SLOW_FEEDRATE);
  specialMoveY(localPosition.y - 10.f, SLOW_FEEDRATE);                                

  // Wait for movements to finish, then check we hit the stop
  waitFor_qEmpty();
  
  localPosition.y = 0.f;
  localZeroOffset.y = 0.f;
  absolutePosition.y = 0;
  
  if(!isEndstopHit(Y_LOW_HIT))
  {
    sprintf(talkToHost.string(), "Y endstop not hit - hard fault.");
    talkToHost.setFatal();
  }   
   
}

void MachineModel::zeroZ()
{
  //110% for luck
  specialMoveZ(localPosition.z + MACHINE_MAX_Z_MM*1.1, FAST_Z_FEEDRATE);
  localPosition.z = (float)MACHINE_MAX_Z_MM;
  specialMoveZ(localPosition.z - 1.f, SLOW_FEEDRATE);
  specialMoveZ(localPosition.z + 2.f, SLOW_FEEDRATE);    
  
  // Wait for movements to finish, then check we hit the stop

  waitFor_qEmpty();
                              
  localPosition.z = (float)MACHINE_MAX_Z_MM;
  localZeroOffset.z = 0.f;
  absolutePosition.z = MACHINE_MAX_Z_STEPS;
 
  if(!isEndstopHit(Z_HIGH_HIT))
  {
    sprintf(talkToHost.string(), "Z endstop not hit - hard fault.");
    talkToHost.setFatal();
  }     
}

void MachineModel::manualToolChange(int toolNumber)
{
	// TODO: fetch tool description from tool database
	
	char desc[21];
	desc[0]=0x0;
	if(toolNumber>=0)
	{
		if(toolNumber>0 && toolNumber<=TOOL_COUNT)
		{
			EEPROM_ReadString(EEPROM_ADR_TOOL_BASE+(toolNumber-1)*EEPROM_SIZE_TOOL_RECORD, desc);
		}
		if(strlen(desc)==0)
			sprintf(desc, "Tool #%d",toolNumber);
	}
	else
		strcpy(desc, "Unspecified Tool");
	lcdUi.manualToolChange(desc);
}


void MachineModel::setLocalZero(FloatPoint zeroPoint)
{
	localZeroOffset = localZeroOffset + localPosition - zeroPoint;
	localZeroOffset.f = 0.;
	localPosition = zeroPoint;
}

bool MachineModel::switchToWCS(int number)
{
	bool success = false;
	if(absolutePositionValid && number>=0 && number < WCS_COUNT)
	{
		FloatPoint currentOffset = localPosition+localZeroOffset;
		localZeroOffset = EEPROM_ReadFloatPoint(EEPROM_ADR_WCS_BASE+number*EEPROM_SIZE_WCS_VALUE);
		localPosition = currentOffset-localZeroOffset;
		success = true;
	}
	return success;
}
