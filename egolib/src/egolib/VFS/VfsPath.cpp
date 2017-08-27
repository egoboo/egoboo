#include "egolib/VFS/VfsPath.hpp"

#include <regex>

namespace Ego {

const std::string& VfsPath::getDefaultPathSeparator() {
    static const std::string separator(NETWORK_SLASH_STR);
    return separator;
}

const std::string& VfsPath::getNetworkPathSeparator() {
    static const std::string separator(NETWORK_SLASH_STR);
    return separator;
}

const std::string& VfsPath::getSystemPathSeparator() {
#if defined(ID_WINDOWS)
    static const std::string separator("\\");
#else
    static const std::string separator("/");
#endif
    return separator;
}

// Match sequences of slashes and backslashes.
static const std::regex normalize_regexp("(/|\\\\)+");

VfsPath::VfsPath() : m_string() {}

VfsPath::VfsPath(const std::string& string)
    : m_string(string) {
    // Backslash to Slash. Slash Slash to Slash.
    m_string = std::regex_replace(m_string, normalize_regexp, "/");
}

VfsPath::VfsPath(VfsPath&& other) noexcept : m_string(std::move(other.m_string)) {}

VfsPath::VfsPath(const VfsPath& other) : m_string(other.m_string) {}

VfsPath& VfsPath::operator=(const VfsPath& other) {
    m_string = other.m_string;
    return *this;
}

VfsPath& VfsPath::operator=(VfsPath&& other) {
    m_string = std::move(other.m_string);
    return *this;
}

const std::string& VfsPath::string() const {
    return m_string;
}

bool VfsPath::operator==(const VfsPath& other) const noexcept {
    return m_string == other.m_string;
}

bool VfsPath::operator!=(const VfsPath& other) const noexcept {
    return m_string != other.m_string;
}

bool VfsPath::empty() const noexcept {
    return m_string.empty();
}

std::string VfsPath::string(Kind kind) const {
    switch (kind) {
        case Kind::Standard: {
                return m_string;
            } break;
        case Kind::Network: {
                static const auto transcode = [](char source) {
                    if (source == C_SLASH_CHR || source == C_BACKSLASH_CHR) return NETWORK_SLASH_CHR;
                    else return source;
                };
                auto temporary = m_string;
                std::transform(temporary.begin(), temporary.end(), temporary.begin(), transcode);
                return temporary;
            } break;
        case Kind::System: {
            static const auto transcode = [](char source) {
                if (source == C_SLASH_CHR || source == C_BACKSLASH_CHR) return SYSTEM_SLASH_CHR;
                else return source;
            };
            auto temporary = m_string;
            std::transform(temporary.begin(), temporary.end(), temporary.begin(), transcode);
            return temporary;
        } break;
    }

    return m_string;
}

std::string VfsPath::getExtension() const {
    auto periodPos = m_string.rfind('.');
    // No period found/period at the beginning (end) of the path string  ...
    if (periodPos == std::string::npos || periodPos == 0 || periodPos == m_string.length()) {
        // ... no extension.
        return std::string();
    }
    // Period is located before a slash ...
    auto slashPos = m_string.rfind('/');
    if (slashPos != std::string::npos && slashPos > periodPos) {
        // ... no extension.
        return std::string();
    }
    // Case exhaustion: the extension is non-empty.
    return m_string.substr(periodPos + 1, m_string.length() - periodPos - 1);
}

#if defined(_MSC_VER)
void VfsPath::swap(VfsPath& path) noexcept {
    m_string.swap(path.m_string);
}
#else
void VfsPath::swap(VfsPath& path) {
    m_string.swap(path.m_string);
}
#endif

VfsPath VfsPath::operator+(const VfsPath& other) const {
    VfsPath temporary(*this);
    temporary += other;
    return temporary;
}

VfsPath& VfsPath::operator+=(const VfsPath& other) {
    if (m_string.empty() || other.m_string.empty()) {
        // If this path is empty or the other path is empty, then concatenate both paths
        // (a decent STL implementation should deal with the concatenation two empty strings efficiently).
        m_string += other.m_string;
        return *this;
    }
    // At this point: Both paths are non-empty.
    if (m_string[m_string.length() - 1] != '/' && other.m_string[0] != '/') {
        // If this path does not end with a separator and the other path does not start with a separator,
        // then append a separator.
        m_string += '/';
    }
    m_string += other.m_string;
    return *this;
}

bool VfsPath::operator<(const VfsPath& other) const noexcept {
    return m_string < other.m_string;
}

bool VfsPath::operator<=(const VfsPath& other) const noexcept {
    return m_string <= other.m_string;
}

bool VfsPath::operator>(const VfsPath& other) const noexcept {
    return m_string > other.m_string;
}

bool VfsPath::operator>=(const VfsPath& other) const noexcept {
    return m_string >= other.m_string;
}

} // namespace Ego
