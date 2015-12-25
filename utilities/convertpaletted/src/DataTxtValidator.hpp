#pragma once

#include "Tool.hpp"

namespace Tools {
/**
 * @brief Validate <c>data.txt</c> files.
 */
struct DataTxtValidator : public Editor::Tool {
public:
    /**
     * @brief Construct this tool.
     */
    DataTxtValidator();

    /**
     * @brief Destruct this tool.
     */
    virtual ~DataTxtValidator();

    /**
     * @brief Run this tool with the specified arguments.
     * @param argument the arguments
     */
    void run(const std::vector<std::shared_ptr<CommandLine::Option>>& arguments) override;

private:
    void validate(const std::string& pathname);

}; // struct DataTxtValidator

struct DataTxtValidatorFactory : Editor::ToolFactory {
    Editor::Tool *create() noexcept override {
        try {
            return new DataTxtValidator();
        } catch (...) {
            return nullptr;
        }
    }
}; // struct DataTxtValidatorFactory


} // namespace Tools
