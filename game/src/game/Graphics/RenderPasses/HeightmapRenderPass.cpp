#include "game/Graphics/RenderPasses/HeightmapRenderPass.hpp"
#include "game/graphic.h"
#include "game/Graphics/RenderPasses.hpp"

namespace Ego {
namespace Graphics {

HeightmapRenderPass::HeightmapRenderPass() :
    RenderPass("heightmap")
{}

void HeightmapRenderPass::doRun(::Camera& camera, const TileList& tl, const EntityList& el)
{
    if (egoboo_config_t::get().debug_mesh_renderHeightMap.getValue())
    {
        // restart the mesh texture code
        TileRenderer::invalidate();

        // render the heighmap
        Ego::Graphics::Internal::TileListV2::render_heightmap(*tl.getMesh().get(), tl._all);

        // let the mesh texture code know that someone else is in control now
        TileRenderer::invalidate();
    }
}

} // namespace Graphics
} // namespace Ego
