#pragma once

#include "Tool.hpp"

namespace Tools {

using namespace Standard;

/**
 * @brief Validate <c>enchant.txt</c> files.
 */
struct EnchantTxtValidator : public Editor::Tool {

public:
    /**
     * @brief Construct this tool.
     */
    EnchantTxtValidator();

    /**
     * @brief Destruct this tool.
     */
    virtual ~EnchantTxtValidator();

    /** @copydoc Tool::run */
    void run(const Vector<SharedPtr<CommandLine::Option>>& arguments) override;

    /** @copydoc Tool:getHelp */
    const String& getHelp() const override;

private:
    void validate(const String& pathname);

}; // struct DataTxtValidator

struct EnchantTxtValidatorFactory : Editor::ToolFactory {
    Editor::Tool *create() noexcept override {
        try {
            return new EnchantTxtValidator();
        } catch (...) {
            return nullptr;
        }
    }
}; // struct DataTxtValidatorFactory


} // namespace Tools
