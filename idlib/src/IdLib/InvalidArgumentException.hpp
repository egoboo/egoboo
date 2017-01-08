
/// @file IdLib/InvalidArgumentException.hpp
/// @brief Invalid argument exception
/// @author Michael Heilmann

#pragma once

#if !defined(IDLIB_PRIVATE) || IDLIB_PRIVATE != 1
#error(do not include directly, include `IdLib/IdLib.hpp` instead)
#endif

#include "IdLib/RuntimeErrorException.hpp"

namespace Id {

using namespace std;

/// @brief An exception indicating an invalid argument.
/// @detail Use this exception only if there is no exception type available which is more specific.
class InvalidArgumentException : public RuntimeErrorException
{

public:

    /// @brief Construct this invalid argument exception.
    /// @param file the C++ source file (as obtained by the __FILE__ macro) associated with this exception
    /// @param line the line within the C++ source file (as obtained by the __LINE__ macro) associated with this exception
    /// @param message a message describing the error
    InvalidArgumentException(const char *file, int line, const string& message) :
        RuntimeErrorException(file, line, message)
    {}

    /// @brief Construct this exception with the value of another exception.
    /// @param other the other exception
    InvalidArgumentException(const InvalidArgumentException& other) :
        RuntimeErrorException(other)
    {}

    /// @brief Assign this exception the values of another exception.
    /// @param other the other exception
    /// @return this exception
    InvalidArgumentException& operator=(const InvalidArgumentException& other)
    {
        RuntimeErrorException::operator=(other);
        return *this;
    }

};

} // namespace Id
