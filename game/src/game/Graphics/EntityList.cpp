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
        if (INVALID_CHR_REF == element->ichr && element->iprt != INVALID_PRT_REF)
        {
            const std::shared_ptr<Ego::Particle> &pprt = ParticleHandler::get()[element->iprt];
            if (nullptr != pprt) pprt->inst.indolist = false;
        }
        else if (INVALID_PRT_REF == element->iprt && INVALID_CHR_REF != element->ichr)
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
    if (obj.isHidden())
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

gfx_rv EntityList::test_prt(const std::shared_ptr<Ego::Particle>& prt)
{
    // The entity is not a candidate if the list is full.
    if (_size == CAPACITY)
    {
        return gfx_fail;
    }

    // The entity is not a candidate if it is not displayed.
    if (!prt || prt->isTerminated())
    {
        return gfx_fail;
    }

    // The entity is not a candidate if it is explicitly or implicitly hidden.
    if (prt->isHidden() || 0 == prt->size)
    {
        return gfx_fail;
    }

    return gfx_success;
}

gfx_rv EntityList::add_prt_raw(const std::shared_ptr<Ego::Particle>& prt)
{
    /// @author ZZ
    /// @details This function puts an entity in the list

    _lst[_size].ichr = INVALID_CHR_REF;
    _lst[_size].iprt = prt->getParticleID();
    _size++;

    prt->inst.indolist = true;

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

	Vector3f vcam;
    mat_getCamForward(cam.getViewMatrix(), vcam);

    // Figure the distance of each.
    size_t count = 0;
    for (size_t i = 0; i < _size; ++i)
    {
		Vector3f vtmp;

        if (INVALID_PRT_REF == _lst[i].iprt && INVALID_CHR_REF != _lst[i].ichr)
        {
			Vector3f pos_tmp;

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
        else if (INVALID_CHR_REF == _lst[i].ichr && _lst[i].iprt != INVALID_PRT_REF)
        {
            PRT_REF iprt = _lst[i].iprt;

            if (do_reflect)
            {
                vtmp = ParticleHandler::get()[iprt]->inst.pos - cam.getPosition();
            }
            else
            {
                vtmp = ParticleHandler::get()[iprt]->inst.ref_pos - cam.getPosition();
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
