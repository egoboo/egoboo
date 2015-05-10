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

#pragma once

#include "egolib/vfs.h"

class ImageLoader
{

private:

    using string = std::string;

    // Befriend with image manager.
    friend class ImageManager;

    // Befriend with std::default_delete.
    friend struct std::default_delete<ImageLoader>;

    /**
     * @brief
     *  A file extension of the file format supported by this image loader e.g. <tt>.bmp</tt>, <tt>.png</tt>, ...
     */
    string _extension;

protected:

    /**
     * @brief
     *  Construct this image loader.
     * @param extension
     *  a file extension of the file format supported by this image loader e.g. <tt>.bmp</tt>, <tt>.png</tt>, ...
     */
    ImageLoader(const string& extension);

    /**
     * @brief
     *  Destruct this image loader.
     */
    virtual ~ImageLoader();

public:

    /**
     * @brief
     *  Get a file extension of the file format supported by this image loader.
     * @return
     *  a file extension e.g. <tt>.bmp</tt>, <tt>.png</tt>, ...
     */
    const string getExtension() const;

    /**
     * @brief
     *  Load an image using this image loader.
     * @param file
     *  a file opened for reading
     * @return
     *  an SDL surface on success, @a nullptr on failure
     * @remark
     *  The file is not closed by this function.
     */
    virtual std::shared_ptr<SDL_Surface> load(vfs_FILE *file) const = 0;

};
