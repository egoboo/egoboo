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

#include "game/egoboo.h"
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
struct EntityList {
    /**
     * @brief
     *    The (fixed) capacity of a do list.
     */
    static constexpr size_t CAPACITY = OBJECTS_MAX + PARTICLES_MAX;
    /**
     * @brief
     *  An entry of an entity list.
     */
    struct Element {
        Element()
            : iobj(ObjectRef::Invalid), iprt(ParticleRef::Invalid), dist(0.0f) {}
        Element(const Element& other)
            : iobj(other.iobj), iprt(other.iprt), dist(other.dist) {}
        Element(ObjectRef iobj, ParticleRef iprt)
            : iobj(iobj), iprt(iprt), dist(0.0f) {}

        ObjectRef iobj;
        ParticleRef iprt;
        float dist;
    };
    struct Compare {
        bool operator()(const Element& x, const Element& y) const {
            return x.dist < y.dist;
        }
    };
private:
    /** An array of the entities in this entity list. */
    std::vector<Element> list;
    /** For checking in amortized constant time if an object is already in this entity list. */
    std::unordered_set<void *> set;

private:
    /**
     * @brief Test if the specified object entity is eligible for addition.
     * @return @a true if the specified object entity is eligible for addition, @a false otherwise
     */
    bool test(::Camera& camera, const Object& object);

    /**
     * @brief Test if the specified particle entity is eligible for addition.
     * @return @a true if the specified particle entity is eligible for addition, @a false otherwise
     * @todo Should be a reference to a particle.
     */
    bool test(::Camera& camera, const Ego::Particle& particle);

public:
    EntityList();

    const Element& get(size_t index) const {
        if (index >= list.size()) {
            throw std::out_of_range("index out of range");
        }
        return list[index];
    }

    Element& get(size_t index) {
        if (index >= list.size()) {
            throw std::out_of_range("index out of range");
        }
        return list[index];
    }

    size_t getSize() const {
        return list.size();
    }

    /** @brief Clear this entity list. */
    void clear();
    void sort(Camera& camera, const bool reflect);

    /**
     * @brief Add an object entity if it is eligible for addition.
     * @param obj the object entity to add
     * @return the total number of entities added
     * @remark A (legacy) occlusion test is performed. If the object is occluded,
     * then object.getInstance().isOccluded() is set to @a true, otherwise it is
     * set to @a false.
     */
    size_t add(::Camera& camera, Object& object);

    /**
     * @brief Add a particle entity if it is eligible for addition.
     * @param obj the particle entity to add
     * @return the total number of entities added
     * @remark A (legacy) occlusion test is performed. If the particle is occluded,
     * then particle.getInstance().isOccluded() is set to @a true, otherwise it is
     * set to @a false.
     */
    size_t add(::Camera& camera, Ego::Particle& particle);
};

} // namespace Graphics
} // namespace Ego
