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

/// @file  egolib/Log/Target.cpp
/// @brief Log target

#include "egolib/Log/Target.hpp"

#include "egolib/egoboo_setup.h"

namespace Log {

Target::Target(Level level) : _level(level) {}

Target::~Target() {}

Level Target::getLevel() const {
    return _level;
}

void Target::logv(Level level, const char *format, va_list args) {
    if (getLevel() >= level) {
        writev(level, format, args);
    }
}

void Target::log(Level level, const char *format, ...) {
    va_list args;
    va_start(args, format);
    writev(level, format, args);
    va_end(args);
}

} // namespace Log
