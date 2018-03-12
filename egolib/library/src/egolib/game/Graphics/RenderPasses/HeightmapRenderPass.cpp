#include "egolib/game/Graphics/RenderPasses/HeightmapRenderPass.hpp"
#include "egolib/game/graphic.h"
#include "egolib/game/Graphics/RenderPasses.hpp"

namespace Ego::Graphics {

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
        Graphics::Internal::TileListV2::render_heightmap(*tl.getMesh().get(), tl._all);

        // let the mesh texture code know that someone else is in control now
        TileRenderer::invalidate();
    }
}

} // namespace Ego::Graphics
