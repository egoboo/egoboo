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

/// @file game/Graphics/BillboardSystem.hpp

#pragma once

#include "game/egoboo.h"

// Forward declarations.
class Camera;
namespace Ego { 
class Font; 
namespace Graphics {
struct Billboard;
}
}

namespace Ego {
namespace Graphics {

class BillboardSystem
{
public:
    BillboardSystem();
    virtual ~BillboardSystem();
public:
    bool render_one(Billboard& billboard, const Vector3f& cam_up, const Vector3f& cam_rgt);
    /// @brief Update all billboards in this billboard system with the time of "now".
    void update();
    void reset();
    bool hasBillboard(const Object& object) const;

private:
    // List of used billboards.
    std::list<std::shared_ptr<Billboard>> _billboardList;
    // A vertex type used by the billboard system.
    struct Vertex
    {
        float x, y, z;
        float s, t;
    };
    // A vertex desscriptor & a vertex buffer used by the billboard system.
    VertexDescriptor vertexDescriptor;
    VertexBuffer vertexBuffer;

private:

    /// @brief Create a billboard.
    /// @param lifetime_secs the lifetime of the billboard, in seconds
    /// @param texture a shared pointer to the billboard texture.
    /// Must not be a null pointer.
    /// @param tint the tint
    /// @param options the options
    /// @return the billboard
    /// @throw std::invalid_argument @a texture is a null pointer
    /// @remark The billboard is kept around as long as a reference to the
    /// billboard exists, however, it might exprire during that time.
    std::shared_ptr<Billboard> makeBillboard(::Time::Seconds lifetime_secs, std::shared_ptr<Ego::Texture> texture, const Ego::Math::Colour4f& tint, const BIT_FIELD options, const float size);

public:
    void render_all(::Camera& camera);

    std::shared_ptr<Billboard> makeBillboard(ObjectRef obj_ref, const std::string& text, const Ego::Math::Colour4f& textColor, const Ego::Math::Colour4f& tint, int lifetime_secs, const BIT_FIELD opt_bits, const float size = 0.75f);
};

} // namespace Graphics
} // namespace Ego
