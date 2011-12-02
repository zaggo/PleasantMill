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
 * Important!
 * This code needs the Bounce library:
 * http://www.arduino.cc/playground/Code/Bounce
 */
#include "Arduino.h"
#include <Wire.h>
#include <Bounce.h>
#include <EEPROM.h>
#include "configuration.h"
#include "pins.h"
#include "interruptHandling.h"
#include "hostcom.h"
#include "MachineModel.h"
#include "Persistent.h"

hostcom talkToHost;
MachineModel sharedMachineModel;

word interruptBlink;

extern void setupGcodeProcessor(); // in process_g_code.cpp
extern void init_process_string(); // in process_g_code.cpp
extern void get_and_do_command(); // in process_g_code.cpp


/*
This has an internal flag (nonest) to prevent its being interrupted by another timer interrupt.
It re-enables interrupts internally (not something that one would normally do with an ISR).
This allows USART interrupts to be serviced while this ISR is also live, and so prevents 
communications errors.
*/
volatile bool nonest;

ISR(TIMER1_COMPA_vect)
{
  if(nonest)
    return;
  nonest = true;
  
  sei();
  
  interruptBlink++;
  if(interruptBlink == 0x280)
  {
     sharedMachineModel.blink();
     interruptBlink = 0; 
  }

  if(digitalRead(EMERGENCY_STOP)==LOW)
  {
  	sharedMachineModel.emergencyStop=true;
  }
  
  if(!sharedMachineModel.emergencyStop)
  {
  	sharedMachineModel.handleInterrupt();	
  }
  
  nonest = false;
}

void setup()
{
#if 1
  Serial1.begin(115200);
  Serial1.println("Hello Debugger");
#endif  
  talkToHost.start();
  
  nonest = false;
  disableTimerInterrupt();
  setupTimerInterrupt();
  interruptBlink = 0;
  pinMode(DEBUG_PIN, OUTPUT);

  setupGcodeProcessor();
  
  checkEEPROM();

  setTimer(DEFAULT_TICK);
  enableTimerInterrupt();
  
  sharedMachineModel.startup();
}

void loop()
{
  nonest = false;
  sharedMachineModel.manage(true);
  get_and_do_command();
}
