#include "game/Graphics/EntityList.hpp"
#include "game/graphic.h"
#include "game/graphic_prt.h"
#include "game/Entities/_Include.hpp"

namespace Ego {
namespace Graphics {

EntityList::EntityList()
    : list(), set() {}

void EntityList::clear() {
    if (list.empty()) {
        return;
    }

    set.clear();
    auto& objectHandler = _currentModule->getObjectHandler();
    auto& particleHandler = ParticleHandler::get();
    for (auto& entry : list) {
        if (ParticleRef::Invalid == entry.iprt && ObjectRef::Invalid != entry.iobj) {
            auto obj = objectHandler.get(entry.iobj);
            if (nullptr != obj) {
                obj->inst.indolist = false;
            }
        } else if (ObjectRef::Invalid == entry.iobj && entry.iprt != ParticleRef::Invalid) {
            auto prt = particleHandler[entry.iprt];
            if (nullptr != prt) {
                prt->inst.indolist = false;
            }
        } else {
            continue;
        }
    }
    list.clear();
}

size_t EntityList::add(::Camera& camera, Object& object) {
    size_t count = 0;
    // Only add the object if it is eligible for addition.
    if (!test(camera, object)) {
        object.inst.indolist = false;
        return count;
    }
    object.inst.indolist = true;

    // Add the object.
    list.emplace_back(object.getObjRef(), ParticleRef::Invalid);
    set.emplace((void *)(&object));
    count++;

    // Add any weapons it is holding.
    Object *holding;
    holding = _currentModule->getObjectHandler().get(object.holdingwhich[SLOT_LEFT]);
    if (holding) {
        count += add(camera, *holding);
    }
    holding = _currentModule->getObjectHandler().get(object.holdingwhich[SLOT_RIGHT]);
    if (holding) {
        count += add(camera, *holding);
    }
    return count;
}

size_t EntityList::add(::Camera& camera, Ego::Particle& particle) {
    size_t count = 0;
    if (!test(camera, particle)) {
        particle.inst.indolist = false;
        return count;
    }
    particle.inst.indolist = true;

    list.emplace_back(ObjectRef::Invalid, particle.getParticleID());
    set.emplace((void *)(&particle));
    count++;

    return count;
}

void EntityList::sort(Camera& cam, const bool do_reflect) {
    /// @author ZZ
    /// @details This function orders the entity list based on distance from camera,
    ///    which is needed for reflections to properly clip themselves.
    ///    Order from closest to farthest
    assert(list.size() <= CAPACITY);

    Vector3f vcam;
    mat_getCamForward(cam.getViewMatrix(), vcam);

    // Figure the distance of each.
    size_t count = 0;
    for (size_t i = 0; i < list.size(); ++i) {
        Vector3f vtmp;

        if (ParticleRef::Invalid == list[i].iprt && ObjectRef::Invalid != list[i].iobj) {
            Vector3f pos_tmp;

            ObjectRef iobj = list[i].iobj;

            if (do_reflect) {
                pos_tmp = mat_getTranslate(_currentModule->getObjectHandler().get(iobj)->inst.ref.matrix);
            } else {
                pos_tmp = mat_getTranslate(_currentModule->getObjectHandler().get(iobj)->inst.matrix);
            }

            vtmp = pos_tmp - cam.getPosition();
        } else if (ObjectRef::Invalid == list[i].iobj && list[i].iprt != ParticleRef::Invalid) {
            ParticleRef iprt = list[i].iprt;

            if (do_reflect) {
                vtmp = ParticleHandler::get()[iprt]->inst.pos - cam.getPosition();
            } else {
                vtmp = ParticleHandler::get()[iprt]->inst.ref_pos - cam.getPosition();
            }
        } else {
            continue;
        }

        // If theangle between this vector and the camera vector is greater than 90 degrees,
        // then set the distance to positive infinity.
        float dist = vtmp.dot(vcam);
        if (dist > 0) {
            list[count].iobj = list[i].iobj;
            list[count].iprt = list[i].iprt;
            list[count].dist = dist;
            count++;
        }
    }

    // use qsort to sort the list in-place
    if (count > 1) {
        std::sort(list.begin(), list.end(), Compare());
    }
}

bool EntityList::test(::Camera& camera, const Object& object) {
    // The object is not a candidate if the list is full.
    if (list.size() == CAPACITY) {
        return false;
    }
    // The object is not a candidate if it is terminated.
    if (object.isTerminated()) {
        return false;
    }
    // The object is not a candidate if it is hidden.
    if (object.isHidden()) {
        return false;
    }
    // The object is not a candidate if it is in another object's inventory. 
    if (_currentModule->getObjectHandler().exists(object.inwhich_inventory)) {
        return false;
    }
    // The object is not a candidate if it is already in this entity list.
    return set.find((void *)(&object)) == set.cend();
}

bool EntityList::test(::Camera& camera, const Ego::Particle& particle) {
    // The particle is not a candidate if the list is full.
    if (list.size() == CAPACITY) {
        return false;
    }

    // The particle is not a candidate if it is not displayed.
    if (particle.isTerminated()) {
        return false;
    }

    // The particle is not a candidate if it is explicitly or implicitly hidden.
    if (particle.isHidden() || 0 == particle.size) {
        return false;
    }

    // The particle is not a candidate if
    // both its bounding sphere and its reflected bounding sphere
    // are outside of the frustum.
    const auto& frustum = camera.getFrustum();
    const Sphere3f sphere(particle.getPosition(), particle.bump_real.size_big);
    const Sphere3f reflectedSphere(particle.inst.ref_pos, particle.bump_real.size_big);
    if (Ego::Math::Relation::outside == frustum.intersects(sphere, false) &&
        Ego::Math::Relation::outside == frustum.intersects(reflectedSphere, false)) {
        return false;
    }

    return set.find((void *)(&particle)) == set.cend();
}

} // namespace Graphics
} // namespace Ego
