#pragma once

#include "egolib/Log/_Include.hpp"

namespace Ego {
namespace Script {

/// @brief A log entry of a compiler.
struct CLogEntry : Log::Entry
{
private:
    id::location m_location;

public:
    /// @brief Construct this compiler log entry.
    /// @param level the log level
    /// @param fileName the C/C++ file name
    /// @param lineNumber the C/C++ line number
    /// @param functionName the C/C++ function name
    /// @param location the EgoScript location
    CLogEntry(Log::Level level, const std::string& fileName, int lineNumber,
              const std::string& functionName, const id::location& location);

    /// @brief Get the EgoScript location.
    /// @return the EgoScript location
    const id::location& getLocation() const;

    friend Log::Target& operator<<(Log::Target& target, const CLogEntry& entry);
};

Log::Target& operator<<(Log::Target& target, const CLogEntry& entry);

} // namespace Script
} // namespace Ego