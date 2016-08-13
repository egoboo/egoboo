#include "egolib/VFS/Pathname.hpp"

#include <regex>

namespace Ego {
namespace VFS {

Pathname::Pathname(const std::string& string)
    : _string(string) {
    if (string.empty()) {
        throw std::invalid_argument("invalid pathname string");
    }
    // pathnames may not contain `..` or `:`.
    if (_string.find("..") != std::string::npos && _string.find(":") != std::string::npos) {
        throw std::invalid_argument("invalid pathname string");
    }
    // pathnames may not contain `\`.
    if (_string.find("\\") != std::string::npos) {
        throw std::invalid_argument("invalid pathname string");
    }
    std::regex e("/+");
    // Double slashes are eliminated.
    _string = std::regex_replace(_string, e, "/");
}

Pathname::Pathname(const Pathname& other)
    : _string(other._string) {
}

Pathname& Pathname::operator=(const Pathname& other) {
    _string = other._string;
    return *this;
}

bool Pathname::operator==(const Pathname& other) const {
    return _string == other._string;
}

const std::string& Pathname::toString() const {
    return _string;
}

std::string Pathname::getExtension() const {
    auto pos = _string.rfind('.');
    // No period found/period at the beginning (end) of the pathname string  ...
    if (pos == std::string::npos || pos == 0 || pos == _string.length()) {
        // ... no extension.
        return std::string();
    }
    // Case exhaustion: the extension is non-empty.
    return _string.substr(pos+1, _string.length() - pos - 1);
}

Pathname Pathname::operator+(const Pathname& other) const {
    // Double-slashes are eliminated in the constructor.
    return Pathname(_string + "/" + other._string);
}

} // namespace VFS
} // namespace Ego
