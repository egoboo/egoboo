#pragma once

#include <string>
#include <memory>
#include <vector>

namespace CommandLine {

using namespace std;

/// @brief An option.
struct Option {
public:
    /// An enumeration of types of options.
    enum class Type {
        /// An argument error.
        Error,
        /// A switch argument.
        Switch,
        /// A named value.
        NamedValue,
        /// An unnamed value.
        UnnamedValue,
    };
private:
    Type type;
protected:
    using String = std::string;
    /// @brief Construct this option.
    /// @param type the option type
    Option(Type type);
public:
    /// @brief Get the type of this option.
    /// @return the option
    Type getType() const;
};

/// @brief An unnamed value option of the form
/// @code
/// !`--` value
/// @endcoe
struct UnnamedValue : public Option {
private:
    string value;

public:
    /// Construct this unnamed value option.
    /// @param value the unnamed value option value
    UnnamedValue(const string& value);

    /// @brief Get the value of this named value argument.
    /// @return the named valuen argument value
    const String& getValue() const;
};

/// @brief A named value option of the form
/// @code
/// `--`name`=`value`
/// @endcode
struct NamedValue : Option {
private:
    string name;
    string value;
public:
    /// @brief Construct this named value argument.
    /// @param name the named value argument name
    /// @param value the named value argument value
    NamedValue(const string& name, const string& value);

    /// @brief Get the name of this named value argument.
    /// @return the named value argument name
    const string& getName() const;

    /// @brief Get the value of this named value argument.
    /// @return the named valuen argument value
    const string& getValue() const;

};

/// @brief A switch option of the form
/// @code
/// `--`name
/// @endcode
struct Switch : public Option {
public:
private:
    string name;
    bool on;
public:
    /// Construct this switch option.
    /// @param name the switch option name
    /// @param on is the switch on or off: @a true if the switch is on, @a false otherwise.
    Switch(const string& name, bool on);

    /// @brief Get the name of this switch option.
    /// @return the switch option name
    const string& getName() const;

    /// @brief Get if this switch is on or off.
    /// @return @a true if this switch is on, @a false otherwise
    bool isOn() const;
};

/// @brief An option error.
struct Error : public Option {
private:
    string message;
public:
    /// Construct this option error.
    /// @param message the error message
    Error(const string& message);

    /// @brief Get the message of this option error.
    /// @return the option error message
    const string& getMessage() const;
};

/// @brief Parse the command-line arguments.
/// @param argc the number of elements in the array pointed to by @a argv
/// @param argv a pointer to an array of @a argc pointers to C strings
/// @return the parsed command-line arguments
vector<shared_ptr<Option>> parse(int argc, char **argv);

} // namespace CommandLine
