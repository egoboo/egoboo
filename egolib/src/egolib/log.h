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

namespace Log {
	/**
	 * @brief
	 *  An enumeration of log levels.
	 */
	enum class Level : uint8_t {
		Message,    ///< No log level, always printed directly to output.
		Error,      ///< Fatal unrecoverable error (also terminates program).
		Warning,    ///< Warning, something went wrong but nothing critical.
		Info,       ///< Information logging, default level.
		Debug,      ///< Verbose debug logging, useful for developers and debugging.
	};

	/**
	 * @brief
	 *  A log target.
	 *  Targets support printf-style and stream-style log messages.
	 */
	struct Target {
	private:
		/**
		 * @brief
		 *  The log level.
		 */
		Level _level;
	protected:
		virtual void write(Level level, const char *format, va_list args) = 0;
	public:
		/**
		 * @brief
		 *  Construct this log target.
		 * @param level
		 *  the log level. Default is Level::Warning.
		 */
		Target(Level level = Level::Warning);
		/**
		 * @brief
		 *  Destruct this log target.
		 */
		virtual ~Target();
		/**
		 * @brief
		 *  Get the log level.
		 * @return
		 *  the log level
		 */
		Level getLevel() const;
		/**
		 * @brief
		 *  Write a log message on the specified log level.
		 * @param level
		 *  the log level
		 * @param format, args
		 *  printf-style format string and variadic argument list
		 */
		virtual void log(Level level, const char *format, va_list args);
		/**
		 * @brief
		 *  Write a log message on the specified log level.
		 * @param level
		 *  the log level
		 * @param format, ...
		 *  printf-style format string and variadic argument list
		 */
		virtual void log(Level level, const char *format, ...) GCC_PRINTF_FUNC(2);

		/** @{ */

		/**
		 * @brief
		 *  Write a log message on log level "message".
		 * @param format, args
		 *  printf-style format string and variadic argument list
		 */
		void message(const char *format, va_list args);

		/**
		 * @brief
		 *  Write a log message on log level "message".
		 * @param format, ...
		 *  printf-style format string and variadic argument list
		 */
		void message(const char *format, ...) GCC_PRINTF_FUNC(1);

		/** @} */

		/** @{ */

		/**
		 * @brief
		 *  Write a log message on log level "debug".
		 * @param format, args
		 *  printf-style format string and variadic argument list
		 */
		void debug(const char *format, va_list args);

		/**
		 * @brief
		 *  Write a log message on log level "debug".
		 * @param format, ...
		 *  printf-style format string and variadic argument list
		 */
		void debug(const char *format, ...) GCC_PRINTF_FUNC(1);

		/** @} */


		/** @{ */

		/**
		 * @brief
		 *  Write a log message on log level "info".
		 * @param format, args
		 *  printf-style format string and variadic argument list
		 */
		void info(const char *format, va_list args);

		/**
		 * @brief
		 *  Write a log message on log level "info".
		 * @param format, ...
		 *  printf-style format string and variadic argument list
		 */
		void info(const char *format, ...) GCC_PRINTF_FUNC(1);

		/** @} */

		/** @{ */

		/**
		 * @brief
		 *  Write a log message on log level "warning".
		 * @param format, args
		 *  printf-style format string and variadic argument list
		 */
		void warn(const char *format, va_list args);

		/**
		 * @brief
		 *  Write a log message on log level "warning".
		 * @param format, ....
		 *  printf-style format string and variadic argument list
		 */
		void warn(const char *format, ...) GCC_PRINTF_FUNC(1);

		/** @} */

		/** @{ */

		/**
		 * @brief
		 *  Write a log message on log level "error".
		 * @param format, args
		 *  printf-style format string and variadic argument list
		 */
		void error(const char *format, va_list args);

		/**
		 * @brief
		 *  Write a log message on log level "error".
		 * @param format, ...
		 *  printf-style format string and variadic argument list
		 */
		void error(const char *format, ...) GCC_PRINTF_FUNC(1);

		/** @} */

	};

	/**
	 * @brief
	 *  Initialize the logging system.
	 * @param filename
	 *  the file name
	 * @param level
	 *  the level
	 * @return
	 *  Returns if the logging system is already initialized.
	 * @throw std::runtime_error
	 *  if initialization fails
	 */
	void initialize(const std::string& filename, Log::Level level);
	/**
	 * @brief
	 *  Uninitialize the logging system
	 * @remark
	 *  Returns if the logging system is not initialized.
	 */
	void uninitialize();

	/**
	 * @brief
	 *  Get the default target.
	 * @return
	 *  the default target
	 * @throw std::logic_error
	 *  if the logging system is not initialized
	 */
	Target& get();

	/**
	 * @brief
	 *  Write a log message on log level "message".
	 * @param format, ...
	 *  printf-style format string and variadic argument list
	 */
	void message(const char *format, ...) GCC_PRINTF_FUNC(1);

	/**
	 * @brief
	 *  Write a log message on log level "debug".
	 * @param format, ...
	 *  printf-style format string and variadic argument list
	 */
	void debug(const char *format, ...) GCC_PRINTF_FUNC(1);

	/**
	 * @brief
	 *  Write a log message on log level "info".
	 * @param format, ...
	 *  printf-style format string and variadic argument list
	 */
	void info(const char *format, ...) GCC_PRINTF_FUNC(1);

	/**
	 * @brief
	 *  Write a log message on log level "warning".
	 * @param format, ...
	 *  printf-style format string and variadic argument list
	 */
	void warning(const char *format, ...) GCC_PRINTF_FUNC(1);

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
	void error(const char *format, ...) GCC_PRINTF_FUNC(1);
}

/**
* @brief
*  Write a log message on the specified log level.
* @param level
*   the log level
* @param format, args
*  printf-style format string and variadic argument list
*/
void log(Log::Level level, const char *format, va_list args);

/**
* @brief
*  Write a log message on the specified log level.
* @param level
*   the log level
* @param format, ...
*  printf-style format string and variadic argument list
*/
void log(Log::Level level, const char *format, ...) GCC_PRINTF_FUNC(2);