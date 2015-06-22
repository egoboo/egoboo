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
