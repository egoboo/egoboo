#include "egolib/Graphics/Display.hpp"

namespace Ego {

Display::Display()
{}

Display::~Display()
{}

bool Display::operator==(const Display& other) const
{
	return compare(other);
}

bool Display::operator!=(const Display& other) const
{
	return !compare(other);
}

const std::vector<std::shared_ptr<DisplayMode>>& Display::getDisplayModes() const
{
    return displayModes;
}

} // namespace Ego
