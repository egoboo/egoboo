#include "game/Graphics/RenderPasses/ReflectiveTilesFirstRenderPass.hpp"
#include "game/graphic.h"
#include "game/Graphics/RenderPasses.hpp"

namespace Ego {
namespace Graphics {

ReflectiveTilesFirstRenderPass::ReflectiveTilesFirstRenderPass() :
    RenderPass("reflective tiles first")
{}

void ReflectiveTilesFirstRenderPass::doRun(::Camera& camera, const TileList& tl, const EntityList& el)
{
    if (!gfx.refon)
    {
        return;
    }
    /// @details draw the reflective tiles, but turn off the depth buffer
    ///          this blanks out any background that might've been drawn

    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    {
        auto& renderer = Renderer::get();
        // DO NOT store the surface depth
        renderer.setDepthWriteEnabled(false);

        // do not draw hidden surfaces
        renderer.setDepthTestEnabled(true);
        renderer.setDepthFunction(CompareFunction::LessOrEqual);

        // black out any backgound, but allow the background to show through any holes in the floor
        renderer.setBlendingEnabled(true);
        // use the alpha channel to modulate the transparency
        renderer.setBlendFunction(BlendFunction::Zero, BlendFunction::OneMinusSourceAlpha);
        // do not display the completely transparent portion
        // use alpha test to allow the thatched roof tiles to look like thatch
        renderer.setAlphaTestEnabled(true);
        // speed-up drawing of surfaces with alpha == 0.0f sections
        renderer.setAlphaFunction(CompareFunction::Greater, 0.0f);
        // reduce texture hashing by loading up each texture only once
        Internal::TileListV2::render(*tl.getMesh(), tl._reflective);
    }
}

} // namespace Graphics
} // namespace Ego
