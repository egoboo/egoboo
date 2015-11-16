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

/// @file egolib/Log/Entry.cpp
/// @brief A log entry

#include "egolib/Log/Entry.hpp"

namespace Log {
const Manipulations::EndOfEntryManipulation EndOfEntry;
const Manipulations::EndOfLineManipulation EndOfLine;

Entry::Entry(Level level)
	: _level(level), _sink() {
}

Entry::Entry(Level level, const std::string& file, int line)
	: _level(level), _sink() {
	_sink << file << ":" << line << ": ";
}

Entry::Entry::Entry(Level level, const std::string& file, int line, const std::string& function)
	: _level(level), _sink() {
	_sink << file << ":" << line << ":" << function << ": ";
}

std::string Entry::getText() const {
	return _sink.str();
}

Level Entry::getLevel() const {
	return _level;
}

std::ostringstream& Entry::getSink() {
	return _sink;
}

} // namespace Log

template <>
Log::Entry& operator<< <Log::Manipulations::EndOfLineManipulation>(Log::Entry& entry, const Log::Manipulations::EndOfLineManipulation& value) {
	entry.getSink() << std::endl;
	return entry;
}

template <>
Log::Entry& operator<< <Log::Manipulations::EndOfEntryManipulation>(Log::Entry& entry, const Log::Manipulations::EndOfEntryManipulation& value) {
	entry.getSink() << std::endl;
	return entry;
}

Log::Target& operator<<(Log::Target& target, const Log::Entry& entry) {
	target.log(entry.getLevel(), "%s", entry.getText().c_str());
	return target;
}
