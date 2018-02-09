#include "egolib/game/Graphics/ParticleGraphics.hpp"
#include "egolib/game/Graphics/Camera.hpp"
#include "egolib/game/lighting.h"
#include "egolib/game/graphic.h"
#include "egolib/Entities/_Include.hpp"

namespace Ego {
namespace Graphics {

ParticleGraphics::ParticleGraphics() :
    valid(false),
    indolist(false),

    // basic info
    type(0),
    image_ref(0),
    alpha(0.0f),
    light(0),

    // position info
    pos(idlib::zero<Vector3f>()),
    size(0.0f),
    scale(0.0f),

    // billboard info
    orientation(prt_ori_t::ORIENTATION_B),
    up(idlib::zero<Vector3f>()),
    right(idlib::zero<Vector3f>()),
    nrm(idlib::zero<Vector3f>()),

    // lighting info
    famb(0.0f),
    fdir(0.0f),

    fintens(0.0f),
    falpha(0.0f),

    // pre-compute some values for the reflected particle posisions
    ref_valid(false),
    ref_up(idlib::zero<Vector3f>()),
    ref_right(idlib::zero<Vector3f>()),
    ref_pos(idlib::zero<Vector3f>())
{
    //ctor   
}

void ParticleGraphics::reset()
{
    (*this) = ParticleGraphics();
}

gfx_rv ParticleGraphics::update_vertices(ParticleGraphics& inst, ::Camera& camera, Particle *pprt)
{
    inst.valid = false;
    inst.ref_valid = false;

    if (pprt->isTerminated())
    {
        Log::Entry e(Log::Level::Error, __FILE__, __LINE__);
        e << "invalid particle `" << pprt->getParticleID() << "`" << Log::EndOfEntry;
        Log::get() << e;
        return gfx_error;
    }

    const auto& ppip = pprt->getProfile();

    inst.type = pprt->type;

    inst.image_ref = (pprt->_image._start / EGO_ANIMATION_MULTIPLIER + pprt->_image._offset / EGO_ANIMATION_MULTIPLIER);


    // Set the position.
    inst.pos = pprt->getPosition();
    inst.orientation = ppip->orientation;

    // Calculate the billboard vectors for the reflections.
    inst.ref_pos = pprt->getPosition();
    inst.ref_pos[kZ] = 2 * pprt->enviro.floor_level - inst.pos[kZ];

    // get the vector from the camera to the particle
    Vector3f vfwd = inst.pos - camera.getPosition();
    vfwd = normalize(vfwd).get_vector();

    Vector3f vfwd_ref = inst.ref_pos - camera.getPosition();
    vfwd_ref = normalize(vfwd_ref).get_vector();

    // Set the up and right vectors.
    Vector3f vup = Vector3f(0.0f, 0.0f, 1.0f), vright;
    Vector3f vup_ref = Vector3f(0.0f, 0.0f, 1.0f), vright_ref;
    if (ppip->rotatetoface && !pprt->isAttached() && (idlib::manhattan_norm(pprt->getVelocity()) > 0))
    {
        // The particle points along its direction of travel.

        vup = pprt->getVelocity();
        vup = normalize(vup).get_vector();

        // Get the correct "right" vector.
        vright = cross(vfwd, vup);
        vright = normalize(vright).get_vector();

        vup_ref = vup;
        vright_ref = cross(vfwd_ref, vup);
        vright_ref = normalize(vright_ref).get_vector();
    }
    else if (prt_ori_t::ORIENTATION_B == inst.orientation)
    {
        // Use the camera up vector.
        vup = camera.getUp();
        vup = normalize(vup).get_vector();

        // Get the correct "right" vector.
        vright = cross(vfwd, vup);
        vright = normalize(vright).get_vector();

        vup_ref = vup;
        vright_ref = cross(vfwd_ref, vup);
        vright_ref = normalize(vright_ref).get_vector();
    }
    else if (prt_ori_t::ORIENTATION_V == inst.orientation)
    {
        // Using just the global up vector here is too harsh.
        // Smoothly interpolate the global up vector with the camera up vector
        // so that when the camera is looking straight down, the billboard's plane
        // is turned by 45 degrees to the camera (instead of 90 degrees which is invisible)

        // Use the camera up vector.
        Vector3f vup_cam = camera.getUp();

        // Use the global up vector.
        vup = Vector3f(0.0f, 0.0f, 1.0f);

        // Adjust the vector so that the particle doesn't disappear if
        // you are viewing it from from the top or the bottom.
        float weight = 1.0f - std::abs(vup_cam[kZ]);
        if (vup_cam[kZ] < 0) weight *= -1;

        vup += vup_cam * weight;
        vup = normalize(vup).get_vector();

        // Get the correct "right" vector.
        vright = cross(vfwd, vup);
        vright = normalize(vright).get_vector();

        vright_ref = cross(vfwd, vup_ref);
        vright_ref = normalize(vright_ref).get_vector();

        vup_ref = vup;
        vright_ref = cross(vfwd_ref, vup);
        vright_ref = normalize(vright_ref).get_vector();
    }
    else if (prt_ori_t::ORIENTATION_H == inst.orientation)
    {
        Vector3f vert = Vector3f(0.0f, 0.0f, 1.0f);

        // Force right to be horizontal.
        vright = cross(vfwd, vert);

        // Force "up" to be close to the camera forward, but horizontal.
        vup = cross(vert, vright);
        //vup_ref = vert.cross(vright_ref); //TODO: JJ> is this needed?

        // Normalize them.
        vright = normalize(vright).get_vector();
        vup = normalize(vup).get_vector();

        vright_ref = vright;
        vup_ref = vup;
    }
    else if (pprt->isAttached())
    {
        auto& cinst = pprt->getAttachedObject()->inst;

        if (chr_matrix_valid(pprt->getAttachedObject().get()))
        {
            // Use the character matrix to orient the particle.
            // Assume that the particle "up" is in the z-direction in the object's
            // body fixed axes. Should work for the gonnes & such.

            switch (inst.orientation)
            {
                case prt_ori_t::ORIENTATION_X: vup = mat_getChrForward(cinst.getMatrix()); break;
                case prt_ori_t::ORIENTATION_Y: vup = mat_getChrRight(cinst.getMatrix());   break;
                default:
                case prt_ori_t::ORIENTATION_Z: vup = mat_getChrUp(cinst.getMatrix());      break;
            }

            vup = normalize(vup).get_vector();
        }
        else
        {
            // Use the camera directions?
            switch (inst.orientation)
            {
                case prt_ori_t::ORIENTATION_X: vup = camera.getForward(); break;
                case prt_ori_t::ORIENTATION_Y: vup = camera.getRight(); break;

                default:
                case prt_ori_t::ORIENTATION_Z: vup = camera.getUp(); break;
            }
        }

        vup = normalize(vup).get_vector();

        // Get the correct "right" vector.
        vright = cross(vfwd, vup);
        vright = normalize(vright).get_vector();

        vup_ref = vup;
        vright_ref = cross(vfwd_ref, vup);
        vright_ref = normalize(vright_ref).get_vector();
    }
    else
    {
        // Use the camera up vector.
        vup = camera.getUp();
        vup = normalize(vup).get_vector();

        // Get the correct "right" vector.
        vright = cross(vfwd, vup);
        vright = normalize(vright).get_vector();

        vup_ref = vup;
        vright_ref = cross(vfwd_ref, vup);
        vright_ref = normalize(vright_ref).get_vector();
    }

    // Calculate the actual vectors using the particle rotation.
    /// @todo An optimization for the special case where the angle
    /// a is 0, taking advantage of the obvious fact that cos(a)=1,
    /// sin(a)=0 for a = 0. However, it is a quite special optimization,
    /// as it does not take into account the cases a = n * 360, n =
    /// ..., -1, 0, +1, ...
    if (Facing(0) == pprt->rotate)
    {
        inst.up = vup; // vup * 1 - vright * 0
        inst.right = vright; // vup * 0 + vright * 1

        inst.ref_up = vup_ref; // vup_ref * 1 - vright_ref * 0
        inst.ref_right = vright_ref; // vup_ref * 0 + vright_ref * 1
    }
    else
    {
        float cosval = std::cos(pprt->rotate);
        float sinval = std::sin(pprt->rotate);

        inst.up = vup * cosval - vright * sinval;

        inst.right = vup * sinval + vright * cosval;

        inst.ref_up = vup_ref * cosval - vright_ref * sinval;

        inst.ref_right = vup_ref * sinval + vright_ref * cosval;
    }

    // Calculate the billboard normal.
    inst.nrm = cross(inst.right, inst.up);

    // Flip the normal so that the front front of the quad is toward the camera.
    if (dot(vfwd, inst.nrm) < 0)
    {
        inst.nrm *= -1;
    }

    // Now we have to calculate the mirror-like reflection of the particles.
    // This was a bit hard to figure. What happens is that the components of the
    // up and right vectors that are in the plane of the quad and closest to the world up are reversed.
    //
    // This is easy to think about in a couple of examples:
    // 1) If the quad is like a picture frame then whatever component (up or right)
    //    that actually points in the wodld up direction is reversed.
    //    This corresponds to the case where zdot == +/- 1 in the code below.
    //
    // 2) If the particle is like a rug, then basically nothing happens since
    //    neither the up or right vectors point in the world up direction.
    //    This corresponds to 0 == ndot in the code below.
    //
    // This process does not affect the normal the length of the vector, or the
    // direction of the normal to the quad.

    {
        // The normal sense of "up".
        Vector3f world_up = Vector3f(0.0f, 0.0f, 1.0f);

        // The dot product between the normal vector and the world up vector:
        // The following statement could be optimized
        // since we know the only non-zero component of the world up vector is z.
        float ndot = dot(inst.nrm, world_up);

        // Do nothing if the quad is basically horizontal.
        if (ndot < 1.0f - 1e-6)
        {
            // Do the right vector first.
            {
                // The dot product between the right vector and the world up:
                // The following statement could be optimized
                // since we know the only non-zero component of the world up vector is z.
                float zdot = dot(inst.ref_right, world_up);

                if (std::abs(zdot) > 1e-6)
                {
                    float factor = zdot / (1.0f - ndot * ndot);
                    inst.ref_right += ((inst.nrm * ndot) - world_up) * 2.0f * factor;
                }
            }

            // Do the up vector second.
            {
                // The dot product between the up vector and the world up:
                // The following statement could be optimized
                // since we know the only non-zero component of the world up vector is z.
                float zdot = dot(inst.ref_up, world_up);

                if (std::abs(zdot) > 1e-6)
                {
                    float factor = zdot / (1.0f - ndot * ndot);
                    inst.ref_up += (inst.nrm * ndot - world_up) * 2.0f * factor;
                }
            }
        }
    }

    // Set some particle dependent properties.
    inst.scale = pprt->getScale();
    inst.size = FP8_TO_FLOAT(pprt->size) * inst.scale;

    // This instance is now completely valid.
    inst.valid = true;
    inst.ref_valid = true;

    return gfx_success;
}

Matrix4f4f ParticleGraphics::make_matrix(ParticleGraphics& pinst)
{
    Matrix4f4f mat = idlib::identity<Matrix4f4f>();

    mat(1, 0) = -pinst.up[kX];
    mat(1, 1) = -pinst.up[kY];
    mat(1, 2) = -pinst.up[kZ];

    mat(0, 0) = pinst.right[kX];
    mat(0, 1) = pinst.right[kY];
    mat(0, 2) = pinst.right[kZ];

    mat(2, 0) = pinst.nrm[kX];
    mat(2, 1) = pinst.nrm[kY];
    mat(2, 2) = pinst.nrm[kZ];

    return mat;
}

gfx_rv ParticleGraphics::update_lighting(ParticleGraphics& pinst, Particle *pprt, Uint8 trans, bool do_lighting)
{
    if (!pprt)
    {
        Log::Entry e(Log::Level::Error, __FILE__, __LINE__);
        e << "nullptr == particle" << Log::EndOfEntry;
        Log::get() << e;
        return gfx_error;
    }

    // To make life easier
    uint32_t alpha = trans;

    // interpolate the lighting for the origin of the object
    auto mesh = _currentModule->getMeshPointer();
    if (!mesh)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "mesh");
    }
    lighting_cache_t global_light;
    GridIllumination::grid_lighting_interpolate(*mesh, global_light, Vector2f(pinst.pos[kX], pinst.pos[kY]));

    // rotate the lighting data to body_centered coordinates
    Matrix4f4f mat = ParticleGraphics::make_matrix(pinst);
    lighting_cache_t loc_light;
    lighting_cache_t::lighting_project_cache(loc_light, global_light, mat);

    // determine the normal dependent amount of light
    float amb, dir;
    lighting_cache_t::lighting_evaluate_cache(loc_light, pinst.nrm, pinst.pos[kZ], _currentModule->getMeshPointer()->_tmem._bbox, &amb, &dir);

    // LIGHT-blended sprites automatically glow. ALPHA-blended and SOLID
    // sprites need to convert the light channel into additional alpha
    // lighting to make them "glow"
    int16_t self_light = 0;
    if (SPRITE_LIGHT != pinst.type)
    {
        self_light = (255 == pinst.light) ? 0 : pinst.light;
    }

    // determine the ambient lighting
    pinst.famb = 0.9f * pinst.famb + 0.1f * (self_light + amb);
    pinst.fdir = 0.9f * pinst.fdir + 0.1f * dir;

    // determine the overall lighting
    pinst.fintens = pinst.fdir * idlib::fraction<float, 1, 255>();
    if (do_lighting)
    {
        pinst.fintens += pinst.famb * idlib::fraction<float, 1, 255>();
    }
    pinst.fintens = Ego::Math::constrain(pinst.fintens, 0.0f, 1.0f);

    // determine the alpha component
    pinst.falpha = (alpha * idlib::fraction<float, 1, 255>()) * (pinst.alpha * idlib::fraction<float, 1, 255>());
    pinst.falpha = Ego::Math::constrain(pinst.falpha, 0.0f, 1.0f);

    return gfx_success;
}

gfx_rv ParticleGraphics::update(::Camera& camera, const ParticleRef particle, uint8_t trans, bool do_lighting)
{
    const auto& pprt = ParticleHandler::get()[particle];
    if (!pprt)
    {
        Log::Entry e(Log::Level::Error, __FILE__, __LINE__);
        e << "invalid particle `" << particle << "`" << Log::EndOfEntry;
        Log::get() << e;
        return gfx_error;
    }

    auto& pinst = pprt->inst;

    // assume the best
    gfx_rv retval = gfx_success;

    // make sure that the vertices are interpolated
    if (gfx_error == ParticleGraphics::update_vertices(pinst, camera, pprt.get()))
    {
        retval = gfx_error;
    }

    // do the lighting
    if (gfx_error == ParticleGraphics::update_lighting(pinst, pprt.get(), trans, do_lighting))
    {
        retval = gfx_error;
    }

    return retval;
}

} // namespace Graphics
} // namespace Ego
