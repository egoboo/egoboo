//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file  egolib/Script/TextFile.h
/// @brief textual input/output

#pragma once

#include "egolib/Script/Traits.hpp"

namespace Ego
{
namespace Script
{

using namespace std;

/**
 * @brief
 *  A text file.
 */
template <typename _Traits = Traits<char>>
struct TextFile
{

    /**
     * @brief
     *  The possible modes a text file can be opened with.
     */
    enum class Mode
    {
        /**
         * @brief
         *  The text file is opened for reading.
         */
        Read,
        /**
         * @brief
         *  The text file is opened for writing.
         */
        Write,
    };

    /**
     * @brief
     *  The traits used.
     */
    using Traits = _Traits;

private:

    /**
     * @brief
     *  The file name of the file.
     */
    string _fileName;

    /**
     * @brief
     *  The mode the file was opened in.
     */
    Mode _mode;

protected:

    /**
     * @brief
     *  Construct this text file with the given file name and mode.
     * @param fileName
     *  the file name
     * @param mode
     *  the mode
     */
    TextFile(const string& fileName, Mode mode) :
        _fileName(fileName), _mode(mode)
    {
    }

    /**
     * @brief
     *  Destruct this text file.
     */
    virtual ~TextFile()
    {
    }

public:

    /**
     * @brief
     *  Get the file name of this text file.
     * @return
     *  the file name of this text file
     */
    const string& getFileName() const
    {
        return _fileName;
    }

    /**
     * @brief
     *  The mode this text file was opened with.
     * @return
     *  the mode
     */
    Mode getMode() const
    {
        return _mode;
    }

};

} // namespace Script
} // namespace Ego
