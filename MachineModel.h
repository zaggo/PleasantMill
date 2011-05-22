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
#ifndef MACHINEMODEL_H
#define MACHINEMODEL_H

#include "WProgram.h"
#include "configuration.h"
#include "vectors.h"

class cartesian_dda;
class MachineModel
{	
protected:
	cartesian_dda* cdda[BUFFER_SIZE];
	volatile byte head;
	volatile byte tail;
	
	bool led;

	LongPoint zeroHit; //The coordinates of the last zero positions
	volatile byte endstop_hits; // And what we hit
	FloatPoint units;            // Factors for converting either mm or inches to steps

	// Machine States
	bool using_mm;
	bool abs_mode;					// false = incremental; true = absolute
	bool oldZRetractMode;			// false = return to R level in canned cycles; true = return to old Z level in canned cycles
	int cutterRadiusCompensation;	// 0 = not active; 1 = compensate right of path; -1 = compensate left of path
	float retractHeight;			// for canned cycles
	float clearanceIncrement;		// G73 relative retracting height between delta

	void specialMoveX(const float& x, const float& feed);
	void specialMoveY(const float& y, const float& feed);
	void specialMoveZ(const float& z, const float& feed);

public:
	MachineModel();
	void startup();
	void shutdown();
	
	void handleInterrupt();
	void manage(bool withGUI);
	void blink();
	
	void manualToolChange(int toolNumber);
	
	void setLocalZero(FloatPoint zeroPoint);
	
	FloatPoint livePosition();

	// The Command queue
	void cancelAndClearQueue();
	bool qEmpty();
	bool qFull();
	void waitFor_qEmpty();
	void waitFor_qNotFull();
	void qMove(const FloatPoint& p);
	void dQMove();
  	// True for mm; false for inches
	void setUnits(bool u);
	bool getUnits() { return using_mm; }
	FloatPoint returnUnits() { return units; }
	
	void setAbsMode(bool v) { abs_mode = v; }
	bool getAbsMode() { return abs_mode; }
	void setRetractMode(bool v)  { oldZRetractMode = v; }
	bool getRetractMode() { return oldZRetractMode; }
	void setCutterRadiusCompensation(int v)  { cutterRadiusCompensation = v; }
	int getCutterRadiusCompensation() { return cutterRadiusCompensation; }
	float getClearanceIncrement() { return clearanceIncrement; }
	
	void setRetractHeight(float v) { retractHeight = v; }
	float getRetractHeight() { return retractHeight; }
	
	bool switchToWCS(int number);
	
	void zeroX();
	void zeroY();
	void zeroZ();

	// Endstops
	bool checkEndstops(byte flag, bool endstopHit, long current, bool dir);

	bool isEndstopHit(byte flag) { return (endstop_hits & flag); }
	
	// Emergency Stop
	volatile bool emergencyStop;

	bool receiving;
	FloatPoint localPosition;
	FloatPoint localZeroOffset;
	LongPoint absolutePosition;
	bool absolutePositionValid;
	
	float stickyQ;
	float stickyP;
};

extern MachineModel sharedMachineModel;

#endif