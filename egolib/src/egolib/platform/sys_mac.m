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

/// @file egolib/platform/sys_mac.c
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
void sys_initialize(void)
{
    _sys_startdate = [NSDate date];
    [_sys_startdate retain];
}

//--------------------------------------------------------------------------------------------
double sys_getTime(void)
{
    return [[NSDate date] timeIntervalSinceDate:_sys_startdate];
}

//--------------------------------------------------------------------------------------------
void sys_popup( const char * popup_title, const char * warning, const char * format, va_list args )
{
    /// @author PF5
    /// @details Show error message to Mac users instead of just closing for no reason
    
    //NSStringify the strings
    NSString * warningString = [[NSString alloc] initWithUTF8String:warning];
    NSString * formatString = [[NSString alloc] initWithUTF8String:format];
    
    //Ready the message
    NSString * formattedString = [[NSString alloc] initWithFormat:formatString arguments:args];
    NSString * message = [warningString stringByAppendingString:formattedString];
    
    // Setup the alert
    NSAlert * alert = [[NSAlert alloc] init];
    [alert setAlertStyle:NSInformationalAlertStyle];
    [alert addButtonWithTitle:@"OK"];
    [alert setMessageText:message];
    
    // Run the alert
    [alert runModal];
    
    // Cleanup
    [alert release];
    [warningString release];
    [formatString release];
    [formattedString release];
    
}