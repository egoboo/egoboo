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

/// @file game/Graphics/EntityList.hpp
/// @brief A list of entities as used by the graphics system
/// @author Michael Heilmann

#pragma once

#include "game/egoboo_typedef.h"
#include "game/mesh.h"
#include "game/Graphics/CameraSystem.hpp"

namespace Ego {
namespace Graphics {
/**
 * @brief
 *  List of object  and particle entities to be draw by a renderer.
 *
 *  Entities in this are sorted based on their position from the camera before drawing.
 */
struct EntityList
{
    /**
     * @brief
     *    The (fixed) capacity of a do list.
     */
    static const size_t CAPACITY = OBJECTS_MAX + PARTICLES_MAX;
    /**
     * @brief
     *    An eleemnt of a do list.
     */
    struct element_t
    {
        CHR_REF ichr;
        PRT_REF iprt;
        float dist;
        
        element_t() :
            ichr(INVALID_CHR_REF), iprt(INVALID_PRT_REF), dist(0.0f)
        {}
#if XX == 1
        element_t(const element_t& other) :
            ichr(other.ichr), iprt(other.iprt), dist(other.dist)
        {}
        element_t& operator=(const element_t& other)
        {
            ichr = other.ichr;
            iprt = other.iprt;
            dist = other.dist;
            return *this;
        }
        virtual ~element_t()
        {}
#endif

        static element_t *init(element_t *self);
        static int cmp(const void *left, const void *right);
    };
protected:

    /**
     * @brief
     *  The size of the dolist i.e. the number of character and particle entities in the dolist.
     */
    size_t _size;
    /**
     * @brief
     *  An array of dolist elements.
     *  The first @a _size entries of this array have meaningful values.
     */
    element_t _lst[CAPACITY];
public:
    EntityList();
#if XX == 1
    virtual ~EntityList()
    {}
#endif
    EntityList *init();
    const element_t& get(size_t index) const
    {
        if (index >= _size)
        {
            throw std::out_of_range("index out of range");
        }
        return _lst[index];
    }
    element_t& get(size_t index)
    {
        if (index >= _size)
        {
            throw std::out_of_range("index out of range");
        }
        return _lst[index];
    }
    size_t getSize() const
    {
        return _size;
    }
    gfx_rv reset();
    gfx_rv sort(Camera& camera, const bool reflect);
protected:
    gfx_rv test_obj(const Object& obj);
    gfx_rv add_obj_raw(Object& obj);
    gfx_rv test_prt(const prt_t& prt);
    gfx_rv add_prt_raw(prt_t& prt);
public:
    /// @brief Insert character or particle entities into this dolist.
    /// @param leaves
    gfx_rv add_colst(const Ego::DynamicArray<BSP_leaf_t *> *collisions);
};

} // namespace Graphics
} // namespace Ego
