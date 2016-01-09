#include "DataTxtValidator.hpp"

#include "Filters.hpp"

namespace Tools {

using namespace Standard;
using namespace CommandLine;

DataTxtValidator::DataTxtValidator()
    : Editor::Tool("DataTxtValidator") {}

DataTxtValidator::~DataTxtValidator() {}

void DataTxtValidator::run(const Vector<SharedPtr<Option>>& arguments) {
    if (arguments.size() < 1) {
        StringBuffer sb;
        sb << "wrong number of arguments" << EndOfLine;
        throw RuntimeError(sb.str());
    }

    Deque<String> queue;
    RegexFilter filter("^(?:.*" REGEX_DIRSEP ")?data\\.txt");
    /// @todo Do *not* assume the path is relative. Ensure that it is absolute by a system function.
    for (const auto& argument : arguments) {
        if (argument->getType() != Option::Type::UnnamedValue) {
            StringBuffer sb;
            sb << "unrecognized argument" << EndOfLine;
            throw runtime_error(sb.str());
        }
        auto pathnameArgument = static_pointer_cast<UnnamedValue>(argument);
        queue.emplace_back(FileSystem::sanitize(pathnameArgument->getValue()));
    }
    while (!queue.empty()) {
        string path = queue[0];
        queue.pop_front();
        switch (FileSystem::stat(path)) {
            case FileSystem::PathStat::File:
                if (filter(path)) {
                    validate(path);
                }
                break;
            case FileSystem::PathStat::Directory:
                FileSystem::recurDir(path, queue);
                break;
            case FileSystem::PathStat::Failure:
                break; // stat complains
            default:
            {
                StringBuffer sb;
                sb << "skipping '" << path << "' - not a file or directory" << EndOfLine;
                cerr << sb.str();
            }
        }
    }
}

const String& DataTxtValidator::getHelp() const {
    static const String help = "usage: ego-tools --tool=DataTxtValidator <directories>\n";
    return help;
}

void DataTxtValidator::validate(const String& pathname) {
}

} // namespace Tools
