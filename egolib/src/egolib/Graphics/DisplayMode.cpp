#include "egolib/Graphics/DisplayMode.hpp"

namespace Ego {

DisplayMode::DisplayMode() {
	/* Intentionally empty. */	
}

DisplayMode::~DisplayMode() {
	/* Intentionally empty. */
	
}

bool DisplayMode::operator==(const DisplayMode& other) const {
	return compare(other);
}

bool DisplayMode::operator!=(const DisplayMode& other) const {
	return !compare(other);
}

} // namespace Ego
