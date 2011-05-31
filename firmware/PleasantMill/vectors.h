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
 */

#include "WProgram.h"

#ifndef VECTORS_H
#define VECTORS_H

#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

class LongPoint;

// Real-world units
class FloatPoint
{
public:
	float x;   // Coordinate axes
	float y;
	float z;
	float a;   
	float b;   
	float f;   // Feedrate

	FloatPoint();
	
	operator LongPoint();
	FloatPoint operator+(FloatPoint const& b) const;
	FloatPoint operator-(FloatPoint const& b) const;
	FloatPoint operator*(FloatPoint const& b) const;
};

// Integer numbers of steps
class LongPoint
{
public:
	long x;   // Coordinates
	long y;
	long z;
	long a;   // 
	long b;   // 
	long f;   // Feedrate
	
	LongPoint();

	operator FloatPoint() const;
	LongPoint operator+(LongPoint const& v) const;
	LongPoint operator-(LongPoint const& v) const;
};

inline FloatPoint::FloatPoint()
{
	x = 0;
	y = 0;
	z = 0;
	a = 0;
	b = 0;
	f = 0;
} 

inline FloatPoint::operator LongPoint()
{
	LongPoint result;
	result.x = round(x);
	result.y = round(y);
	result.z = round(z);
	result.a = round(a);
	result.b = round(b);
	result.f = round(f);
	return result;
} 

inline FloatPoint FloatPoint::operator+(FloatPoint const& v) const
{
	FloatPoint result;
	result.x = x + v.x;
	result.y = y + v.y;
	result.z = z + v.z;
	result.a = a + v.a;
	result.b = b + v.b;
	result.f = f + v.f;
	return result;
}  

inline FloatPoint FloatPoint::operator-(FloatPoint const& v) const
{
	FloatPoint result;
	result.x = x - v.x;
	result.y = y - v.y;
	result.z = z - v.z;
	result.a = a - v.a;
	result.b = b - v.b;
	result.f = f - v.f;
	return result;
} 


// NB - the next gives neither the scalar nor the vector product

inline FloatPoint FloatPoint::operator*(FloatPoint const& v) const
{
	FloatPoint result;
	result.x = x * v.x;
	result.y = y * v.y;
	result.z = z * v.z;
	result.a = a * v.a;
	result.b = b * v.b;
	result.f = f * v.f;
	return result;
} 

inline LongPoint::LongPoint()
{
	x = 0;
	y = 0;
	z = 0;
	a = 0;
	b = 0;
	f = 0;
} 

inline LongPoint::operator FloatPoint() const
{
	FloatPoint v;
	v.x = (float)x;
	v.y = (float)y;
	v.z = (float)z;
	v.a = (float)a;
	v.b = (float)b;
	v.f = (float)f;
	return v;
}  

inline LongPoint LongPoint::operator+(LongPoint const& v) const
{
	LongPoint result;
	result.x = x + v.x;
	result.y = y + v.y;
	result.z = z + v.z;
	result.a = a + v.a;
	result.b = b + v.b;
	result.f = f + v.f;
	return result;
}  

inline LongPoint LongPoint::operator-(LongPoint const& v) const
{
	LongPoint result;
	result.x = x - v.x;
	result.y = y - v.y;
	result.z = z - v.z;
	result.a = a - v.a;
	result.b = b - v.b;
	result.f = f - v.f;
	return result;
} 

inline LongPoint absv(const LongPoint& v)
{
	LongPoint result;
	result.x = abs(v.x);
	result.y = abs(v.y);
	result.z = abs(v.z);
	result.a = abs(v.a);
	result.b = abs(v.b);
	result.f = abs(v.f);
	return result;
} 


// Can't use fabs for this as it's defined somewhere in a #define

inline FloatPoint fabsv(const FloatPoint& a)
{
	FloatPoint result;
	result.x = fabs(a.x);
	result.y = fabs(a.y);
	result.z = fabs(a.z);
	result.a = fabs(a.a);
	result.b = fabs(a.b);
	result.f = fabs(a.f);
	return result;
} 

inline LongPoint to_steps(const FloatPoint& units, const FloatPoint& position)
{
	return (LongPoint)(units*position);
}

inline FloatPoint from_steps(const FloatPoint& units, const LongPoint& position)
{
	FloatPoint inv = units;
	inv.x = 1.0/inv.x;
	inv.y = 1.0/inv.y;
	inv.z = 1.0/inv.z;
	inv.a = 1.0/inv.a;
	inv.b = 1.0/inv.b;
	inv.f = 1.0/inv.f;

	return inv*(FloatPoint)position;
}
#endif
