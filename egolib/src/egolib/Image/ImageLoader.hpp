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

namespace Ego {

class ImageLoader {
protected:
    using String = std::string;
    template <typename T>
    using Set = std::unordered_set<T>;

private:
    friend struct std::default_delete<ImageLoader>;
    /**
     * @brief
     *  A set of file extensions supported by this image loader e.g. <tt>.bmp</tt>, <tt>.png</tt>, ...
     */
    Set<String> extensions;

protected:

    /**
     * @brief
     *  Construct this image loader.
     * @param extensions
     *  the set of file extensions supported by this image loader e.g. <tt>.bmp</tt>, <tt>.png</tt>, ...
     */
    ImageLoader(const Set<String>& extensions);

    /**
     * @brief
     *  Destruct this image loader.
     */
    virtual ~ImageLoader();

public:

    /**
     * @brief
     *  Get the set of file extensions supported by this image loader.
     * @return
     *  a file extension e.g. <tt>.bmp</tt>, <tt>.png</tt>, ...
     */
    Set<String> getExtensions() const;

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

} // namespace Ego
