#pragma once

#include "game/Graphics/RenderPass.hpp"

namespace Ego {
namespace Graphics {

/// The render pass for water tiles.
struct WaterTilesRenderPass : public RenderPass
{
public:
    WaterTilesRenderPass();
protected:
    void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;

    /// @brief Draw water fans.
    /// @param mesh the mesh
    /// @param tiles the list of tiles
    static void render_water(ego_mesh_t& mesh, const std::vector<ClippingEntry>& tiles, const Uint8 layer);

    /// @brief Draw a water fan.
    /// @param mesh the mesh
    /// @param tileIndex the tile index
    /// @param the water layer
    static gfx_rv render_water_fan(ego_mesh_t& mesh, const Index1D& tileIndex, const Uint8 layer);
};

} // namespace Graphics
} // namespace Ego
