#pragma once

#include "Tool.hpp"

namespace Editor {
namespace Tools {

/// @brief Convert <c>enchant.txt</c> to <c>enchant2.txt</c> files.
class EnchantMigrator : public Tool
{
public:
    /// @brief Construct this tool.
    /// @param fileSystem a pointer to the files system
    EnchantMigrator(std::shared_ptr<FileSystem> fileSystem);

    /// @brief Destruct this tool.
    virtual ~EnchantMigrator();

    /** @copydoc Tool::run */
    void run(const std::vector<std::shared_ptr<CommandLine::Option>>& arguments) override;

    /** @copydoc Tool:getHelp */
    const std::string& getHelp() const override;

private:
    void run(const std::string& pathname);

}; // class EnchantMigrator

class EnchantMigratorFactory : public ToolFactory
{
public:
    /** @copydoc Editor::ToolFactory::create */
    std::shared_ptr<Tool> create(std::shared_ptr<FileSystem> fileSystem) const override;

}; // class EnchantMigratorFactory

} } // namespace Tools::Editor
