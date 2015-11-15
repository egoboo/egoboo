#pragma once

#include "egolib/Log/Target.hpp"
#include "egolib/Log/Level.hpp"

namespace Log {
	/**
	 * @brief
	 *  Manipulators influence the way characters, strings and entries are processed.
	 */
	namespace Manipulations {

		/**
		 * @internal
		 * @brief
		 *  The "end of line" manipulator type.
		 */
		struct EndOfLineManipulation {};

		/**
		 * @internal
		 * @brief The "end of entry" manipulator type.
		 */
		struct EndOfEntryManipulation {};

	}

	/**
	 * @brief
	 *  "end of entry" manipulation.
	 */
	extern const Manipulations::EndOfEntryManipulation EndOfEntry;
	/**
	 * @brief
	 *  "end of line" manipulation.
	 */
	extern const Manipulations::EndOfLineManipulation EndOfLine;

	struct Entry {
	private:
		Level _level;

		std::ostringstream _sink;

	public:
		Entry(Level level);

		Entry(Level level, const std::string& file, int line);

		Entry(Level level, const std::string& file, int line, const std::string& function);

		std::string getText() const;

		Level getLevel() const;

		std::ostringstream& getSink();

	};
}

template <typename T>
Log::Entry& operator<<(Log::Entry& entry, const T& value) {
	entry.getSink() << value;
	return entry;
}

template <>
Log::Entry& operator<< <Log::Manipulations::EndOfLineManipulation>(Log::Entry& entry, const Log::Manipulations::EndOfLineManipulation& value);

template <>
Log::Entry& operator<< <Log::Manipulations::EndOfEntryManipulation>(Log::Entry& entry, const Log::Manipulations::EndOfEntryManipulation& value);

Log::Target& operator<<(Log::Target& target, const Log::Entry& entry);