#pragma once

#include "egolib/platform.h"

namespace Ego {

/// @brief Represents a path in the virtual file system.
struct VfsPath {

private:
    std::string m_string;

public:
    /// @brief Get the default path separator.
    /// @return the default path separator.
    static const std::string& getDefaultPathSeparator();

    /// @brief Get the network path separator.
    /// @return the network path separator
    static const std::string& getNetworkPathSeparator();

    /// @brief Get the system path separator.
    /// @return the system path separator
    static const std::string& getSystemPathSeparator();

public:
    /// @brief Construct this path.
    /// @post This path is empty.
    VfsPath();

    /// @brief Construct this path with a string.
    /// @param string the path string
    /// @post This path is assigned the specified string which is transformed by applying one of the following rules until no more rule is applicable:
    /// - any network path separator or system path separator is replaced by a default path separator
    /// - two consecutive default path separators are replaced by a single default path separator
    explicit VfsPath(const std::string& string);

    /// @brief Construct this path with the values of another pathname.
    /// @param other the other path
    VfsPath(const VfsPath& other);

    /// @brief Move-construct this path with the values of another pathname.
    /// @param other the other path
    VfsPath(VfsPath&& other) noexcept;

    /// @brief Assign this path with the values of another path.
    /// @param other the other path
    /// @return this path
    VfsPath& operator=(const VfsPath& other);

    /// @brief Move-assign this path the values of another path.
    /// @param other the other path
    /// @return this path
    VfsPath& operator=(VfsPath&& other);

    /// @brief Get this path as a string.
    /// @return the string
    const std::string& string() const;

    /// @brief Get if this path is lexicographically equal to another path.
    /// @param other the other path
    /// @return @a true if this path is lexicographically equal to the other path, @false otherwise
    bool operator==(const VfsPath& other) const noexcept;

    /// @brief Get if this path is not lexicographically equal to another path.
    /// @param other the other path
    /// @return @a true if this path is not lexicographically equal to the other path, @a false otherwise
    bool operator!=(const VfsPath& other) const noexcept;

    /// @brief Get if this path is empty.
    /// @return @a true if this path is empty, @a false otherwise
    bool empty() const noexcept;

public:
    enum class Kind {
        System,
        Network,
        Standard,
    };
    std::string string(Kind kind) const;



    /// @brief Get the extension of this path.
    /// @return the extension of this path if this path has one, the empty string otherwise
    /// @remark 
    /// The extension of a path is the part of the path string starting after the last period character in the path.
    /// However, if
    /// - no period is found in the path,
    /// - the period is the first character or the last character of the path, or
    /// - is succeeded by a path separator
    /// then the extension is empty.
    std::string getExtension() const;

public:
#if defined(_MSC_VER)
    void swap(VfsPath& other) noexcept;
#else
    void swap(VfsPath& other);
#endif

public:
    /// @brief Concatenate this path with another path.
    /// @param other the other path
    /// @return the concatenation of this path with the other path
    VfsPath operator+(const VfsPath& other) const;

    /// @brief Concatenate this path with another path and assign the result to this path.
    /// @param other the other path
    /// @return this path
    VfsPath& operator+=(const VfsPath& other);

public:
    /// @brief Lexicographically compare this string with another string.
    /// @param other the other string
    /// @return @a true if this path is lexicographically smaller than the other string, @a false otherwise
    bool operator<(const VfsPath& other) const noexcept;
    
    /// @brief Lexicographically compare this string with another string.
    /// @param other the other string
    /// @return @a true if this path is lexicographically smaller than or equal to the other string, @a false otherwise
    bool operator<=(const VfsPath& other) const noexcept;

    /// @brief Lexicographically compare this string with another string.
    /// @param other the other string
    /// @return @a true if this path is lexicographically greater than the other string, @a false otherwise
    bool operator>(const VfsPath& other) const noexcept;

    /// @brief Lexicographically compare this string with another string.
    /// @param other the other string
    /// @return @a true if this path is lexicographically greater than or equal to the other string, @a false otherwise
    bool operator>=(const VfsPath& other) const noexcept;
};

/// @brief The types Ego::VFS::Path and Ego::VFS::Extension aid polymorphism.
/// Without them, it would be impossible to distinguish between the polymorphic methods
/// <c>(path, extension)</c>, <c>(extension, path)</c>, <c>(path)</c>, and <c>(extension)</c>.
struct Extension {
    std::string m_string;
    explicit Extension(const std::string& string) : m_string(string) {}
    bool empty() const { return m_string.empty(); }
    bool operator==(const Extension& other) const { return m_string == other.m_string; }
    bool operator!=(const Extension& other) const { return m_string != other.m_string; }
    const std::string& toString() const { return m_string; }
};

} // namespace Ego
