#include "egolib/game/Graphics/RenderPasses/WaterTilesRenderPass.hpp"
#include "egolib/game/Module/Module.hpp"
#include "egolib/game/graphic.h"
#include "egolib/game/graphic_fan.h"
#include "egolib/FileFormats/Globals.hpp"
#include "egolib/Graphics/VertexFormat.hpp"

namespace Ego {
namespace Graphics {

WaterTilesRenderPass::WaterTilesRenderPass() :
    RenderPass("water tiles")
{}

void WaterTilesRenderPass::doRun(::Camera& camera, const TileList& tl, const EntityList& el)
{
    if (!tl.getMesh())
    {
        throw idlib::runtime_error(__FILE__, __LINE__, "tile list not bound to a mesh");
    }

    auto& renderer = Renderer::get();
    // Set projection matrix.
    renderer.setProjectionMatrix(camera.getProjectionMatrix());
    // Set view matrix.
    renderer.setViewMatrix(camera.getViewMatrix());
    // Set world matrix.
    renderer.setWorldMatrix(idlib::identity<Matrix4f4f>());

    // Get the mesh.
    ego_mesh_t& mesh = *tl.getMesh().get();

    // Restart the mesh texture code.
    TileRenderer::invalidate();

    // Bottom layer first.
    if (gfx.draw_water_1 && _currentModule->getWater()._layer_count > 1)
    {
        render_water(mesh, tl._water, 1);
    }

    // Top layer second.
    if (gfx.draw_water_0 && _currentModule->getWater()._layer_count > 0)
    {
        render_water(mesh, tl._water, 0);
    }

    // let the mesh texture code know that someone else is in control now
    TileRenderer::invalidate();
}

void WaterTilesRenderPass::render_water(ego_mesh_t& mesh, const std::vector<ClippingEntry>& tiles, const Uint8 layer)
{
    for (const auto& tile : tiles)
    {
        render_water_fan(mesh, tile.getIndex(), layer);
    }
}

gfx_rv WaterTilesRenderPass::render_water_fan(ego_mesh_t& mesh, const Index1D& tileIndex, const Uint8 layer)
{

    static const int ix_off[4] = {1, 1, 0, 0}, iy_off[4] = {0, 1, 1, 0};

    int    tnc;
    size_t badvertex;
    int  imap[4];
    float fx_off[4], fy_off[4];

    const MeshInfo& info = mesh._info;

    const ego_tile_info_t& ptile = mesh.getTileInfo(tileIndex);

    float falpha;
    falpha = FF_TO_FLOAT(_currentModule->getWater()._layers[layer]._alpha);
    falpha = Math::constrain(falpha, 0.0f, 1.0f);

    /// @note BB@> the water info is for TILES, not for vertices, so ignore all vertex info and just draw the water
    ///            tile where it's supposed to go
    auto i2 = Grid::map<int>(tileIndex, info.getTileCountX());

    // To make life easier
    uint16_t type = 0;                                         // Command type ( index to points in tile )
    tile_definition_t *pdef = tile_dict.get(type);
    if (NULL == pdef)
    {
        Log::Entry e(Log::Level::Error, __FILE__, __LINE__);
        e << "unknown tile type `" << type << "`" << Log::EndOfEntry;
        Log::get() << e;
        return gfx_error;
    }
    float offu = _currentModule->getWater()._layers[layer]._tx[XX];               // Texture offsets
    float offv = _currentModule->getWater()._layers[layer]._tx[YY];
    uint16_t frame = _currentModule->getWater()._layers[layer]._frame;                // Frame

    std::shared_ptr<const Texture> ptex = _currentModule->getWaterTexture(layer);

    float x1 = (float)ptex->getWidth() / (float)ptex->getSourceWidth();
    float y1 = (float)ptex->getHeight() / (float)ptex->getSourceHeight();

    for (size_t cnt = 0; cnt < 4; cnt++)
    {
        fx_off[cnt] = x1 * ix_off[cnt];
        fy_off[cnt] = y1 * iy_off[cnt];

        imap[cnt] = cnt;
    }

    // flip the coordinates around based on the "mode" of the tile
    if ((i2.x() & 1) == 0)
    {
        std::swap(imap[0], imap[3]);
        std::swap(imap[1], imap[2]);
    }

    if ((i2.y() & 1) == 0)
    {
        std::swap(imap[0], imap[1]);
        std::swap(imap[2], imap[3]);
    }

    // draw draw front and back faces of polygons
    Renderer::get().setCullingMode(idlib::culling_mode::none);

    struct Vertex
    {
        float x, y, z;
        float r, g, b, a;
        float s, t;
    };
    auto vd = descriptor_factory<idlib::vertex_format::P3FC4FT2F>()();
    auto vb = idlib::video_buffer_manager::get().create_vertex_buffer(4, vd.get_size());
    Vertex *v = static_cast<Vertex *>(vb->lock());

    // Original points
    badvertex = ptile._vrtstart;
    {
        GLXvector3f nrm = {0, 0, 1};

        float alight = get_ambient_level() + _currentModule->getWater()._layers->_light_add;
        alight = Math::constrain(alight / 255.0f, 0.0f, 1.0f);

        for (size_t cnt = 0; cnt < 4; cnt++)
        {
            Vertex& v0 = v[cnt];

            tnc = imap[cnt];

            int jx = i2.x() + ix_off[cnt];
            int jy = i2.y() + iy_off[cnt];

            v0.x = jx * Info<float>::Grid::Size();
            v0.y = jy * Info<float>::Grid::Size();
            v0.z = _currentModule->getWater()._layer_z_add[layer][frame][tnc] + _currentModule->getWater()._layers[layer]._z;

            v0.s = fx_off[cnt] + offu;
            v0.t = fy_off[cnt] + offv;

            float dlight = 0.0f;
            if (jx <= 0 || jy <= 0 || jx >= info.getTileCountX() || jy >= info.getTileCountY())
            {
                //All water is dark near edges of the map
                dlight = 0.0f;
            }
            else
            {
                //Else interpolate using ligh levels of nearby tiles
                Index1D jtile = mesh.getTileIndex(Index2D(jx, jy));
                GridIllumination::light_corner(mesh, jtile, v0.z, nrm, dlight);
            }

            // take the v[cnt].color from the tnc vertices so that it is oriented properly
            v0.r = Math::constrain(dlight * idlib::fraction<float, 1, 255>() + alight, 0.0f, 1.0f);
            v0.g = Math::constrain(dlight * idlib::fraction<float, 1, 255>() + alight, 0.0f, 1.0f);
            v0.b = Math::constrain(dlight * idlib::fraction<float, 1, 255>() + alight, 0.0f, 1.0f);

            // the application of alpha to the tile depends on the blending mode
            if (_currentModule->getWater()._light)
            {
                // blend using light
                v0.r *= falpha;
                v0.g *= falpha;
                v0.b *= falpha;
                v0.a = 1.0f;
            }
            else
            {
                // blend using alpha
                v0.a = falpha;
            }

            badvertex++;
        }
    }

    vb->unlock();

    // tell the mesh texture code that someone else is controlling the texture
    TileRenderer::invalidate();

    auto& renderer = Renderer::get();

    // set the texture
    renderer.getTextureUnit().setActivated(ptex.get());

    OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT);
    {
        bool use_depth_mask = (!_currentModule->getWater()._light && (1.0f == falpha));

        // do not draw hidden surfaces
        renderer.setDepthTestEnabled(true);
        renderer.setDepthFunction(idlib::compare_function::less_or_equal);

        // only use the depth mask if the tile is NOT transparent
        renderer.setDepthWriteEnabled(use_depth_mask);

        // cull backward facing polygons
        // use clockwise orientation to determine backfaces
        renderer.setCullingMode(idlib::culling_mode::back);
        renderer.setWindingMode(MAP_NRM_CULL);

        // set the blending mode
        renderer.setBlendingEnabled(true);
        if (_currentModule->getWater()._light)
        {
            renderer.setBlendFunction(idlib::color_blend_parameter::one, idlib::color_blend_parameter::one_minus_source0_color);
        }
        else
        {
            renderer.setBlendFunction(idlib::color_blend_parameter::source0_alpha, idlib::color_blend_parameter::one_minus_source0_alpha);
        }

        // per-vertex coloring
        renderer.setGouraudShadingEnabled(true);
        renderer.render(*vb, vd, idlib::primitive_type::triangle_fan, 0, 4);
    }

    return gfx_success;
}

} // namespace Graphics
} // namespace Ego
