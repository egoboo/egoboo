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

/// @file  egolib/Log/DefaultTarget.hpp
/// @brief Log default target

#pragma once

#include "egolib/Log/Target.hpp"

namespace Log {

class DefaultTarget : public Target {
public:
	DefaultTarget(const std::string& filename, Level level = Level::Warning);
	virtual ~DefaultTarget();
	void writev(Level level, const char *format, va_list args) override;

private:
	char* _applicationPath;
	/**
	* @brief
	*  The log file.
	*/
	std::unique_ptr<std::ofstream> _file;
};

} // namespace Log
