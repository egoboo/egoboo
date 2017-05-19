#include "EnchantTxtValidator.hpp"

#include "Filters.hpp"
#include "FileSystem.hpp"

namespace Editor {
namespace Tools {

using namespace Standard;
using namespace CommandLine;

EnchantTxtValidator::EnchantTxtValidator(std::shared_ptr<FileSystem> fileSystem)
    : Tool("EnchantTxtValidator", fileSystem)
{}

EnchantTxtValidator::~EnchantTxtValidator()
{}

void EnchantTxtValidator::run(const std::vector<std::shared_ptr<Option>>& arguments)
{
    if (arguments.size() < 1)
    {
        StringBuffer sb;
        sb << "wrong number of arguments" << EndOfLine;
        throw RuntimeError(sb.str());
    }

    std::deque<std::string> queue;
    RegexFilter filter("^(?:.*" REGEX_DIRSEP ")?enchant\\.txt");
    /// @todo Do *not* assume the path is relative. Ensure that it is absolute by a system function.
    for (const auto& argument : arguments)
    {
        if (argument->getType() != Option::Type::UnnamedValue)
        {
            StringBuffer sb;
            sb << "unrecognized argument" << EndOfLine;
            throw RuntimeError(sb.str());
        }
        auto pathnameArgument = std::static_pointer_cast<UnnamedValue>(argument);
        queue.emplace_back(getFileSystem()->sanitize(pathnameArgument->getValue()));
    }
    while (!queue.empty())
    {
        std::string path = queue[0];
        queue.pop_front();
        switch (getFileSystem()->stat(path))
        {
            case FileSystem::PathStat::File:
                if (filter(path))
                {
                    validate(path);
                }
                break;
            case FileSystem::PathStat::Directory:
                getFileSystem()->recurDir(path, queue);
                break;
            case FileSystem::PathStat::Failure:
                break; // stat complains
            default:
            {
                StringBuffer sb;
                sb << "skipping '" << path << "' - not a file or directory" << EndOfLine;
                std::cerr << sb.str();
            }
        }
    }
}

const std::string& EnchantTxtValidator::getHelp() const
{
    static const std::string help = "usage: ego-tools --tool=EnchantTxtValidator <directories>\n";
    return help;
}

void EnchantTxtValidator::validate(const std::string& pathname)
{}

std::shared_ptr<Tool> EnchantTxtValidatorFactory::create(std::shared_ptr<FileSystem> fileSystem) const
{
    return std::make_shared<EnchantTxtValidator>(fileSystem);
}

} // namespace Tools
} // namespace Editor
