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

/// @file game/Graphics/RenderPasses.hpp
/// @brief Implementation of Egoboo's render passes
/// @author Michael Heilmann

#pragma once

#include "game/Graphics/RenderPass.hpp"
#include "game/Graphics/Vertex.hpp"

namespace Ego {
namespace Graphics {

namespace Internal {

struct ElementV2 {
private:
    float distance;
    Index1D tileIndex;
    uint32_t textureIndex;
public:
    ElementV2();
    ElementV2(float distance, const Index1D& tileIndex, uint32_t textureIndex);
    ElementV2(const ElementV2& other);
    const ElementV2& operator=(const ElementV2& other);
public:
    float getDistance() const;
    const Index1D& getTileIndex() const;
    uint32_t getTextureIndex() const;
public:
    static bool compare(const ElementV2& x, const ElementV2& y);
};

struct TileListV2 {
public:
    /// @brief Draw fans.
    /// @param mesh the mesh
    /// @param tiles the list of tiles
    static void render(ego_mesh_t& mesh, const std::vector<ClippingEntry>& tiles);

    /// @brief Draw heightmap fans.
    /// @param mesh the mesh
    /// @param tiles the list of tiles
    static void render_heightmap(ego_mesh_t& mesh, const std::vector<ClippingEntry>& tiles);

private:
    /// @brief Draw a fan.
    /// @param mesh the mesh
    /// @param tileIndex the tile index
    static gfx_rv render_fan(ego_mesh_t& mesh, const Index1D& tileIndex);

    /// @brief Draw a heightmap fan.
    /// @param mesh the mesh
    /// @param tileIndex the tile index
    static gfx_rv render_heightmap_fan(ego_mesh_t& mesh, const Index1D& tileIndex);

};

}









}
}
