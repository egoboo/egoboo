#pragma once

#include "Tool.hpp"

namespace Tools {

using namespace Standard;

/**
 * @brief Convert paletted images to non-paletted images.
 */
struct ConvertPaletted : public Editor::Tool {

public:
    /**
     * @brief Construct this tool.
     */
    ConvertPaletted();

    /**
     * @brief Destruct this tool.
     */
    virtual ~ConvertPaletted();

    /** @copydoc Tool::run */
    void run(const Vector<SharedPtr<CommandLine::Option>>& arguments) override;

    /** @copydoc Tool:getHelp */
    const String& getHelp() const override;

private:
    void convert(const String& fileName);

}; // struct ConvertPaletted

struct ConvertPalettedFactory : Editor::ToolFactory {
    Editor::Tool *create() noexcept override {
        try {
            return new ConvertPaletted();
        } catch (...) {
            return nullptr;
        }
    }
}; // struct ConvertPalettedFactory

} // namespace Tools
