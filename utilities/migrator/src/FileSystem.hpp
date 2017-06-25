#pragma once

#include "idlib/idlib.hpp"

namespace Editor {

/// @brief The implementation of a file system.
class FileSystem
{
public:
    /// @brief Enumeration of objects a pathname might refer to.
    enum class PathStat
    {
        File,
        Directory,
        Other,
        Failure
    };

    /// @brief Get the type of file system object a pathname refers to.
    /// @param pathname the pathname
    /// @return the type of file system object the pathname refers to
    PathStat stat(const std::string &pathName);

    /// @brief Get the directory separator.
    /// @return the directory separator
    std::string getDirectorySeparator();

    /// @brief Get the current working directory of this process.
    /// @return the current working directory of this process
    /// @throw std::runtime_error the current working directory can not be obtained
    /// @throw std::bad_alloc an out of memory situation occurred
    std::string getWorkingDirectory();

    std::string sanitize(const std::string& pathName);

    void recurDir(const std::string &pathName, std::deque<std::string> &queue);
}; // class FileSystem

} // namespace Editor
