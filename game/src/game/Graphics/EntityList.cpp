#include "game/Graphics/EntityList.hpp"
#include "game/graphic.h"
#include "game/graphic_prt.h"
#include "game/char.h"

namespace Ego {
namespace Graphics {

EntityList::element_t *EntityList::element_t::init(EntityList::element_t * ptr)
{
	if (NULL == ptr) return NULL;

	BLANK_STRUCT_PTR(ptr)

	ptr->ichr = INVALID_CHR_REF;
	ptr->iprt = INVALID_PRT_REF;

	return ptr;
}

int EntityList::element_t::cmp(const void * pleft, const void * pright)
{
	element_t * dleft = (element_t *)pleft;
	element_t * dright = (element_t *)pright;

	int   rv;
	float diff;

	diff = dleft->dist - dright->dist;

	if (diff < 0.0f)
	{
		rv = -1;
	}
	else if (diff > 0.0f)
	{
		rv = 1;
	}
	else
	{
		rv = 0;
	}

	return rv;
}

EntityList::EntityList() :
    _size(0), _lst()
{}

EntityList *EntityList::init()
{
    for (size_t i = 0; i < CAPACITY; ++i)
    {
        element_t::init(&(_lst[i]));
    }
    _size = 0;
    return this;
}

gfx_rv EntityList::reset()
{
    // If there is nothing in the dolist, we are done.
    if (0 == _size)
    {
        return gfx_success;
    }

    for (size_t i = 0, n = _size; i < n; ++i)
    {
        element_t *element = &(_lst[i]);

        // Tell all valid objects that they are removed from this dolist.
        if (INVALID_CHR_REF == element->ichr && VALID_PRT_RANGE(element->iprt))
        {
            prt_t *pprt = ParticleHandler::get().get_ptr(element->iprt);
            if (nullptr != pprt) pprt->inst.indolist = false;
        }
        else if (INVALID_PRT_REF == element->iprt && VALID_CHR_RANGE(element->ichr))
        {
            Object *pobj = _currentModule->getObjectHandler().get(element->ichr);
            if (nullptr != pobj) pobj->inst.indolist = false;
        }
    }
    _size = 0;
    return gfx_success;
}

gfx_rv EntityList::test_obj(const Object& obj)
{
    // The entity is not a candidate if the list is full.
    if (_size == CAPACITY)
    {
        return gfx_fail;
    }
    // The entity is not a candidate if it is not in game.
    if (!INGAME_PCHR(&obj))
    {
        return gfx_fail;
    }

    return gfx_success;
}

gfx_rv EntityList::add_obj_raw(Object& obj)
{
    /// @author ZZ
    /// @details This function puts an entity in the list

    // Don't add if it is hidden.
    if (obj.is_hidden)
    {
        return gfx_fail;
    }

    // Don't add if it's in another character's inventory.
    if (_currentModule->getObjectHandler().exists(obj.inwhich_inventory))
    {
        return gfx_fail;
    }

    // Add!
    _lst[_size].ichr = GET_INDEX_PCHR(&obj);
    _lst[_size].iprt = INVALID_PRT_REF;
    _size++;

    // Notify it that it is in a do list.
    obj.inst.indolist = true;

    // Add any weapons it is holding.
    Object *holding;
    holding = _currentModule->getObjectHandler().get(obj.holdingwhich[SLOT_LEFT]);
    if (holding && _size < CAPACITY)
    {
        add_obj_raw(*holding);
    }
    holding = _currentModule->getObjectHandler().get(obj.holdingwhich[SLOT_RIGHT]);
    if (holding && _size < CAPACITY)
    {
        add_obj_raw(*holding);
    }
    return gfx_success;
}

gfx_rv EntityList::test_prt(const prt_t& prt)
{
    // The entity is not a candidate if the list is full.
    if (_size == CAPACITY)
    {
        return gfx_fail;
    }

    // The entity is not a candidate if it is not displayed.
    if (!DISPLAY_PPRT(&prt))
    {
        return gfx_fail;
    }

    // The entity is not a candidate if it is explicitly or implicitly hidden.
    if (prt.is_hidden || 0 == prt.size)
    {
        return gfx_fail;
    }

    return gfx_success;
}

gfx_rv EntityList::add_prt_raw(prt_t& prt)
{
    /// @author ZZ
    /// @details This function puts an entity in the list

    _lst[_size].ichr = INVALID_CHR_REF;
    _lst[_size].iprt = GET_REF_PPRT(&prt);
    _size++;

    prt.inst.indolist = true;

    return gfx_success;
}

gfx_rv EntityList::add_colst(const Ego::DynamicArray<BSP_leaf_t *> *leaves)
{
    if (!leaves)
    {
        throw std::invalid_argument("nullptr == self");
    }

    if (leaves->empty())
    {
        return gfx_fail;
    }
    size_t sizeLeaves = leaves->size();

    for (size_t j = 0; j < sizeLeaves; j++)
    {
        BSP_leaf_t *pleaf = leaves->ary[j];

        if (!pleaf) continue;
        if (!pleaf->valid()) continue;

        if (BSP_LEAF_CHR == pleaf->_type)
        {
            // Get the reference.
            CHR_REF iobj = (CHR_REF)(pleaf->_index);

            // Is this a valid object reference?
            if (!VALID_CHR_RANGE(iobj))
            {
                continue;
            }
            Object *pobj = _currentModule->getObjectHandler().get(iobj);
            if (!pobj)
            {
                continue;
            }

            // Do some more obvious tests before testing the frustum.
            if (test_obj(*pobj))
            {
                // Add the object.
                if (gfx_error == add_obj_raw(*pobj))
                {
                    return gfx_error;
                }
            }
        }
        else if (BSP_LEAF_PRT == pleaf->_type)
        {
            // Get the reference.
            PRT_REF iprt = (PRT_REF)(pleaf->_index);

            // Is it a valid reference.
            if (!VALID_PRT_RANGE(iprt))
            {
                continue;
            }
            prt_t *pprt = ParticleHandler::get().get_ptr(iprt);
            if (!pprt)
            {
                continue;
            }

            // Do some more obvious tests before testing the frustum.
            if (test_prt(*pprt))
            {
                // Add the particle.
                if (gfx_error == add_prt_raw(*pprt))
                {
                    return gfx_error;
                }
            }
        }
        else
        {
            // how did we get here?
            log_warning("%s-%s-%d- found unknown type in a dolist BSP\n", __FILE__, __FUNCTION__, __LINE__);
        }
    }

    return gfx_success;
}

gfx_rv EntityList::sort(Camera& cam, const bool do_reflect)
{
    /// @author ZZ
    /// @details This function orders the entity list based on distance from camera,
    ///    which is needed for reflections to properly clip themselves.
    ///    Order from closest to farthest
    if (_size >= CAPACITY)
    {
        throw std::logic_error("invalid entity list size");
    }

    fvec3_t vcam;
    mat_getCamForward(cam.getView(), vcam);

    // Figure the distance of each.
    size_t count = 0;
    for (size_t i = 0; i < _size; ++i)
    {
        fvec3_t vtmp;

        if (INVALID_PRT_REF == _lst[i].iprt && VALID_CHR_RANGE(_lst[i].ichr))
        {
            fvec3_t pos_tmp;

            CHR_REF iobj = _lst[i].ichr;

            if (do_reflect)
            {
                pos_tmp = mat_getTranslate(_currentModule->getObjectHandler().get(iobj)->inst.ref.matrix);
            }
            else
            {
                pos_tmp = mat_getTranslate(_currentModule->getObjectHandler().get(iobj)->inst.matrix);
            }

            vtmp = pos_tmp - cam.getPosition();
        }
        else if (INVALID_CHR_REF == _lst[i].ichr && VALID_PRT_RANGE(_lst[i].iprt))
        {
            PRT_REF iprt = _lst[i].iprt;

            if (do_reflect)
            {
                vtmp = ParticleHandler::get().get_ptr(iprt)->inst.pos - cam.getPosition();
            }
            else
            {
                vtmp = ParticleHandler::get().get_ptr(iprt)->inst.ref_pos - cam.getPosition();
            }
        }
        else
        {
            continue;
        }

        float dist = vtmp.dot(vcam);
        if (dist > 0)
        {
            _lst[count].ichr = _lst[i].ichr;
            _lst[count].iprt = _lst[i].iprt;
            _lst[count].dist = dist;
            count++;
        }
    }
    _size = count;

    // use qsort to sort the list in-place
    if (_size > 1)
    {
        qsort(_lst, _size, sizeof(element_t), element_t::cmp);
    }

    return gfx_success;
}

} // namespace Graphics
} // namespace Ego
