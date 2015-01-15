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

/// @file egolib/log.c
/// @brief Basic logging functionality.
/// @details

#include "egolib/log.h"

#include "egolib/file_common.h"
#include "egolib/strutil.h"
#include "egolib/egoboo_setup.h"
#include "egolib/platform.h"
#include "egolib/system.h"

#ifdef __WINDOWS__
#include <windows.h>
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static constexpr size_t MAX_LOG_MESSAGE = 1024; ///< Max length of log messages

static FILE *logFile = nullptr;
static LogLevel   _logLevel = LOG_WARNING;   ///default log level

static int _atexit_registered = 0;

enum ConsoleColor
{
    CONSOLE_TEXT_RED,
    CONSOLE_TEXT_YELLOW,
    CONSOLE_TEXT_WHITE,
    CONSOLE_TEXT_GRAY
};

/**
* Setting console colours is not cross-platform, so we have to do it with macros
**/
static void setConsoleColor(ConsoleColor color)
{

    //Windows implementation to set console colour
#ifdef __WINDOWS__
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    switch(color)
    {
        case CONSOLE_TEXT_RED:
            SetConsoleTextAttribute(consoleHandle, FOREGROUND_RED | FOREGROUND_INTENSITY);
        break;

        case CONSOLE_TEXT_YELLOW:
            SetConsoleTextAttribute(consoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        break;

        case CONSOLE_TEXT_WHITE:
            SetConsoleTextAttribute(consoleHandle, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        break;

        case CONSOLE_TEXT_GRAY:
            SetConsoleTextAttribute(consoleHandle, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
        break;
    }
#endif

    //unix implementation to set console colour
#if defined(__unix__) || defined(__APPLE__) || defined(macintosh)
    switch(color)
    {
        case CONSOLE_TEXT_RED:
            fputs("\e[0;31m", stdout);
        break;
        
        case CONSOLE_TEXT_YELLOW:
            fputs("\e[1;33m", stdout);
        break;

        case CONSOLE_TEXT_WHITE:
            fputs("\e[0;37m", stdout);
        break;

        case CONSOLE_TEXT_GRAY:
            fputs("\e[0;30m", stdout);
        break;
    }
#endif
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void writeLogMessage(LogLevel logLevel, const char *format, va_list args)
{
    char  logBuffer[MAX_LOG_MESSAGE] = EMPTY_CSTR;

    //Add prefix
    const char *prefix;
    switch(logLevel)
    {
        case LOG_ERROR:
            setConsoleColor(CONSOLE_TEXT_RED);
            prefix = "FATAL ERROR: ";
        break;

        case LOG_WARNING:
            setConsoleColor(CONSOLE_TEXT_YELLOW);
            prefix = "WARNING: ";
        break;

        case LOG_INFO:
            setConsoleColor(CONSOLE_TEXT_WHITE);
            prefix = "INFO: ";
        break;

        case LOG_DEBUG:
            setConsoleColor(CONSOLE_TEXT_GRAY);
            prefix = "DEBUG: ";
        break;

        default:
        case LOG_NONE:
            setConsoleColor(CONSOLE_TEXT_WHITE);
            prefix = "";//no prefix
        break;
    }

    //Build log message
    vsnprintf(logBuffer, MAX_LOG_MESSAGE - 1, format, args);

    if (nullptr != logFile)
    {
        //Log to file
        fputs(prefix, logFile);
        fputs(logBuffer, logFile);
        fflush(logFile);
    }

    //Log to console
    fputs(prefix, stdout);
    fputs(logBuffer, stdout);

    //Restore default color
    setConsoleColor(CONSOLE_TEXT_GRAY);
}

//--------------------------------------------------------------------------------------------
void log_init(const char *logname, LogLevel logLevel)
{
    _logLevel = logLevel;

    if (nullptr == logFile)
    {
        logFile = fopen(logname, "wt");
        if (nullptr != logFile && !_atexit_registered)
        {
            _atexit_registered = 1;
            atexit( log_shutdown );
        }
    }
}

//--------------------------------------------------------------------------------------------
void log_shutdown()
{
    if (nullptr != logFile)
    {
        fclose(logFile);
        logFile = nullptr;
    }
}

//--------------------------------------------------------------------------------------------
void log_message(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    writeLogMessage(LOG_NONE, format, args);
    va_end(args);
}

//--------------------------------------------------------------------------------------------
void log_debug(const char *format, ...)
{
    va_list args;

    // Only if developer mode is enabled
    if (!cfg.dev_mode) return;

    va_start(args, format);
    if (_logLevel >= LOG_DEBUG)
    {
        writeLogMessage(LOG_DEBUG, format, args);
    }
    va_end(args);
}

//--------------------------------------------------------------------------------------------
void log_info(const char *format, ...)
{
    va_list args;
    if (_logLevel >= LOG_INFO)
    {
        va_start(args, format);
        writeLogMessage(LOG_INFO, format, args);
        va_end(args);
    }
}

//--------------------------------------------------------------------------------------------
void log_warning(const char *format, ...)
{
    va_list args;
    if (_logLevel >= LOG_WARNING)
    {
        va_start(args, format);
        writeLogMessage(LOG_WARNING, format, args);
        va_end(args);
    }
}

//--------------------------------------------------------------------------------------------
void log_error(const char *format, ...)
{
    va_list args, args2;

    va_start( args, format );
    va_copy( args2, args );
    writeLogMessage(LOG_ERROR, format, args );

    //Display an OS messagebox
    sys_popup( "Egoboo: Fatal Error", "Egoboo has encountered a problem and is exiting. \nThis is the error report: \n", format, args2 );

    va_end( args );
    va_end( args2 );

    fflush(logFile);
    exit(EXIT_FAILURE);
}

//--------------------------------------------------------------------------------------------
FILE *log_get_file()
{
    return logFile;
}
