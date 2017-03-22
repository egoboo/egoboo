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
    std::basic_ostream<CharT, Traits>& EndOfLine(std::basic_ostream<CharT, Traits>& os) {
        os << std::endl;
        return os;
    }
    using RuntimeError = std::runtime_error;
    using StringBuffer = std::ostringstream;
}

struct FileSystem {
    /**
     * @brief Enumeration of objects a pathname might refer to.
     */
    enum class PathStat {
        File,
        Directory,
        Other,
        Failure
    };

    /**
     * @brief Get the type of file system object a pathname refers to.
     * @param pathname the pathname
     * @return the type of file system object the pathname refers to
     */
    static PathStat stat(const std::string &pathName);

    static std::string getDirectorySeparator();

    /**
     * @brief
     *  Get the current working directory of this process.
     * @return
     *  the current working directory of this process
     * @throw std::runtime_error
     *  if the current working directory can not be obtained
     * @throw std::bad_alloc
     *  if an out of memory situation occurred
     */
    static std::string getWorkingDirectory();

    static std::string sanitize(const std::string& pathName);

    static void recurDir(const std::string &pathName, std::deque<std::string> &queue);
};

namespace Editor {
using namespace Standard;
/**
 * @brief An abstract tool class . Derive your own tools from this abstract class by overriding
 * Tool::getHelp() and Tool::run(const std::vector<std::shared_ptr<CommandLine::Option>>&).
 */
class Tool {
private:
	/**
	 * @brief The name of this tool.
	 */
	std::string name;

protected:
	/** 
	 * @brief Construct this tool.
	 * @param name the name of this tool
	 */
	Tool(const std::string& name);

public:
	/** 
	 * @brief Destruct this tool.
	 */
	virtual ~Tool();

	/**
	 * @brief Get the name of this tool.
	 * @return the name of this tool
	 */
	const std::string& getName() const;

    /**
     * @brief Run this tool with the specified arguments.
     * @param argument the arguments
     */
    virtual void run(const std::vector<std::shared_ptr<CommandLine::Option>>& arguments) = 0;

    /**
     * @brief Get the help text for this tool.
     * @return the help text for this tool
     */
    virtual const std::string& getHelp() const = 0;

}; // class Tool

/// @brief A factory for a tool.
class ToolFactory {
public:
    /// @brief Create a tool.
    /// @return a pointer to the tool on success, a null pointer on failure
    virtual Tool *create() noexcept = 0;
}; // class ToolFactory
	
} // namespace Editor
