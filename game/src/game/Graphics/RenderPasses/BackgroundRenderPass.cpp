#include "game/Graphics/RenderPasses/BackgroundRenderPass.hpp"
#include "game/Module/Module.hpp"
#include "game/graphic.h"

namespace Ego {
namespace Graphics {

BackgroundRenderPass::BackgroundRenderPass() :
    RenderPass("background"),
    _vertexDescriptor(VertexFormatFactory::get<VertexFormat::P3FT2F>()),
    _vertexBuffer(4, _vertexDescriptor.getVertexSize())
{}

void BackgroundRenderPass::doRun(::Camera& camera, const TileList& tl, const EntityList& el)
{
    auto& renderer = Renderer::get();
    renderer.setProjectionMatrix(camera.getProjectionMatrix());
    renderer.setWorldMatrix(Matrix4f4f::identity());
    renderer.setViewMatrix(camera.getViewMatrix());

    if (!gfx.draw_background || !_currentModule->getWater()._background_req)
    {
        return;
    }

    tile_mem_t& tmem = _currentModule->getMeshPointer()->_tmem;

    // which layer
    water_instance_layer_t *ilayer = _currentModule->getWater()._layers + 0;

    // the "official" camera height
    float z0 = 1500;

    // clip the waterlayer uv offset
    ilayer->_tx[XX] = ilayer->_tx[XX] - (float)std::floor(ilayer->_tx[XX]);
    ilayer->_tx[YY] = ilayer->_tx[YY] - (float)std::floor(ilayer->_tx[YY]);

    float xmag, Cx_0, Cx_1;
    float ymag, Cy_0, Cy_1;

    // determine the constants for the x-coordinate
    xmag = _currentModule->getWater()._backgroundrepeat / 4 / (1.0f + z0 * ilayer->_dist[XX]) / Info<float>::Grid::Size();
    Cx_0 = xmag * (1.0f + camera.getPosition()[kZ] * ilayer->_dist[XX]);
    Cx_1 = -xmag * (1.0f + (camera.getPosition()[kZ] - z0) * ilayer->_dist[XX]);

    // determine the constants for the y-coordinate
    ymag = _currentModule->getWater()._backgroundrepeat / 4 / (1.0f + z0 * ilayer->_dist[YY]) / Info<float>::Grid::Size();
    Cy_0 = ymag * (1.0f + camera.getPosition()[kZ] * ilayer->_dist[YY]);
    Cy_1 = -ymag * (1.0f + (camera.getPosition()[kZ] - z0) * ilayer->_dist[YY]);

    float Qx, Qy;

    {
        BufferScopedLock lock(_vertexBuffer);
        Vertex *vertices = lock.get<Vertex>();
        // Figure out the coordinates of its corners
        Qx = -tmem._edge_x;
        Qy = -tmem._edge_y;
        vertices[0].x = Qx;
        vertices[0].y = Qy;
        vertices[0].z = camera.getPosition()[kZ] - z0;
        vertices[0].s = Cx_0 * Qx + Cx_1 * camera.getPosition()[kX] + ilayer->_tx[XX];
        vertices[0].t = Cy_0 * Qy + Cy_1 * camera.getPosition()[kY] + ilayer->_tx[YY];

        Qx = 2 * tmem._edge_x;
        Qy = -tmem._edge_y;
        vertices[1].x = Qx;
        vertices[1].y = Qy;
        vertices[1].z = camera.getPosition()[kZ] - z0;
        vertices[1].s = Cx_0 * Qx + Cx_1 * camera.getPosition()[kX] + ilayer->_tx[XX];
        vertices[1].t = Cy_0 * Qy + Cy_1 * camera.getPosition()[kY] + ilayer->_tx[YY];

        Qx = 2 * tmem._edge_x;
        Qy = 2 * tmem._edge_y;
        vertices[2].x = Qx;
        vertices[2].y = Qy;
        vertices[2].z = camera.getPosition()[kZ] - z0;
        vertices[2].s = Cx_0 * Qx + Cx_1 * camera.getPosition()[kX] + ilayer->_tx[XX];
        vertices[2].t = Cy_0 * Qy + Cy_1 * camera.getPosition()[kY] + ilayer->_tx[YY];

        Qx = -tmem._edge_x;
        Qy = 2 * tmem._edge_y;
        vertices[3].x = Qx;
        vertices[3].y = Qy;
        vertices[3].z = camera.getPosition()[kZ] - z0;
        vertices[3].s = Cx_0 * Qx + Cx_1 * camera.getPosition()[kX] + ilayer->_tx[XX];
        vertices[3].t = Cy_0 * Qy + Cy_1 * camera.getPosition()[kY] + ilayer->_tx[YY];
    }

    float light = _currentModule->getWater()._light ? 1.0f : 0.0f;
    float alpha = ilayer->_alpha * id::fraction<float, 1, 255>();

    float intens = 1.0f;

    if (gfx.usefaredge)
    {
        intens = light_a * ilayer->_light_add;

        float fcos = light_nrm[kZ];
        if (fcos > 0.0f)
        {
            intens += fcos * fcos * light_d * ilayer->_light_dir;
        }

        intens = Math::constrain(intens, 0.0f, 1.0f);
    }

    renderer.getTextureUnit().setActivated(_currentModule->getWaterTexture(0).get());

    {
        OpenGL::PushAttrib pa(GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT);
        {
            // flat shading
            renderer.setGouraudShadingEnabled(false);

            // Do not write into the depth buffer.
            renderer.setDepthWriteEnabled(false);

            // Essentially disable the depth test without calling
            // renderer.setDepthTestEnabled(false).
            renderer.setDepthTestEnabled(true);
            renderer.setDepthFunction(CompareFunction::AlwaysPass);

            // draw draw front and back faces of polygons
            renderer.setCullingMode(id::culling_mode::none);

            if (alpha > 0.0f)
            {
                {
                    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
                    {
                        renderer.setColour(Math::Colour4f(intens, intens, intens, alpha));

                        if (alpha >= 1.0f)
                        {
                            renderer.setBlendingEnabled(false);
                        }
                        else
                        {
                            renderer.setBlendingEnabled(true);
                            renderer.setBlendFunction(id::blend_function::source_alpha, id::blend_function::one_minus_source_alpha);
                        }

                        renderer.render(_vertexBuffer, _vertexDescriptor, id::primitive_type::triangle_fan, 0, 4);
                    }
                }
            }

            if (light > 0.0f)
            {
                {
                    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
                    {
                        renderer.setBlendingEnabled(false);

                        renderer.setColour(Math::Colour4f(light, light, light, 1.0f));

                        renderer.render(_vertexBuffer, _vertexDescriptor, id::primitive_type::triangle_fan, 0, 4);
                    }
                }
            }
        }
    }
}

} // namespace Graphics
} // namespace Ego
