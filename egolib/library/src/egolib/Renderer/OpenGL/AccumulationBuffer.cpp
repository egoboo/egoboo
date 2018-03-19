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

/// @file egolib/Renderer/OpenGL/AccumulationBuffer.cpp
/// @brief Implementation of an Accumulation buffer facade for OpenGL 2.1.
/// @author Michael Heilmann

#include "egolib/Renderer/OpenGL/AccumulationBuffer.hpp"
#include "egolib/Renderer/OpenGL/Utilities.hpp"

namespace Ego::OpenGL {
struct utilities
{
    static uint8_t get_r_depth()
    {
        GLint v;
        glGetIntegerv(GL_ACCUM_RED_BITS, &v);
        if (v < 0)
            throw idlib::environment_error(__FILE__, __LINE__, "OpenGL", "negative number of red bits");
        else if (v > std::numeric_limits<uint8_t>::max())
            throw idlib::environment_error(__FILE__, __LINE__, "OpenGL", "number of red bits exceed support maximum");
        return (uint8_t)v;
    }

    static uint8_t get_g_depth()
    {
        GLint v;
        glGetIntegerv(GL_ACCUM_GREEN_BITS, &v);
        if (v < 0)
            throw idlib::environment_error(__FILE__, __LINE__, "OpenGL", "negative number of green bits");
        else if (v > std::numeric_limits<uint8_t>::max())
            throw idlib::environment_error(__FILE__, __LINE__, "OpenGL", "number of green bits exceed support maximum");
        return (uint8_t)v;
    }

    static uint8_t get_b_depth()
    {
        GLint v;
        glGetIntegerv(GL_ACCUM_BLUE_BITS, &v);
        if (v < 0)
            throw idlib::environment_error(__FILE__, __LINE__, "OpenGL", "negative number of blue bits");
        else if (v > std::numeric_limits<uint8_t>::max())
            throw idlib::environment_error(__FILE__, __LINE__, "OpenGL", "number of blue bits exceed support maximum");
        return (uint8_t)v;
    }

    static uint8_t get_a_depth()
    {
        GLint v;
        glGetIntegerv(GL_ACCUM_ALPHA_BITS, &v);
        if (v < 0)
            throw idlib::environment_error(__FILE__, __LINE__, "OpenGL", "negative number of alpha bits");
        else if (v > std::numeric_limits<uint8_t>::max())
            throw idlib::environment_error(__FILE__, __LINE__, "OpenGL", "number of alpha bits exceed support maximum");
        return (uint8_t)v;
    }
};

AccumulationBuffer::AccumulationBuffer() :
    Ego::AccumulationBuffer(), colourDepth({ utilities::get_r_depth(),
                                             utilities::get_g_depth(),
                                             utilities::get_b_depth() },
                                             utilities::get_a_depth())
{}

AccumulationBuffer::~AccumulationBuffer()
{}

void AccumulationBuffer::clear() {
    glClear(GL_ACCUM_BUFFER_BIT);
    Utilities2::isError();
}

void AccumulationBuffer::setClearValue(const Colour4f& value) {
    glClearAccum(value.get_r(), value.get_g(), value.get_b(), value.get_a());
    Utilities2::isError();
}

const idlib::rgba_depth& AccumulationBuffer::getColourDepth() {
	return colourDepth;
}

} // namespace Ego::OpenGL
