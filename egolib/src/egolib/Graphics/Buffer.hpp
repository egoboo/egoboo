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

/// @file egolib/Graphics/Buffer.hpp
/// @brief Abstract base class of all buffers.
/// @author Michael Heilmann

#include "egolib/platform.h"

namespace Ego {
    
/**
 * @brief
 *  The abstract base class of all buffers.
 * @author
 *  Michael Heilmann
 */
class Buffer : Id::NonCopyable {    
protected:
    /**
     * @brief
     *  Construct this buffer.
     */
    Buffer();

    /** 
     * @brief
     *  Destruct this buffer.
     */
    virtual ~Buffer();
    
}; // class Buffer
	
} // namespace Ego
