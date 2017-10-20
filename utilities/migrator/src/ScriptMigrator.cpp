#include "ScriptMigrator.hpp"

#include "Filters.hpp"
#include "FileSystem.hpp"

#include "ScriptMigrator/parser.hpp"

namespace Editor { namespace Tools {

using namespace Standard;
using namespace CommandLine;

ScriptMigrator::ScriptMigrator(std::shared_ptr<FileSystem> fileSystem)
    : Tool("ScriptMigrator", fileSystem)
{}

ScriptMigrator::~ScriptMigrator()
{}

void ScriptMigrator::run(const std::vector<std::shared_ptr<Option>>& arguments)
{
    if (arguments.size() < 1)
    {
        StringBuffer sb;
        sb << "wrong number of arguments" << EndOfLine;
        throw RuntimeError(sb.str());
    }

    std::deque<std::string> queue;
    RegexFilter filter("^(?:.*" REGEX_DIRSEP ")?script\\.txt");
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
                run(path);
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

const std::string& ScriptMigrator::getHelp() const
{
    static const std::string help = "usage: ego-tools --tool=ScriptMigrator <directories>\n";
    return help;
}

void ScriptMigrator::run(const std::string& pathname)
{
    std::cout << "processing " << pathname << std::endl;
    id::file_system::mapped_file_descriptor source_fd;
    source_fd.open_read(pathname, id::file_system::create_mode::open_existing);
    if (!source_fd.is_open())
    {
        return;
    }
    std::vector<char> buffer;
    buffer.reserve(source_fd.size());
    std::copy(source_fd.data(), source_fd.data() + source_fd.size(), std::back_inserter(buffer));
    scanner s(id::c::location(pathname, 1), buffer.data(), buffer.data() + buffer.size(), buffer.data());
    parser p(&s);
    p.run();
    id::file_system::mapped_file_descriptor target_fd;
    target_fd.open_write(pathname + std::string("2"), id::file_system::create_mode::create_not_existing, buffer.size());
    if (!target_fd.is_open())
    {
        return;
    }
    memcpy(target_fd.data(), buffer.data(), buffer.size());
}

std::shared_ptr<Tool> ScriptMigratorFactory::create(std::shared_ptr<FileSystem> fileSystem) const
{
    return std::make_shared<ScriptMigrator>(fileSystem);
}

} } // namespace Editor::Tools
