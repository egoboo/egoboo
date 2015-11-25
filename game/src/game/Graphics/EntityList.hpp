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
    static constexpr size_t CAPACITY = OBJECTS_MAX + PARTICLES_MAX;
    /**
     * @brief
     *    An eleemnt of a do list.
     */
    struct element_t
    {
        element_t(const element_t& other)
            : iobj(other.iobj), iprt(other.iprt), dist(other.dist)
        { }
        element_t(ObjectRef iobj, PRT_REF iprt)
            : iobj(iobj), iprt(iprt), dist(0.0f)
        { }

        ObjectRef iobj;
        PRT_REF iprt;
        float dist;
    };
    struct Compare {
        bool operator()(const element_t& x, const element_t& y) const {
            return x.dist < y.dist;
        }
    };
protected:

    /**
    * @brief
    *  An array of dolist elements.
    */
    std::vector<element_t> _lst;

public:
    EntityList();
    void init();
    
    const element_t& get(size_t index) const
    {
        if (index >= _lst.size())
        {
            throw std::out_of_range("index out of range");
        }
        return _lst[index];
    }

    element_t& get(size_t index)
    {
        if (index >= _lst.size())
        {
            throw std::out_of_range("index out of range");
        }
        return _lst[index];
    }
    
    size_t getSize() const
    {
        return _lst.size();
    }

    gfx_rv reset();
    gfx_rv sort(Camera& camera, const bool reflect);

    gfx_rv test_obj(const Object& obj);
    gfx_rv add_obj_raw(Object& obj);

    gfx_rv test_prt(const std::shared_ptr<Ego::Particle>& prt);
    gfx_rv add_prt_raw(const std::shared_ptr<Ego::Particle>& prt);    
};

} // namespace Graphics
} // namespace Ego
