#include "egolib/game/Graphics/RenderPasses/ForegroundRenderPass.hpp"
#include "egolib/game/Module/Module.hpp"
#include "egolib/game/graphic.h"
#include "egolib/Graphics/VertexFormat.hpp"

namespace Ego {
namespace Graphics {

ForegroundRenderPass::ForegroundRenderPass() :
    RenderPass("foreground"),
    _vertexDescriptor(descriptor_factory<idlib::vertex_format::P3FT2F>()()),
    _vertexBuffer(idlib::video_buffer_manager::get().create_vertex_buffer(4, _vertexDescriptor.get_size()))
{}

void ForegroundRenderPass::doRun(::Camera& camera, const TileList& tl, const EntityList& el)
{
    if (!gfx.draw_overlay || !_currentModule->getWater()._background_req)
    {
        return;
    }

    auto& renderer = Renderer::get();
    renderer.setProjectionMatrix(camera.getProjectionMatrix());
    renderer.setWorldMatrix(idlib::identity<Matrix4f4f>());
    renderer.setViewMatrix(camera.getViewMatrix());

    water_instance_layer_t *ilayer = _currentModule->getWater()._layers + 1;

    Vector3f vforw_wind(ilayer->_tx_add[XX], ilayer->_tx_add[YY], 0.0f);
    vforw_wind = normalize(vforw_wind).get_vector();

    Vector3f vforw_cam = mat_getCamForward(camera.getViewMatrix());
    vforw_cam = normalize(vforw_cam).get_vector();

    // make the texture begin to disappear if you are not looking straight down
    float ftmp = dot(vforw_wind, vforw_cam);

    float alpha = (1.0f - ftmp * ftmp) * (ilayer->_alpha * idlib::fraction<float, 1, 255>());

    if (alpha != 0.0f)
    {
        // Figure out the screen coordinates of its corners
        auto windowSize = GraphicsSystem::get().window->size();
		static const float p = std::pow(2, 6);
        float x = windowSize.x() * p;
        float y = windowSize.y() * p;
        float z = 0;
        float size = x + y + 1;
        static const Facing default_turn = Facing((3 * 2047) << 2);
        float sinsize = std::sin(default_turn) * size;
        float cossize = std::cos(default_turn) * size;
        // TODO: Shouldn't this be std::min(x / windowSize.width(), y / windowSize.height())?
        float loc_foregroundrepeat = _currentModule->getWater()._foregroundrepeat *
            std::min(x / windowSize.x(), y / windowSize.y());

        {
            idlib::buffer_scoped_lock lock(*_vertexBuffer);
            Vertex *vertices = lock.get<Vertex>();

            vertices[0].x = x + cossize;
            vertices[0].y = y - sinsize;
            vertices[0].z = z;
            vertices[0].s = ilayer->_tx[XX];
            vertices[0].t = ilayer->_tx[YY];

            vertices[1].x = x + sinsize;
            vertices[1].y = y + cossize;
            vertices[1].z = z;
            vertices[1].s = ilayer->_tx[XX] + loc_foregroundrepeat;
            vertices[1].t = ilayer->_tx[YY];

            vertices[2].x = x - cossize;
            vertices[2].y = y + sinsize;
            vertices[2].z = z;
            vertices[2].s = ilayer->_tx[SS] + loc_foregroundrepeat;
            vertices[2].t = ilayer->_tx[TT] + loc_foregroundrepeat;

            vertices[3].x = x - sinsize;
            vertices[3].y = y - cossize;
            vertices[3].z = z;
            vertices[3].s = ilayer->_tx[SS];
            vertices[3].t = ilayer->_tx[TT] + loc_foregroundrepeat;
        }

        renderer.getTextureUnit().setActivated(_currentModule->getWaterTexture(1).get());

        {
            OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT | GL_HINT_BIT);
            {
                // make sure that the texture is as smooth as possible
                GL_DEBUG(glHint)(GL_POLYGON_SMOOTH_HINT, GL_NICEST);          // GL_HINT_BIT

                                                                              // flat shading
                renderer.setGouraudShadingEnabled(false);                     // GL_LIGHTING_BIT

                                                                              // Do not write into the depth buffer.
                renderer.setDepthWriteEnabled(false);

                // Essentially disable the depth test without calling
                // renderer.setDepthTestEnabled(false).
                renderer.setDepthTestEnabled(true);
                renderer.setDepthFunction(idlib::compare_function::always_pass);

                // draw draw front and back faces of polygons
                renderer.setCullingMode(idlib::culling_mode::none);

                // do not display the completely transparent portion
                renderer.setAlphaTestEnabled(true);
                renderer.setAlphaFunction(idlib::compare_function::greater, 0.0f);

                // make the texture a filter
                renderer.setBlendingEnabled(true);
                renderer.setBlendFunction(idlib::color_blend_parameter::source0_alpha, idlib::color_blend_parameter::one_minus_source0_color);

                renderer.getTextureUnit().setActivated(_currentModule->getWaterTexture(1).get());

                renderer.setColour(Colour4f(1.0f, 1.0f, 1.0f, 1.0f - std::abs(alpha)));
                renderer.render(*_vertexBuffer, _vertexDescriptor, idlib::primitive_type::triangle_fan, 0, 4);
            }
        }
    }
}

} // namespace Graphics
} // namespace Ego
