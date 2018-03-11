#pragma once

#include "idlib/platform.hpp"
#include <string>

namespace Ego {

/// @brief Represents a path in the file system.
struct FsPath {
private:
    std::string m_string;

public:
    /// @brief Construct this path.
    /// @post This path is empty.
    FsPath();

    /// @brief Construct this path with a string.
    /// @param string the path string
    /// @post This path is assigned the specified string.
    explicit FsPath(const std::string& string);

    /// @brief Construct this path with the values of another pathname.
    /// @param other the other path
    FsPath(const FsPath& other);

    /// @brief Move-construct this pathname with the values of another pathname.
    /// @param other the other path
    FsPath(FsPath&& other) noexcept;

    /// @brief Assign this path with the values of another path.
    /// @param other the other path
    /// @return this path
    FsPath& operator=(const FsPath& other);

    /// @brief Move-assign this path the values of another path.
    /// @param other the other path
    /// @return this path
    FsPath& operator=(FsPath&& other);

    /// @brief Get this path as a string.
    /// @return the string
    const std::string& string() const;

    /// @brief Get if this path is lexicographically equal to another path.
    /// @param other the other path
    /// @return @a true if this path is lexicographically equal to the other path, @false otherwise
    bool operator==(const FsPath& other) const noexcept;

    /// @brief Get if this path is not lexicographically equal to another path.
    /// @param other the other path
    /// @return @a true if this path is not lexicographically equal to the other path, @a false otherwise
    bool operator!=(const FsPath& other) const noexcept;

    /// @brief Get if this path is empty.
    /// @return @a true if this path is empty, @a false otherwise
    bool empty() const noexcept;
};


} // namespace Ego
