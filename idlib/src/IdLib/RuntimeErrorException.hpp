
/// @file IdLib/AbstractSyntacticalError.hpp
/// @brief Generic runtime error exception.
/// @author Michael Heilmann

#pragma once

#if !defined(IDLIB_PRIVATE) || IDLIB_PRIVATE != 1
#error(do not include directly, include `IdLib/IdLib.hpp` instead)
#endif

#include "IdLib/Exception.hpp"

namespace Id {

using namespace std;

/// @brief A generic exception indicating a runtime error.
/// @detail Use this exception only if there is no exception type available which is more specific.
class RuntimeErrorException : public Exception
{
public:
    /// @brief Construct this runtime error exception.
    /// @param file the C++ source file (as obtained by the __FILE__ macro) associated with this exception
    /// @param line the line within the C++ source file (as obtained by the __LINE__ macro) associated with this exception
    /// @param message a message describing the error
    RuntimeErrorException(const char *file, int line, const string& message) :
        Exception(file, line), _message(message)
    {}

    /// @brief Construct this exception with the value of another exception.
    /// @param other the other exception
    RuntimeErrorException(const RuntimeErrorException& other) :
        Exception(other), _message(other._message)
    {}

    /// @brief Assign this exception the values of another exception.
    /// @param other the other exception
    /// @return this exception
    RuntimeErrorException& operator=(const RuntimeErrorException& other)
    {
        Exception::operator=(other);
        _message = other._message;
        return *this;
    }

public:
    /// @brief Get the message associated with this environment error.
    /// @return the message associated with this environment error
    const string& getMessage() const
    {
        return _message;
    }

    /// @brief Overloaded cast operator for casting into std::string.
    /// @return a human-readable textual description of the string.
    virtual operator ::std::string() const override
    {
        ostringstream buffer;
        buffer << "(raised in file " << getFile() << ", line " << getLine() << ")"
            << ":" << std::endl;
        buffer << _message;
        return buffer.str();
    }

private:
    /// @brief The exception message.
    string _message;
};

} // namespace Id
