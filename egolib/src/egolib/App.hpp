#pragma once

#include "egolib/platform.h"

namespace Ego {
// for sharing code between Cartman and Game.
struct App {
	static void initialize(const std::string& name, const std::string& version);
	static void uninitialize();
};
}
