#include "EnchantTxtValidator.hpp"

#include "Filters.hpp"

namespace Tools {

#if 0
template <class CharT, class Traits>
std::basic_ostream<CharT, Traits>& EndOfLine(std::basic_ostream<CharT, Traits>& os) {
    os << std::endl;
    return os;
}
//template<class CharT, class Traits> using EndOfLine(std::basic_ostream<CharT, Traits>&) = std::endl<CharT, Traits>(std::basic_ostream<CharT, Traits>&);
using RuntimeError = std::runtime_error;
using StringBuffer = std::ostringstream;
template <typename T> using Deque = std::deque<T>;
#endif
using namespace Standard;
using namespace CommandLine;

EnchantTxtValidator::EnchantTxtValidator()
    : Editor::Tool("EnchantTxtValidator") {}

EnchantTxtValidator::~EnchantTxtValidator() {}

void EnchantTxtValidator::run(const Vector<SharedPtr<Option>>& arguments) {
    if (arguments.size() < 1) {
        StringBuffer sb;
        sb << "wrong number of arguments" << EndOfLine;
        throw RuntimeError(sb.str());
    }

    Deque<String> queue;
    RegexFilter filter("^(?:.*" REGEX_DIRSEP ")?enchant\\.txt");
    /// @todo Do *not* assume the path is relative. Ensure that it is absolute by a system function.
    for (const auto& argument : arguments) {
        if (argument->getType() != Option::Type::UnnamedValue) {
            StringBuffer sb;
            sb << "unrecognized argument" << EndOfLine;
            throw RuntimeError(sb.str());
        }
        auto& pathnameArgument = static_pointer_cast<UnnamedValue>(argument);
        queue.emplace_back(FileSystem::sanitize(pathnameArgument->getValue()));
    }
    while (!queue.empty()) {
        String path = queue[0];
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

const String& EnchantTxtValidator::getHelp() const {
    static const String help = "usage: ego-tools --tool=EnchantTxtValidator <directories>\n";
    return help;
}

void EnchantTxtValidator::validate(const String& pathname) {}

} // namespace Tools
