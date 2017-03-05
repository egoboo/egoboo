#include "ConvertPaletted.hpp"

#include "Filters.hpp"

#include <SDL.h>
#include <SDL_image.h>


namespace Tools {

using namespace Standard;
using namespace CommandLine;

ConvertPaletted::ConvertPaletted()
    : Editor::Tool("ConvertPaletted") {}

ConvertPaletted::~ConvertPaletted() {}

void ConvertPaletted::run(const std::vector<std::shared_ptr<Option>>& arguments) {
	if (arguments.size() == 1) {
		StringBuffer sb;
	    sb << "wrong number of arguments" << EndOfLine;
		throw RuntimeError(sb.str());
	}

	SDL_Init(0);
	IMG_Init(IMG_INIT_PNG);
	std::deque<std::string> queue;
	RegexFilter filter("^(?:.*" REGEX_DIRSEP ")?(?:tris|tile)[0-9]+\\.bmp$");
	/// @todo Do *not* assume the path is relative. Ensure that it is absolute by a system function.
	for (const auto& argument : arguments) {
        if (argument->getType() != Option::Type::UnnamedValue) {
            StringBuffer sb;
            sb << "unrecognized argument" << EndOfLine;
            throw RuntimeError(sb.str());
        }
        auto pathnameArgument = static_pointer_cast<UnnamedValue>(argument);
		queue.emplace_back(FileSystem::sanitize(pathnameArgument->getValue()));
	}
	while (!queue.empty()) {
		std::string path = queue[0];
		queue.pop_front();
		switch (FileSystem::stat(path)) {
		case FileSystem::PathStat::File:
			if (filter(path)) {
				convert(path);
			}
			break;
		case FileSystem::PathStat::Directory:
			FileSystem::recurDir(path, queue);
			break;
        case FileSystem::PathStat::Failure:
			break; // stat complains
		default:
			{
				StringBuffer sb;
				sb << "skipping '" << path << "' - not a file or directory" << EndOfLine;
				cerr << sb.str();
			}
		}
	}
	IMG_Quit();
	SDL_Quit();
}

/// Perform a dry-run, just print the files which would have been converted.
static bool g_dryrun = false;

void ConvertPaletted::convert(const std::string& fileName) {
    if (fileName.rfind(".bmp") == std::string::npos) return;
    std::string out = fileName;
    size_t extPos = out.rfind('.');
    if (extPos != std::string::npos) out = out.substr(0, extPos);
    out += ".png";
    if (!g_dryrun) {
        SDL_Surface *surf = IMG_Load(fileName.c_str());
        if (!surf) {
            std::cerr << std::string("Couldn't open ") << fileName << ": " << IMG_GetError() << std::endl;
            return;
        }
        uint8_t r, g, b, a;
        if (surf->format->palette) {
            SDL_GetRGBA(0, surf->format, &r, &g, &b, &a);
            std::cout << fileName << ": converting [" << uint16_t(r) << ", " << uint16_t(g) << ", " <<
                uint16_t(b) << ", " << uint16_t(a) << "] to alpha" << std::endl;
            SDL_SetColorKey(surf, SDL_TRUE, 0);
        } else {
            std::cout << fileName << ": no palette" << std::endl;
        }
        SDL_Surface *other = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA8888, 0);
        SDL_FreeSurface(surf);
        if (!other) {
            std::cerr << "Couldn't convert " << fileName << ": " << SDL_GetError() << std::endl;
            return;
        }
        uint32_t *pixels = (uint32_t *)other->pixels;
        for (int i = 0; i < other->w * other->h; i++, pixels++) {
            SDL_GetRGBA(*pixels, other->format, &r, &g, &b, &a);
            float aFloat = a / 255;
            r *= aFloat; g *= aFloat; b *= aFloat;
            *pixels = SDL_MapRGBA(other->format, r, g, b, a);
        }
        if (IMG_SavePNG(other, out.c_str()) == -1)
            std::cerr << "Couldn't save " << out << ":" << IMG_GetError() << std::endl;
        SDL_FreeSurface(other);
    } else {
        std::cout << "converting " << fileName << " to " << out << std::endl;
    }
}

const std::string& ConvertPaletted::getHelp() const {
    static const std::string help = "usage: ego-tools --tool=ConvertPaletted <directories>\n";
    return help;
}

} // namespace Tools
