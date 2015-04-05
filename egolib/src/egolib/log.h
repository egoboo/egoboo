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

/// @file egolib/log.h
/// @details Basic logging functionality.
#pragma once

#include "egolib/typedef.h"
#include "egolib/vfs.h"

enum LogLevel : uint8_t
{
    LOG_NONE,       ///< No log level, always printed directly to output
    LOG_ERROR,      ///< Fatal unrecoverable error (also terminates program)
    LOG_WARNING,    ///< Warning, something went wrong but nothing critical
    LOG_INFO,       ///< Information logging, default level
    LOG_DEBUG       ///< Verbose debug logging, useful for developers and debugging
};

//--------------------------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

void log_initialize(const char *logname, LogLevel logLevel);
void log_uninitialize();

vfs_FILE *log_get_file();

/**
 * @brief
 *  Write a log message on the specified log level.
 * @param level
 *   the log level
 * @param format, args
 *  printf-style format string and variadic argument list
 */
void logv(LogLevel level, const char *format, va_list args);

/**
 * @brief
 *  Write a log message on the specified log level.
 * @param level
 *   the log level
 * @param format, ...
 *  printf-style format string and variadic argument list
 */
void log(LogLevel level, const char *format, ...) GCC_PRINTF_FUNC(2);

/**
 * @brief
 *  Write a log message on log level "none".
 * @param format, ...
 *  printf-style format string and variadic argument list
 */
void log_message(const char *format, ...) GCC_PRINTF_FUNC(1);

/**
 * @brief
 *  Write a log message on log level "none".
 * @param format, ...
 *  printf-style format string and variadic argument list
 */
void log_debug(const char *format, ...) GCC_PRINTF_FUNC(1);

/**
 * @brief
 *  Write a log message on log level "info".
 * @param format, ...
 *  printf-style format string and variadic argument list
 */
void log_info(const char *format, ...) GCC_PRINTF_FUNC(1);

/**
 * @brief
 *  Write a log message on log level "warning".
 * @param format, ...
 *  printf-style format string and variadic argument list
 */
void log_warning(const char *format, ...) GCC_PRINTF_FUNC(1);

/**
 * @brief
 *  Write a log message on log level "warning".
 * @param format, ...
 *  printf-style format string and variadic argument list
 * @todo
 *  "logging" should have no side effects except of (eventually)
 *  writing the log entry. However, log_error in fact terminates the
 *  program.
 */
void log_error(const char *format, ...) GCC_PRINTF_FUNC(1);
