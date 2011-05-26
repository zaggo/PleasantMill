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
 
#include "vectors.h"

 // EEPROM
#define EEPROM_IDENTIFIER0 'P'
#define EEPROM_IDENTIFIER1 'M'
#define EEPROM_IDENTIFIER2 '1'

#define EEPROM_ADR_IDENT 0
#define EEPROM_SIZE_IDENT 3

#define WCS_COUNT 6
#define EEPROM_ADR_WCS_BASE EEPROM_SIZE_IDENT
#define EEPROM_SIZE_WCS_VALUE sizeof(FloatPoint)
#define EEPROM_SIZE_WCS (WCS_COUNT*EEPROM_SIZE_WCS_VALUE)

#define TOOL_COUNT 6
#define EEPROM_ADR_TOOL_BASE (EEPROM_ADR_WCS_BASE+EEPROM_SIZE_WCS)
#define EEPROM_SIZE_TOOL_RECORD 21
#define EEPROM_SIZE_TOOL (TOOL_COUNT*EEPROM_SIZE_TOOL_RECORD)

 
void checkEEPROM();

void EEPROM_WriteFloat(int address, float value);
float EEPROM_ReadFloat(int address);

void EEPROM_WriteFloatPoint(int address, FloatPoint value);
FloatPoint EEPROM_ReadFloatPoint(int address);

void EEPROM_WriteString(int address, const char* value);
char* EEPROM_ReadString(int address, char* buffer);
