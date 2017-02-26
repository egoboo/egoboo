#include "egolib/Script/CLogEntry.hpp"

namespace Ego {
namespace Script {

CLogEntry::CLogEntry(Log::Level level, const std::string& fileName, int lineNumber,
                     const std::string& functionName, const id::location& location)
    : Entry(level, fileName, lineNumber, functionName), m_location(location)
{}

const id::location& CLogEntry::getLocation() const
{
    return m_location;
}

Log::Target& operator<<(Log::Target& target, const CLogEntry& entry)
{
    std::ostringstream temporary;
    if (entry.hasAttribute("C/C++ file name"))
    {
        temporary << entry.getAttribute("C/C++ file name") << ":";
    }
    if (entry.hasAttribute("C/C++ line number"))
    {
        temporary << entry.getAttribute("C/C++ line number") << ":";
    }
    if (entry.hasAttribute("C/C++ function namer"))
    {
        temporary << entry.getAttribute("C/C++ function name") << ":";
    }
    temporary << entry.getLocation().file_name() << ":"
              << entry.getLocation().line_number() << ":";
    target.log(entry.getLevel(), "%s%s", temporary.str().c_str(), entry.getText().c_str());
    return target;
}

} // namespace Script
} // namespace Ego