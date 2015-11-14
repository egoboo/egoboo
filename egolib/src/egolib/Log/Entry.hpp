#pragma once

#include "egolib/log.h"

namespace Log {
	/** @brief Manipulators influence the way characters, strings and entries are processed. */
	namespace Manipulations {
		/** @brief The "end of line" manipulator type. */
		struct EndOfLineManipulation {};
		/** @internal @brief The "end of entry" manipulator type */
		struct EndOfEntryManipulation {};
	}
	/** @brief "end of entry" manipulation. */
	Manipulations::EndOfEntryManipulation EndOfEntry;
	/** @brief "end of line" manipulation. */
	Manipulations::EndOfLineManipulation EndOfLine;
	struct Entry {
	private:
		Level _level;
		std::ostringstream _sink;
	public:
		Entry(Level level)
			: _level(level), _sink() {
		}
		Entry(Level level, const std::string& file, int line)
			: _level(level), _sink() {
			_sink << file << ":" << line << ": ";
		}
		Entry(Level level, const std::string& file, int line, const std::string& function)
			: _level(level), _sink() {
			_sink << file << ":" << line << ":" << function << ": ";
		}
		std::string getText() const {
			return _sink.str();
		}
		Level getLevel() const {
			return _level;
		}
		std::ostringstream& getSink() {
			return _sink;
		}
	};
}

template <typename T>
Log::Entry& operator<<(Log::Entry& entry, const T& value) {
	entry.getSink() << value;
	return entry;
}

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
