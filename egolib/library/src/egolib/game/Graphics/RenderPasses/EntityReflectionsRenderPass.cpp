#include "egolib/game/Graphics/RenderPasses/EntityReflectionsRenderPass.hpp"
#include "egolib/game/Module/Module.hpp"
#include "egolib/game/graphic.h"
#include "egolib/Entities/_Include.hpp"
#include "egolib/game/graphic_fan.h"
#include "egolib/game/graphic_mad.h"
#include "egolib/game/graphic_prt.h"

namespace Ego::Graphics {

EntityReflectionsRenderPass::EntityReflectionsRenderPass() :
    RenderPass("entity reflections")
{}

void EntityReflectionsRenderPass::doRun(::Camera& camera, const TileList& tl, const EntityList& el)
{
    if (!gfx.refon)
    {
        return;
    }

    auto mesh = tl.getMesh();
    if (!mesh)
    {
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "tile list not attached to a mesh - skipping pass", Log::EndOfEntry);
        return;
    }

    OpenGL::Utilities::isError();
    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT);
    {
        auto& renderer = Renderer::get();
        // don't write into the depth buffer (disable glDepthMask for transparent objects)
        // turn off the depth mask by default. Can cause glitches if used improperly.
        renderer.setDepthWriteEnabled(false);

        // do not draw hidden surfaces
        renderer.setDepthTestEnabled(true);
        // surfaces must be closer to the camera to be drawn
        renderer.setDepthFunction(idlib::compare_function::less_or_equal);

        for (size_t j = el.getSize(); j > 0; --j)
        {
            size_t i = j - 1;
            if (ParticleRef::Invalid == el.get(i).iprt && ObjectRef::Invalid != el.get(i).iobj)
            {
                const std::shared_ptr<Object> &object = _currentModule->getObjectHandler()[el.get(i).iobj];
                if (!object || object->isTerminated())
                {
                    continue;
                }

                // cull backward facing polygons
                // use couter-clockwise orientation to determine backfaces
                renderer.setCullingMode(idlib::culling_mode::back);
                renderer.setWindingMode(MAP_REF_CULL);

                // allow transparent objects
                renderer.setBlendingEnabled(true);

                // use the alpha channel to modulate the transparency
                renderer.setBlendFunction(idlib::color_blend_parameter::source0_alpha, idlib::color_blend_parameter::one_minus_source0_alpha);
                Index1D itile = object->getTile();

                if (mesh->grid_is_valid(itile) && (0 != mesh->test_fx(itile, MAPFX_REFLECTIVE)))
                {
                    renderer.setColour(Colour4f::white());

                    ObjectGraphicsRenderer::render_ref(camera, object);
                }
            }
            else if (ObjectRef::Invalid == el.get(i).iobj && ParticleRef::Invalid != el.get(i).iprt)
            {
                // draw draw front and back faces of polygons
                renderer.setCullingMode(idlib::culling_mode::none);

                // render_one_prt_ref() actually sets its own blend function, but just to be safe
                // allow transparent objects
                renderer.setBlendingEnabled(true);
                // set the default particle blending
                renderer.setBlendFunction(idlib::color_blend_parameter::source0_alpha, idlib::color_blend_parameter::one_minus_source0_alpha);
                ParticleRef iprt = el.get(i).iprt;
                Index1D itile = ParticleHandler::get()[iprt]->getTile();

                if (mesh->grid_is_valid(itile) && (0 != mesh->test_fx(itile, MAPFX_REFLECTIVE)))
                {
                    renderer.setColour(Colour4f::white());
                    ParticleGraphicsRenderer::render_one_prt_ref(iprt);
                }
            }
        }
    }
}

} // namespace Ego::Graphics
