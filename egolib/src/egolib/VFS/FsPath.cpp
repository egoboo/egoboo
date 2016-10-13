#include "egolib/VFS/FsPath.hpp"

namespace Ego {

#if defined(ID_WINDOWS)

#endif

FsPath::FsPath() : m_string() {}

FsPath::FsPath(const std::string& string) : m_string(string) {}

FsPath::FsPath(const FsPath& other) : m_string(other.m_string) {}

FsPath::FsPath(FsPath&& other) noexcept : m_string(std::move(other.m_string)) {}

FsPath& FsPath::operator=(const FsPath& other) {
    m_string = other.m_string;
    return *this;
}

FsPath& FsPath::operator=(FsPath&& other) {
    m_string = std::move(other.m_string);
    return *this;
}

const std::string& FsPath::string() const {
    return m_string;
}

bool FsPath::operator==(const FsPath& other) const noexcept {
    return m_string == other.m_string;
}

bool FsPath::operator!=(const FsPath& other) const noexcept {
    return m_string != other.m_string;
}

bool FsPath::empty() const noexcept {
    return m_string.empty();
}

} // namespace Ego
