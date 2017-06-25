#pragma once

#include "Tool.hpp"

namespace Editor { namespace Tools {
	
/// @brief Convert <c>script.txt</c> in <c>script2.txt</c> files.
class ScriptMigrator : public Tool
{
public:
    /// @brief Construct this tool.
    /// @param fileSystem a pointer to the files system
    ScriptMigrator(std::shared_ptr<FileSystem> fileSystem);

    /// @brief Destruct this tool.
    virtual ~ScriptMigrator();

    /** @copydoc Tool::run */
    void run(const std::vector<std::shared_ptr<CommandLine::Option>>& arguments) override;
	
    /** @copydoc Tool:getHelp */
    const std::string& getHelp() const override;

    void run(const std::string& pathname);

}; // class ScriptMigrator

class ScriptMigratorFactory : public ToolFactory
{
public:
    /** @copydoc Editor::ToolFactory::create */
    std::shared_ptr<Tool> create(std::shared_ptr<FileSystem> fileSystem) const override;
}; // class ScriptMigratorFactory
	
} } // namespace Editor::Tools
