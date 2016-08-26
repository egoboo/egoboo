#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace VFS {

/**
 * @brief
 *  Represents a pathname in the virtual file system.
 */
struct Pathname {

private:
    std::string _string;

public:

    /**
     * @brief
     *  Construct this pathname with a string.
     * @param string
     *  the pathname string
     */
    Pathname(const std::string& string);

    /**
     * @brief
     *  Construct this pathname with the values of another pathname.
     * @param other
     *  the other pathname
     */
    Pathname(const Pathname& other);

    /**
     * @brief
     *  Assign this pathname with the values of another pathname.
     * @param other
     *  the other pathname
     * @return
     *  this pathname
     */
    Pathname& operator=(const Pathname& other);

    /**
     * @brief
     *  Get if this pathname is equal to another pathname.
     * @param other
     *  the other pathname
     * @return
     *  @a true if this pathname is equal to the other pathname,
     *  @false otherwise
     */
    bool operator==(const Pathname& other) const;
    
    /**
     * @brief
     *  Get the pathname as a string.
     * @return
     *  the string
     */
    const std::string& toString() const;

    /**
     * @brief
     *  Get the extension of this pathname.
     * @return
     *  the extension of this pathname if the pathname has one,
     *  the empty string otherwise
     * @remark
     *  The extension of a pathname is the part of the pathname starting after the last period character in the pathname.
     *  However, if
     *  - no period is found in the pathname or
     *  - the period is the first character or the last character of the pathname,
     *  then the extension is empty.
     */
    std::string getExtension() const;

    /**
     * @brief
     *  Concatenate this pathname with another pathname.
     * @param other
     *  the other pathname
     */
    Pathname operator+(const Pathname& other) const;

};

} // namespace VFS
} // namespace Ego
