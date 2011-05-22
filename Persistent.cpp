/************
 * Persistent Helper Funktionen (EEPROM)
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
 
 
#include <EEPROM.h>
#include "WProgram.h"
#include "Persistent.h"
 
void checkEEPROM()
{
   char ident0 = (char)EEPROM.read(EEPROM_ADR_IDENT);
   char ident1 = (char)EEPROM.read(EEPROM_ADR_IDENT+1);
   char ident2 = (char)EEPROM.read(EEPROM_ADR_IDENT+2);
   if(ident0 != EEPROM_IDENTIFIER0 || ident1 != EEPROM_IDENTIFIER1 || ident2 != EEPROM_IDENTIFIER2) // EEPROM not initialized!
   {
     // Initialize EEPROM with defaults
     
     // 6 WCS coordinates
     FloatPoint zeroPoint = FloatPoint();
     for(int i=0;i<WCS_COUNT; i++)
     	EEPROM_WriteFloatPoint(EEPROM_ADR_WCS_BASE+i*EEPROM_SIZE_WCS_VALUE, zeroPoint);
     
     // 6 Tool Records
     char* nullString = "";
     for(int i=0; i<TOOL_COUNT; i++)
     	EEPROM_WriteString(EEPROM_ADR_TOOL_BASE+i*EEPROM_SIZE_TOOL_RECORD, nullString);
          
     EEPROM.write(EEPROM_ADR_IDENT, EEPROM_IDENTIFIER0);
     EEPROM.write(EEPROM_ADR_IDENT+1, EEPROM_IDENTIFIER1);
     EEPROM.write(EEPROM_ADR_IDENT+2, EEPROM_IDENTIFIER2);
   }
}

void EEPROM_WriteFloat(int address, float value)
{
    byte* p = (byte*)(void*)&value;
    for (int i = 0; i < sizeof(value); i++)
	  EEPROM.write(address++, *p++);
}

float EEPROM_ReadFloat(int address)
{
    float value = 0.0;
    byte* p = (byte*)(void*)&value;
    for (int i = 0; i < sizeof(value); i++)
	  *p++ = EEPROM.read(address++);
    return value;
} 

void EEPROM_WriteFloatPoint(int address, FloatPoint value)
{
	EEPROM_WriteFloat(address, value.x);
	EEPROM_WriteFloat(address+sizeof(float), value.y);
	EEPROM_WriteFloat(address+2*sizeof(float), value.z);
	EEPROM_WriteFloat(address+3*sizeof(float), value.a);
	EEPROM_WriteFloat(address+4*sizeof(float), value.b);
}

FloatPoint EEPROM_ReadFloatPoint(int address)
{
	FloatPoint value = FloatPoint();
	value.x = EEPROM_ReadFloat(address);
	value.y = EEPROM_ReadFloat(address+sizeof(float));
	value.z = EEPROM_ReadFloat(address+2*sizeof(float));
	value.a = EEPROM_ReadFloat(address+3*sizeof(float));
	value.b = EEPROM_ReadFloat(address+4*sizeof(float));
	return value;
}

void EEPROM_WriteString(int address, const char* value)
{
	for(int i=0; i<strlen(value); i++)
	  EEPROM.write(address++, *value++);
}

char* EEPROM_ReadString(int address, char* buffer)
{
	char c;
	do {
		c = EEPROM.read(address++);
		*buffer++ = c;
	} while(c!=0x0);
	return buffer;
}