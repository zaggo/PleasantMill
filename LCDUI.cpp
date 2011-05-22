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

#include <Wire.h>
#include "LCDUI.h"
#include "MachineModel.h"
#include "Persistent.h"

// The slave address defaults to 0xfe (8-bit), i.e. 0x7f (7-bit)
#define kI2C_LCD_SLAVE_ADR  (uint8_t)0x7f

// I2C Commands for Coptonix I2C LC-Display Adapter
#define kI2C_ClrDisplay     (uint8_t)0x61
#define kI2C_ReturnHome     (uint8_t)0x62
#define kI2C_SetCursor      (uint8_t)0x63
#define kI2C_CharToLCD      (uint8_t)0x64
#define kI2C_CMDToLCD       (uint8_t)0x65
#define kI2C_Delete         (uint8_t)0x66
#define kI2C_Copy           (uint8_t)0x67
#define kI2C_Paste          (uint8_t)0x68
#define kI2C_GetCursorAdr   (uint8_t)0x69
#define kI2C_GetCharAtCur   (uint8_t)0x6A
#define kI2C_ReadRAM        (uint8_t)0x6B
#define kI2C_DisplayON_OFF  (uint8_t)0x6C
#define kI2C_Shift          (uint8_t)0x6D
#define kI2C_WriteString    (uint8_t)0x76

// ClrDisplay parameters
#define kI2C_DisplayON  ((uint8_t)1<<2)
#define kI2C_CursorON  ((uint8_t)1<<1)
#define kI2C_BlinkON  ((uint8_t)1<<0)

// Display Data Address Chart for 4x20 MC2004E Series Display
#define kLCD_DDA_Line0  (uint8_t)0x00
#define kLCD_DDA_Line1  (uint8_t)0x40
#define kLCD_DDA_Line2  (uint8_t)0x14
#define kLCD_DDA_Line3  (uint8_t)0x54

#define kKeyStateNone (byte)(0x0)
#define kKeyStateA (byte)(0x1<<0)
#define kKeyStateB (byte)(0x1<<1)
#define kKeyStateC (byte)(0x1<<2)
#define kKeyStateD (byte)(0x1<<3)
#define kKeyStateP (byte)(0x1<<4)

static Bounce pButton = Bounce(JOYSTICK_P,15);
static Bounce aButton = Bounce(JOYSTICK_A,15);
static Bounce bButton = Bounce(JOYSTICK_B,15);
static Bounce cButton = Bounce(JOYSTICK_C,15);
static Bounce dButton = Bounce(JOYSTICK_D,15);

enum {
  kUIStateMain,
  kUIStateCartesian,
  kUIStateReceive,
  kUIStateJogXY,
  kUIStateJogZ,
  kUIStateJogAB,
  kUIStateHomeing,
  kUIStateSetWCS,
  kUIStateShowWCS,
  kUIStateSetZero,
  kUIStateDisableSteppers,
  kUIStateEmergencyStop,
  kUIStateLast
};

const char kMainMenu[][20] = {
	"Arm for data",
	"Cartesian",
	"(Temperatures)"
};
enum {
	kMainItemArm,
	kMainItemCartesian,
	kMainItemTemperatures,
	kMainItemCount
};

const char kCartesianMenu[][20] = {
	"Back to main",
	"Jog XY",
	"Jog Z",
	"Jog AB",
	"Set Zero All",
	"Set Zero XY",
	"Set Zero Z",
	"Set WCS",
	"Show WCS",
	"Find Home",
	"Disable Steppers"
};

enum {
	kCartesianItemBack,
	kCartesianItemJogXY,
	kCartesianItemJogZ,
	kCartesianItemJogAB,
	kCartesianItemZeroAll,
	kCartesianItemZeroXY,
	kCartesianItemZeroZ,
	kCartesianItemSetWCS,
	kCartesianItemShowWCS,
	kCartesianItemHome,
	kCartesianItemDisableSteppers,
	kCartesianItemCount
};

const char kSetWCSMenu[][20] = {
	"Cancel",
	"Save as G54",
	"Save as G55",
	"Save as G56",
	"Save as G57",
	"Save as G58",
	"Save as G59"
};
enum {
	kSetWCSCancel,
	kSetWCSG54,
	kSetWCSG55,
	kSetWCSG56,
	kSetWCSG57,
	kSetWCSG58,
	kSetWCSG59,
	kSetWCSItemCount
};

const char kEmergencyStopMenu[][20] = {
	"Abort",
	"Resume"
};
enum {
	kEmergencyItemAbort,
	kEmergencyItemResume,
	kEmergencyItemCount
};

static char displayBuffer[21];

static char *ftoa(char *a, float f, int width, int precision)
{
  static double p[] = {0., 10., 100., 1000., 10000., 100000., 1000000., 10000000., 100000000.};
  
  char *ret = a;
  static char front[5];
  static char back[5];
  char *pFront = front;
  char *pBack = back;
  
  long integer = (long)f;
  itoa(integer, front, 10);

  long mantisse = abs((long)round(((double)f - (double)integer) * p[precision]));
  itoa(mantisse, back, 10);
  
  int frontWidth = width-precision-1;
  int leading = frontWidth-strlen(front);
  int tailing = precision-strlen(back);
  
  while(leading-->0)
  	*a++ = ' ';
  while(*pFront!='\0')
  	*a++ = *pFront++;
  *a++ = '.';
  while(*pBack != '\0')
  	*a++ = *pBack++; 	
  while(tailing-->0)
  	*a++ = '0';
  *a='\0';
  return ret;
}
    
    
void LcdUi::startup()
{
    uiState = kUIStateMain;
    savedUIState = uiState;
  
  	lastPositionUpdate=millis();
  	
  	initializeLCDHardware();
  	        
    clearDisplay();
   	
   	sprintf(fwNameAndVersion, "*PleasantMill %s*",FW_VERSION);
   	
   	writeAtLine(1, fwNameAndVersion);
    writeAtLine(4, "Initializing...");
	delay(500);
	
    pinMode(JOYSTICK_P, INPUT);
    pinMode(JOYSTICK_A, INPUT);
    pinMode(JOYSTICK_B, INPUT);
    pinMode(JOYSTICK_C, INPUT);
    pinMode(JOYSTICK_D, INPUT);   
    pinMode(EMERGENCY_STOP, INPUT);
    
    // Internal Pullups
    digitalWrite(EMERGENCY_STOP,HIGH);   
   	pButton.write(HIGH);
	aButton.write(HIGH);
	bButton.write(HIGH);
	cButton.write(HIGH);
	dButton.write(HIGH);
  	
  	selectedMenuItem = kMainItemArm;
  	switchState(kUIStateMain);
}

void LcdUi::shutdown()
{
	clearDisplay();
	writeAtLine(1, "********************");
	writeAtLine(2, "* MACHINE SHUTDOWN *");
	writeAtLine(3, "********************");
	writeAtLine(4, "Reset to restart...");
}

void LcdUi::waitForPushButtonRelease()
{
	do {
	  pButton.update();
	  sharedMachineModel.manage(false);
	} while(pButton.read()==LOW);
}

void LcdUi::handleUI()
{
  if(sharedMachineModel.emergencyStop && uiState != kUIStateEmergencyStop)
  {
    savedUIState = uiState;
    switchState(kUIStateEmergencyStop);
  }

  refreshLivePosition();
	
  pButton.update();
  aButton.update();
  bButton.update();
  cButton.update();
  dButton.update();

  byte keyState = kKeyStateNone;
  if(pButton.read()==LOW)  keyState |= kKeyStateP; 
  if(aButton.read()==LOW)  keyState |= kKeyStateA;
  if(bButton.read()==LOW)  keyState |= kKeyStateB;
  if(cButton.read()==LOW)  keyState |= kKeyStateC;
  if(dButton.read()==LOW)  keyState |= kKeyStateD; 
   
	switch(uiState)
	{
	  case kUIStateMain:
		if(handleMenu(keyState, kMainMenu, kMainItemCount))
			handleMainMenuSelection();
		break;
	  case kUIStateReceive:
	  	if(sharedMachineModel.qEmpty())
			writeAtLine(1, "Waiting for data");
		else
			writeAtLine(1, "Processing data");			
	  	displayCartesianInfo(); 
		if(keyState!=lastKeyState && lastKeyState==kKeyStateNone && keyState==kKeyStateP)
		{
			waitForPushButtonRelease();
			sharedMachineModel.receiving=false;
			switchState(kUIStateMain);
		}
		break;
	  case kUIStateHomeing:
		displayCartesianInfo();
		break;
	  case kUIStateDisableSteppers:
		if(keyState!=lastKeyState && lastKeyState==kKeyStateNone && keyState==kKeyStateP)
		{
			waitForPushButtonRelease();
#if ENABLE_LINES == HAS_ENABLE_LINES
			// Motors on
			digitalWrite(X_ENABLE_PIN, ENABLE_ON);
			digitalWrite(Y_ENABLE_PIN, ENABLE_ON);
			digitalWrite(Z_ENABLE_PIN, ENABLE_ON);
			digitalWrite(A_ENABLE_PIN, ENABLE_ON);
			digitalWrite(B_ENABLE_PIN, ENABLE_ON);
#endif
			switchState(kUIStateCartesian);
		}
		break;
	  case kUIStateCartesian:
		if(handleMenu(keyState, kCartesianMenu, kCartesianItemCount))
			handleCartesianMenuSelection();
		break;
	  case kUIStateSetWCS:
		if(handleMenu(keyState, kSetWCSMenu, kSetWCSItemCount))
			handleSetWCSMenuSelection();
		break;
	  case kUIStateJogXY:
		jogXY(keyState);
		break;
	  case kUIStateJogZ:
		jogZ(keyState);
		break;
	  case kUIStateShowWCS:
		handleShowWCS(keyState);
		break;
	  case kUIStateEmergencyStop:
		if(handleMenu(keyState, kEmergencyStopMenu, kEmergencyItemCount))
			handleEmergencyStop();
		break;
	}
  
  lastKeyState = keyState;
}

void LcdUi::switchState(int newState)
{
//	Serial.print("switchState:");
//	Serial.println(newState);
//	
	uiState = newState;
	clearDisplay();
	switch(uiState)
	{
	  case kUIStateMain:
	  	displayMenu(fwNameAndVersion, kMainMenu, kMainItemCount);
	  	break;
	  case kUIStateReceive:
		writeAtLine(1, "Waiting for data");
		sharedMachineModel.receiving=true;
	  	displayCartesianInfo(); 
	  	break;
	  case kUIStateCartesian:
	  	displayMenu("** Cartesian **", kCartesianMenu, kCartesianItemCount);
	  	break;
	  case kUIStateJogXY:
		writeAtLine(1, "Jog XY (Push = Back)");
		displayXY(3);
		break;
	  case kUIStateJogZ:
		writeAtLine(1, "Jog Z  (Push = Back)");
		displayZ(3); 
		break;
	  case kUIStateSetWCS:
	  	selectedMenuItem = 0;
	  	displayMenu("** Set WCS **", kSetWCSMenu, kSetWCSItemCount);
	  	break;
	  case kUIStateShowWCS:
	  	selectedMenuItem = 0;
	  	showWCS();
		break;
	  case kUIStateHomeing:
		writeAtLine(1, "Finding Home...");
		sharedMachineModel.zeroZ();
		sharedMachineModel.zeroX();
		sharedMachineModel.zeroY();
		sharedMachineModel.absolutePositionValid=true;
		sharedMachineModel.localPosition.f = SLOW_FEEDRATE;     // Most sensible feedrate to leave it in
		switchState(kUIStateCartesian);
		break;
	  case kUIStateDisableSteppers:
		writeAtLine(1, "Steppers disabled!");
		writeAtLine(3, "Push to re-enable");
		writeAtLine(4, "stepper motors");
#if ENABLE_LINES == HAS_ENABLE_LINES
		// Motors off
		// Note - we ignore DISABLE_X etc here
		digitalWrite(X_ENABLE_PIN, !ENABLE_ON);
  		digitalWrite(Y_ENABLE_PIN, !ENABLE_ON);
  		digitalWrite(Z_ENABLE_PIN, !ENABLE_ON);
  		digitalWrite(A_ENABLE_PIN, !ENABLE_ON);
  		digitalWrite(B_ENABLE_PIN, !ENABLE_ON);
		sharedMachineModel.absolutePositionValid=false; // we loose certainty here...
#endif
		break;
	  case kUIStateEmergencyStop:
		selectedMenuItem = kEmergencyItemAbort;
	  	displayMenu("** EMERGENCY STOP **", kEmergencyStopMenu, kEmergencyItemCount);
		break;
	}
}
		
//void printKeyStates(byte keyState)
//{
//	Serial.print("P:");
//	Serial.print((keyState & kKeyStateP)?"X":" ");
//	Serial.print(" A:");
//	Serial.print((keyState & kKeyStateA)?"X":" ");
//	Serial.print(" B:");
//	Serial.print((keyState & kKeyStateB)?"X":" ");
//	Serial.print(" C:");
//	Serial.print((keyState & kKeyStateC)?"X":" ");
//	Serial.print(" D:");
//	Serial.println((keyState & kKeyStateD)?"X":" ");
//}
	
void LcdUi::handleEmergencyStop()
{
	if(selectedMenuItem==kEmergencyItemAbort) // Reset
	{
		// Delete everything in the ring buffer
		sharedMachineModel.cancelAndClearQueue();
		savedUIState = kUIStateMain;
	}
	sharedMachineModel.emergencyStop=false;
	switchState(savedUIState);
}

void LcdUi::handleMainMenuSelection()
{
	switch(selectedMenuItem)
	{
		case kMainItemArm: // Receive
			switchState(kUIStateReceive);
			break;
		case kMainItemCartesian: // Cartesian
			selectedMenuItem = kCartesianItemJogXY;
			switchState(kUIStateCartesian);
			break;
	}
}


void LcdUi::handleCartesianMenuSelection()
{
	FloatPoint tempPosition;
	switch(selectedMenuItem)
	{
		case kCartesianItemBack: // Back
			selectedMenuItem = kMainItemCartesian;
			switchState(kUIStateMain);
			break;
		case kCartesianItemJogXY:
			switchState(kUIStateJogXY);
			break;
		case kCartesianItemJogZ: // Cartesian
			switchState(kUIStateJogZ);
			break;
		case kCartesianItemZeroAll: // Set Zero
			clearDisplay();
			writeAtLine(1, "*** Set Zero All ***");
			displayCartesianInfo();
			delay(500);
			tempPosition = FloatPoint();
			sharedMachineModel.setLocalZero(tempPosition);
			lastPositionUpdate=0L; // Force refresh
			refreshLivePosition();
			displayCartesianInfo();
			delay(1000);
	  		displayMenu("** Cartesian **", kCartesianMenu, kCartesianItemCount);
			break;
		case kCartesianItemZeroXY: // Set Zero
			clearDisplay();
			writeAtLine(1, "*** Set Zero XY ***");
			displayCartesianInfo();
			delay(500);
			tempPosition=sharedMachineModel.localPosition;
			tempPosition.x=0;
			tempPosition.y=0;
			sharedMachineModel.setLocalZero(tempPosition);
			lastPositionUpdate=0L; // Force refresh
			refreshLivePosition();
			displayCartesianInfo();
			delay(1000);
	  		displayMenu("** Cartesian **", kCartesianMenu, kCartesianItemCount);
			break;
		case kCartesianItemZeroZ: // Set Zero
			clearDisplay();
			writeAtLine(1, "*** Set Zero Z ***");
			displayCartesianInfo();
			delay(500);
			tempPosition=sharedMachineModel.localPosition;
			tempPosition.z=0;
			sharedMachineModel.setLocalZero(tempPosition);
			lastPositionUpdate=0L; // Force refresh
			refreshLivePosition();
			displayCartesianInfo();
			delay(1000);
	  		displayMenu("** Cartesian **", kCartesianMenu, kCartesianItemCount);
			break;
		case kCartesianItemSetWCS:
			if(sharedMachineModel.absolutePositionValid)
				switchState(kUIStateSetWCS);
			else
				displayError("Machine not homed", "'Find Home' first", kUIStateCartesian);
			break;
		case kCartesianItemShowWCS:
			switchState(kUIStateShowWCS);
			break;
		case kCartesianItemHome: // Find home	
			switchState(kUIStateHomeing);
			break;                    
		case kCartesianItemDisableSteppers: // Disable Steppers
			switchState(kUIStateDisableSteppers);
			break;
	}
}

void LcdUi::handleSetWCSMenuSelection()
{
	if(selectedMenuItem!=kSetWCSCancel)
	{
		clearDisplay();
		int address = EEPROM_ADR_WCS_BASE;
		switch(selectedMenuItem)
		{
			case kSetWCSG54:
				writeAtLine(1,"*** Set WCS G54 ***");
				break;
			case kSetWCSG55:
				writeAtLine(1,"*** Set WCS G55 ***");
				address+=EEPROM_SIZE_WCS_VALUE;
				break;
			case kSetWCSG56:
				writeAtLine(1,"*** Set WCS G56 ***");
				address+=2*EEPROM_SIZE_WCS_VALUE;
				break;
			case kSetWCSG57:
				writeAtLine(1,"*** Set WCS G57 ***");
				address+=3*EEPROM_SIZE_WCS_VALUE;
				break;
			case kSetWCSG58:
				writeAtLine(1,"*** Set WCS G58 ***");
				address+=4*EEPROM_SIZE_WCS_VALUE;
				break;
			case kSetWCSG59:
				writeAtLine(1,"*** Set WCS G59 ***");
				address+=5*EEPROM_SIZE_WCS_VALUE;
				break;
		}
		FloatPoint offset = sharedMachineModel.localPosition + sharedMachineModel.localZeroOffset;
		displayWCSInfo(offset);
		EEPROM_WriteFloatPoint(address, offset);
		delay(1000);
	}
	selectedMenuItem = kCartesianItemSetWCS;
	switchState(kUIStateCartesian);
}

void LcdUi::manualToolChange(char* desc)
{
	clearDisplay();
	writeAtLine(1,"*** Tool change ***");
	writeAtLine(2,"Insert tool:");
	writeAtLine(3,desc);
	writeAtLine(4,"(Push when ready)");
	waitForPushButtonRelease();
	while(pButton.read()!=LOW)
	{
	  pButton.update();
	  sharedMachineModel.manage(false);
	  delay(10);
	}
	waitForPushButtonRelease();
	switchState(kUIStateReceive);
}


void LcdUi::displayMenu(char* title, const char menuText[][20], int mcount)
{
	clearDisplay();
	writeAtLine(1,title);
	updateMenuSelection(menuText, mcount);
}

void LcdUi::updateMenuSelection(const char menuText[][20], int mcount)
{
	int offset=1;
	if(selectedMenuItem<1)
		offset=0;
	else if(mcount>2 && selectedMenuItem>=mcount-1)
		offset=2;
		
	int firstDisplayLine = 2;
	if(mcount<3)
		firstDisplayLine=3;
	for(int i=0; i<2+3-firstDisplayLine;i++)
	{
		int mIndex=selectedMenuItem-offset+i;
//		Serial.print("I");
//		Serial.print(i);
//		Serial.print(" S");
//		Serial.print(selectedMenuItem);
//		Serial.print(" O");
//		Serial.print(offset);
//		Serial.print(" M");
//		Serial.print(mIndex);
//		Serial.print(" F");
//		Serial.println(firstDisplayLine);
		
		sprintf(displayBuffer,"%c%s", (mIndex==selectedMenuItem)?0x7e:' ', menuText[mIndex]);//, (mIndex==selectedMenuItem)?0x7f:' ');
		writeAtLine(i+firstDisplayLine, displayBuffer);
	}
}

bool LcdUi::handleMenu(byte keyState, const char menuText[][20], int mcount)
{
	bool itemSelected=false;
	if(lastKeyState==kKeyStateNone)
	{
		if(keyState==kKeyStateP)
		{
			waitForPushButtonRelease();
			lastKeyState==kKeyStateP;
			itemSelected=true;
		}
		else if(keyState&kKeyStateA && selectedMenuItem > 0)
		{
		  selectedMenuItem--;
		  updateMenuSelection(menuText, mcount);
		}
		else if(keyState&kKeyStateC && selectedMenuItem < mcount-1)
		{
		  selectedMenuItem++;
		  updateMenuSelection(menuText, mcount);
		}
	}
	return itemSelected;
}

void LcdUi::jogXY(byte keyState)
{
  displayXY(3);
  if(keyState!=lastKeyState)
  {
  	if(lastKeyState==kKeyStateNone && keyState==kKeyStateP)
  	{
		waitForPushButtonRelease();
		switchState(kUIStateCartesian);
	}
  	else
	{
		sharedMachineModel.cancelAndClearQueue();
		
		if(lastKeyState!=kKeyStateNone)
		{
			float rf = sharedMachineModel.localPosition.f;
			sharedMachineModel.localPosition = sharedMachineModel.livePosition();
			sharedMachineModel.localPosition.f = rf;
		}

		FloatPoint p=sharedMachineModel.localPosition;			
		if(keyState&kKeyStateA)
		  p.y-=MACHINE_MAX_Y_MM;
		if(keyState&kKeyStateD)
		  p.x+=MACHINE_MAX_X_MM;
		if(keyState&kKeyStateC)
		  p.y+=MACHINE_MAX_Y_MM;
		if(keyState&kKeyStateB)
		  p.x-=MACHINE_MAX_X_MM;
		if(keyState!=kKeyStateNone)
		{
			p.f=FAST_XY_FEEDRATE;
			sharedMachineModel.qMove(p);
		}
	}
  }
}

void LcdUi::jogZ(byte keyState)
{
  displayZ(3);
  if(keyState!=lastKeyState)
  {
  	if(lastKeyState==kKeyStateNone && keyState==kKeyStateP)
  	{
		waitForPushButtonRelease();
		switchState(kUIStateCartesian);
	}
  	else
	{
		sharedMachineModel.cancelAndClearQueue();
		
		if(lastKeyState!=kKeyStateNone)
		{
			float rf = sharedMachineModel.localPosition.f;
			sharedMachineModel.localPosition = sharedMachineModel.livePosition();
			sharedMachineModel.localPosition.f = rf;
		}
			
		FloatPoint p=sharedMachineModel.localPosition;
			
		if(keyState&kKeyStateA)
		  p.z+=MACHINE_MAX_Z_MM;
		if(keyState&kKeyStateC)
		  p.z-=MACHINE_MAX_Z_MM;
		if(keyState!=kKeyStateNone)
		{
			p.f=FAST_Z_FEEDRATE;
			sharedMachineModel.qMove(p);
		}
	}
  }
}

void LcdUi::showWCS()
{
	sprintf(displayBuffer, "** Show WCS: G5%d **", selectedMenuItem+4);
	writeAtLine(1, displayBuffer);
	FloatPoint offset = EEPROM_ReadFloatPoint(EEPROM_ADR_WCS_BASE+selectedMenuItem*EEPROM_SIZE_WCS_VALUE);			
	displayWCSInfo(offset);
}

void LcdUi::handleShowWCS(byte keyState)
{
  if(keyState!=lastKeyState)
  {
  	if(lastKeyState==kKeyStateNone && keyState==kKeyStateP)
  	{
		waitForPushButtonRelease();
		selectedMenuItem=kUIStateShowWCS;
		switchState(kUIStateCartesian);
	}
  	else
	{
		if(keyState&kKeyStateC)
		{
			selectedMenuItem++;
			if(selectedMenuItem>=WCS_COUNT)
				selectedMenuItem=0;
			showWCS();
			waitForPushButtonRelease();
		}
		else if(keyState&kKeyStateA)
		{
			selectedMenuItem--;
			if(selectedMenuItem<0)
				selectedMenuItem=WCS_COUNT-1;
			showWCS();
			waitForPushButtonRelease();
		}
	}
  }
}

static char ab[11], bb[11], xb[11], yb[11], zb[11], fb[11];
void LcdUi::displayXY(int line)
{
  sprintf(displayBuffer, "X:%s Y:%s", ftoa(xb,livePosition.x, 7, 2),  ftoa(yb,livePosition.y, 7, 2));
  
  writeAtLine(line, displayBuffer);
}

void LcdUi::displayAB(int line)
{
  sprintf(displayBuffer, "A:%s B:%s", ftoa(ab,livePosition.a, 7, 2),  ftoa(bb,livePosition.b, 7, 2));  
  writeAtLine(line, displayBuffer);
}

void LcdUi::displayZ(int line)
{
  sprintf(displayBuffer, "Z:%s", ftoa(zb,livePosition.z, 7, 2));
  writeAtLine(line, displayBuffer);
}

void LcdUi::displayCartesianInfo()
{
  sprintf(displayBuffer, "X:%s A:%s", ftoa(xb,livePosition.x, 7, 2), ftoa(ab,livePosition.a, 7, 2));
  writeAtLine(2, displayBuffer);
  sprintf(displayBuffer, "Y:%s B:%s", ftoa(yb,livePosition.y, 7, 2), ftoa(bb,livePosition.b, 7, 2));
  writeAtLine(3, displayBuffer);
  sprintf(displayBuffer, "Z:%s F:%s", ftoa(zb,livePosition.z, 7, 2), ftoa(fb,livePosition.f, 7, 2));
  writeAtLine(4, displayBuffer);
}

void LcdUi::displayWCSInfo(FloatPoint wcsPoint)
{
  sprintf(displayBuffer, "X:%s A:%s", ftoa(xb,wcsPoint.x, 7, 2), ftoa(ab,wcsPoint.a, 7, 2));
  writeAtLine(2, displayBuffer);
  sprintf(displayBuffer, "Y:%s B:%s", ftoa(yb,wcsPoint.y, 7, 2), ftoa(bb,wcsPoint.b, 7, 2));
  writeAtLine(3, displayBuffer);
  sprintf(displayBuffer, "Z:%s", ftoa(zb,wcsPoint.z, 7, 2));
  writeAtLine(4, displayBuffer);
}

void LcdUi::displayError(const char* line1, const char* line2, int nextState)
{
	clearDisplay();
	writeAtLine(1,"*** ERROR ***");
	writeAtLine(2,line1);
	writeAtLine(3,line2);
	writeAtLine(4,"Push Button...");
	waitForPushButtonRelease();
	while(pButton.read()!=LOW)
	{
	  pButton.update();
	  sharedMachineModel.manage(false);
	  delay(10);
	}
	waitForPushButtonRelease();
	switchState(nextState);
}


void LcdUi::refreshLivePosition()
{
	if(millis()-lastPositionUpdate>100)
	{
		lastPositionUpdate=millis();
		livePosition = sharedMachineModel.livePosition();
	}
}

//
// Hardware abstraction for LCD
//

void LcdUi::initializeLCDHardware()
{
    // Setup I2C hardware
    Wire.begin();
    // Switch on LCD
    Wire.beginTransmission(kI2C_LCD_SLAVE_ADR);
    Wire.send(kI2C_DisplayON_OFF);
    Wire.send(kI2C_DisplayON); // On, no Cursor, no Blink
    Wire.endTransmission();
}

void LcdUi::clearDisplay()
{
    // Clear the Display
    Wire.beginTransmission(kI2C_LCD_SLAVE_ADR);
    Wire.send(kI2C_ClrDisplay);
    Wire.endTransmission();
}


void LcdUi::writeAtLine(int line, const char* text)
{
  int len = strlen(text);
  Wire.beginTransmission(kI2C_LCD_SLAVE_ADR);
  Wire.send(kI2C_SetCursor);
  switch(line)
  {
    case 1: Wire.send(kLCD_DDA_Line0); break;
    case 2: Wire.send(kLCD_DDA_Line1); break;
    case 3: Wire.send(kLCD_DDA_Line2); break;
    case 4: Wire.send(kLCD_DDA_Line3); break;
  }
  Wire.endTransmission();
  
  Wire.beginTransmission(kI2C_LCD_SLAVE_ADR);
  Wire.send(kI2C_WriteString);
  Wire.send((char*)text);
  Wire.endTransmission();
  
  if(len<20)
  {
    Wire.beginTransmission(kI2C_LCD_SLAVE_ADR);
    Wire.send(kI2C_Delete);
    Wire.send(line);
    Wire.send(len);
    Wire.send(20-len);
    Wire.endTransmission();
  }
}

// writeAtRange is not fully tested yet...
//void LcdUi::writeAtLine(int line, const char* text)
//{
//	writeAtRange(line, 0, 20, text);
//}
//
//void LcdUi::writeAtRange(int line, int from, int len, const char* text)
//{
//	byte cursorAddress = from;
//	switch(line)
//	{
//		case 1: cursorAddress+=kLCD_DDA_Line0; break;
//		case 2: cursorAddress+=kLCD_DDA_Line1; break;
//		case 3: cursorAddress+=kLCD_DDA_Line2; break;
//		case 4: cursorAddress+=kLCD_DDA_Line3; break;
//	}
//	
//	Wire.beginTransmission(kI2C_LCD_SLAVE_ADR);
//	Wire.send(kI2C_SetCursor);
//	Wire.send(cursorAddress);
//	Wire.endTransmission();
//
//	Wire.beginTransmission(kI2C_LCD_SLAVE_ADR);
//  	Wire.send(kI2C_WriteString);
//  	Wire.send((char*)text);
//  	Wire.endTransmission();
//
//	int textlen = strlen(text);
//
//	if(textlen<len)
//	{
//		Wire.beginTransmission(kI2C_LCD_SLAVE_ADR);
//		Wire.send(kI2C_Delete);
//		Wire.send(line);
//		Wire.send(cursorAddress+textlen);
//		Wire.send(cursorAddress+len);
//		Wire.endTransmission();
//	}
//	
//}
