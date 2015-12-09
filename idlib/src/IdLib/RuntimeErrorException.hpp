
/// @file   IdLib/AbstractSyntacticalError.hpp
/// @brief  Generic runtime error exception
/// @author Michael Heilmann

#pragma once

#if !defined(IDLIB_PRIVATE) || IDLIB_PRIVATE != 1
#error(do not include directly, include `IdLib/IdLib.hpp` instead)
#endif

#include "IdLib/Exception.hpp"

namespace Id {

using namespace std;

/**
 * @brief
 *  A generic exception for runtime error. Use this exception only if there is
 *  no exception type available which is more specific
 * @author
 *  Michael Heilmann
 */
class RuntimeErrorException : public Exception {

private:

    /**
     * @brief
     *  The messsage associated with this environment error.
     */
    string _message;

public:

    /**
     * @brief
     *  Construct this runtime  error.
     * @param file
     *  the C++ source file (as obtained by the __FILE__ macro) associated with this exception
     * @param line
     *  the line within the C++ source file (as obtained by the __LINE__ macro) associated with this exception
     * @param message
     *  a message describing the error
     * @remark
     *  Intentionally protected.
     */
    RuntimeErrorException(const char *file, int line, const string& message) :
        Exception(file, line), _message(message) {}
    RuntimeErrorException(const RuntimeErrorException& other) :
        Exception(other), _message(other._message) {}
    RuntimeErrorException& operator=(const RuntimeErrorException& other) {
        Exception::operator=(other);
        _message = other._message;
        return *this;
    }

public:

    /**
     * @brief
     *  Get the message associated with this environment error.
     * @return
     *  the message associated with this environment error
     */
    const string& getMessage() const {
        return _message;
    }

    /**
     * @brief
     *  Overloaded cast operator for casting into std::string.
     * @return
     *  a human-readable textual description of the string.
     */
    virtual operator ::std::string() const {
        ostringstream buffer;
        buffer << "(raised in file " << getFile() << ", line " << getLine() << ")"
            << ":" << std::endl;
        buffer << _message;
        return buffer.str();
    }
};

} // namespace Id
