#include "convertpaletted.hpp"
#include <stdlib.h>
#include <SDL.h>

int SDL_main(int argc, char **argv) {
	try {
		ConvertPaletted::run(argc, argv);
	} catch (...) {
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}