#pragma once

#include "Tool.hpp"

namespace Editor {
namespace Tools {

/// @brief Validate <c>data.txt</c> files.
class DataMigrator : public Tool
{
public:
    /// @brief Construct this tool.
    /// @param fileSystem a pointer to the files system
    DataMigrator(std::shared_ptr<FileSystem> fileSystem);

    /// @brief Destruct this tool.
    virtual ~DataMigrator();

    /** @copydoc Tool::run */
    void run(const std::vector<std::shared_ptr<CommandLine::Option>>& arguments) override;

    /** @copydoc Tool:getHelp */
    const std::string& getHelp() const override;

private:
    void run(const std::string& pathname);

}; // class DataMigrator

class DataMigratorFactory : public ToolFactory
{
public:
    /** @copydoc Editor::ToolFactory::create */
    std::shared_ptr<Tool> create(std::shared_ptr<FileSystem> fileSystem) const override;

}; // class DataMigratorFactory

} // namespace Tools
} // namespace Editor
