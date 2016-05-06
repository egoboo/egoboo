#pragma once

namespace Ego {
// for sharing code between Cartman and Game.
struct App {
	static void initialize();
	static void uninitialize();
};
}