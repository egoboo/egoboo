#pragma once

#include "egolib/game/egoboo.h"

// Forward declarations.
class Camera;
namespace Ego {
class Particle;
} // namespace Ego

namespace Ego {
namespace Graphics {

/// All the data necessary to display a particle.
struct ParticleGraphics
{
    bool valid;                ///< is the infor in this struct valid?

                               // graphical optimizations
    bool         indolist;     ///< Has it been added yet?

                               // basic info
    uint8_t  type;               ///< particle type
    uint32_t image_ref;          ///< which sub image within the texture?
    float    alpha;              ///< base alpha
    uint8_t  light;              ///< base self lighting

                                 // position info
    Vector3f  pos;
    float     size;
    float     scale;

    // billboard info
    prt_ori_t orientation;
    Vector3f  up;
    Vector3f  right;
    Vector3f  nrm;

    // lighting info
    float    famb;               ///< cached ambient light
    float    fdir;               ///< cached directional light

    float    fintens;            ///< current brightness
    float    falpha;             ///< current alpha

                                 // pre-compute some values for the reflected particle posisions
    bool ref_valid;
    Vector3f ref_up;
    Vector3f ref_right;
    Vector3f ref_pos;

    ParticleGraphics();
    void reset();
    static gfx_rv update(::Camera& camera, const ParticleRef particle, Uint8 trans, bool do_lighting);
protected:
    static gfx_rv update_vertices(ParticleGraphics& inst, ::Camera& camera, Ego::Particle *pprt);
    static Matrix4f4f make_matrix(ParticleGraphics& inst);
    static gfx_rv update_lighting(ParticleGraphics& inst, Ego::Particle *pprt, Uint8 trans, bool do_lighting);
};

} // namespace Graphics
} // namespace Ego
