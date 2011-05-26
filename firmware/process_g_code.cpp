/************
 * GCode Interpreter
 * 
 *  Based on the RepRap GCode interpreter.
 *  see http://www.reprap.org
 *
 *  IMPORTANT
 *	Before changing this interpreter,read this page:
 *	http://objects.reprap.org/wiki/Mendel_User_Manual:_RepRapGCodes
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
#include <string.h>
#include "WProgram.h"
#include "MachineModel.h"
#include "pins.h"
//#include "extruder.h"
#include "vectors.h"
#include "cartesian_dda.h"
#include "hostcom.h"

#define MIN(x, y) (x<y)?x:y

/* bit-flags for commands and parameters */
enum
{
	GCODE_G,
	GCODE_M,
	GCODE_P,
	GCODE_X,
	GCODE_Y,
	GCODE_Z,
	GCODE_I,
	GCODE_N,
	GCODE_CHECKSUM,
	GCODE_F,
	GCODE_S,
	GCODE_Q,
	GCODE_R,
	GCODE_E,
	GCODE_T,
	GCODE_J,
	GCODE_A,
	GCODE_B,
	GCODE_L,
	GCODE_COUNT
};

#define PARSE_INT(ch, str, len, val, seen, flag) \
	case ch: \
		len = scan_int(str, &val, seen, flag); \
		break;

#define PARSE_LONG(ch, str, len, val, seen, flag) \
	case ch: \
		len = scan_long(str, &val, seen, flag); \
		break;

#define PARSE_FLOAT(ch, str, len, val, seen, flag) \
	case ch: \
		len = scan_float(str, &val, seen, flag); \
		break;

void rapidMove(FloatPoint targetPoint);
void drawArc(float centerX, float centerY, float endpointX, float endpointY, boolean clockwise);
void doDrillCycle(int gCode, FloatPoint &fp);

void process_string(char instruction[], int size);
int scan_int(char *str, int *valp, bool seen[], unsigned int flag);
int scan_float(char *str, float *valp, bool seen[], unsigned int flag);
int scan_long(char *str, long *valp, bool seen[], unsigned int flag);

#define kMaxGCommands 5

/* gcode line parse results */
struct GcodeParser
{
    bool seen[GCODE_COUNT]; // More than 16 GCodes, a bitfield won't do
        
    short GIndex;
    int G[kMaxGCommands];
    int M;
    int T;
    float P;
    float X;
    float Y;
    float Z;
    float A;
    float B;
    float I;
    float J;
    float F;
    float S;
    float R;
    float Q;
    int L;
    int Checksum;
    long N;
    long LastLineNrRecieved;
};

//our command string
char cmdbuffer[COMMAND_SIZE];
char c = '?';
byte serial_count = 0;
boolean comment = false;

FloatPoint fp;

#define DEBUG_ECHO (1<<0)
#define DEBUG_INFO (1<<1)
#define DEBUG_ERRORS (1<<2)

byte SendDebug =  DEBUG_INFO | DEBUG_ERRORS | DEBUG_ECHO;

/* keep track of the last G code - this is the command mode to use
 * if there is no command in the current string 
 */
int last_gcode_g = -1;

float extruder_speed = 0;

GcodeParser gc;	/* string parse result */

inline bool seenAnything()
{
	for(int i=0; i<GCODE_COUNT;i++)
		if(gc.seen[i])
			return true;
	return false;
}

//init our string processing
inline void init_process_string()
{
  serial_count = 0;
  comment = false;
  gc.GIndex=0;
}

// Get a command and process it

void get_and_do_command()
{         
	c = ' ';
	while(talkToHost.gotData() && c != '\n')
	{
		c = talkToHost.get();
		sharedMachineModel.blink();
		if(c == '\r')
			c = '\n';
		// Throw away control chars except \n
		if(c >= ' ' || c == '\n')
		{

		  //newlines are ends of commands.
		  if (c != '\n')
		  {
			// Start of comment - ignore any bytes received from now on
			if (c == ';' || c=='(')
				comment = true;
				
			// If we're not in comment mode, add it to our array.
			if (!comment)
				cmdbuffer[serial_count++] = toupper(c);
			else if(c == ')')
				comment = false;
		  }
		}
		// Buffer overflow?
		if(serial_count >= COMMAND_SIZE)
			init_process_string();
	}

	//if we've got a real command, do it
	if ((comment || serial_count) && c == '\n')
	{
		if(serial_count>0)
		{
			// Terminate string
			cmdbuffer[serial_count] = 0;
					
			if(SendDebug & DEBUG_ECHO)
				sprintf(talkToHost.string(), "Echo: %s", cmdbuffer);
					   
			//process our command!
			process_string(cmdbuffer, serial_count);
		}
		//clear command.
		init_process_string();
		
		// Say we're ready for the next one
		talkToHost.sendMessage(SendDebug & DEBUG_INFO);
	}
}



void parse_string(char instruction[ ], int size)
{
	int ind;
	int len;	/* length of parameter argument */

	for(int i=0; i<GCODE_COUNT;i++)
		gc.seen[i] = false;

	len=0;
	/* scan the string for commands and parameters, recording the arguments for each,
	 * and setting the seen flag for each that is seen
	 */
	for (ind=0; ind<size; ind += (1+len))
	{
		len = 0;
		switch (instruction[ind])
		{
			case 'G':
					if(gc.GIndex<kMaxGCommands)
						len = scan_int(&instruction[ind+1], &(gc.G[gc.GIndex++]), gc.seen, GCODE_G);
					else
					{
						sprintf(talkToHost.string(), "Too many G codes per line");
						talkToHost.setFatal();
						talkToHost.sendMessage(true);
					}
					break;
			PARSE_INT('M', &instruction[ind+1], len, gc.M, gc.seen, GCODE_M);
			PARSE_INT('T', &instruction[ind+1], len, gc.T, gc.seen, GCODE_T);
			PARSE_INT('L', &instruction[ind+1], len, gc.L, gc.seen, GCODE_L);
			PARSE_FLOAT('S', &instruction[ind+1], len, gc.S, gc.seen, GCODE_S);
			PARSE_FLOAT('P', &instruction[ind+1], len, gc.P, gc.seen, GCODE_P);
			PARSE_FLOAT('X', &instruction[ind+1], len, gc.X, gc.seen, GCODE_X);
			PARSE_FLOAT('Y', &instruction[ind+1], len, gc.Y, gc.seen, GCODE_Y);
			PARSE_FLOAT('Z', &instruction[ind+1], len, gc.Z, gc.seen, GCODE_Z);
			PARSE_FLOAT('I', &instruction[ind+1], len, gc.I, gc.seen, GCODE_I);
			PARSE_FLOAT('J', &instruction[ind+1], len, gc.J, gc.seen, GCODE_J);
			PARSE_FLOAT('F', &instruction[ind+1], len, gc.F, gc.seen, GCODE_F);
			PARSE_FLOAT('R', &instruction[ind+1], len, gc.R, gc.seen, GCODE_R);
			PARSE_FLOAT('Q', &instruction[ind+1], len, gc.Q, gc.seen, GCODE_Q);
			PARSE_FLOAT('E', &instruction[ind+1], len, gc.A, gc.seen, GCODE_E);
			PARSE_FLOAT('A', &instruction[ind+1], len, gc.A, gc.seen, GCODE_A);
			PARSE_FLOAT('B', &instruction[ind+1], len, gc.B, gc.seen, GCODE_B);
			PARSE_LONG('N', &instruction[ind+1], len, gc.N, gc.seen, GCODE_N);
			PARSE_INT('*', &instruction[ind+1], len, gc.Checksum, gc.seen, GCODE_CHECKSUM);
			default:
				break;
		}
	}
}

void fetchCartesianParameters()
{
	fp = sharedMachineModel.localPosition;
	if (sharedMachineModel.getAbsMode())
	{
		if (gc.seen[GCODE_X])
			fp.x = gc.X;
		if (gc.seen[GCODE_Y])
			fp.y = gc.Y;
		if (gc.seen[GCODE_Z])
			fp.z = gc.Z;
		if (gc.seen[GCODE_E]) // Legacy
			fp.a = gc.A;
		if (gc.seen[GCODE_A])
			fp.a = gc.A;
		if (gc.seen[GCODE_B])
			fp.a = gc.B;
	}
	else
	{
		if (gc.seen[GCODE_X])
			fp.x += gc.X;
		if (gc.seen[GCODE_Y])
			fp.y += gc.Y;
		if (gc.seen[GCODE_Z])
			fp.z += gc.Z;
		if (gc.seen[GCODE_E])
			fp.a += gc.A;
		if (gc.seen[GCODE_A])
			fp.a += gc.A;
		if (gc.seen[GCODE_B])
			fp.b += gc.B;
	}

	// Get feedrate if supplied - feedrates are always absolute???
	if ( gc.seen[GCODE_F] )
		fp.f = MIN(gc.F, FAST_XY_FEEDRATE);
}

//Read the string and execute instructions
void process_string(char instruction[], int size)
{
	//the character / means delete block... used for comments and stuff.
	if (instruction[0] == '/')	
		return;

	bool axisSelected;
        
	fp.x = 0.0;
	fp.y = 0.0;
	fp.z = 0.0;
	fp.a = 0.0;
	fp.b = 0.0;
	fp.f = 0.0;

	//get all our parameters!
	parse_string(instruction, size);
  
	// Do we have lineNr and checksums in this gcode?
	if((bool)(gc.seen[GCODE_CHECKSUM]) | (bool)(gc.seen[GCODE_N]))
	{
		// Check that if recieved a L code, we also got a C code. If not, one of them has been lost, and we have to reset queue
		if( (bool)(gc.seen[GCODE_CHECKSUM]) != (bool)(gc.seen[GCODE_N]) )
		{
           if(SendDebug & DEBUG_ERRORS)
           {
              if(gc.seen[GCODE_CHECKSUM])
                sprintf(talkToHost.string(), "Serial Error: checksum without line number. Checksum: %d, line received: %s", gc.Checksum, instruction);
              else
                sprintf(talkToHost.string(), "Serial Error: line number without checksum. Linenumber: %ld, line received: %s", gc.N, instruction);
           }
           talkToHost.setResend(gc.LastLineNrRecieved+1);
           return;
		}
		// Check checksum of this string. Flush buffers and re-request line of error is found
		if(gc.seen[GCODE_CHECKSUM])  // if we recieved a line nr, we know we also recieved a Checksum, so check it
		{
            // Calc checksum.
            byte checksum = 0;
            byte count=0;
            while(instruction[count] != '*')
              checksum = checksum^instruction[count++];
            // Check checksum.
            if(gc.Checksum != (int)checksum)
            {
              if(SendDebug & DEBUG_ERRORS)
                sprintf(talkToHost.string(), "Serial Error: checksum mismatch.  Remote (%d) not equal to local (%d), line received: %s", 
                			gc.Checksum, (int)checksum, instruction);
              talkToHost.setResend(gc.LastLineNrRecieved+1);
              return;
            }
			// Check that this lineNr is LastLineNrRecieved+1. If not, flush
			if(!( (bool)(gc.seen[GCODE_M]) && gc.M == 110)) // unless this is a reset-lineNr command
				if(gc.N != gc.LastLineNrRecieved+1)
				{
					if(SendDebug & DEBUG_ERRORS)
						sprintf(talkToHost.string(), "Serial Error: Linenumber (%ld) is not last + 1 (%ld), line received: %s", gc.N, gc.LastLineNrRecieved+1, instruction);
					talkToHost.setResend(gc.LastLineNrRecieved+1);
					return;
				}
			//If we reach this point, communication is a succes, update our "last good line nr" and continue
			gc.LastLineNrRecieved = gc.N;
		}
	}


	/* if no command was seen, but parameters were, then use the last G code as 
	* the current command
	*/
	if ((!(gc.seen[GCODE_G] | gc.seen[GCODE_M] | gc.seen[GCODE_T])) && (seenAnything() && (last_gcode_g >= 0)))
	{
		/* yes - so use the previous command with the new parameters */
		gc.G[0] = last_gcode_g;
		gc.GIndex=1;
		gc.seen[GCODE_G]=true;
	}

	// Deal with emergency stop as No 1 priority
	if ((gc.seen[GCODE_M]) && (gc.M == 112))
		sharedMachineModel.shutdown();
	
	//did we get a gcode?
	if (gc.seen[GCODE_G])
	{		   
		// Handle all GCodes in this line
		for(int gIndex=0; gIndex<gc.GIndex; gIndex++)
		{
			last_gcode_g = gc.G[gIndex];	/* remember this for future instructions */

		    unsigned long endTime; // For Dwell
		    
			// Process the buffered move commands first
			bool gCodeHandled=false;
			switch (gc.G[gIndex])
			{
				////////////////////////
				// Buffered commands
				////////////////////////
				
				case 0:		//Rapid move
							fetchCartesianParameters();
							rapidMove(fp);
							break;
							
				case 1:		// Controlled move;
							fetchCartesianParameters();
							sharedMachineModel.qMove(fp);
							break;
															  
				case 2:		// G2, Clockwise arc
				case 3: 	// G3, Counterclockwise arc
							fetchCartesianParameters();
							if(gc.seen[GCODE_R])
							{
							  //drawRadius(tempX, tempY, rVal, (gc.G[gIndex]==2));
								if(SendDebug & DEBUG_ERRORS)
									sprintf(talkToHost.string(), "Dud G code: G%d with R param not yet implemented", gc.G[gIndex]);
								talkToHost.setResend(gc.LastLineNrRecieved+1);
							}
							else if(gc.seen[GCODE_I] || gc.seen[GCODE_J])
							{
								drawArc(fp.x+gc.I, fp.y+gc.J, fp.x, fp.y, (gc.G[gIndex]==2));
							}
							else
							{
								if(SendDebug & DEBUG_ERRORS)
									sprintf(talkToHost.string(), "Dud G code: G%d without I or J params", gc.G[gIndex]);
								talkToHost.setResend(gc.LastLineNrRecieved+1);
							}
							break;
				
							
				case 28:	//go home.  If we send coordinates (regardless of their value) only zero those axes
							fetchCartesianParameters();
							axisSelected = false;
							if(gc.seen[GCODE_Z])
							{
							  sharedMachineModel.zeroZ();
							  axisSelected = true;
							}
							if(gc.seen[GCODE_X])
							{
							  sharedMachineModel.zeroX();
							  axisSelected = true;
							}
							if(gc.seen[GCODE_Y])
							{
							  sharedMachineModel.zeroY();
							  axisSelected = true;
							}                                
							if(!axisSelected)
							{
							  sharedMachineModel.zeroZ();
							  sharedMachineModel.zeroX();
							  sharedMachineModel.zeroY();
							  sharedMachineModel.absolutePositionValid=true;
							}
							sharedMachineModel.localPosition.f = SLOW_FEEDRATE;     // Most sensible feedrate to leave it in                    

							break;							

				////////////////////////
				// Non-Buffered commands
				////////////////////////
							
				case 4: 	//Dwell
							sharedMachineModel.waitFor_qEmpty(); // Non-buffered G command. Wait till the buffer q is empty first
							endTime = millis() + (int)(gc.P + 0.5);
							while(millis() < endTime) 
								sharedMachineModel.manage(true);
							break;
		
				case 20:	//Inches for Units
							sharedMachineModel.waitFor_qEmpty(); // Non-buffered G command. Wait till the buffer q is empty first
							sharedMachineModel.setUnits(false);
							break;
		
				case 21:	//mm for Units
							sharedMachineModel.waitFor_qEmpty(); // Non-buffered G command. Wait till the buffer q is empty first
							sharedMachineModel.setUnits(true);
							break;
							
				case 54:
				case 55:
				case 56:
				case 57:
				case 58:
				case 59:	// Switch to Workin Coordinate System
							sharedMachineModel.waitFor_qEmpty(); // Non-buffered G command. Wait till the buffer q is empty first
							if(!sharedMachineModel.switchToWCS(gc.G[gIndex]-54))
							{
								if(SendDebug & DEBUG_ERRORS)
									sprintf(talkToHost.string(), "Dud G code: G%d not possible, probably machine not homed", gc.G[gIndex]);
								talkToHost.setResend(gc.LastLineNrRecieved+1);
							}
							break;
		
				case 73:	// Peck drilling cycle for milling - high-speed
				case 81:	// Drill Cycle
				case 82:	// Drill Cycle with dwell
				case 83:	// Drill Cycle peck drilling
				case 85:	// Drill Cycle, slow retract
				case 89:	// Drill Cycle with dwell and slow reredract
							sharedMachineModel.waitFor_qEmpty(); // Non-buffered G command. Wait till the buffer q is empty first
							fetchCartesianParameters();
							doDrillCycle(gc.G[gIndex], fp);
							break;
																					
				case 90: 	//Absolute Positioning
							sharedMachineModel.waitFor_qEmpty(); // Non-buffered G command. Wait till the buffer q is empty first
							sharedMachineModel.setAbsMode(true);
							break;

				case 91: 	//Incremental Positioning
							sharedMachineModel.waitFor_qEmpty(); // Non-buffered G command. Wait till the buffer q is empty first
							sharedMachineModel.setAbsMode(false);
							break;

				case 92:	//Set position as fp
							sharedMachineModel.waitFor_qEmpty(); // Non-buffered G command. Wait till the buffer q is empty first
							fetchCartesianParameters();
							sharedMachineModel.setLocalZero(fp);
							break;

				case 98:	// Return to initial Z level in canned cycle
							sharedMachineModel.waitFor_qEmpty(); // Non-buffered G command. Wait till the buffer q is empty first
							sharedMachineModel.setRetractMode(true);
							break;

				case 99:	// Return to R level in canned cycle
							sharedMachineModel.waitFor_qEmpty(); // Non-buffered G command. Wait till the buffer q is empty first
							sharedMachineModel.setRetractMode(false);
							break;

				default:
							if(SendDebug & DEBUG_ERRORS)
								sprintf(talkToHost.string(), "Dud G code: G%d", gc.G[gIndex]);
							talkToHost.setResend(gc.LastLineNrRecieved+1);
			}	// switch
		}		// for
	}			// Gcode
	
	// Get feedrate if supplied and queue is empty
	if ( gc.seen[GCODE_F] && sharedMachineModel.qEmpty())
		sharedMachineModel.localPosition.f=MIN(gc.F, FAST_XY_FEEDRATE);
		
	//find us an m code.
	if (gc.seen[GCODE_M])
	{
		// Wait till the q is empty first
		sharedMachineModel.waitFor_qEmpty();
		switch (gc.M)
		{
			case 0:
				sharedMachineModel.shutdown();
				break;
		    case 1:
				 //todo: optional stop
				 break;
			case 2:
				 //todo: program end
				 break;
				 
			case 6:
				 // Tool change
				 if(gc.seen[GCODE_T])
				 	sharedMachineModel.manualToolChange(gc.T);
				 else
				 	sharedMachineModel.manualToolChange(-1);
				 break;

			//custom code for temperature control
//			case 104:
//				if (gc.seen[GCODE_S])
//				{
//					ex[extruder_in_use]->setTemperature((int)gc.S);
//				}
//				break;

			//custom code for temperature reading
//			case 105:
//				talkToHost.setETemp(ex[extruder_in_use]->getTemperature());
//				talkToHost.setBTemp(heatedBed.getTemperature());
//				break;
//
//			//turn fan on
//			case 106:
//				ex[extruder_in_use]->setCooler(255);
//				break;
//
//			//turn fan off
//			case 107:
//				ex[extruder_in_use]->setCooler(0);
//				break;
//
//
//			// Set the temperature and wait for it to get there
//			case 109:
//				ex[extruder_in_use]->setTemperature((int)gc.S);
//				ex[extruder_in_use]->waitForTemperature();
//				break;
			// Starting a new print, reset the gc.LastLineNrRecieved counter
			case 110:
				if (gc.seen[GCODE_N])
					gc.LastLineNrRecieved = gc.N;
				break;
			case 111:
				SendDebug = gc.S;
				break;
			case 112:	// STOP!
				sharedMachineModel.shutdown();
				break;

			//custom code for returning current coordinates
			case 114:
				talkToHost.setCoords(sharedMachineModel.localPosition);
				break;

			//Reserved for returning machine capabilities in keyword:value pairs
			//custom code for returning Firmware Version and Capabilities 
			case 115:
				talkToHost.capabilities();
				break;

//			// TODO: make this work properly
//			case 116:
//				ex[extruder_in_use]->waitForTemperature();
//				break;

			//custom code for returning zero-hit coordinates
//			case 117:
//				talkToHost.setCoords(zeroHit);
//				break;

			// The valve (real, or virtual...) is now the way to control any extruder (such as
			// a pressurised paste extruder) that cannot move using E codes.

//			// Open the valve
//			case 126:
//				ex[extruder_in_use]->valveSet(true, (int)(gc.P + 0.5));
//				break;
//                                
//			// Close the valve
//			case 127:
//				ex[extruder_in_use]->valveSet(false, (int)(gc.P + 0.5));
//				break;
                                                                
//			case 140:
//				if (gc.seen[GCODE_S])
//				{
//					heatedBed.setTemperature((int)gc.S);
//				}
//				break;

			case 141: //TODO: set chamber temperature
				break;
                                
			case 142: //TODO: set holding pressure
				break;                                

			default:
				if(SendDebug & DEBUG_ERRORS)
					sprintf(talkToHost.string(), "Dud M code: M%d", gc.M);
				talkToHost.setResend(gc.LastLineNrRecieved+1);
		}
	}

	// Tool (i.e. extruder) change?
	if (gc.seen[GCODE_T])
	{
		sharedMachineModel.waitFor_qEmpty(); 
//		newExtruder(gc.T);
	}
}

int scan_float(char *str, float *valp, bool seen[], unsigned int flag)
{
	float res;
	int len;
	char *end;
     
	res = (float)strtod(str, &end);
      
	len = end - str;

	if (len > 0)
	{
		*valp = res;
		seen[flag]=true;
	}
	else
		*valp = 0;
          
	return len;	/* length of number */
}

int scan_int(char *str, int *valp, bool seen[], unsigned int flag)
{
	int res;
	int len;
	char *end;

	res = (int)strtol(str, &end, 10);
	len = end - str;

	if (len > 0)
	{
		*valp = res;
		seen[flag]=true;
	}
	else
		*valp = 0;
          
	return len;	/* length of number */
}

int scan_long(char *str, long *valp, bool seen[], unsigned int flag)
{
	long res;
	int len;
	char *end;

	res = strtol(str, &end, 10);
	len = end - str;

	if (len > 0)
	{
		*valp = res;
		seen[flag]=true;
	}
	else
		*valp = 0;
          
	return len;	/* length of number in ascii world */
}

void setupGcodeProcessor()
{
  gc.LastLineNrRecieved = -1;
  init_process_string();
}

void rapidMove(FloatPoint targetPoint)
{
	float fr = targetPoint.f;
	targetPoint.f = FAST_XY_FEEDRATE;
	sharedMachineModel.qMove(targetPoint);
	sharedMachineModel.localPosition.f = fr;
	targetPoint.f = fr;
}

/* This code was ported from the Makerbot/ReplicatorG java sources */
void drawArc(float centerX, float centerY, float endpointX, float endpointY, boolean clockwise) 
{
  // angle variables.
  float angleA;
  float angleB;
  float angle;
  float radius;
  float length;

  // delta variables.
  float aX;
  float aY;
  float bX;
  float bY;

  // figure out our deltas
  aX = sharedMachineModel.localPosition.x - centerX;
  aY = sharedMachineModel.localPosition.y - centerY;
  bX = endpointX - centerX;
  bY = endpointY - centerY;

  // Clockwise
  if (clockwise) {
    angleA = atan2(bY, bX);
    angleB = atan2(aY, aX);
  }
  // Counterclockwise
  else {
    angleA = atan2(aY, aX);
    angleB = atan2(bY, bX);
  }

  // Make sure angleB is always greater than angleA
  // and if not add 2PI so that it is (this also takes
  // care of the special case of angleA == angleB,
  // ie we want a complete circle)
  if (angleB <= angleA)
    angleB += 2. * M_PI;
  angle = angleB - angleA;
		
  // calculate a couple useful things.
  radius = sqrt(aX * aX + aY * aY);
  length = radius * angle;

  // for doing the actual move.
  int steps;
  int s;
  int step;

  // Maximum of either 2.4 times the angle in radians
  // or the length of the curve divided by the curve section constant
  steps = (int)ceil(max(angle * 2.4, length));

  // this is the real draw action.
  float newPointX = 0.;
  float newPointY = 0.;
  
  FloatPoint circlePoint = sharedMachineModel.localPosition;
  
  for (s = 1; s <= steps; s++) {
    // Forwards for CCW, backwards for CW
    if (!clockwise)
		step = s;
    else
		step = steps - s;

    // calculate our waypoint.
    circlePoint.x = centerX + radius * cos(angleA + angle * ((float) step / steps));
    circlePoint.y = centerY + radius	* sin(angleA + angle * ((float) step / steps));

    // start the move
	sharedMachineModel.qMove(circlePoint);
  }
}

void doDrillCycle(int gCode, FloatPoint &fp)
{
	unsigned int dwell = 0;
	float delta = 0.;
	bool slowRetract=false;
	bool fullRetract=true;		

	float oldZ = sharedMachineModel.localPosition.z;
	fp.z=oldZ;
	
	bool error = false;
	switch(gCode)
	{
		case 85:
				slowRetract=true;
				// fall through
		case 81: 
				// Check for error conditions
				if(		!(gc.seen[GCODE_X] || gc.seen[GCODE_Y])  // No X or Y given
					//||	!(gc.seen[GCODE_Z])					   // Z must be given
					||	(gc.seen[GCODE_L] && gc.L<=0)			   // Loops given but not positive
					|| 	gc.seen[GCODE_A] || gc.seen[GCODE_B]	   // Movements on rotational axes
					||	sharedMachineModel.getCutterRadiusCompensation()!=0	// cutter radius compensation is active
				  )
				  	error = true;
				 break;
		case 89:
				slowRetract=true;
				// fall through
		case 82: 
				if(!gc.seen[GCODE_P])
					gc.P = sharedMachineModel.stickyP;
				// Check for error conditions
				if(		!(gc.seen[GCODE_X] || gc.seen[GCODE_Y])  // No X or Y given
					//||	!(gc.seen[GCODE_Z])					   // Z must be given
					//||	!(gc.seen[GCODE_P])					   // P must be given
					||	gc.P<0									   // and >= 0
					||	(gc.seen[GCODE_L] && gc.L<=0)			   // Loops given but not positive
					|| 	gc.seen[GCODE_A] || gc.seen[GCODE_B]	   // Movements on rotational axes
					||	sharedMachineModel.getCutterRadiusCompensation()!=0	// cutter radius compensation is active
				  )
				  	error = true;
				 dwell = gc.P;
				 sharedMachineModel.stickyP = gc.P;
				 break;
		case 73: 
				fullRetract=false;
				// fall through
		case 83: 
				if(!gc.seen[GCODE_Q])
					gc.Q = sharedMachineModel.stickyQ;
				// Check for error conditions
				if(		!(gc.seen[GCODE_X] || gc.seen[GCODE_Y])  // No X or Y given
					//||	!(gc.seen[GCODE_Z])					   // Z must be given
					//||	!(gc.seen[GCODE_Q])					   // Q must be given
					||	gc.Q<=0.									   // and > 0
					||	(gc.seen[GCODE_L] && gc.L<=0)			   // Loops given but not positive
					|| 	gc.seen[GCODE_A] || gc.seen[GCODE_B]	   // Movements on rotational axes
					||	sharedMachineModel.getCutterRadiusCompensation()!=0	// cutter radius compensation is active
				  )
				  	error = true;
				 delta = gc.Q;
				 sharedMachineModel.stickyQ = gc.Q;
				 break;
	}
	
	if(error)
	{
		if(SendDebug & DEBUG_ERRORS)
			sprintf(talkToHost.string(), "Dud G code: G81 with invalid or missing parameters");
		talkToHost.setResend(gc.LastLineNrRecieved+1);
	}
	else
	{
		int loops = 1;
		if(gc.seen[GCODE_L])
			loops = gc.L;
		if(gc.seen[GCODE_R])
			sharedMachineModel.setRetractHeight(gc.R);
		
		if(fp.z<sharedMachineModel.getRetractHeight())
		{
			FloatPoint move = sharedMachineModel.localPosition;
			move.z = sharedMachineModel.getRetractHeight();
			rapidMove(move);
			fp.z = sharedMachineModel.getRetractHeight();
		}
		
		while(loops-->0)
		{
			rapidMove(fp);
			if(fp.z!=sharedMachineModel.getRetractHeight())
			{
				fp.z = sharedMachineModel.getRetractHeight();
				rapidMove(fp);
			}
			
			if(delta>0.)
			{
				float z = fp.z;
				while(z>gc.Z)
				{
					z -= delta;
					if(z<gc.Z)
						z=gc.Z;

					fp.z=z;
					sharedMachineModel.qMove(fp);
					
					if(z>gc.Z)
					{
						if(fullRetract)
							fp.z=sharedMachineModel.getRetractHeight();
						else
							fp.z=z+sharedMachineModel.getClearanceIncrement();
						rapidMove(fp);
						fp.z=z-.5;
						rapidMove(fp);
					}
				}
			}
			else
			{
				fp.z = gc.Z;
				sharedMachineModel.qMove(fp);
			}
			
			if(dwell>0)
			{
				sharedMachineModel.waitFor_qEmpty(); // Non-buffered G command. Wait till the buffer q is empty first
				unsigned long endTime = millis() + (int)(dwell + 0.5);
				while(millis() < endTime) 
					sharedMachineModel.manage(true);
			}

			if(loops==0 && sharedMachineModel.getRetractMode())
				fp.z = oldZ;
			else
				fp.z = sharedMachineModel.getRetractHeight();
				
			if(slowRetract)
				sharedMachineModel.qMove(fp);
			else
				rapidMove(fp);

			if(loops>0 && !sharedMachineModel.getAbsMode())
			{
				if (gc.seen[GCODE_X])
					fp.x += gc.X;
				if (gc.seen[GCODE_Y])
					fp.y += gc.Y;
			}
		}
	}
}