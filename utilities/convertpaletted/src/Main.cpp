#include "convertpaletted.hpp"
#include <stdlib.h>
#include <SDL.h>

#include "CommandLine.hpp"

#include "ConvertPaletted.hpp"
#include "DataTxtValidator.hpp"
#include "EnchantTxtValidator.hpp"

int SDL_main(int argc, char **argv) {
	try {
        using namespace std;
        using namespace CommandLine;

        // (1) Register the known tools.
        unordered_map<string, shared_ptr<Editor::ToolFactory>> factories;
        factories.emplace("DataTxtValidator", make_shared<Tools::DataTxtValidatorFactory>());
        factories.emplace("ConvertPaletted", make_shared<Tools::ConvertPalettedFactory>());
        factories.emplace("EnchantTxtValidator", make_shared <Tools::EnchantTxtValidatorFactory>());

        // (2) Parse the argument list.
        auto args = CommandLine::parse(argc, argv);

        // (3) Find the option which determines the tool to execute and remove that option.
        auto predicate = 
            [](const shared_ptr<Option>& x) {
                if (x->getType() == Option::Type::NamedValue) {
                    auto namedValue = static_pointer_cast<NamedValue>(x);
                    return "tool" == namedValue->getName();
                } else {
                    return false;
                }};
        auto it = find_if(args.cbegin(), args.cend(), predicate);
        if (it == args.cend()) {
            cout << "no tool selected" << endl;
            return EXIT_FAILURE;
        }
        auto option = *it;
        args.erase(it);
        // (4) Multiple tools may not be supplied.
        it = find_if(args.cbegin(), args.cend(), predicate);
        if (it != args.cend()) {
            cout << "multiple tools specified" << endl;
            return EXIT_FAILURE;
        }
        // (5) Extract the tool name.
        auto name = static_pointer_cast<NamedValue>(option)->getValue();
        auto factory = factories.find(name);
        if (factory == factories.cend()) {
            cout << "no tool of name `" << name << "` found" << std::endl;
            return EXIT_FAILURE;
        }

        // (6) Create the tool.
        auto tool = factory->second->create();
        if (!tool) {
            return EXIT_FAILURE;
        }
        // (7) Execute it.
        try {
            tool->run(args);
        } catch (...) {
            delete tool;
            rethrow_exception(current_exception());
        }
        delete tool;
	} catch (...) {
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
