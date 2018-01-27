#include "egolib/game/Graphics/RenderPasses/ReflectiveTilesSecondRenderPass.hpp"
#include "egolib/game/graphic.h"
#include "egolib/game/Graphics/RenderPasses.hpp"

namespace Ego {
namespace Graphics {

ReflectiveTilesSecondRenderPass::ReflectiveTilesSecondRenderPass() :
    RenderPass("reflective tiles second")
{}

void ReflectiveTilesSecondRenderPass::doRun(::Camera& camera, const TileList& tl, const EntityList& el)
{
    auto& renderer = Renderer::get();
    // Set projection matrix.
    renderer.setProjectionMatrix(camera.getProjectionMatrix());
    // Set view matrix.
    renderer.setViewMatrix(camera.getViewMatrix());
    // Set world matrix.
    renderer.setWorldMatrix(Matrix4f4f::identity());
    // Disable culling.
    renderer.setCullingMode(idlib::culling_mode::none);
    // Perform less-or-equal depth testing.
    renderer.setDepthTestEnabled(true);
    renderer.setDepthFunction(idlib::compare_function::less_or_equal);
    // Write to depth buffer.
    renderer.setDepthWriteEnabled(true);
    if (gfx.refon)
    {
        doReflectionsEnabled(camera, tl, el);
    }
    else
    {
        doReflectionsDisabled(camera, tl, el);
    }
}

void ReflectiveTilesSecondRenderPass::doReflectionsEnabled(::Camera& camera, const TileList& tl, const EntityList& el)
{
    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    {
        // Enable blending.
        auto& renderer = Renderer::get();
        // Enable blending.
        renderer.setBlendingEnabled(true);
        renderer.setBlendFunction(idlib::color_blend_parameter::source0_alpha, idlib::color_blend_parameter::one);

        // reduce texture hashing by loading up each texture only once
        Internal::TileListV2::render(*tl.getMesh(), tl._reflective);
    }
}

void ReflectiveTilesSecondRenderPass::doReflectionsDisabled(::Camera& camera, const TileList& tl, const EntityList& el)
{
    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    {
        auto& renderer = Renderer::get();
        // Disable blending.
        renderer.setBlendingEnabled(false);
        // Do not display the completely transparent portion:
        // Use alpha test to allow the thatched roof tiles to look like thatch.
        renderer.setAlphaTestEnabled(true);
        // Speed-up drawing of surfaces with alpha = 0.0f sections
        renderer.setAlphaFunction(idlib::compare_function::greater, 0.0f);

        // reduce texture hashing by loading up each texture only once
        Internal::TileListV2::render(*tl.getMesh(), tl._reflective);
    }
}

} // namespace Graphics
} // namespace Ego
