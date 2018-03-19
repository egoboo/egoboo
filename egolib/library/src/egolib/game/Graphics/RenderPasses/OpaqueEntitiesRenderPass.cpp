#include "egolib/game/Graphics/RenderPasses/OpaqueEntitiesRenderPass.hpp"
#include "egolib/game/Module/Module.hpp"
#include "egolib/game/graphic_mad.h"
#include "egolib/game/graphic_prt.h"
#include "egolib/Entities/_Include.hpp"

namespace Ego::Graphics {

OpaqueEntitiesRenderPass::OpaqueEntitiesRenderPass() :
    RenderPass("opaque entities")
{}

void OpaqueEntitiesRenderPass::doRun(::Camera& camera, const TileList& tl, const EntityList& el)
{
    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    {
        // scan for solid objects
        for (size_t i = 0, n = el.getSize(); i < n; ++i)
        {
            auto& renderer = Renderer::get();
            // solid objects draw into the depth buffer for hidden surface removal
            renderer.setDepthWriteEnabled(true);

            // do not draw hidden surfaces
            renderer.setDepthTestEnabled(true);
            renderer.setDepthFunction(idlib::compare_function::less);

            renderer.setAlphaTestEnabled(true);
            renderer.setAlphaFunction(idlib::compare_function::greater, 0.0f);

            if (ParticleRef::Invalid == el.get(i).iprt && ObjectRef::Invalid != el.get(i).iobj)
            {
                ObjectGraphicsRenderer::render_solid(camera, _currentModule->getObjectHandler()[el.get(i).iobj]);
            }
            else if (ObjectRef::Invalid == el.get(i).iobj && ParticleHandler::get()[el.get(i).iprt] != nullptr)
            {
                // draw draw front and back faces of polygons
                renderer.setCullingMode(idlib::culling_mode::none);

                ParticleGraphicsRenderer::render_one_prt_solid(el.get(i).iprt);
            }
        }
    }
}

} // namespace Ego::Graphics
