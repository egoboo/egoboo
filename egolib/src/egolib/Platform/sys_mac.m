//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file egolib/Platform/sys_mac.m
/// @brief Implementation of mac system-dependent functions
/// @details

#import <AppKit/NSAlert.h>
#import <Foundation/NSString.h>
#import <Foundation/NSDate.h>

#include "egolib/system.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static NSDate * _sys_startdate;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void sys_initialize()
{
    _sys_startdate = [[NSDate alloc] init];
}

//--------------------------------------------------------------------------------------------
double sys_getTime()
{
    return -[_sys_startdate timeIntervalSinceNow];
}
