#include "egolib/Graphics/DisplayMode.hpp"

namespace Ego {

DisplayMode::DisplayMode()
{}

DisplayMode::~DisplayMode()
{}

bool DisplayMode::operator==(const DisplayMode& other) const
{
    return compare(other);
}

bool DisplayMode::operator!=(const DisplayMode& other) const
{
    return !compare(other);
}

} // namespace Ego

Log::Entry& operator<<(Log::Entry& logEntry, const Ego::DisplayMode& displayMode)
{
    logEntry << displayMode.getHorizontalResolution() << " pixels x "
             << displayMode.getVerticalResolution() << " pixels x "
             << displayMode.getRefreshRate() << " Hz";
    return logEntry;
}
