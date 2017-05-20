#include <stdlib.h>
#include <SDL.h>

#include "CommandLine.hpp"
#include "FileSystem.hpp"

#include "ConvertPaletted.hpp"
#include "DataMigrator.hpp"
#include "EnchantMigrator.hpp"
#include "EnvironmentMigrator.hpp"
#include "ScriptMigrator.hpp"

int SDL_main(int argc, char **argv) {
	try {
        using namespace std;
        using namespace CommandLine;

        // (1) Register the known tools.
        unordered_map<string, shared_ptr<Editor::ToolFactory>> factories;
        factories.emplace("ConvertPaletted", make_shared<Editor::Tools::ConvertPalettedFactory>());
        factories.emplace("DataMigrator", make_shared<Editor::Tools::DataMigratorFactory>());
        factories.emplace("EnchantMigrator", make_shared<Editor::Tools::EnchantMigratorFactory>());
        factories.emplace("EnvironmentMigrator", make_shared<Editor::Tools::EnvironmentMigratorFactory>());
        factories.emplace("ScriptMigrator", make_shared<Editor::Tools::ScriptMigratorFactory>());

        // (2) Parse the argument list.
        auto args = CommandLine::parse(argc, argv);

        // (3) Find the option which determines the tool to execute and remove that option.
        auto predicate = 
            [](const std::shared_ptr<Option>& x) {
                if (x->getType() == Option::Type::NamedValue) {
                    auto namedValue = std::static_pointer_cast<NamedValue>(x);
                    return "tool" == namedValue->getName();
                } else {
                    return false;
                }};
        auto it = std::find_if(args.cbegin(), args.cend(), predicate);
        if (it == args.cend()) {
            cout << "no tool selected" << endl;
            return EXIT_FAILURE;
        }
        auto option = *it;
        args.erase(it);
        // (4) Multiple tools may not be supplied.
        it = std::find_if(args.cbegin(), args.cend(), predicate);
        if (it != args.cend()) {
            cout << "multiple tools specified" << endl;
            return EXIT_FAILURE;
        }
        // (5) Extract the tool name.
        auto name = std::static_pointer_cast<NamedValue>(option)->getValue();
        auto factory = factories.find(name);
        if (factory == factories.cend()) {
            cout << "no tool of name `" << name << "` found" << std::endl;
            return EXIT_FAILURE;
        }

        // (6) Create the file system.
        auto fileSystem = std::make_shared<Editor::FileSystem>();

        // (7) Create the tool.
        auto tool = factory->second->create(fileSystem);

        // (8) Execute it.
        try {
            tool->run(args);
        } catch (...) {
            fileSystem = nullptr;
            tool = nullptr;
            rethrow_exception(current_exception());
        }
        fileSystem = nullptr;
        tool = nullptr;
	} catch (...) {
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
