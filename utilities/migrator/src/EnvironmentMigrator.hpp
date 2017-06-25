#pragma once

#include "Tool.hpp"

namespace Editor { namespace Tools {
	
/// @brief Convert <c>wawalite.txt</c> files into <c>wawalite2.txt</c>.
class EnvironmentMigrator : public Tool
{
public:
    /// @brief Construct this tool.
    /// @param fileSystem a pointer to the files system
    EnvironmentMigrator(std::shared_ptr<FileSystem> fileSystem);
	
    /// @brief Destruct this tool.
    virtual ~EnvironmentMigrator();

    /** @copydoc Tool::run */
    void run(const std::vector<std::shared_ptr<CommandLine::Option>>& arguments) override;
	
    /** @copydoc Tool:getHelp */
    const std::string& getHelp() const override;

    void run(const std::string& pathname);
	
}; // class EnvironmentMigrator
	
class EnvironmentMigratorFactory : public ToolFactory
{
public:
    /** @copydoc Editor::ToolFactory::create */
    std::shared_ptr<Tool> create(std::shared_ptr<FileSystem> fileSystem) const override;

}; // class EnvironmentMigratorFactory

} } // namespace Editor::Tools
