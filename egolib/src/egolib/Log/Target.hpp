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

/// @file  egolib/Log/Target.hpp
/// @brief Log target

#pragma once

#include "egolib/Log/Level.hpp"

namespace Log {

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
    virtual void writev(Level level, const char *format, va_list args) = 0;
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
    virtual void logv(Level level, const char *format, va_list args);
    /**
     * @brief
     *  Write a log message on the specified log level.
     * @param level
     *  the log level
     * @param format, ...
     *  printf-style format string and variadic argument list
     */
    virtual void log(Level level, const char *format, ...) GCC_PRINTF_FUNC(3);
};

} // namespace Log
