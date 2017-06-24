//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file game/graphic.h

#pragma once

#include "egolib/Graphics/MD2Model.hpp"
#include "game/lighting.h"
#include "egolib/Extensions/ogl_extensions.h"

// Forward declaration.
class Camera;
class ego_mesh_t;
class ego_tile_info_t;
namespace Ego {
namespace Graphics {
class BillboardSystem;
class Md2ModelRenderer;
struct RenderPass;
struct TileList;
struct EntityList;
} }

void  draw_hud();

//--------------------------------------------------------------------------------------------

/// the default icon size in pixels
#define ICON_SIZE 32


/// max number of blips on the minimap
#define MAXBLIP        128                          ///<Max blips on the screen

//--------------------------------------------------------------------------------------------
#define TABX                            32// 16      ///< Size of little name tag on the bar
#define BARX                            112// 216         ///< Size of bar
#define BARY                            16// 8
#define NUMTICK                         10// 50          ///< Number of ticks per row
#define MAXTICK                         (NUMTICK*10) ///< Max number of ticks to draw
#define XPTICK                          6.00f

#define NUMBAR                          6               ///< Number of status bars

#define MAXLIGHTLEVEL                   16          ///< Number of premade light intensities
#define MAXSPEKLEVEL                    16          ///< Number of premade specularities
#define MAXLIGHTROTATION                256         ///< Number of premade light maps

#define DONTFLASH                       255
#define SEEKURSEAND                     31          ///< Blacking flash

#define SHADOWRAISE                       5

/// The supported colors of bars and blips
enum HUDColors : uint8_t
{
    COLOR_WHITE = 0,
    COLOR_RED,
    COLOR_YELLOW,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_PURPLE,
    COLOR_MAX
};

namespace Ego 
{
namespace Graphics
{
    // The map file only supports 256 tile textures:
    // Four texture atlases each with 8 x 8 textures hence 4 * (8 * 8) = 256 different tile textures.
    static constexpr size_t MESH_IMG_COUNT = 256;
}
}

//--------------------------------------------------------------------------------------------
// encapsulation of all graphics options
struct gfx_config_t
{
    bool gouraudShading_enable;
    bool refon;
    bool antialiasing;
    bool dither;
    bool perspective;
    bool phongon;
    bool shadows_enable;
    bool shadows_highQuality_enable;

    bool draw_background;   ///< Do we draw the background image?
    bool draw_overlay;      ///< Draw overlay?
    bool draw_water_0;      ///< Do we draw water layer 1 (TX_WATER_LOW)
    bool draw_water_1;      ///< Do we draw water layer 2 (TX_WATER_TOP)

    size_t dynalist_max;     ///< Max number of dynamic lights to draw
    bool exploremode;       ///< fog of war mode for mesh display
    bool usefaredge;        ///< Far edge maps? (Outdoor)

    static void init(gfx_config_t& self);
    static void download(gfx_config_t& self, egoboo_config_t& cfg);
};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern gfx_config_t gfx;

extern float           indextoenvirox[MD2Model::normalCount];                    ///< Environment map

//--------------------------------------------------------------------------------------------
/// The active dynamic lights
struct dynalist_t
{
    int frame; ///< The last frame in shich the list was updated. @a -1 if there was no update yet.
    size_t size; ///< The size of the list.
    dynalight_data_t lst[TOTAL_MAX_DYNA];  ///< The list.
    dynalist_t();
    static void init(dynalist_t& self);
};
//--------------------------------------------------------------------------------------------
// Function prototypes

struct GameAppImpl
{
private:
    dynalist_t dynalist;
    std::unique_ptr<Ego::Graphics::BillboardSystem> billboardSystem;
    std::unique_ptr<Ego::Graphics::Md2ModelRenderer> md2ModelRenderer;
public:
    GameAppImpl();
    ~GameAppImpl();
    dynalist_t& getDynalist();
    Ego::Graphics::BillboardSystem& getBillboardSystem() const;
    Ego::Graphics::Md2ModelRenderer& getMd2ModelRenderer() const;
};

template <typename T>
struct GameApp : public Ego::App<T>
{
private:
    std::unique_ptr<GameAppImpl> impl;
protected:
    GameApp(const std::string& title, const std::string& version) : Ego::App<T>(title, version),
        impl(std::make_unique<GameAppImpl>())
    {}
    virtual ~GameApp()
    {}
public:
    dynalist_t& getDynalist()
    {
        return impl->getDynalist();
    }
    Ego::Graphics::BillboardSystem& getBillboardSystem() const
    {
        return impl->getBillboardSystem();
    }
    Ego::Graphics::Md2ModelRenderer& getMd2ModelRenderer() const
    {
        return impl->getMd2ModelRenderer();
    }
};

struct GFX : public GameApp<GFX>
{
private:
    friend Ego::Core::Singleton<GFX>::CreateFunctorType;
    friend Ego::Core::Singleton<GFX>::DestroyFunctorType;
    GFX();
    ~GFX();

private:
    std::unique_ptr<Ego::Graphics::RenderPass> nonOpaqueEntities;
    std::unique_ptr<Ego::Graphics::RenderPass> opaqueEntities;
    std::unique_ptr<Ego::Graphics::RenderPass> reflective0;
    std::unique_ptr<Ego::Graphics::RenderPass> reflective1;
    std::unique_ptr<Ego::Graphics::RenderPass> nonReflective;
    std::unique_ptr<Ego::Graphics::RenderPass> entityShadows;
    std::unique_ptr<Ego::Graphics::RenderPass> water;
    std::unique_ptr<Ego::Graphics::RenderPass> entityReflections;
    std::unique_ptr<Ego::Graphics::RenderPass> foreground;
    std::unique_ptr<Ego::Graphics::RenderPass> background;
    std::unique_ptr<Ego::Graphics::RenderPass> motionBlur;
    std::unique_ptr<Ego::Graphics::RenderPass> heightmap;

public:
    Ego::Time::Clock<Ego::Time::ClockPolicy::NonRecursive> update_object_instances_timer;
    gfx_rv update_object_instances(Camera& cam);
    Ego::Time::Clock<Ego::Time::ClockPolicy::NonRecursive> update_particle_instances_timer;
    gfx_rv update_particle_instances(Camera& cam);

public:
    Ego::Graphics::RenderPass& getNonOpaqueEntities() const
    {
        return *nonOpaqueEntities;
    }

    Ego::Graphics::RenderPass& getOpaqueEntities() const
    {
        return *opaqueEntities;
    }

    Ego::Graphics::RenderPass& getReflective0() const
    {
        return *reflective0;
    }

    Ego::Graphics::RenderPass& getReflective1() const
    {
        return *reflective1;
    }

    Ego::Graphics::RenderPass& getNonReflective() const
    {
        return *nonReflective;
    }

    Ego::Graphics::RenderPass& getEntityShadows() const
    {
        return *entityShadows;
    }

    Ego::Graphics::RenderPass& getWater() const
    {
        return *water;
    }

    Ego::Graphics::RenderPass& getEntityReflections() const
    {
        return *entityReflections;
    }

    Ego::Graphics::RenderPass& getForeground() const
    {
        return *foreground;
    }

    Ego::Graphics::RenderPass& getBackground() const
    {
        return *background;
    }

    Ego::Graphics::RenderPass& getMotionBlur() const
    {
        return *motionBlur;
    }

    Ego::Graphics::RenderPass& getHeightmap() const
    {
        return *heightmap;
    }
};

/// SDL destroys the OpenGL context at various occassions (e.g. when changing the video mode).
/// In particular this happens when the video mode is changed from and to fullscreen. In that
/// case, OpenGL texture become invalid and need to be reloaded. Reloading means here: From
/// the stored state of the texture and its surface, the backing OpenGL texture needs to be
/// reconstructed.
void gfx_system_reload_all_textures();
void gfx_system_make_enviro();
void gfx_system_init_all_graphics();
void gfx_system_release_all_graphics();
void gfx_system_load_assets();

// the render engine callback
void gfx_system_render_world(const std::shared_ptr<Camera> camera, std::shared_ptr<Ego::Graphics::TileList> tl, std::shared_ptr<Ego::Graphics::EntityList> el);

void gfx_do_clear_screen();
void gfx_do_flip_pages();

float draw_icon_texture(const std::shared_ptr<const Ego::Texture>& ptex, float x, float y, uint8_t sparkle_color, uint32_t sparkle_timer, float size, bool useAlpha = false);
float draw_game_icon(const std::shared_ptr<const Ego::Texture>& icontype, float x, float y, uint8_t sparkle, uint32_t delta_update, float size);
void draw_blip(float sizeFactor, uint8_t color, float x, float y);
void draw_mouse_cursor();
void draw_passages(Camera& cam);


/// Structure for keeping track of which dynalights are visible
struct dynalight_registry_t {
    int reference;
    ego_frect_t bound;
};

/// Illuminate the "grid".
struct GridIllumination {
private:
    static float grid_get_mix(float u0, float u, float v0, float v);
    static float ego_mesh_interpolate_vertex(const ego_tile_info_t& info, const GLXvector3f& position);
	static void test_one_corner(const ego_mesh_t& mesh, GLXvector3f pos, float& pdelta);
	static bool test_corners(const ego_mesh_t& mesh, ego_tile_info_t& tile, float threshold);
	static void light_one_corner(ego_mesh_t& mesh, ego_tile_info_t& tile, const bool reflective, const Vector3f& pos, const Vector3f& nrm, float& plight);
	static float grid_lighting_test(const ego_mesh_t& mesh, GLXvector3f pos, float& low_diff, float& hgh_diff);
	static void light_fans_update_clst(Ego::Graphics::TileList& tl);
	static gfx_rv light_fans_throttle_update(ego_mesh_t * mesh, ego_tile_info_t& tile, const Index1D& tileIndex, float threshold);
	static void light_fans_update_lcache(Ego::Graphics::TileList& tl);
public:
	static gfx_rv do_grid_lighting(Ego::Graphics::TileList& tl, dynalist_t& dyl, Camera& cam);
	static void light_fans(Ego::Graphics::TileList& tl);
	static float light_corners(ego_mesh_t& mesh, ego_tile_info_t& tile, bool reflective, float mesh_lighting_keep);
	static bool grid_lighting_interpolate(const ego_mesh_t& mesh, lighting_cache_t& dst, const Vector2f& pos);
	static bool light_corner(ego_mesh_t& mesh, const Index1D& fan, float height, float nrm[], float& plight);
};


float  get_ambient_level();

/// An ineffective and obtrusive caching mechanism to avoid texture unit state changes.
struct TileRenderer {
private:
    /** @{ @brief Variables to optimize calls to bind the textures. */
    // variables to optimize calls to bind the textures
    /** @brief Disable texturing completely? */
    static bool disableTexturing;
    /** @brief The last texture used. */
    static TX_REF image;
    /** @brief The size of the last texture used. */
    static uint8_t size;
    /**@}*/
    static std::shared_ptr<Ego::Texture> get_texture(uint8_t image, uint8_t size);
public:
    /// Invalidate the cache: Must be inovked if the texture unit state changes from outside of the tile renderer.
    static void invalidate();
    /// Bind the texture of the tile to the texture unit.
    static void bind(const ego_tile_info_t& tile);
};
