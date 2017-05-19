#pragma once

#include "egolib/egolib.h"
#include "CommandLine.hpp"

#if defined(_WIN32)
#include <Windows.h>
#define REGEX_DIRSEP "[/\\\\]"
#else
#include <dirent.h>
#include <sys/stat.h>
#define REGEX_DIRSEP "/"
#endif

namespace Standard {
template <class CharT, class Traits>
std::basic_ostream<CharT, Traits>& EndOfLine(std::basic_ostream<CharT, Traits>& os)
{
    os << std::endl;
    return os;
}
using RuntimeError = std::runtime_error;
using StringBuffer = std::ostringstream;
}

namespace Editor {

// Forward declaration.
class FileSystem;

using namespace Standard;
/// @brief An abstract tool class . Derive your own tools from this abstract class
/// by overriding
/// Tool::getHelp()
/// and
/// Tool::run(const std::vector<std::shared_ptr<CommandLine::Option>>&).
class Tool
{
private:
    /// @brief The name of this tool.
    std::string name;

    /// @brief A pointer to the file system of this tool.
    std::shared_ptr<FileSystem> fileSystem;

protected:
    /// @brief Construct this tool.
    /// @param name the name of this tool
    /// @param fileSystem a pointer to the files system
    Tool(const std::string& name, std::shared_ptr<FileSystem> fileSystem);

public:
    /// @brief Destruct this tool.
    virtual ~Tool();

    /// @brief Get the name of this tool.
    /// @return the name of this tool
    const std::string& getName() const;

    /// @brief Get the file system of this tool.
    /// @return a pointer to the file system of this tool
    const std::shared_ptr<FileSystem>& getFileSystem() const;

    /// @brief Run this tool with the specified arguments.
    /// @param argument the arguments
    virtual void run(const std::vector<std::shared_ptr<CommandLine::Option>>& arguments) = 0;

    /// @brief Get the help text for this tool.
    /// @return the help text for this tool
    virtual const std::string& getHelp() const = 0;

}; // class Tool

/// @brief A factory for a tool.
class ToolFactory
{
public:
    /// @brief Create a tool.
    /// @param fileSystem a pointer to the file system
    /// @return a pointer to the tool on success, a null pointer on failure
    virtual std::shared_ptr<Tool> create(std::shared_ptr<FileSystem> fileSystem) const = 0;
}; // class ToolFactory

} // namespace Editor
