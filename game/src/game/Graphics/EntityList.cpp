#include "game/Graphics/EntityList.hpp"
#include "game/graphic.h"
#include "game/graphic_prt.h"
#include "game/char.h"

namespace Ego {
namespace Graphics {

EntityList::EntityList()
    : _lst()
{ }

void EntityList::init()
{
    reset();
}

gfx_rv EntityList::reset()
{
    // If there is nothing in the dolist, we are done.
    if (_lst.empty())
    {
        return gfx_success;
    }

    for(element_t &element : _lst)
    {
        // Tell all valid objects that they are removed from this dolist.
        if (ObjectRef::Invalid == element.iobj && element.iprt != ParticleRef::Invalid)
        {
            const std::shared_ptr<Ego::Particle> &pprt = ParticleHandler::get()[element.iprt.get()];
            if (nullptr != pprt) pprt->inst.indolist = false;
        }
        else if (ParticleRef::Invalid == element.iprt && ObjectRef::Invalid != element.iobj)
        {
            const std::shared_ptr<Object> &pobj = _currentModule->getObjectHandler()[element.iobj];
            if (nullptr != pobj) pobj->inst.indolist = false;
        }
    }

    _lst.clear();
    return gfx_success;
}

gfx_rv EntityList::test_obj(const Object& obj)
{
    // The entity is not a candidate if the list is full.
    if (_lst.size() == CAPACITY)
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
    _lst.emplace_back(obj.getObjRef(), ParticleRef::Invalid);

    // Notify it that it is in a do list.
    obj.inst.indolist = true;

    // Add any weapons it is holding.
    Object *holding;
    holding = _currentModule->getObjectHandler().get(obj.holdingwhich[SLOT_LEFT]);
    if (holding && _lst.size() < CAPACITY)
    {
        add_obj_raw(*holding);
    }
    holding = _currentModule->getObjectHandler().get(obj.holdingwhich[SLOT_RIGHT]);
    if (holding && _lst.size() < CAPACITY)
    {
        add_obj_raw(*holding);
    }
    return gfx_success;
}

gfx_rv EntityList::test_prt(const std::shared_ptr<Ego::Particle>& prt)
{
    // The entity is not a candidate if the list is full.
    if (_lst.size() == CAPACITY)
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

    _lst.emplace_back(ObjectRef::Invalid, prt->getParticleID());
    prt->inst.indolist = true;

    return gfx_success;
}

gfx_rv EntityList::sort(Camera& cam, const bool do_reflect)
{
    /// @author ZZ
    /// @details This function orders the entity list based on distance from camera,
    ///    which is needed for reflections to properly clip themselves.
    ///    Order from closest to farthest
    if (_lst.size() >= CAPACITY)
    {
        throw std::logic_error("invalid entity list size");
    }

	Vector3f vcam;
    mat_getCamForward(cam.getViewMatrix(), vcam);

    // Figure the distance of each.
    size_t count = 0;
    for (size_t i = 0; i < _lst.size(); ++i)
    {
		Vector3f vtmp;

        if (ParticleRef::Invalid == _lst[i].iprt && ObjectRef::Invalid != _lst[i].iobj)
        {
			Vector3f pos_tmp;

            ObjectRef iobj = _lst[i].iobj;

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
        else if (ObjectRef::Invalid == _lst[i].iobj && _lst[i].iprt != ParticleRef::Invalid)
        {
            ParticleRef iprt = _lst[i].iprt;

            if (do_reflect)
            {
                vtmp = ParticleHandler::get()[iprt.get()]->inst.pos - cam.getPosition();
            }
            else
            {
                vtmp = ParticleHandler::get()[iprt.get()]->inst.ref_pos - cam.getPosition();
            }
        }
        else
        {
            continue;
        }

        float dist = vtmp.dot(vcam);
        if (dist > 0)
        {
            _lst[count].iobj = _lst[i].iobj;
            _lst[count].iprt = _lst[i].iprt;
            _lst[count].dist = dist;
            count++;
        }
    }

    // use qsort to sort the list in-place
    if (count > 1)
    {
        std::sort(_lst.begin(), _lst.end(), Compare());
    }

    return gfx_success;
}

} // namespace Graphics
} // namespace Ego
