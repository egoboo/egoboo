#include "CommandLine.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace CommandLine {

using namespace std;

Option::Option(Type type) : type(type) {}

Option::Type Option::getType() const {
    return type;
}

UnnamedValue::UnnamedValue(const string& value)
    : Option(Type::UnnamedValue), value(value) {}

const string& UnnamedValue::getValue() const {
    return value;
}

NamedValue::NamedValue(const string& name, const string& value)
    : Option(Type::NamedValue), name(name), value(value) {}

const string& NamedValue::getName() const {
    return name;
}

const string& NamedValue::getValue() const {
    return value;
}

Switch::Switch(const String& name, bool on)
    : Option(Type::Switch), name(name), on(on) {}

const string& Switch::getName() const {
    return name;
}

bool Switch::isOn() const {
    return on;
}

Error::Error(const string& message)
    : Option(Type::Error), message(message) {}

const string& Error::getMessage() const {
    return message;
}

struct Parser {
    typedef function<bool(const string::const_iterator&, const string::const_iterator&, const string::const_iterator&)> Predicate;
    /// @brief Get if the current character fulfills the specified property.
    /// @return @a true if the current input character fulfills the specified property,
    ///         @a false otherwise
    static bool test(string::const_iterator& at, const string::const_iterator& begin,
              const string::const_iterator& end, Predicate predicate) {
        return predicate(at, begin, end);
    }
    /// @brief Get if the current character fulfills the specified property.
    /// @return @a true if the current input character fulfills the specified property,
    ///         @a false otherwise
    static bool test(string::const_iterator& at, const string::const_iterator& begin,
              const string::const_iterator& end, char chr) {
        return test(at, begin, end, [chr](const string::const_iterator& at, const string::const_iterator& begin, const string::const_iterator& end) {
            if (at == end) {
                return false;
            }
            if (*at != chr) {
                return false;
            }
            return true;
        });
    }
    static bool expect(string::const_iterator& at, const string::const_iterator& begin,
                       const string::const_iterator& end, char chr) {
        if (test(at, begin, end, chr)) {
            at++;
            return true;
        }
        return false;
    }
    static shared_ptr<Option> parse(const string& argument) {
        using namespace std;
        const auto begin = argument.cbegin(),
            end = argument.cend();
        auto p = begin;
        if (!Parser::test(p, begin, end, '-')) {
            auto q = p;
            while (q != end) {
                q++;
            }
            return make_shared<UnnamedValue>(string(p, q));
        }
        // `--`
        if (!Parser::expect(p, begin, end, '-')) {
            return make_shared<Error>("expected '`-`'");
        }
        if (!Parser::expect(p, begin, end, '-')) {
            return make_shared<Error>("expected '`-`'");
        }
        // name
        auto q = p;
        while (q != end && *q != '=') {
            q++;
        }
        if (q == end || q == p) {
            return make_shared<Error>("expected 'name'");
        }
        const string name = string(p, q);
        // `=`?
        if (!Parser::test(q, begin, end, '=')) {
            return make_shared<Switch>(name, true);
        }
        q++;
        // <value>
        p = q;
        if (p == end) {
            return make_shared<Error>("expected 'value'");
        }
        q = p;
        while (q != end) {
            q++;
        }
        if (q == p) {
            return make_shared<Error>("empty value");
        }
        const string value = string(p, q);
        return make_shared<NamedValue>(name, value);
    }
};

using namespace std;

vector<shared_ptr<Option>> parse(int argc, char **argv) {
    vector<shared_ptr<Option>> result;
    for (int i = 1; i < argc; ++i) {
        result.push_back(Parser::parse(argv[i]));
    }
    return result;
}

} // namespace CommandLine
