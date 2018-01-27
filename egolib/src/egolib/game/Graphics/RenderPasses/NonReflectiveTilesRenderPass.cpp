#include "egolib/game/Graphics/RenderPasses/NonReflectiveTilesRenderPass.hpp"
#include "egolib/game/Graphics/RenderPasses.hpp"
#include "egolib/FileFormats/Globals.hpp"
#include "egolib/game/graphic.h"

namespace Ego {
namespace Graphics {

NonReflectiveTilesRenderPass::NonReflectiveTilesRenderPass() :
    RenderPass("non reflective tiles render pass")
{}

void NonReflectiveTilesRenderPass::doRun(::Camera& camera, const TileList& tl, const EntityList& el)
{
    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    {
        auto& renderer = Renderer::get();
        // draw draw front and back faces of polygons
        renderer.setCullingMode(id::culling_mode::none);
        // Write to the depth buffer.
        renderer.setDepthWriteEnabled(true);
        // Do not draw hidden surfaces.
        renderer.setDepthTestEnabled(true);
        renderer.setDepthFunction(id::compare_function::less_or_equal);

        // Disable blending.
        renderer.setBlendingEnabled(false);
        // Do not display the completely transparent portion:
        // Use alpha test to allow the thatched roof tiles to look like thatch.
        renderer.setAlphaTestEnabled(true);
        // Speed-up drawing of surfaces with alpha == 0.0f sections.
        renderer.setAlphaFunction(id::compare_function::greater, 0.0f);

        // reduce texture hashing by loading up each texture only once
        Internal::TileListV2::render(*tl.getMesh(), tl._nonReflective);
    }
    OpenGL::Utilities::isError();
}

} // namespace Graphics
} // namespace Ego
