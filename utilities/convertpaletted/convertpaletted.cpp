#include <SDL.h>
#include <SDL_image.h>
#include <dirent.h>
#include <iostream>
#include <sys/stat.h>
#include <cerrno>
#include <deque>

enum class PathStat {
    FILE,
    DIRECTORY,
    OTHER,
    FAILURE
};

void convert(const std::string &fileName);
void recurDir(const std::string &dirName, std::deque<std::string> &queue);
PathStat stat(const std::string &pathName);

int main(int argc, char **argv) {
    SDL_Init(0);
    IMG_Init(IMG_INIT_PNG);
    std::deque<std::string> queue;
    for (int i = 1; i < argc; i++) {
        queue.emplace_back(argv[i]);
    }
    while (!queue.empty()) {
        std::string path = queue[0];
        queue.pop_front();
        switch (stat(path)) {
            case PathStat::FILE:
                convert(path);
                break;
            case PathStat::DIRECTORY:
                recurDir(path, queue);
                break;
            case PathStat::FAILURE:
                break; // stat complains
            default:
                std::cerr << "'" << path << "' not a file or directory" << std::endl;
        }
    }
    IMG_Quit();
    SDL_Quit();
}

PathStat stat(const std::string &pathName) {
    struct stat out;
    int success = lstat(pathName.c_str(), &out);
    if (success == -1) {
        std::cerr << pathName << ": " << strerror(errno) << std::endl;
        return PathStat::FAILURE;
    }
    if (S_ISREG(out.st_mode)) return PathStat::FILE;
    if (S_ISDIR(out.st_mode)) return PathStat::DIRECTORY;
    return PathStat::OTHER;
}

void recurDir(const std::string &pathName, std::deque<std::string> &queue) {
    DIR *dir = opendir(pathName.c_str());
    if (!dir) {
        std::cerr << pathName << ": " << strerror(errno) << std::endl;
        return;
    }
    while (dirent *aFile = readdir(dir)) {
        if (aFile->d_name[0] == '.') continue;
        std::string path = pathName + "/" + aFile->d_name;
        queue.push_back(path);
        /*switch (stat(path)) {
            case PathStat::FILE:
                convert(path);
                break;
            case PathStat::DIRECTORY:
                recurDir(path);
                break;
        }*/
    }
    closedir(dir);
}

void convert(const std::string &fileName) {
    if (fileName.rfind(".bmp") == std::string::npos) return;
    std::string out = fileName;
    size_t extPos = out.rfind('.');
    if (extPos != std::string::npos) out = out.substr(0, extPos);
    out += ".png";
    SDL_Surface *surf = IMG_Load(fileName.c_str());
    if (!surf) {
        std::cerr << "Couldn't open " << fileName << ": " << IMG_GetError() << std::endl;
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
    if (IMG_SavePNG(other, out.c_str() == -1)
        std::cerr << "Couldn't save " << out << ":" << IMG_GetError() << std::endl;
    SDL_FreeSurface(other);
}