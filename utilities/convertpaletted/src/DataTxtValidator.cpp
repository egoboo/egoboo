#include "DataTxtValidator.hpp"

#include "Filters.hpp"

namespace Tools {

using namespace std;
using namespace CommandLine;

DataTxtValidator::DataTxtValidator()
    : Editor::Tool("DataTxtValidator") {}

DataTxtValidator::~DataTxtValidator() {}

void DataTxtValidator::run(const vector<shared_ptr<Option>>& arguments) {
    if (arguments.size() < 1) {
        ostringstream os;
        os << "wrong number of arguments" << endl;
        throw runtime_error(os.str());
    }

    deque<std::string> queue;
    RegexFilter filter("^(?:.*" REGEX_DIRSEP ")?data\\.txt");
    /// @todo Do *not* assume the path is relative. Ensure that it is absolute by a system function.
    for (const auto& argument : arguments) {
        if (argument->getType() != Option::Type::UnnamedValue) {
            ostringstream os;
            os << "unrecognized argument" << endl;
            throw runtime_error(os.str());
        }
        auto& pathnameArgument = static_pointer_cast<UnnamedValue>(argument);
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
                ostringstream os;
                os << "skipping '" << path << "' - not a file or directory" << endl;
                cerr << os.str();
            }
        }
    }
}

void DataTxtValidator::validate(const string& pathname) {
}

} // namespace Tools
