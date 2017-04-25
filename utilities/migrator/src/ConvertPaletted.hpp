#pragma once

#include "Tool.hpp"

namespace Editor {
namespace Tools {

/// @brief Convert paletted images to non-paletted images.
class ConvertPaletted : public Tool
{
public:
    /// @brief Construct this tool.
    /// @param fileSystem a pointer to the files system
    ConvertPaletted(std::shared_ptr<FileSystem> fileSystem);

    /// @brief Destruct this tool.
    virtual ~ConvertPaletted();

    /** @copydoc Tool::run */
    void run(const std::vector<std::shared_ptr<CommandLine::Option>>& arguments) override;

    /** @copydoc Tool:getHelp */
    const std::string& getHelp() const override;

private:
    void convert(const std::string& fileName);

}; // class ConvertPaletted

class ConvertPalettedFactory : public ToolFactory
{
public:
    /** @copydoc Editor::ToolFactory::create */
    std::shared_ptr<Tool> create(std::shared_ptr<FileSystem> fileSystem) const override;

}; // class ConvertPalettedFactory

} // namespace Tools
} // namespace Editor
