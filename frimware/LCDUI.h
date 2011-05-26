/************
 * LCD GUI Handling
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
 * This code need the Bounce library:
 * http://www.arduino.cc/playground/Code/Bounce
 */

#ifndef LCD_UI_H
#define LCD_UI_H
#include "WProgram.h"
#include <Bounce.h>
#include "pins.h"
#include "vectors.h"
#include "Persistent.h"

class LcdUi
{
protected:
  char fwNameAndVersion[21];
  
  int uiState;
  int savedUIState; // For EmergencyStop
  
  byte lastKeyState;
  int  selectedMenuItem;  
  
  FloatPoint livePosition;
  unsigned long lastPositionUpdate;
  
  void initializeLCDHardware();
  void clearDisplay();
  void writeAtLine(int line, const char* text);
//  void writeAtRange(int line, int from, int len, const char* text);
  
  void displayMenu(char* title, const char menuText[][20], int mcount);
  void updateMenuSelection(const char menuText[][20], int mcount);
  bool handleMenu(byte keyState, const char menuText[][20], int mcount);

  void switchState(int newState);

  void handleMainMenuSelection();
  void handleCartesianMenuSelection();
  void handleSetWCSMenuSelection();
  void jogXY(byte keyState);
  void jogZ(byte keyState);
  void handleShowWCS(byte keyState);
  void showWCS();
  void handleEmergencyStop();
  
  void refreshLivePosition();
  void displayXY(int line);
  void displayZ(int line);
  void displayAB(int line);
  void displayCartesianInfo();
  void displayWCSInfo(FloatPoint wcsPoint);
  void displayError(const char* line1, const char* line2, int nextState);
  void waitForPushButtonRelease();

public:
  void startup();
  void shutdown();
  
  void handleUI();

  void manualToolChange(char* desc);
};

#endif
