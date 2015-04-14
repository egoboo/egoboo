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
#include "egolib/vfs.h"

#ifdef __WINDOWS__
#include <windows.h>
#endif

static CONSTEXPR size_t MAX_LOG_MESSAGE = 1024; ///< Max length of log messages.

static vfs_FILE *logFile = nullptr;             ///< Log file.
static LogLevel _logLevel = LOG_WARNING;        ///< Default log level.

static bool _atexit_registered = false;

enum ConsoleColor
{
    CONSOLE_TEXT_RED,
    CONSOLE_TEXT_YELLOW,
    CONSOLE_TEXT_WHITE,
    CONSOLE_TEXT_GRAY,
    CONSOLE_TEXT_DEFAULT
};

/**
 * Setting console colours is not cross-platform, so we have to do it with macros
 */
static void setConsoleColor(ConsoleColor color)
{

    // Windows implementation to set console colour
#ifdef _WIN32
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
        case CONSOLE_TEXT_DEFAULT:
            SetConsoleTextAttribute(consoleHandle, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
        break;
    }
#endif

    // unix implementation to set console colour
#if defined(__unix__)
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
        
        case CONSOLE_TEXT_DEFAULT:
            fputs("\e[0m", stdout);
        break;
    }
    fflush(stdout);
#endif
}

static void writeLogMessage(LogLevel logLevel, const char *format, va_list args)
{
    char logBuffer[MAX_LOG_MESSAGE] = EMPTY_CSTR;

    // Add prefix
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
            prefix = ""; // no prefix
        break;
    }

    // Build log message
    vsnprintf(logBuffer, MAX_LOG_MESSAGE - 1, format, args);

    if (nullptr != logFile)
    {
        // Log to file
        vfs_puts(prefix, logFile);
        vfs_puts(logBuffer, logFile);
    }

    // Log to console
    fputs(prefix, stdout);
    fputs(logBuffer, stdout);

    // Restore default color
    setConsoleColor(CONSOLE_TEXT_DEFAULT);
}

void log_initialize(const char *logname, LogLevel logLevel)
{
    _logLevel = logLevel;

    if (nullptr == logFile)
    {
        logFile = vfs_openWriteB(logname);
        if (!logFile)
        {
            _logLevel = LOG_WARNING;
            throw std::runtime_error("unable to initialize logging system");
        }
    }
    if (!_atexit_registered)
    {
        if (atexit(log_uninitialize))
        {
            vfs_close(logFile);
            logFile = nullptr;
            _logLevel = LOG_WARNING;
            throw std::runtime_error("unable to initialize logging system");
        }
        _atexit_registered = true;
    }
}

void log_uninitialize()
{
    if (nullptr != logFile)
    {
        vfs_close(logFile);
        logFile = nullptr;
    }
    _logLevel = LOG_WARNING;
}

void logv(LogLevel level, const char *format, va_list args)
{
    writeLogMessage(level, format, args);
}

void log(LogLevel level, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    writeLogMessage(level, format, args);
    va_end(args);
}

void log_message(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    logv(LOG_NONE, format, args);
    va_end(args);
}

void log_debug(const char *format, ...)
{
    // Only if developer mode is enabled.
    if (!egoboo_config_t::get().debug_developerMode_enable.getValue())
    {
        return;
    }

    va_list args;
    va_start(args, format);
    if (_logLevel >= LOG_DEBUG)
    {
        logv(LOG_DEBUG, format, args);
    }
    va_end(args);
}

void log_info(const char *format, ...)
{
    if (_logLevel >= LOG_INFO)
    {
        va_list args;
        va_start(args, format);
        logv(LOG_INFO, format, args);
        va_end(args);
    }
}

void log_warning(const char *format, ...)
{
    if (_logLevel >= LOG_WARNING)
    {
        va_list args;
        va_start(args, format);
        logv(LOG_WARNING, format, args);
        va_end(args);
    }
}

void log_error(const char *format, ...)
{
    va_list args, args2;

    va_start(args, format);
    va_copy(args2, args);
    logv(LOG_ERROR, format, args);

    // Display an OS messagebox.
    sys_popup("Egoboo: Fatal Error", "Egoboo has encountered a problem and is exiting.\n"
              "This is the error report: \n", format, args2 );

    va_end(args);
    va_end(args2);

    exit(EXIT_FAILURE);
}

vfs_FILE *log_get_file()
{
    return logFile;
}
