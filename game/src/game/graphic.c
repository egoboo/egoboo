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

/// @file game/graphic.c
/// @brief Simple Egoboo renderer
/// @details All sorts of stuff related to drawing the game

#include "game/Core/GameEngine.hpp"
#include "game/GUI/MiniMap.hpp"
#include "egolib/egolib.h"
#include "egolib/bsp.h"
#include "game/graphic.h"
#include "game/graphic_prt.h"
#include "game/graphic_mad.h"
#include "game/graphic_fan.h"
#include "game/graphic_billboard.h"
#include "game/renderer_2d.h"
#include "game/renderer_3d.h"
#include "egolib/Graphics/mad.h"
#include "game/bsp.h"
#include "game/player.h"
#include "game/collision.h"
#include "egolib/Script/script.h"
#include "game/input.h"
#include "game/script_compile.h"
#include "game/game.h"
#include "game/lighting.h"
#include "game/egoboo.h"
#include "game/char.h"
#include "game/mesh.h"
#include "game/Graphics/CameraSystem.hpp"
#include "game/Module/Module.hpp"
#include "game/Entities/_Include.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define SPARKLE_SIZE ICON_SIZE
#define SPARKLE_AND  (SPARKLE_SIZE - 1)

#define BLIPSIZE 6

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Structure for keeping track of which dynalights are visible
struct dynalight_registry_t
{
    int         reference;
    ego_frect_t bound;
};

//--------------------------------------------------------------------------------------------
// dynalist
//--------------------------------------------------------------------------------------------

/// The active dynamic lights
struct dynalist_t
{
    int frame; ///< The last frame in shich the list was updated. @a -1 if there was no update yet.
    size_t size; ///< The size of the list.
    dynalight_data_t lst[TOTAL_MAX_DYNA];  ///< The list.
};

static gfx_rv dynalist_init(dynalist_t *self);
static gfx_rv do_grid_lighting(Ego::Graphics::TileList& tl, dynalist_t& dyl, Camera& cam);

#define DYNALIST_INIT { -1 /* frame */, 0 /* count */ }

//--------------------------------------------------------------------------------------------

static gfx_rv light_fans(Ego::Graphics::TileList& tl);


//--------------------------------------------------------------------------------------------

/// @todo All this crap can be implemented using a single clock with a window size of 1 and a histogram.

using namespace Ego::Time;

/// Profiling timer for sorting the dolist(s) for unreflected rendering.
Clock<ClockPolicy::NonRecursive> sortDoListUnreflected_timer("render.sortDoListUnreflected", 512);
/// Profiling timer for sorting the dolist(s) for reflected rendering.
Clock<ClockPolicy::NonRecursive> sortDoListReflected_timer("render.sortDoListReflected", 512);

Clock<ClockPolicy::NonRecursive>  render_scene_init_timer("render.scene.init", 512);
Clock<ClockPolicy::NonRecursive>  render_scene_mesh_timer("render.scene.mesh", 512);

Clock<ClockPolicy::NonRecursive>  gfx_make_tileList_timer("gfx.make.tileList", 512);
Clock<ClockPolicy::NonRecursive>  gfx_make_entityList_timer("gfx.make.entityList", 512);
Clock<ClockPolicy::NonRecursive>  do_grid_lighting_timer("do.grid.lighting", 512);
Clock<ClockPolicy::NonRecursive>  light_fans_timer("light.fans", 512);
Clock<ClockPolicy::NonRecursive>  gfx_update_all_chr_instance_timer("gfx.update.all.chr.instance", 512);
Clock<ClockPolicy::NonRecursive>  update_all_prt_instance_timer("update.all.prt.instance", 512);


Uint32          game_frame_all = 0;             ///< The total number of frames drawn so far
Uint32          menu_frame_all = 0;             ///< The total number of frames drawn so far

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int GFX_WIDTH = 800;
int GFX_HEIGHT = 600;

gfx_config_t     gfx;

float            indextoenvirox[EGO_NORMAL_COUNT];
float            lighttoenviroy[256];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static gfx_error_stack_t gfx_error_stack = GFX_ERROR_STACK_INIT;

static SDLX_video_parameters_t sdl_vparam;
static oglx_video_parameters_t ogl_vparam;

static bool _sdl_initialized_graphics = false;
static bool _ogl_initialized = false;

//static float sinlut[MAXLIGHTROTATION];
//static float coslut[MAXLIGHTROTATION];

// Interface stuff
static irect_t tabrect[NUMBAR];            // The tab rectangles
static irect_t barrect[NUMBAR];            // The bar rectangles
static irect_t bliprect[COLOR_MAX];        // The blip rectangles

static bool  gfx_page_flip_requested = false;
static bool  gfx_page_clear_requested = true;

const static float DYNALIGHT_KEEP = 0.9f;

egolib_timer_t gfx_update_timer;

static dynalist_t _dynalist = DYNALIST_INIT;

renderlist_mgr_t *renderlist_mgr_t::_singleton = nullptr;
dolist_mgr_t *dolist_mgr_t::_singleton = nullptr;

/// @todo Rename to _tileList_colst.
static Ego::DynamicArray<BSP_leaf_t *> _renderlist_colst = DYNAMIC_ARY_INIT_VALS;
/// @todo Rename to _entityList colst.
static Ego::DynamicArray<BSP_leaf_t *> _dolist_colst = DYNAMIC_ARY_INIT_VALS;

//--------------------------------------------------------------------------------------------


void reinitClocks() {
	sortDoListUnreflected_timer.reinit();
	sortDoListReflected_timer.reinit();

	render_scene_init_timer.reinit();
	render_scene_mesh_timer.reinit();

	gfx_make_tileList_timer.reinit();
	gfx_make_entityList_timer.reinit();
	do_grid_lighting_timer.reinit();
	light_fans_timer.reinit();
	gfx_update_all_chr_instance_timer.reinit();
	update_all_prt_instance_timer.reinit();

	Ego::Graphics::RenderPasses::g_entityReflections._clock.reinit();
	Ego::Graphics::RenderPasses::g_entityShadows._clock.reinit();
	Ego::Graphics::RenderPasses::g_solidEntities._clock.reinit();
	Ego::Graphics::RenderPasses::g_transparentEntities._clock.reinit();
	Ego::Graphics::RenderPasses::g_water._clock.reinit();
	Ego::Graphics::RenderPasses::g_reflective0._clock.reinit();
	Ego::Graphics::RenderPasses::g_reflective1._clock.reinit();
	Ego::Graphics::RenderPasses::g_nonReflective._clock.reinit();
	Ego::Graphics::RenderPasses::g_foreground._clock.reinit();
	Ego::Graphics::RenderPasses::g_background._clock.reinit();
}

static void _flip_pages();

static gfx_rv render_scene_init(Ego::Graphics::TileList& tl, Ego::Graphics::EntityList& el, dynalist_t& dyl, Camera& cam);
static gfx_rv render_scene_mesh(Camera& cam, const Ego::Graphics::TileList& tl, const Ego::Graphics::EntityList& el);
static gfx_rv render_scene(Camera& cam, Ego::Graphics::TileList& tl, Ego::Graphics::EntityList& el);
static gfx_rv render_scene(Camera& cam, std::shared_ptr<Ego::Graphics::TileList> tl, std::shared_ptr<Ego::Graphics::EntityList> pel);

/**
 * @brief
 *  Find characters that need to be drawn and put them in the list.
 * @param dolist
 *  the list to add characters to
 * @param camera
 *	the camera
 */
static gfx_rv gfx_make_entityList(Ego::Graphics::EntityList& el, Camera& camera);
static gfx_rv gfx_make_tileList(Ego::Graphics::TileList& tl, Camera& camera);
static gfx_rv gfx_make_dynalist(dynalist_t& dyl, Camera& camera);

static float draw_one_xp_bar(float x, float y, Uint8 ticks);
static float draw_fps(float y);
static float draw_help(float y);
static float draw_debug(float y);
static float draw_timer(float y);
static float draw_game_status(float y);
static void  draw_hud();
//static void  draw_inventory();

static gfx_rv update_one_chr_instance(Object * pchr);
static gfx_rv gfx_update_all_chr_instance();
static gfx_rv gfx_update_flashing(Ego::Graphics::EntityList& el);

static gfx_rv light_fans_throttle_update(ego_mesh_t * mesh, ego_tile_info_t * ptile, int fan, float threshold);
static gfx_rv light_fans_update_lcache(Ego::Graphics::TileList& tl);
static gfx_rv light_fans_update_clst(Ego::Graphics::TileList& tl);
static bool sum_global_lighting(std::array<float, LIGHTING_VEC_SIZE> &lighting);
//static float calc_light_rotation(int rotation, int normal);
//static float calc_light_global(int rotation, int normal, float lx, float ly, float lz);

//static void gfx_init_icon_data();
static void   gfx_init_bar_data();
static void   gfx_init_blip_data();

//--------------------------------------------------------------------------------------------
// renderlist_ary implementation
//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
// renderlist manager implementation
//--------------------------------------------------------------------------------------------

renderlist_mgr_t::renderlist_mgr_t() :
    Pool<Ego::Graphics::TileList, MAX_CAMERAS>()
{}

renderlist_mgr_t::~renderlist_mgr_t()
{}

void renderlist_mgr_t::initialize()
{
    if (_singleton)
    {
        return;
    }
    _singleton = new renderlist_mgr_t();
}

void renderlist_mgr_t::uninitialize()
{
    if (!_singleton)
    {
        return;
    }
    delete _singleton;
    _singleton = nullptr;
}

renderlist_mgr_t& renderlist_mgr_t::get()
{
    if (!_singleton)
    {
        throw std::logic_error("renderlist manager is not initialized");
    }
    return *_singleton;
}

//--------------------------------------------------------------------------------------------
// dolist implementation
//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
// dolist manager implementation
//--------------------------------------------------------------------------------------------

dolist_mgr_t::dolist_mgr_t() :
    Pool<Ego::Graphics::EntityList, MAX_CAMERAS>()
{}

dolist_mgr_t::~dolist_mgr_t()
{}

void dolist_mgr_t::initialize()
{
    if (_singleton)
    {
        return;
    }
    _singleton = new dolist_mgr_t();
}

void dolist_mgr_t::uninitialize()
{
    if (!_singleton)
    {
        return;
    }
    delete _singleton;
    _singleton = nullptr;
}

dolist_mgr_t& dolist_mgr_t::get()
{
    if (!_singleton)
    {
        throw std::logic_error("dolist manager is not initialized");
    }
    return *_singleton;
}

//--------------------------------------------------------------------------------------------
// GFX implementation
//--------------------------------------------------------------------------------------------
void GFX::initialize()
{
    // Initialize SDL and initialize OpenGL.
    GFX::initializeSDLGraphics(); ///< @todo Error handling.
    GFX::initializeOpenGL();      ///< @todo Error handling.
    Ego::FontManager::initialize();

    // Initialize the dolist manager.
    dolist_mgr_t::initialize(); ///< @todo Error handling.

    // Initialize the renderlist manager.
    renderlist_mgr_t::initialize(); ///< @todo Error handling.

    // initialize the dynalist frame
    // otherwise, it will not update until the frame count reaches whatever
    // left over or random value is in this counter
    _dynalist.frame = -1;
    _dynalist.size = 0;

    // begin the billboard system
    BillboardSystem::initialize();



    // Initialize the texture atlas manager.
    TextureAtlasManager::initialize();

    // initialize the profiling variables.
    gfx_clear_loops = 0;

    // allocate the specailized "collision lists"
    if (!_dolist_colst.ctor(Ego::Graphics::EntityList::CAPACITY))
    {
        log_error("%s-%s-%d - Could not allocate entity collision list", __FILE__, __FUNCTION__, __LINE__);
    }

    if (!_renderlist_colst.ctor(Ego::Graphics::renderlist_lst_t::CAPACITY))
    {
        log_error("%s-%s-%d - Could not allocate tile collision list", __FILE__, __FUNCTION__, __LINE__);
    }

    egolib_timer__init(&gfx_update_timer);
}

void GFX::uninitialize()
{
    // End the billboard system.
    BillboardSystem::uninitialize();

    // Uninitialize the texture atlas manager.
    TextureAtlasManager::uninitialize();

    // Uninitialize the renderlist manager.
    renderlist_mgr_t::uninitialize();

    // Uninitialize the dolist manager.
    dolist_mgr_t::uninitialize();

	// Uninitialize the profiling variables.
    gfx_clear_loops = 0;
	reinitClocks(); // Important: clear out the sliding windows of the clocks.

    // deallocate the specailized "collistion lists"
    _dolist_colst.dtor();
    _renderlist_colst.dtor();

    Ego::FontManager::uninitialize();
    TextureManager::get().release_all(); ///< @todo Remove this.

    GFX::uninitializeOpenGL();
    GFX::uninitializeSDLGraphics();
}

void GFX::uninitializeOpenGL()
{
    TextureManager::uninitialize();
    Ego::Renderer::uninitialize();
}

int GFX::initializeOpenGL()
{
    using namespace Ego;
    // Start-up the renderer.
    Renderer::initialize(); ///< @todo Add error handling.
    // Start-up the texture manager.
    TextureManager::initialize(); ///< @todo Add error handling.

    auto& renderer = Renderer::get();
    // Set clear colour and clear depth.
    renderer.getColourBuffer().setClearValue(Colour4f(0, 0, 0, 0));
    renderer.getDepthBuffer().setClearValue(1.0f);

    // Enable writing to the depth buffer.
    renderer.setDepthWriteEnabled(true);

    // Enable depth test. Incoming fragment's depth value must be less.
    renderer.setDepthTestEnabled(true);
    renderer.setDepthFunction(CompareFunction::Less);

    // Disable blending.
    renderer.setBlendingEnabled(false);

    // Enable alpha testing: Hide fully transparent parts.
    renderer.setAlphaTestEnabled(true);
	renderer.setAlphaFunction(Ego::CompareFunction::Greater, 0.0f);

    /// @todo Including backface culling here prevents the mesh from getting rendered
    /// backface culling

    // oglx_begin_culling( GL_BACK, GL_CW );            // GL_ENABLE_BIT | GL_POLYGON_BIT

    // disable OpenGL lighting
	renderer.setLightingEnabled(false);

    // fill mode
	renderer.setRasterizationMode(Ego::RasterizationMode::Solid);

    // set up environment mapping
    /// @todo: this isn't used anywhere
    GL_DEBUG(glTexGeni)(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);  // Set The Texture Generation Mode For S To Sphere Mapping (NEW)
    GL_DEBUG(glTexGeni)(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);  // Set The Texture Generation Mode For T To Sphere Mapping (NEW)

    //Initialize the motion blur buffer
    renderer.getAccumulationBuffer().setClearValue(Colour4f(0.0f, 0.0f, 0.0f, 1.0f));
    renderer.getAccumulationBuffer().clear();

    // Load the current graphical settings
    // gfx_system_load_assets();

    _ogl_initialized = true;

    return _ogl_initialized && _sdl_initialized_graphics;
}

//--------------------------------------------------------------------------------------------
void GFX::uninitializeSDLGraphics()
{
    if (!_sdl_initialized_graphics)
    {
        return;
    }
}

void GFX::initializeSDLGraphics()
{
    if (_sdl_initialized_graphics)
    {
        return;
    }

    // The flags to pass to SDL_SetVideoMode.
    SDLX_video_parameters_t::download(&sdl_vparam, &egoboo_config_t::get());

    sdl_vparam.flags.opengl = true;
    sdl_vparam.gl_att.doublebuffer = true;
    sdl_vparam.gl_att.accelerated_visual = GL_TRUE;
    sdl_vparam.gl_att.accum[0] = 8;
    sdl_vparam.gl_att.accum[1] = 8;
    sdl_vparam.gl_att.accum[2] = 8;
    sdl_vparam.gl_att.accum[3] = 8;

    oglx_video_parameters_t::download(&ogl_vparam, &egoboo_config_t::get());

    log_info("Opening SDL Video Mode...\n");

    bool setVideoMode = false;

    // Actually set the video mode.
    if (!SDL_GL_set_mode(nullptr, &sdl_vparam, &ogl_vparam, _sdl_initialized_graphics))
    {
        log_message("Failed!\n");
        if (egoboo_config_t::get().graphic_fullscreen.getValue())
        {
            log_info("SDL error with fullscreen mode on: %s\n", SDL_GetError());
            log_info("Trying again in windowed mode...\n");
            sdl_vparam.flags.full_screen = SDL_FALSE;
            if (!SDL_GL_set_mode(nullptr, &sdl_vparam, &ogl_vparam, _sdl_initialized_graphics))
            {
                log_message("Failed!\n");
            }
            else
            {
                egoboo_config_t::get().graphic_fullscreen.setValue(false);
                setVideoMode = true;
            }
        }
    }
    else
    {
        setVideoMode = true;
    }

    if (!setVideoMode)
    {
        log_error("I can't get SDL to set any video mode: %s\n", SDL_GetError());
    }
    else
    {
        GFX_WIDTH = (float)GFX_HEIGHT / (float)sdl_vparam.verticalResolution * (float)sdl_vparam.horizontalResolution;
        log_message("Success!\n");
    }
    
    SDL_Window *window = sdl_scr.window;
    
#if !defined(ID_OSX)
    {
        // Setup the cute windows manager icon, don't do this on Mac.
        const std::string fileName = "icon.bmp";
        auto pathName = "mp_data/" + fileName;
        SDL_Surface *theSurface = IMG_Load_RW(vfs_openRWopsRead(pathName.c_str()), 1);
        if (!theSurface)
        {
            log_warning("unable to load icon `%s` - reason: %s\n", pathName.c_str(), SDL_GetError());
        }
        else
        {
            SDL_SetWindowIcon(window, theSurface);
        }
    }
#endif
    
    // Set the window name.
    auto title = std::string("Egoboo ") + GameEngine::GAME_VERSION;
    SDL_SetWindowTitle(window, title.c_str());

    _sdl_initialized_graphics = true;
}

//--------------------------------------------------------------------------------------------
void gfx_system_render_world(std::shared_ptr<Camera> camera, std::shared_ptr<Ego::Graphics::TileList> tileList, std::shared_ptr<Ego::Graphics::EntityList> entityList)
{
    gfx_error_state_t * err_tmp;

    gfx_error_clear();

    if (!camera)
    {
        throw std::invalid_argument("nullptr == camera");
    }

    gfx_begin_3d(*camera);
    {
		Ego::Graphics::RenderPasses::g_background.run(*camera, *tileList, *entityList);
        render_scene(*camera, tileList, entityList);
		Ego::Graphics::RenderPasses::g_foreground.run(*camera, *tileList, *entityList);

        if (camera->getMotionBlur() > 0)
        {
            if (camera->getMotionBlurOld() < 0.001f)
            {
                GL_DEBUG(glAccum)(GL_LOAD, 1);
            }
            // Do motion blur.
            if (true /*currentState != playingState*/) //ZF> TODO: disable motion blur in in-game menu
            {
                GL_DEBUG(glAccum)(GL_MULT, camera->getMotionBlur());
                GL_DEBUG(glAccum)(GL_ACCUM, 1.0f - camera->getMotionBlur());
            }
            GL_DEBUG(glAccum)(GL_RETURN, 1.0f);
        }
    }
    gfx_end_3d();

    // Render the billboards
    BillboardSystem::get().render_all(*camera);

    err_tmp = gfx_error_pop();
    if (err_tmp)
    {
        printf("**** Encountered graphics errors in frame %d ****\n\n", game_frame_all);
        while (err_tmp)
        {
            printf("vvvv\n");
            printf(
                "\tfile     == %s\n"
                "\tline     == %d\n"
                "\tfunction == %s\n"
                "\tcode     == %d\n"
                "\tstring   == %s\n",
                err_tmp->file, err_tmp->line, err_tmp->function, err_tmp->type, err_tmp->string);
            printf("^^^^\n\n");

            err_tmp = gfx_error_pop();
        }
        printf("****\n\n");
    }
}

//--------------------------------------------------------------------------------------------
void gfx_system_main()
{
    /// @author ZZ
    /// @details This function does all the drawing stuff

    CameraSystem::get()->renderAll(gfx_system_render_world);

    draw_hud();

    gfx_request_flip_pages();
}

//--------------------------------------------------------------------------------------------
renderlist_mgr_t *gfx_system_get_renderlist_mgr()
{
    try
    {
        renderlist_mgr_t::initialize();
    }
    catch (...)
    {
        return nullptr;
    }

    return &renderlist_mgr_t::get();
}

//--------------------------------------------------------------------------------------------
dolist_mgr_t *gfx_system_get_dolist_mgr()
{
    try
    {
        dolist_mgr_t::initialize();
    }
    catch (...)
    {
        return nullptr;
    }
    return &dolist_mgr_t::get();
}

//--------------------------------------------------------------------------------------------
void gfx_system_load_assets()
{
    /// @author ZF
    /// @details This function loads all the graphics based on the game settings
    // Enable prespective correction?
    Ego::Renderer::get().setPerspectiveCorrectionEnabled(gfx.perspective);
    // Enable dithering?
    Ego::Renderer::get().setDitheringEnabled(gfx.dither);
    // Enable Gouraud shading?
    Ego::Renderer::get().setGouraudShadingEnabled(gfx.gouraudShading_enable);
    // Enable antialiasing (via multisamples)?
    Ego::Renderer::get().setMultisamplesEnabled(gfx.antialiasing);
}

//--------------------------------------------------------------------------------------------
void gfx_system_init_all_graphics()
{
    gfx_init_bar_data();
    gfx_init_blip_data();
    font_bmp_init();

	reinitClocks();
}

//--------------------------------------------------------------------------------------------
void gfx_system_release_all_graphics()
{
    gfx_init_bar_data();
    gfx_init_blip_data();
    BillboardSystem::get().reset();
    TextureManager::get().release_all();
}

//--------------------------------------------------------------------------------------------
void gfx_system_delete_all_graphics()
{
    gfx_init_bar_data();
    gfx_init_blip_data();
    BillboardSystem::get().reset();
}

//--------------------------------------------------------------------------------------------
void gfx_system_load_basic_textures()
{
    /// @author ZZ
    /// @details This function loads the standard textures for a module

    // load the bitmapped font (must be done after gfx_system_init_all_graphics())
    font_bmp_load_vfs(TextureManager::get().get_valid_ptr(static_cast<TX_REF>(TX_FONT_BMP)), "mp_data/font_new_shadow", "mp_data/font.txt");

    // Cursor
    TextureManager::get().load("mp_data/cursor", static_cast<TX_REF>(TX_CURSOR));

    // Skull
    TextureManager::get().load("mp_data/skull", static_cast<TX_REF>(TX_SKULL));

    // Particle sprites
    TextureManager::get().load("mp_data/particle_trans", (TX_REF)TX_PARTICLE_TRANS, TRANSCOLOR);
    prt_set_texture_params((TX_REF)TX_PARTICLE_TRANS);

    TextureManager::get().load("mp_data/particle_light", (TX_REF)TX_PARTICLE_LIGHT);
    prt_set_texture_params((TX_REF)TX_PARTICLE_LIGHT);

    // Module background tiles
    TextureManager::get().load("mp_data/tile0", (TX_REF)TX_TILE_0);
    TextureManager::get().load("mp_data/tile1", (TX_REF)TX_TILE_1);
    TextureManager::get().load("mp_data/tile2", (TX_REF)TX_TILE_2);
    TextureManager::get().load("mp_data/tile3", (TX_REF)TX_TILE_3);

    // Water textures
    TextureManager::get().load("mp_data/watertop", (TX_REF)TX_WATER_TOP);
    TextureManager::get().load("mp_data/waterlow", (TX_REF)TX_WATER_LOW);

    // The phong map
    TextureManager::get().load("mp_data/phong", (TX_REF)TX_PHONG, TRANSCOLOR);

    //Input icons
    TextureManager::get().load("mp_data/keybicon", static_cast<TX_REF>(TX_ICON_KEYB));
    TextureManager::get().load("mp_data/mousicon", static_cast<TX_REF>(TX_ICON_MOUS));
    TextureManager::get().load("mp_data/joyaicon", static_cast<TX_REF>(TX_ICON_JOYA));
    TextureManager::get().load("mp_data/joybicon", static_cast<TX_REF>(TX_ICON_JOYB));

    TextureAtlasManager::decimate_all_mesh_textures();

	reinitClocks();
}

//--------------------------------------------------------------------------------------------
void gfx_system_make_enviro()
{
    /// @author ZZ
    /// @details This function sets up the environment mapping table

    // Find the environment map positions
    for (size_t i = 0; i < EGO_NORMAL_COUNT; ++i)
    {
        float x = MD2Model::getMD2Normal(i, 0);
        float y = MD2Model::getMD2Normal(i, 1);
        indextoenvirox[i] = std::atan2(y, x) * Ego::Math::invTwoPi<float>();
    }

    for (size_t i = 0; i < 256; ++i)
    {
        float z = i / INV_FF;  // z is between 0.0 and 1.0.
        lighttoenviroy[i] = z;
    }
}

//--------------------------------------------------------------------------------------------
void gfx_system_reload_all_textures()
{
    /// @author BB
    /// @details function is called when the graphics mode is changed or the program is
    /// restored from a minimized state. Otherwise, all OpenGL bitmaps return to a random state.

    TextureManager::get().reload_all();
    TextureAtlasManager::reload_all();
}

//--------------------------------------------------------------------------------------------
// 2D RENDERER FUNCTIONS
//--------------------------------------------------------------------------------------------
void draw_blip(float sizeFactor, Uint8 color, float x, float y)
{
    /// @author ZZ
    /// @details This function draws a single blip
    ego_frect_t tx_rect, sc_rect;

    float width, height;

    //Now draw it
    if (x > 0.0f && y > 0.0f)
    {
        oglx_texture_t * ptex = TextureManager::get().get_valid_ptr((TX_REF)TX_BLIP);

        tx_rect.xmin = (float)bliprect[color]._left / (float)ptex->getWidth();
        tx_rect.xmax = (float)bliprect[color]._right / (float)ptex->getWidth();
        tx_rect.ymin = (float)bliprect[color]._top / (float)ptex->getHeight();
        tx_rect.ymax = (float)bliprect[color]._bottom / (float)ptex->getHeight();

        width = sizeFactor * (bliprect[color]._right - bliprect[color]._left);
        height = sizeFactor * (bliprect[color]._bottom - bliprect[color]._top);

        sc_rect.xmin = x - (width / 2);
        sc_rect.xmax = x + (width / 2);
        sc_rect.ymin = y - (height / 2);
        sc_rect.ymax = y + (height / 2);

        draw_quad_2d(ptex, sc_rect, tx_rect, true);
    }
}

//--------------------------------------------------------------------------------------------
float draw_icon_texture(oglx_texture_t * ptex, float x, float y, Uint8 sparkle_color, Uint32 sparkle_timer, float size, bool useAlpha)
{
    float       width, height;
    ego_frect_t tx_rect, sc_rect;

    if (NULL == ptex)
    {
        // defaults
        tx_rect.xmin = 0.0f;
        tx_rect.xmax = 1.0f;
        tx_rect.ymin = 0.0f;
        tx_rect.ymax = 1.0f;
    }
    else
    {
        tx_rect.xmin = 0.0f;
        tx_rect.xmax = (float)ptex->getSourceWidth() / (float)ptex->getWidth();
        tx_rect.ymin = 0.0f;
        /// @todo Is this a bug? Tis only works if the images are rectangular.
        tx_rect.ymax = (float)ptex->getSourceWidth() / (float)ptex->getWidth();
    }

    width = ICON_SIZE;
    height = ICON_SIZE;

    // handle non-default behavior
    if (size >= 0.0f)
    {
        float factor_wid = (float)size / width;
        float factor_hgt = (float)size / height;
        float factor = std::min(factor_wid, factor_hgt);

        width *= factor;
        height *= factor;
    }

    sc_rect.xmin = x;
    sc_rect.xmax = x + width;
    sc_rect.ymin = y;
    sc_rect.ymax = y + height;

    draw_quad_2d(ptex, sc_rect, tx_rect, useAlpha);

    if (NOSPARKLE != sparkle_color)
    {
        int         position;
        float       loc_blip_x, loc_blip_y;

        position = sparkle_timer & SPARKLE_AND;

        loc_blip_x = x + position * (width / SPARKLE_SIZE);
        loc_blip_y = y;
        draw_blip(0.5f, sparkle_color, loc_blip_x, loc_blip_y);

        loc_blip_x = x + width;
        loc_blip_y = y + position * (height / SPARKLE_SIZE);
        draw_blip(0.5f, sparkle_color, loc_blip_x, loc_blip_y);

        loc_blip_x = loc_blip_x - position  * (width / SPARKLE_SIZE);
        loc_blip_y = y + height;
        draw_blip(0.5f, sparkle_color, loc_blip_x, loc_blip_y);

        loc_blip_x = x;
        loc_blip_y = loc_blip_y - position * (height / SPARKLE_SIZE);
        draw_blip(0.5f, sparkle_color, loc_blip_x, loc_blip_y);
    }

    return y + height;
}

//--------------------------------------------------------------------------------------------
float draw_game_icon(const TX_REF icontype, float x, float y, Uint8 sparkle_color, Uint32 sparkle_timer, float size)
{
    /// @author ZZ
    /// @details This function draws an icon

    return draw_icon_texture(TextureManager::get().get_valid_ptr(icontype), x, y, sparkle_color, sparkle_timer, size);
}

//--------------------------------------------------------------------------------------------
float draw_menu_icon(const TX_REF icontype, float x, float y, Uint8 sparkle_color, Uint32 sparkle_timer, float size)
{
    return draw_icon_texture(TextureManager::get().get_valid_ptr(icontype), x, y, sparkle_color, sparkle_timer, size);
}

//--------------------------------------------------------------------------------------------
float draw_one_xp_bar(float x, float y, Uint8 ticks)
{
    /// @author ZF
    /// @details This function draws a xp bar and returns the y position for the next one

    int width, height;
    Uint8 cnt;
    ego_frect_t tx_rect, sc_rect;

    ticks = std::min(ticks, (Uint8)NUMTICK);

    Ego::Renderer::get().setColour(Ego::Math::Colour4f::white());

    //---- Draw the tab (always colored)

    width = 16;
    height = XPTICK;

    tx_rect.xmin = 0;
    tx_rect.xmax = 32.00f / 128;
    tx_rect.ymin = XPTICK / 16;
    tx_rect.ymax = XPTICK * 2 / 16;

    sc_rect.xmin = x;
    sc_rect.xmax = x + width;
    sc_rect.ymin = y;
    sc_rect.ymax = y + height;

    draw_quad_2d(TextureManager::get().get_valid_ptr((TX_REF)TX_XP_BAR), sc_rect, tx_rect, true);

    x += width;

    //---- Draw the filled ones
    tx_rect.xmin = 0.0f;
    tx_rect.xmax = 32 / 128.0f;
    tx_rect.ymin = XPTICK / 16.0f;
    tx_rect.ymax = 2 * XPTICK / 16.0f;

    width = XPTICK;
    height = XPTICK;

    for (cnt = 0; cnt < ticks; cnt++)
    {
        sc_rect.xmin = x + (cnt * width);
        sc_rect.xmax = x + (cnt * width) + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d(TextureManager::get().get_valid_ptr((TX_REF)TX_XP_BAR), sc_rect, tx_rect, true);
    }

    //---- Draw the remaining empty ones
    tx_rect.xmin = 0;
    tx_rect.xmax = 32 / 128.0f;
    tx_rect.ymin = 0;
    tx_rect.ymax = XPTICK / 16.0f;

    for ( /*nothing*/; cnt < NUMTICK; cnt++)
    {
        sc_rect.xmin = x + (cnt * width);
        sc_rect.xmax = x + (cnt * width) + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d(TextureManager::get().get_valid_ptr((TX_REF)TX_XP_BAR), sc_rect, tx_rect, true);
    }

    return y + height;
}

//--------------------------------------------------------------------------------------------
float draw_one_bar(Uint8 bartype, float x_stt, float y_stt, int ticks, int maxticks)
{
    /// @author ZZ
    /// @details This function draws a bar and returns the y position for the next one

    const float scale = 1.0f;

    float       width, height;
    ego_frect_t tx_rect, sc_rect;
    oglx_texture_t * tx_ptr;

    float tx_width, tx_height, img_width;
    float tab_width, tick_width, tick_height;

    int total_ticks = maxticks;
    int tmp_bartype = bartype;

    float x_left = x_stt;
    float x = x_stt;
    float y = y_stt;

    if (maxticks <= 0 || ticks < 0 || bartype > NUMBAR) return y;

    // limit the values to reasonable ones
    if (total_ticks > MAXTICK) total_ticks = MAXTICK;
    if (ticks > total_ticks) ticks = total_ticks;

    // grab a pointer to the bar texture
    tx_ptr = TextureManager::get().get_valid_ptr((TX_REF)TX_BARS);

    // allow the bitmap to be scaled to arbitrary size
    tx_width = 128.0f;
    tx_height = 128.0f;
    img_width = 112.0f;
    if (NULL != tx_ptr)
    {
        tx_width = tx_ptr->getWidth();
        tx_height = tx_ptr->getHeight();
        img_width = tx_ptr->getSourceWidth();
    }

    // calculate the bar parameters
    tick_width = img_width / 14.0f;
    tick_height = img_width / 7.0f;
    tab_width = img_width / 3.5f;

    //---- Draw the tab
    tmp_bartype = bartype;

    tx_rect.xmin = 0.0f / tx_width;
    tx_rect.xmax = tab_width / tx_width;
    tx_rect.ymin = tick_height * (tmp_bartype + 0) / tx_height;
    tx_rect.ymax = tick_height * (tmp_bartype + 1) / tx_height;

    width = (tx_rect.xmax - tx_rect.xmin) * scale * tx_width;
    height = (tx_rect.ymax - tx_rect.ymin) * scale * tx_height;

    sc_rect.xmin = x;
    sc_rect.xmax = x + width;
    sc_rect.ymin = y;
    sc_rect.ymax = y + height;

    draw_quad_2d(tx_ptr, sc_rect, tx_rect, true);

    // make the new left-hand margin after the tab
    x_left = x_stt + width;
    x = x_left;

    //---- Draw the full rows of ticks
    while (ticks >= NUMTICK)
    {
        tmp_bartype = bartype;

        tx_rect.xmin = tab_width / tx_width;
        tx_rect.xmax = img_width / tx_width;
        tx_rect.ymin = tick_height * (tmp_bartype + 0) / tx_height;
        tx_rect.ymax = tick_height * (tmp_bartype + 1) / tx_height;

        width = (tx_rect.xmax - tx_rect.xmin) * scale * tx_width;
        height = (tx_rect.ymax - tx_rect.ymin) * scale * tx_height;

        sc_rect.xmin = x;
        sc_rect.xmax = x + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d(tx_ptr, sc_rect, tx_rect, true);

        y += height;
        ticks -= NUMTICK;
        total_ticks -= NUMTICK;
    }

    if (ticks > 0)
    {
        int full_ticks = NUMTICK - ticks;
        int empty_ticks = NUMTICK - (std::min(NUMTICK, total_ticks) - ticks);

        //---- draw a partial row of full ticks
        tx_rect.xmin = tab_width / tx_width;
        tx_rect.xmax = (img_width - tick_width * full_ticks) / tx_width;
        tx_rect.ymin = tick_height * (tmp_bartype + 0) / tx_height;
        tx_rect.ymax = tick_height * (tmp_bartype + 1) / tx_height;

        width = (tx_rect.xmax - tx_rect.xmin) * scale * tx_width;
        height = (tx_rect.ymax - tx_rect.ymin) * scale * tx_height;

        sc_rect.xmin = x;
        sc_rect.xmax = x + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d(tx_ptr, sc_rect, tx_rect, true);

        // move to the right after drawing the full ticks
        x += width;

        //---- draw a partial row of empty ticks
        tmp_bartype = 0;

        tx_rect.xmin = tab_width / tx_width;
        tx_rect.xmax = (img_width - tick_width * empty_ticks) / tx_width;
        tx_rect.ymin = tick_height * (tmp_bartype + 0) / tx_height;
        tx_rect.ymax = tick_height * (tmp_bartype + 1) / tx_height;

        width = (tx_rect.xmax - tx_rect.xmin) * scale * tx_width;
        height = (tx_rect.ymax - tx_rect.ymin) * scale * tx_height;

        sc_rect.xmin = x;
        sc_rect.xmax = x + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d(tx_ptr, sc_rect, tx_rect, true);

        y += height;
        ticks = 0;
        total_ticks -= NUMTICK;
    }

    // reset the x position
    x = x_left;

    // Draw full rows of empty ticks
    while (total_ticks >= NUMTICK)
    {
        tmp_bartype = 0;

        tx_rect.xmin = tab_width / tx_width;
        tx_rect.xmax = img_width / tx_width;
        tx_rect.ymin = tick_height * (tmp_bartype + 0) / tx_height;
        tx_rect.ymax = tick_height * (tmp_bartype + 1) / tx_height;

        width = (tx_rect.xmax - tx_rect.xmin) * scale * tx_width;
        height = (tx_rect.ymax - tx_rect.ymin) * scale * tx_height;

        sc_rect.xmin = x;
        sc_rect.xmax = x + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d(tx_ptr, sc_rect, tx_rect, true);

        y += height;
        total_ticks -= NUMTICK;
    }

    // Draw the last of the empty ones
    if (total_ticks > 0)
    {
        int remaining = NUMTICK - total_ticks;

        //---- draw a partial row of empty ticks
        tmp_bartype = 0;

        tx_rect.xmin = tab_width / tx_width;
        tx_rect.xmax = (img_width - tick_width * remaining) / tx_width;
        tx_rect.ymin = tick_height * (tmp_bartype + 0) / tx_height;
        tx_rect.ymax = tick_height * (tmp_bartype + 1) / tx_height;

        width = (tx_rect.xmax - tx_rect.xmin) * scale * tx_width;
        height = (tx_rect.ymax - tx_rect.ymin) * scale * tx_height;

        sc_rect.xmin = x;
        sc_rect.xmax = x + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d(tx_ptr, sc_rect, tx_rect, true);

        y += height;
    }

    return y;
}

//--------------------------------------------------------------------------------------------
void draw_one_character_icon(const CHR_REF item, float x, float y, bool draw_ammo, Uint8 draw_sparkle)
{
    /// @author BB
    /// @details Draw an icon for the given item at the position <x,y>.
    ///     If the object is invalid, draw the null icon instead of failing
    ///     If NOSPARKLE is specified the default item sparkle will be used (default behaviour)

    TX_REF icon_ref;

    Object * pitem = !_currentModule->getObjectHandler().exists(item) ? NULL : _currentModule->getObjectHandler().get(item);

    // grab the icon reference
    icon_ref = chr_get_txtexture_icon_ref(item);

    // draw the icon
    if (draw_sparkle == NOSPARKLE) draw_sparkle = (NULL == pitem) ? NOSPARKLE : pitem->sparkle;
    draw_game_icon(icon_ref, x, y, draw_sparkle, update_wld, -1);

    // draw the ammo, if requested
    if (draw_ammo && (NULL != pitem))
    {
        if (0 != pitem->ammomax && pitem->ammoknown)
        {
            if ((!pitem->getProfile()->isStackable()) || pitem->ammo > 1)
            {
                // Show amount of ammo left
                draw_string_raw(x, y - 8, "%2d", pitem->ammo);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
float draw_character_xp_bar(const CHR_REF character, float x, float y)
{
    Object * pchr;

    if (!_currentModule->getObjectHandler().exists(character)) return y;
    pchr = _currentModule->getObjectHandler().get(character);

    //Draw the small XP progress bar
    if (pchr->experiencelevel < MAXLEVEL - 1)
    {
        std::shared_ptr<ObjectProfile> profile = ProfileSystem::get().getProfile(pchr->profile_ref);

        uint8_t  curlevel = pchr->experiencelevel + 1;
        uint32_t xplastlevel = profile->getXPNeededForLevel(curlevel - 1);
        uint32_t xpneed = profile->getXPNeededForLevel(curlevel);

        while (pchr->experience < xplastlevel && curlevel > 1) {
            curlevel--;
            xplastlevel = profile->getXPNeededForLevel(curlevel - 1);
        }

        float fraction = ((float)(pchr->experience - xplastlevel)) / (float)std::max<uint32_t>(xpneed - xplastlevel, 1);
        int   numticks = fraction * NUMTICK;

        y = draw_one_xp_bar(x, y, CLIP(numticks, 0, NUMTICK));
    }

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_fps(float y)
{
    // FPS text

    parser_state_t * ps = parser_state_t::get();

    if (outofsync)
    {
        y = draw_string_raw(0, y, "OUT OF SYNC");
    }

    if (parser_state_t::get_error(ps))
    {
        y = draw_string_raw(0, y, "SCRIPT ERROR ( see \"/debug/log.txt\" )");
    }

    /// @todo Add extra options for UPS and update lag don't display UPS or update lag just because FPS are displayed.
    if (egoboo_config_t::get().hud_displayFramesPerSecond.getValue())
    {
        y = draw_string_raw(0, y, "%2.3f FPS, %2.3f UPS, Update lag = %d", _gameEngine->getFPS(), _gameEngine->getUPS(), _gameEngine->getFrameSkip());

        // Extra debug info
        if (egoboo_config_t::get().debug_developerMode_enable.getValue())
        {
			/** @todo This should be made available through the GUI. Too much information just to print out things on screen. */
        }
    }

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_help(float y)
{
    if (SDL_KEYDOWN(keyb, SDLK_F1))
    {
        // In-Game help
        y = draw_string_raw(0, y, "!!!MOUSE HELP!!!");
        y = draw_string_raw(0, y, "~~Go to input settings to change");
        y = draw_string_raw(0, y, "Default settings");
        y = draw_string_raw(0, y, "~~Left Click to use an item");
        y = draw_string_raw(0, y, "~~Left and Right Click to grab");
        y = draw_string_raw(0, y, "~~Middle Click to jump");
        y = draw_string_raw(0, y, "~~A and S keys do stuff");
        y = draw_string_raw(0, y, "~~Right Drag to move camera");
    }
    if (SDL_KEYDOWN(keyb, SDLK_F2))
    {
        // In-Game help
        y = draw_string_raw(0, y, "!!!JOYSTICK HELP!!!");
        y = draw_string_raw(0, y, "~~Go to input settings to change.");
        y = draw_string_raw(0, y, "~~Hit the buttons");
        y = draw_string_raw(0, y, "~~You'll figure it out");
    }
    if (SDL_KEYDOWN(keyb, SDLK_F3))
    {
        // In-Game help
        y = draw_string_raw(0, y, "!!!KEYBOARD HELP!!!");
        y = draw_string_raw(0, y, "~~Go to input settings to change.");
        y = draw_string_raw(0, y, "Default settings");
        y = draw_string_raw(0, y, "~~TGB control left hand");
        y = draw_string_raw(0, y, "~~YHN control right hand");
        y = draw_string_raw(0, y, "~~Keypad to move and jump");
        y = draw_string_raw(0, y, "~~Number keys for stats");
    }

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_debug(float y)
{
    if (!egoboo_config_t::get().debug_developerMode_enable.getValue())
    {
        return y;
    }

    if (SDL_KEYDOWN(keyb, SDLK_F5))
    {
        CHR_REF ichr;
        PLA_REF ipla;

        // Debug information
        y = draw_string_raw(0, y, "!!!DEBUG MODE-5!!!");
        y = draw_string_raw(0, y, "~~CAM %f %f %f", CameraSystem::get()->getMainCamera()->getPosition()[kX], CameraSystem::get()->getMainCamera()->getPosition()[kY], CameraSystem::get()->getMainCamera()->getPosition()[kZ]);
        ipla = (PLA_REF)0;
        if (VALID_PLA(ipla))
        {
            ichr = PlaStack.lst[ipla].index;
            y = draw_string_raw(0, y, "~~PLA0DEF %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f",
                _currentModule->getObjectHandler().get(ichr)->damage_resistance[DAMAGE_SLASH],
                _currentModule->getObjectHandler().get(ichr)->damage_resistance[DAMAGE_CRUSH],
                _currentModule->getObjectHandler().get(ichr)->damage_resistance[DAMAGE_POKE],
                _currentModule->getObjectHandler().get(ichr)->damage_resistance[DAMAGE_HOLY],
                _currentModule->getObjectHandler().get(ichr)->damage_resistance[DAMAGE_EVIL],
                _currentModule->getObjectHandler().get(ichr)->damage_resistance[DAMAGE_FIRE],
                _currentModule->getObjectHandler().get(ichr)->damage_resistance[DAMAGE_ICE],
                _currentModule->getObjectHandler().get(ichr)->damage_resistance[DAMAGE_ZAP]);

            ichr = PlaStack.lst[ipla].index;
            y = draw_string_raw(0, y, "~~PLA0 %5.1f %5.1f", _currentModule->getObjectHandler().get(ichr)->getPosX() / GRID_FSIZE, _currentModule->getObjectHandler().get(ichr)->getPosY() / GRID_FSIZE);
        }

        ipla = (PLA_REF)1;
        if (VALID_PLA(ipla))
        {
            ichr = PlaStack.lst[ipla].index;
            y = draw_string_raw(0, y, "~~PLA1 %5.1f %5.1f", _currentModule->getObjectHandler().get(ichr)->getPosY() / GRID_FSIZE, _currentModule->getObjectHandler().get(ichr)->getPosY() / GRID_FSIZE);
        }
    }

    if (SDL_KEYDOWN(keyb, SDLK_F6))
    {
        // More debug information
        y = draw_string_raw(0, y, "!!!DEBUG MODE-6!!!");
        y = draw_string_raw(0, y, "~~FREEPRT %" PRIuZ, ParticleHandler::get().getFreeCount());
        y = draw_string_raw(0, y, "~~FREECHR %" PRIuZ, OBJECTS_MAX - _currentModule->getObjectHandler().getObjectCount());
#if 0
        y = draw_string_raw( 0, y, "~~MACHINE %d", egonet_get_local_machine() );
#endif
        y = draw_string_raw(0, y, _currentModule->isExportValid() ? "~~EXPORT: TRUE" : "~~EXPORT: FALSE");
        y = draw_string_raw(0, y, "~~PASS %d", _currentModule->getPassageCount());
#if 0
        y = draw_string_raw( 0, y, "~~NETPLAYERS %d", egonet_get_client_count() );
#endif
        y = draw_string_raw(0, y, "~~DAMAGEPART %d", damagetile.part_gpip.get());

        // y = draw_string_raw( 0, y, "~~FOGAFF %d", fog_data.affects_water );
    }

    if (SDL_KEYDOWN(keyb, SDLK_F7))
    {
        std::shared_ptr<Camera> camera = CameraSystem::get()->getMainCamera();

        // White debug mode
        y = draw_string_raw(0, y, "!!!DEBUG MODE-7!!!");
        y = draw_string_raw(0, y, "CAM <%f, %f, %f, %f>", camera->getView()(0, 0), camera->getView()(0, 1), camera->getView()(0, 2), camera->getView()(0, 3));
        y = draw_string_raw(0, y, "CAM <%f, %f, %f, %f>", camera->getView()(1, 0), camera->getView()(1, 1), camera->getView()(1, 2), camera->getView()(1, 3));
        y = draw_string_raw(0, y, "CAM <%f, %f, %f, %f>", camera->getView()(2, 0), camera->getView()(2, 1), camera->getView()(2, 2), camera->getView()(2, 3));
        y = draw_string_raw(0, y, "CAM <%f, %f, %f, %f>", camera->getView()(3, 0), camera->getView()(3, 1), camera->getView()(3, 2), camera->getView()(3, 3));
        y = draw_string_raw(0, y, "CAM center <%f, %f>", camera->getCenter()[kX], camera->getCenter()[kY]);
        y = draw_string_raw(0, y, "CAM turn %d %d", static_cast<int>(camera->getTurnMode()), camera->getTurnTime());
    }

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_timer(float y)
{
    int fifties, seconds, minutes;

    if (timeron)
    {
        fifties = (timervalue % 50) << 1;
        seconds = ((timervalue / 50) % 60);
        minutes = (timervalue / 3000);
        y = draw_string_raw(0, y, "=%d:%02d:%02d=", minutes, seconds, fifties);
    }

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_game_status(float y)
{
#if 0
    if ( egonet_getWaitingForClients() )
    {
        y = draw_string_raw( 0, y, "Waiting for players... " );
    }
    else if (g_serverState.player_count > 0 )
#endif
    {
        if (local_stats.allpladead || _currentModule->canRespawnAnyTime())
        {
            if (_currentModule->isRespawnValid() && egoboo_config_t::get().game_difficulty.getValue() < Ego::GameDifficulty::Hard)
            {
                y = draw_string_raw(0, y, "PRESS SPACE TO RESPAWN");
            }
            else
            {
                y = draw_string_raw(0, y, "PRESS ESCAPE TO QUIT");
            }
        }
        else if (_currentModule->isBeaten())
        {
            y = draw_string_raw(0, y, "VICTORY!  PRESS ESCAPE");
        }
    }
#if 0
    else
    {
        y = draw_string_raw( 0, y, "ERROR: MISSING PLAYERS" );
    }
#endif

    return y;
}

//--------------------------------------------------------------------------------------------
void draw_hud()
{
    /// @author ZZ
    /// @details draw in-game heads up display

    int y;

    gfx_begin_2d();
    {
        //draw_inventory();

        y = draw_fps(0);
        y = draw_help(y);
        y = draw_debug(y);
        y = draw_timer(y);
        y = draw_game_status(y);

        // Network message input
        if (keyb.chat_mode)
        {
            char buffer[CHAT_BUFFER_SIZE + 128] = EMPTY_CSTR;

            snprintf(buffer, SDL_arraysize(buffer), "%s > %s%s", egoboo_config_t::get().network_playerName.getValue().c_str(),
                net_chat.buffer, HAS_NO_BITS(update_wld, 8) ? "x" : "+");

            y = draw_wrap_string(buffer, 0, y, sdl_scr.x - WRAP_TOLERANCE);
        }

        y = DisplayMsg_draw_all(y);
    }
    gfx_end_2d();
}

//--------------------------------------------------------------------------------------------
#if 0
void draw_inventory()
{
    /// @author ZF
    /// @details This renders the open inventories of all local players

    PLA_REF ipla;
    player_t * ppla;
    Ego::Colour4f background_color(0.66f, 0.0f, 0.0f, 0.95f);

    CHR_REF ichr;
    Object *pchr;

    PLA_REF draw_list[MAX_LOCAL_PLAYERS];
    size_t cnt, draw_list_length = 0;

    float sttx, stty;
    int width, height;

    static int lerp_time[MAX_LOCAL_PLAYERS] = { 0 };

    //figure out what we have to draw
    for (ipla = 0; ipla < MAX_PLAYER; ipla++)
    {
        //valid player?
        ppla = PlaStack.get_ptr(ipla);
        if (!ppla->valid) continue;

        //draw inventory?
        if (!ppla->draw_inventory) continue;
        ichr = ppla->index;

        //valid character?
        if (!_currentModule->getObjectHandler().exists(ichr)) continue;
        pchr = _currentModule->getObjectHandler().get(ichr);

        //don't draw inventories of network players
        if (!pchr->islocalplayer) continue;

        draw_list[draw_list_length++] = ipla;
    }

    //figure out size and position of the inventory
    width = 180;
    height = 140;

    sttx = 0;
    stty = GFX_HEIGHT / 2 - height / 2;
    stty -= height * (draw_list_length - 1);
    stty = std::max(0.0f, stty);

    //now draw each inventory
    for (cnt = 0; cnt < draw_list_length; cnt++)
    {
        size_t i;
        STRING buffer;
        int icon_count, item_count, weight_sum, max_weight;
        float x, y, edgex;

        //Figure out who this is
        ipla = draw_list[cnt];
        ppla = PlaStack.get_ptr(ipla);

        ichr = ppla->index;
        pchr = _currentModule->getObjectHandler().get(ichr);

        //handle inventories sliding into view
        ppla->inventory_lerp = std::min(ppla->inventory_lerp, width);
        if (ppla->inventory_lerp > 0 && lerp_time[cnt] < SDL_GetTicks())
        {
            lerp_time[cnt] = SDL_GetTicks() + 1;
            ppla->inventory_lerp = std::max(0, ppla->inventory_lerp - 16);
        }

        //set initial positions
        x = sttx - ppla->inventory_lerp;
        y = stty;

        //threshold to start a new row
        edgex = sttx + width + 5 - 32;

        //calculate max carry weight
        max_weight = 200 + pchr->getAttribute(Ego::Attribute::MIGHT) * pchr->getAttribute(Ego::Attribute::MIGHT);

        //draw the backdrop
        const Ego::Math::Colour4f INVENTORY_COLOUR(0.6f, 0.0f, 0.0f, 0.6f);
        Ego::Renderer::get().setColour(INVENTORY_COLOUR);
        GL_DEBUG(glBegin)(GL_QUADS);
        {
            GL_DEBUG(glVertex2f)(x, y);
            GL_DEBUG(glVertex2f)(x, y + height);
            GL_DEBUG(glVertex2f)(x + width, y + height);
            GL_DEBUG(glVertex2f)(x + width, y);
        }
        GL_DEBUG_END();
        x += 5;

        //draw title
        draw_wrap_string(pchr->getName(false, false, true).c_str(), x, y, x + width);
        y += 32;

        //draw each inventory icon
        weight_sum = 0;
        icon_count = 0;
        item_count = 0;
        for(size_t i = 0; i < pchr->getInventory().getMaxItems(); ++i)
        {
            std::shared_ptr<Object> item = pchr->getInventory().getItem(i);
            
            //calculate the sum of the weight of all items in inventory
            if (item) weight_sum += item->getProfile()->getWeight();

            //draw icon
            draw_one_character_icon(item != nullptr ? item->getCharacterID() : INVALID_CHR_REF, x, y, true, (item_count == ppla->inventory_slot) ? COLOR_WHITE : NOSPARKLE);
            icon_count++;
            item_count++;
            x += 32 + 5;

            //new row?
            if (x >= edgex || icon_count >= pchr->getInventory().getMaxItems() / 2)
            {
                x = sttx + 5 - ppla->inventory_lerp;
                y += 32 + 5;
                icon_count = 0;
            }
        }

        //Draw weight
        x = sttx + 5 - ppla->inventory_lerp;
        y = stty + height - 42;
        snprintf(buffer, SDL_arraysize(buffer), "Weight: %d/%d", weight_sum, max_weight);
        draw_wrap_string(buffer, x, y, sttx + width + 5);

        //prepare drawing the next inventory
        stty += height + 10;
    }
}
#endif

//--------------------------------------------------------------------------------------------
void draw_mouse_cursor()
{
    if (!mous.on)
    {
        SDL_ShowCursor(SDL_DISABLE);
        return;
    }

    gfx_begin_2d();

    oglx_texture_t *pcursor = TextureManager::get().get_valid_ptr(TX_CURSOR);

    // Invalid texture?
    if (nullptr == pcursor)
    {
        // Show the system mouse cursor.
        SDL_ShowCursor(SDL_ENABLE);
    }
    else
    {
        ego_frect_t sc_rect;
        ego_frect_t tx_rect;

        //Get current mouse position
        int x, y;
        SDL_GetMouseState(&x, &y);

        // Compute the texture coordinate rectangle (in texture coordinates).
        tx_rect.xmin = 0.0f;
        tx_rect.ymin = 0.0f;
        auto textureWidth = pcursor->getWidth();
        auto textureHeight = pcursor->getHeight();
        tx_rect.xmax = (0 == textureWidth) ? 1.0f :
                       (float)pcursor->getSourceWidth() / (float)textureWidth;
        tx_rect.ymax = (0 == textureHeight) ? 1.0f :
                       (float)pcursor->getSourceHeight() / (float)textureHeight;
        // Compute the target coordinate rectangle (in pixels).
        sc_rect.xmin = x;
        sc_rect.ymin = y;
        sc_rect.xmax = x + pcursor->getSourceWidth();
        sc_rect.ymax = y + pcursor->getSourceHeight();
        // Hide the system mouse cursor.
        SDL_ShowCursor(SDL_DISABLE);
        // Draw the cursor.
        draw_quad_2d(pcursor, sc_rect, tx_rect, true);
    }
    gfx_end_2d();
}

//--------------------------------------------------------------------------------------------
// 3D RENDERER FUNCTIONS
//--------------------------------------------------------------------------------------------
struct by_element_t
{
    float  dist;
    Uint32 tile;
    Uint32 texture;
};

int by_element_cmp(const void *lhs, const void *rhs)
{
    int retval = 0;
    by_element_t * loc_lhs = (by_element_t *)lhs;
    by_element_t * loc_rhs = (by_element_t *)rhs;

    if (NULL == loc_lhs && NULL == loc_rhs)
    {
        retval = 0;
    }
    else if (NULL == loc_lhs && NULL != loc_rhs)
    {
        retval = 1;
    }
    else if (NULL == loc_rhs)
    {
        retval = -1;
    }
    else
    {
        retval = loc_lhs->texture - loc_rhs->texture;
        if (0 == retval)
        {
            float ftmp = loc_lhs->dist - loc_rhs->dist;

            if (ftmp < 0.0f)
            {
                retval = -1;
            }
            else if (ftmp > 0.0f)
            {
                retval = 1;
            }
        }
    }

    return retval;
}

struct by_list_t
{
    size_t       count;
    by_element_t lst[Ego::Graphics::renderlist_lst_t::CAPACITY];
    static by_list_t *sort(by_list_t *self)
    {
        if (!self)
        {
            return nullptr;
        }
        qsort(self->lst, self->count, sizeof(self->lst[0]), by_element_cmp);
        return self;
    }
};

//--------------------------------------------------------------------------------------------
gfx_rv render_fans_by_list(const ego_mesh_t * mesh, const Ego::Graphics::renderlist_lst_t * rlst)
{
    if (NULL == mesh) mesh = _currentModule->getMeshPointer();
    if (NULL == mesh)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid mesh");
        return gfx_error;
    }
    size_t tcnt = mesh->tmem.tile_count;
    const ego_tile_info_t *tlst = tile_mem_t::get(&(mesh->tmem), 0);

    if (!rlst)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "NULL tile rlst");
        return gfx_error;
    }

    if (0 == rlst->size)
    {
        return gfx_success;
    }

    // insert the rlst values into lst_vals
    by_list_t lst_vals = { 0 };
    lst_vals.count = rlst->size;
    for (Uint32 cnt = 0; cnt < rlst->size; cnt++)
    {
        lst_vals.lst[cnt].tile = rlst->lst[cnt].index;
        lst_vals.lst[cnt].dist = rlst->lst[cnt].distance;

        if (rlst->lst[cnt].index >= tcnt)
        {
            lst_vals.lst[cnt].texture = (Uint32)(~0);
        }
        else
        {
            const ego_tile_info_t * ptile = tlst + rlst->lst[cnt].index;

            int img = TILE_GET_LOWER_BITS(ptile->img);
            if (ptile->type >= tile_dict.offset)
            {
                img += MESH_IMG_COUNT;
            }

            lst_vals.lst[cnt].texture = img;
        }
    }

    by_list_t::sort(&lst_vals);

    // restart the mesh texture code
    mesh_texture_invalidate();

    for (size_t i = 0; i < rlst->size; ++i)
    {
        Uint32 tmp_itile = lst_vals.lst[i].tile;

        gfx_rv render_rv = render_fan(mesh, tmp_itile);
        if (egoboo_config_t::get().debug_developerMode_enable.getValue() && gfx_error == render_rv)
        {
            log_warning("%s - error rendering tile %d.\n", __FUNCTION__, tmp_itile);
        }
    }

    // let the mesh texture code know that someone else is in control now
    mesh_texture_invalidate();

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
// render_scene FUNCTIONS
//--------------------------------------------------------------------------------------------
gfx_rv render_scene_init(Ego::Graphics::TileList& tl, Ego::Graphics::EntityList& el, dynalist_t& dyl, Camera& cam)
{
    // assume the best;
    gfx_rv retval = gfx_success;

    {
		ClockScope<ClockPolicy::NonRecursive> scope(gfx_make_tileList_timer);
        // Which tiles can be displayed
        if (gfx_error == gfx_make_tileList(tl, cam))
        {
            retval = gfx_error;
        }
    }

    ego_mesh_t *mesh = tl.getMesh();
    if (!mesh)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "tile list is not attached to a mesh");
        return gfx_error;
    }

    {
		ClockScope<ClockPolicy::NonRecursive> scope(gfx_make_entityList_timer);
        // determine which objects are visible
        if (gfx_error == gfx_make_entityList(el, cam))
        {
            retval = gfx_error;
        }
    }

    // put off sorting the entity list until later
    // because it has to be sorted differently for reflected and non-reflected objects

    {
		ClockScope<ClockPolicy::NonRecursive> scope(do_grid_lighting_timer);
        // figure out the terrain lighting
        if (gfx_error == do_grid_lighting(tl, dyl, cam))
        {
            retval = gfx_error;
        }
    }

    {
		ClockScope<ClockPolicy::NonRecursive> scope(light_fans_timer);
        // apply the lighting to the characters and particles
        if (gfx_error == light_fans(tl))
        {
            retval = gfx_error;
        }
    }

    {
		ClockScope<ClockPolicy::NonRecursive> scope(gfx_update_all_chr_instance_timer);
        // make sure the characters are ready to draw
        if (gfx_error == gfx_update_all_chr_instance())
        {
            retval = gfx_error;
        }
    }

    {
		ClockScope<ClockPolicy::NonRecursive> scope(update_all_prt_instance_timer);
        // make sure the particles are ready to draw
        if (gfx_error == update_all_prt_instance(cam))
        {
            retval = gfx_error;
        }
    }

    // do the flashing for kursed objects
    if (gfx_error == gfx_update_flashing(el))
    {
        retval = gfx_error;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene_mesh(Camera& cam, const Ego::Graphics::TileList& tl, const Ego::Graphics::EntityList& el)
{
    /// @author BB
    /// @details draw the mesh and reflections of entities

    gfx_rv retval;

    // assume the best
    retval = gfx_success;
    //--------------------------------
    // advance the animation of all animated tiles
    animate_all_tiles(tl._mesh);

	// Render non-reflective tiles.
	Ego::Graphics::RenderPasses::g_nonReflective.run(cam, tl, el);

    //--------------------------------
    // draw the reflective tiles and reflections of entities
    if (gfx.refon)
    {
		// Clear background behind reflective tiles.
		Ego::Graphics::RenderPasses::g_reflective0.run(cam, tl, el);

		// Render reflections of entities.
		Ego::Graphics::RenderPasses::g_entityReflections.run(cam, tl, el);
    }
    else
    {
    }
	Ego::Graphics::RenderPasses::g_reflective1.run(cam, tl, el);

#if defined(RENDER_HMAP) && defined(_DEBUG)

    // restart the mesh texture code
    mesh_texture_invalidate();

    // render the heighmap
    for (size_t i = 0; i < rl._all.count; ++i)
    {
        render_hmap_fan(mesh, rl._all[i]);
    }

    // let the mesh texture code know that someone else is in control now
    mesh_texture_invalidate();

#endif

    // Render the shadows of entities.
	Ego::Graphics::RenderPasses::g_entityShadows.run(cam, tl, el);

    return retval;
}

//--------------------------------------------------------------------------------------------

gfx_rv render_scene(Camera& cam, Ego::Graphics::TileList& tl, Ego::Graphics::EntityList& el)
{
    // assume the best
    gfx_rv retval = gfx_success;
    {
		ClockScope<ClockPolicy::NonRecursive> clockScope(render_scene_init_timer);
        if (gfx_error == render_scene_init(tl, el, _dynalist, cam))
        {
            retval = gfx_error;
        }
    }
    {
		ClockScope<ClockPolicy::NonRecursive> clockScope(render_scene_mesh_timer);
        {
			// Sort dolist for reflected rendering.
			ClockScope<ClockPolicy::NonRecursive> clockScope(sortDoListReflected_timer);
			el.sort(cam, true);
        }
        // Render the mesh tiles and reflections of entities.
        if (gfx_error == render_scene_mesh(cam, tl, el))
        {
            retval = gfx_error;
        }
    }
	{
		// Sort dolist for unreflected rendering.
		ClockScope<ClockPolicy::NonRecursive> scope(sortDoListUnreflected_timer);
		if (gfx_error == el.sort(cam, false))
		{
			retval = gfx_error;
		}
	}

    // Render solid entities.
	Ego::Graphics::RenderPasses::g_solidEntities.run(cam, tl, el);
	// Render water.
	Ego::Graphics::RenderPasses::g_water.run(cam, tl, el);
	// Render transparent entities.
	Ego::Graphics::RenderPasses::g_transparentEntities.run(cam, tl, el);
#if defined(_DEBUG)
    render_all_prt_attachment();
#endif

#if defined(DRAW_LISTS)
    // draw some debugging lines
    line_list_draw_all(cam);
    point_list_draw_all(cam);
#endif

#if defined(DRAW_PRT_BBOX)
    render_all_prt_bbox();
#endif
    return retval;
}

gfx_rv render_scene(Camera& cam, std::shared_ptr<Ego::Graphics::TileList> ptl, std::shared_ptr<Ego::Graphics::EntityList> pel)
{
    /// @author ZZ
    /// @details This function draws 3D objects
    if (!ptl)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "could lock a tile list");
        return gfx_error;
    }

    if (!pel)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "could lock an entity list");
        return gfx_error;
    }
    return render_scene(cam, *ptl, *pel);
}

//--------------------------------------------------------------------------------------------
// gfx_config_t FUNCTIONS
//--------------------------------------------------------------------------------------------
void gfx_config_t::download(gfx_config_t *self, egoboo_config_t *cfg)
{
    // Load GFX configuration values, even if no Egoboo configuration is provided.
    init(self);

    if (!cfg)
    {
        throw std::invalid_argument("nullptr == cfg");
    }

    self->antialiasing = cfg->graphic_antialiasing.getValue() > 0;

    self->refon = cfg->graphic_reflections_enable.getValue();

    self->shadows_enable = cfg->graphic_shadows_enable.getValue();
    self->shadows_highQuality_enable = !cfg->graphic_shadows_highQuality_enable.getValue();

    self->gouraudShading_enable = cfg->graphic_gouraudShading_enable.getValue();
    self->dither = cfg->graphic_dithering_enable.getValue();
    self->perspective = cfg->graphic_perspectiveCorrection_enable.getValue();
    self->phongon = cfg->graphic_specularHighlights_enable.getValue();

    self->draw_background = cfg->graphic_background_enable.getValue() && water._background_req;
    self->draw_overlay = cfg->graphic_overlay_enable.getValue() && water._overlay_req;

    self->dynalist_max = CLIP(cfg->graphic_simultaneousDynamicLights_max.getValue(), (uint16_t)0, (uint16_t)TOTAL_MAX_DYNA);

    self->draw_water_0 = !self->draw_overlay && (water._layer_count > 0);
    self->clearson = !self->draw_background;
    self->draw_water_1 = !self->draw_background && (water._layer_count > 1);
}

void gfx_config_t::init(gfx_config_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }

    self->gouraudShading_enable = true;
    self->refon = true;
    self->antialiasing = false;
    self->dither = false;
    self->perspective = false;
    self->phongon = true;
    self->shadows_enable = true;
    self->shadows_highQuality_enable = true;

    self->clearson = true;
    self->draw_background = false;
    self->draw_overlay = false;
    self->draw_water_0 = true;
    self->draw_water_1 = true;

    self->dynalist_max = 8;
}

//--------------------------------------------------------------------------------------------
// gfx_error FUNCTIONS
//--------------------------------------------------------------------------------------------
egolib_rv gfx_error_add(const char * file, const char * function, int line, int id, const char * sz)
{
    gfx_error_state_t * pstate;

    // too many errors?
    if (gfx_error_stack.count >= GFX_ERROR_MAX) return rv_fail;

    // grab an error state
    pstate = gfx_error_stack.lst + gfx_error_stack.count;
    gfx_error_stack.count++;

    // where is the error
    strncpy(pstate->file, file, SDL_arraysize(pstate->file));
    strncpy(pstate->function, function, SDL_arraysize(pstate->function));
    pstate->line = line;

    // what is the error
    pstate->type = id;
    strncpy(pstate->string, sz, SDL_arraysize(pstate->string));

    return rv_success;
}

//--------------------------------------------------------------------------------------------
gfx_error_state_t * gfx_error_pop()
{
    gfx_error_state_t * retval;

    if (0 == gfx_error_stack.count || gfx_error_stack.count >= GFX_ERROR_MAX) return NULL;

    gfx_error_stack.count--;
    retval = gfx_error_stack.lst + gfx_error_stack.count;

    return retval;
}

//--------------------------------------------------------------------------------------------
void gfx_error_clear()
{
    gfx_error_stack.count = 0;
}

//--------------------------------------------------------------------------------------------
// grid_lighting FUNCTIONS
//--------------------------------------------------------------------------------------------
float grid_lighting_test(ego_mesh_t * mesh, GLXvector3f pos, float * low_diff, float * hgh_diff)
{
    int ix, iy, cnt;

    float u, v;

    const lighting_cache_t * cache_list[4];
    ego_grid_info_t  * pgrid;

    if (NULL == mesh) mesh = _currentModule->getMeshPointer();
    if (NULL == mesh)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid mesh");
        return 0.0f;
    }

    ix = std::floor(pos[XX] / GRID_FSIZE);
    iy = std::floor(pos[YY] / GRID_FSIZE);

    TileIndex fan[4];
    fan[0] = ego_mesh_t::get_tile_int(mesh, PointGrid(ix, iy));
    fan[1] = ego_mesh_t::get_tile_int(mesh, PointGrid(ix + 1, iy));
    fan[2] = ego_mesh_t::get_tile_int(mesh, PointGrid(ix, iy + 1));
    fan[3] = ego_mesh_t::get_tile_int(mesh, PointGrid(ix + 1, iy + 1));

    for (cnt = 0; cnt < 4; cnt++)
    {
        cache_list[cnt] = NULL;

        pgrid = mesh->get_pgrid(fan[cnt]);
        if (NULL == pgrid)
        {
            cache_list[cnt] = NULL;
        }
        else
        {
            cache_list[cnt] = &(pgrid->cache);
        }
    }

    u = pos[XX] / GRID_FSIZE - ix;
    v = pos[YY] / GRID_FSIZE - iy;

    return lighting_cache_test(cache_list, u, v, low_diff, hgh_diff);
}

//--------------------------------------------------------------------------------------------
bool grid_lighting_interpolate(const ego_mesh_t * mesh, lighting_cache_t * dst, const fvec2_t& pos)
{
    int ix, iy, cnt;
    TileIndex fan[4];
    float u, v;
    fvec2_t tpos;

    ego_grid_info_t  * pgrid;
    const lighting_cache_t * cache_list[4];

    if (NULL == mesh) mesh = _currentModule->getMeshPointer();
    if (NULL == mesh)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid mesh");
        return false;
    }

    if (NULL == dst)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "no valid lighting cache");
        return false;
    }

    // calculate the "tile position"
    tpos = pos * (1.0f / GRID_FSIZE);

    // grab this tile's coordinates
    ix = std::floor(tpos[XX]);
    iy = std::floor(tpos[YY]);

    // find the tile id for the surrounding tiles
    fan[0] = ego_mesh_t::get_tile_int(mesh, PointGrid(ix, iy));
    fan[1] = ego_mesh_t::get_tile_int(mesh, PointGrid(ix + 1, iy));
    fan[2] = ego_mesh_t::get_tile_int(mesh, PointGrid(ix, iy + 1));
    fan[3] = ego_mesh_t::get_tile_int(mesh, PointGrid(ix + 1, iy + 1));

    for (cnt = 0; cnt < 4; cnt++)
    {
        pgrid = mesh->get_pgrid(fan[cnt]);

        if (NULL == pgrid)
        {
            cache_list[cnt] = NULL;
        }
        else
        {
            cache_list[cnt] = &(pgrid->cache);
        }
    }

    // grab the coordinates relative to the parent tile
    u = tpos[XX] - ix;
    v = tpos[YY] - iy;

    return lighting_cache_interpolate(dst, cache_list, u, v);
}

//--------------------------------------------------------------------------------------------
// ASSET INITIALIZATION
//--------------------------------------------------------------------------------------------

void gfx_init_bar_data()
{
    Uint8 cnt;

    // Initialize the life and mana bars
    for (cnt = 0; cnt < NUMBAR; cnt++)
    {
        tabrect[cnt]._left = 0;
        tabrect[cnt]._right = TABX;
        tabrect[cnt]._top = cnt * BARY;
        tabrect[cnt]._bottom = (cnt + 1) * BARY;

        barrect[cnt]._left = TABX;
        barrect[cnt]._right = BARX;  // This is reset whenever a bar is drawn
        barrect[cnt]._top = tabrect[cnt]._top;
        barrect[cnt]._bottom = tabrect[cnt]._bottom;

    }
}

//--------------------------------------------------------------------------------------------
void gfx_init_blip_data()
{
    int cnt;

    // Set up the rectangles
    for (cnt = 0; cnt < COLOR_MAX; cnt++)
    {
        bliprect[cnt]._left = cnt * BLIPSIZE;
        bliprect[cnt]._right = cnt * BLIPSIZE + BLIPSIZE;
        bliprect[cnt]._top = 0;
        bliprect[cnt]._bottom = BLIPSIZE;
    }
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_load_bars()
{
    /// @author ZZ
    /// @details This function loads the status bar bitmap

    gfx_rv retval = gfx_success;

    const char *barFile = "mp_data/bars";
    if (!VALID_TX_RANGE(TextureManager::get().load(barFile, (TX_REF)TX_BARS)))
    {
        log_warning("%s - Cannot load file! (\"%s\")\n", __FUNCTION__, barFile);
        retval = gfx_fail;
    }

    const char *xpBarFile = "mp_data/xpbar";
    if (!VALID_TX_RANGE(TextureManager::get().load(xpBarFile, (TX_REF)TX_XP_BAR)))
    {
        log_warning("%s - Cannot load file! (\"%s\")\n", __FUNCTION__, xpBarFile);
        retval = gfx_fail;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_load_map()
{
    /// @author ZZ
    /// @details This function loads the map bitmap

    const char* szMap = "mp_data/plan";
    gfx_rv retval = gfx_success;

    // Load the images
    if (!VALID_TX_RANGE(TextureManager::get().load(szMap, (TX_REF)TX_MAP)))
    {
        log_debug("%s - Cannot load file! (\"%s\")\n", __FUNCTION__, szMap);
        retval = gfx_fail;
    }
    else
    {
        retval = gfx_success;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_load_blips()
{
    /// @author ZZ
    /// @details This function loads the blip bitmaps

    const char * pname = "mp_data/blip";
    gfx_rv retval = gfx_success;

    if (!VALID_TX_RANGE(TextureManager::get().load(pname, (TX_REF)TX_BLIP)))
    {
        log_warning("%s - Blip bitmap not loaded! (\"%s\")\n", __FUNCTION__, pname);
        retval = gfx_fail;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_load_icons()
{
    const char * pname = "mp_data/nullicon";
    gfx_rv retval = gfx_success;

    if (!VALID_TX_RANGE(TextureManager::get().load(pname, (TX_REF)TX_ICON_NULL)))
    {
        log_warning("%s - cannot load \"empty hand\" icon! (\"%s\")\n", __FUNCTION__, pname);
        retval = gfx_fail;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// MODE CONTROL
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
void gfx_request_clear_screen()
{
    gfx_page_clear_requested = true;
}

//--------------------------------------------------------------------------------------------
void gfx_do_clear_screen()
{
    if (!gfx_page_clear_requested) return;

    // Clear the depth buffer.
    Ego::Renderer::get().setDepthWriteEnabled(true);
    GL_DEBUG(glClear)(GL_DEPTH_BUFFER_BIT);

    // Clear the colour buffer.
    GL_DEBUG(glClear)(GL_COLOR_BUFFER_BIT);

    gfx_page_clear_requested = false;

    gfx_clear_loops++;
}

//--------------------------------------------------------------------------------------------
void gfx_do_flip_pages()
{
    if (gfx_page_flip_requested)

    {
        gfx_page_flip_requested = false;
        _flip_pages();

        gfx_page_clear_requested = true;
    }
}

//--------------------------------------------------------------------------------------------
void gfx_request_flip_pages()
{
    gfx_page_flip_requested = true;
}

//--------------------------------------------------------------------------------------------
bool gfx_flip_pages_requested()
{
    return gfx_page_flip_requested;
}

//--------------------------------------------------------------------------------------------
void _flip_pages()
{
    //GL_DEBUG( glFlush )();

    // draw the console on top of everything
    egolib_console_handler_t::draw_all();

    SDL_GL_SwapWindow(sdl_scr.window);

}

//--------------------------------------------------------------------------------------------
// LIGHTING FUNCTIONS
//--------------------------------------------------------------------------------------------
gfx_rv light_fans_throttle_update(ego_mesh_t * mesh, ego_tile_info_t * ptile, int fan, float threshold)
{
    grid_mem_t * pgmem = NULL;
    bool       retval = false;

    if (NULL == mesh)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "no valid mesh");
        return gfx_error;
    }
    pgmem = &(mesh->gmem);

    if (NULL == ptile)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "no valid tile");
        return gfx_error;
    }

#if defined(CLIP_LIGHT_FANS) && !defined(CLIP_ALL_LIGHT_FANS)

    // visible fans based on the update "need"
    retval = ego_mesh_test_corners(mesh, ptile, threshold);

    // update every 4 fans even if there is no need
    if (!retval)
    {
        int ix, iy;

        // use a kind of checkerboard pattern
        ix = fan % pgmem->grids_x;
        iy = fan / pgmem->grids_x;
        if (0 != (((ix ^ iy) + game_frame_all) & 0x03))
        {
            retval = true;
        }
    }

#else
    retval = true;
#endif

    return retval ? gfx_success : gfx_fail;
}

//--------------------------------------------------------------------------------------------
gfx_rv light_fans_update_lcache(Ego::Graphics::TileList& tl)
{
    const int frame_skip = 1 << 2;
#if defined(CLIP_ALL_LIGHT_FANS)
    const int frame_mask = frame_skip - 1;
#endif

    int    entry;
    float  local_mesh_lighting_keep;

    /// @note we are measuring the change in the intensity at the corner of a tile (the "delta") as
    /// a fraction of the current intensity. This is because your eye is much more sensitive to
    /// intensity differences when the intensity is low.
    ///
    /// @note it is normally assumed that 64 colors of gray can make a smoothly colored black and white picture
    /// which means that the threshold could be set as low as 1/64 = 0.015625.
    const float delta_threshold = 0.05f;

    ego_mesh_t *mesh = tl.getMesh();
    if (NULL == mesh)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist mesh");
        return gfx_error;
    }

#if defined(CLIP_ALL_LIGHT_FANS)
    // update all visible fans once every 4 frames
    if ( 0 != ( game_frame_all & frame_mask ) ) return gfx_success;
#endif

#if !defined(CLIP_LIGHT_FANS)
    // update only every frame
    local_mesh_lighting_keep = 0.9f;
#else
    // update only every 4 frames
    local_mesh_lighting_keep = std::pow(0.9f, frame_skip);
#endif

    // cache the grid lighting
    for (entry = 0; entry < tl._all.size; entry++)
    {
        bool reflective;
        float delta;
        ego_grid_info_t * pgrid;

        // which tile?
        int fan = tl._all.lst[entry].index;

        // grab a pointer to the tile
		ego_tile_info_t *ptile = mesh->get_ptile(fan);
        if (NULL == ptile) continue;

        // Test to see whether the lcache was already updated
        // - ptile->lcache_frame < 0 means that the cache value is invalid.
        // - ptile->lcache_frame is updated inside ego_mesh_light_corners()
#if defined(CLIP_LIGHT_FANS)
        // clip the updated on each individual tile
        if (ptile->lcache_frame >= 0 && (Uint32)ptile->lcache_frame + frame_skip >= game_frame_all)
#else
        // let the function clip all tile updates
        if ( ptile->lcache_frame >= 0 && ( Uint32 )ptile->lcache_frame >= game_frame_all )
#endif
        {
            continue;
        }

        // did someone else request an update?
        if (!ptile->request_lcache_update)
        {
            // is someone else did not request an update, do we need an one?
            gfx_rv light_fans_rv = light_fans_throttle_update(mesh, ptile, fan, delta_threshold);
            ptile->request_lcache_update = (gfx_success == light_fans_rv);
        }

        // if there's no need for an update, go to the next tile
        if (!ptile->request_lcache_update) continue;

        // is the tile reflective?
        pgrid = mesh->get_pgrid(fan);
        reflective = (0 != ego_grid_info_t::test_all_fx(pgrid, MAPFX_REFLECTIVE));

        // light the corners of this tile
        delta = ego_mesh_light_corners(tl._mesh, ptile, reflective, local_mesh_lighting_keep);

#if defined(CLIP_LIGHT_FANS)
        // use the actual maximum change in the intensity at a tile corner to
        // signal whether we need to calculate the next stage
        ptile->request_clst_update = (delta > delta_threshold);
#else
        // make sure that ego_mesh_light_corners() did not return an "error value"
        ptile->request_clst_update = ( delta > 0.0f );
#endif
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv light_fans_update_clst(Ego::Graphics::TileList& tl)
{
    /// @author BB
    /// @details update the tile's color list, if needed

    gfx_rv retval;
    int numvertices;
    int ivrt, vertex;
    float light;

    ego_tile_info_t   * ptile = NULL;
    ego_mesh_t         * mesh = NULL;
    tile_mem_t        * ptmem = NULL;
    tile_definition_t * pdef = NULL;

    mesh = tl.getMesh();
    if (NULL == mesh)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist mesh");
        return gfx_error;
    }

    // alias the tile memory
    ptmem = &(mesh->tmem);

    // assume the best
    retval = gfx_success;

    // use the grid to light the tiles
    for (size_t entry = 0; entry < tl._all.size; entry++)
    {
        TileIndex fan = tl._all.lst[entry].index;
        if (TileIndex::Invalid == fan) continue;

        // valid tile?
        ptile = mesh->get_ptile(fan);
        if (NULL == ptile)
        {
            retval = gfx_fail;
            continue;
        }

        // do nothing if this tile has not been marked as needing an update
        if (!ptile->request_clst_update)
        {
            continue;
        }

        // do nothing if the color list has already been computed this update
        if (ptile->clst_frame >= 0 && (Uint32)ptile->clst_frame >= game_frame_all)
        {
            continue;
        }

        pdef = TILE_DICT_PTR(tile_dict, ptile->type);
        if (NULL != pdef)
        {
            numvertices = pdef->numvertices;
        }
        else
        {
            numvertices = 4;
        }

        // copy the 1st 4 vertices
        for (ivrt = 0, vertex = ptile->vrtstart; ivrt < 4; ivrt++, vertex++)
        {
            GLXvector3f * pcol = ptmem->clst + vertex;

            light = ptile->lcache[ivrt];

            (*pcol)[RR] = (*pcol)[GG] = (*pcol)[BB] = INV_FF * CLIP(light, 0.0f, 255.0f);
        };

        for ( /* nothing */; ivrt < numvertices; ivrt++, vertex++)
        {
            bool was_calculated;
            GLXvector3f * pcol, *ppos;

            pcol = ptmem->clst + vertex;
            ppos = ptmem->plst + vertex;

            light = 0;
            was_calculated = ego_mesh_interpolate_vertex(ptmem, ptile, *ppos, &light);
            if (!was_calculated) continue;

            (*pcol)[RR] = (*pcol)[GG] = (*pcol)[BB] = INV_FF * CLIP(light, 0.0f, 255.0f);
        };

        // clear out the deltas
        BLANK_ARY(ptile->d1_cache);
        BLANK_ARY(ptile->d2_cache);

        // untag this tile
        ptile->request_clst_update = false;
        ptile->clst_frame = game_frame_all;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv light_fans(Ego::Graphics::TileList& tl)
{
    if (gfx_error == light_fans_update_lcache(tl))
    {
        return gfx_error;
    }

    if (gfx_error == light_fans_update_clst(tl))
    {
        return gfx_error;
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
float get_ambient_level()
{
    /// @author BB
    /// @details get the actual global ambient level

    float glob_amb, min_amb;

    glob_amb = 0.0f;
    min_amb = 0.0f;
    if (gfx.usefaredge)
    {
        // for outside modules, max light_a means bright sunlight
        // this should be handled with directional lighting, so ambient light is 0
        //glob_amb = light_a * 255.0f;
        glob_amb = 0;
    }
    else
    {
        // for inside modules, max light_a means dingy dungeon lighting
        glob_amb = light_a * 32.0f;
    }

    // determine the minimum ambient, based on darkvision
    min_amb = INVISIBLE / 4;
    if (local_stats.seedark_mag != 1.0f)
    {
        // start with the global light
        min_amb = glob_amb;

        // give a iny boost in the case of no light_a
        if (local_stats.seedark_mag > 0.0f) min_amb += 1.0f;

        // light_a can be quite dark, so we need a large magnification
        min_amb *= std::pow(local_stats.seedark_mag, 5);
    }

    return std::max(glob_amb, min_amb);
}

//--------------------------------------------------------------------------------------------
bool sum_global_lighting(std::array<float, LIGHTING_VEC_SIZE> &lighting)
{
    /// @author BB
    /// @details do ambient lighting. if the module is inside, the ambient lighting
    /// is reduced by up to a factor of 8. It is still kept just high enough
    /// so that ordnary objects will not be made invisible. This was breaking some of the AIs

    int cnt;
    float glob_amb;

    glob_amb = get_ambient_level();

    for (cnt = 0; cnt < LVEC_AMB; cnt++)
    {
        lighting[cnt] = 0.0f;
    }
    lighting[LVEC_AMB] = glob_amb;

    if (!gfx.usefaredge) return true;

    // do "outside" directional lighting (i.e. sunlight)
    lighting_vector_sum(lighting, light_nrm, light_d * 255, 0.0f);

    return true;
}

//--------------------------------------------------------------------------------------------
// SEMI OBSOLETE FUNCTIONS
//--------------------------------------------------------------------------------------------
gfx_rv dynalist_init(dynalist_t * pdylist)
{
    if (NULL == pdylist)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "NULL dynalist");
        return gfx_error;
    }

    pdylist->size = 0;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_make_dynalist(dynalist_t& dyl, Camera& cam)
{
    /// @author ZZ
    /// @details This function figures out which particles are visible, and it sets up dynamic
    ///    lighting

    size_t   tnc;
    fvec3_t  vdist;

    float         distance = 0.0f;
    dynalight_data_t * plight = NULL;

    float         distance_max = 0.0f;
    dynalight_data_t * plight_max = NULL;

    // HACK: if dynalist is ahead of the game by 30 frames or more, reset and force an update
    if ((Uint32)(dyl.frame + 30) >= game_frame_all)
        dyl.frame = -1;

    // do not update the dynalist more than once a frame
    if (dyl.frame >= 0 && (Uint32)dyl.frame >= game_frame_all)
    {
        return gfx_success;
    }

    // Don't really make a list, just set to visible or not
    dynalist_init(&dyl);

    for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
    {
        dynalight_info_t * pprt_dyna = &(particle->dynalight);

        // is the light on?
        if (!pprt_dyna->on || 0.0f == pprt_dyna->level) continue;

        // reset the dynalight pointer
        plight = NULL;

        // find the distance to the camera
        vdist = particle->getPosition() - cam.getTrackPosition();
        distance = vdist.length_2();

        // insert the dynalight
        if (dyl.size < gfx.dynalist_max && dyl.size < TOTAL_MAX_DYNA)
        {
            if (0 == dyl.size)
            {
                distance_max = distance;
            }
            else
            {
                distance_max = std::max(distance_max, distance);
            }

            // grab a new light from the list
            plight = dyl.lst + dyl.size;
            dyl.size++;

            if (distance_max == distance)
            {
                plight_max = plight;
            }
        }
        else if (distance < distance_max)
        {
            plight = plight_max;

            // find the new maximum distance
            distance_max = dyl.lst[0].distance;
            plight_max = dyl.lst + 0;
            for (tnc = 1; tnc < gfx.dynalist_max; tnc++)
            {
                if (dyl.lst[tnc].distance > distance_max)
                {
                    plight_max = dyl.lst + tnc;
                    distance_max = plight->distance;
                }
            }
        }

        if (NULL != plight)
        {
            plight->distance = distance;
            plight->pos = particle->getPosition();
            plight->level = pprt_dyna->level;
            plight->falloff = pprt_dyna->falloff;
        }
    }

    // the list is updated, so update the frame count
    dyl.frame = game_frame_all;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv do_grid_lighting(Ego::Graphics::TileList& tl, dynalist_t& dyl, Camera& cam)
{
    /// @author ZZ
    /// @details Do all tile lighting, dynamic and global

    size_t cnt;

    int    tnc;
    int ix, iy;
    float x0, y0, local_keep;
    bool needs_dynalight;

    std::array<float, LIGHTING_VEC_SIZE> global_lighting = {0};

    size_t               reg_count = 0;
    dynalight_registry_t reg[TOTAL_MAX_DYNA];

    ego_frect_t mesh_bound, light_bound;
    oct_bb_t        * poct;

    dynalight_data_t fake_dynalight;

	ego_mesh_t *mesh = tl.getMesh();
    if (NULL == mesh)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist mesh");
        return gfx_error;
    }

	ego_mesh_info_t *pinfo = &(mesh->info);
	grid_mem_t *pgmem = &(mesh->gmem);
	tile_mem_t *ptmem = &(mesh->tmem);

    // find a bounding box for the "frustum"
    mesh_bound.xmin = pgmem->edge_x;
    mesh_bound.xmax = 0;
    mesh_bound.ymin = pgmem->edge_y;
    mesh_bound.ymax = 0;
    for (size_t entry = 0; entry < tl._all.size; entry++)
    {
        TileIndex fan = tl._all.lst[entry].index;
        if (fan.getI() >= pinfo->tiles_count) continue;

        poct = &(tile_mem_t::get(ptmem, fan)->oct);

        mesh_bound.xmin = std::min(mesh_bound.xmin, poct->_mins[OCT_X]);
        mesh_bound.xmax = std::max(mesh_bound.xmax, poct->_maxs[OCT_X]);
        mesh_bound.ymin = std::min(mesh_bound.ymin, poct->_mins[OCT_Y]);
        mesh_bound.ymax = std::max(mesh_bound.ymax, poct->_maxs[OCT_Y]);
    }

    // is the visible mesh list empty?
    if (mesh_bound.xmin >= mesh_bound.xmax || mesh_bound.ymin >= mesh_bound.ymax)
        return gfx_success;

    // clear out the dynalight registry
    reg_count = 0;

    // refresh the dynamic light list
    gfx_make_dynalist(dyl, cam);

    // assume no dynamic lighting
    needs_dynalight = false;

    // assume no "extra help" for systems with only flat lighting
    dynalight_data__init(&fake_dynalight);

    // initialize the light_bound
    light_bound.xmin = pgmem->edge_x;
    light_bound.xmax = 0;
    light_bound.ymin = pgmem->edge_y;
    light_bound.ymax = 0;

    // make bounding boxes for each dynamic light
    if (gfx.gouraudShading_enable)
    {
        for (cnt = 0; cnt < dyl.size; cnt++)
        {
            float radius;
            ego_frect_t ftmp;

            dynalight_data_t * pdyna = dyl.lst + cnt;

            if (pdyna->falloff <= 0.0f || 0.0f == pdyna->level) continue;

            radius = std::sqrt(pdyna->falloff * 765.0f * 0.5f);

            // find the intersection with the frustum boundary
            ftmp.xmin = std::max(pdyna->pos[kX] - radius, mesh_bound.xmin);
            ftmp.xmax = std::min(pdyna->pos[kX] + radius, mesh_bound.xmax);
            ftmp.ymin = std::max(pdyna->pos[kY] - radius, mesh_bound.ymin);
            ftmp.ymax = std::min(pdyna->pos[kY] + radius, mesh_bound.ymax);

            // check to see if it intersects the "frustum"
            if (ftmp.xmin >= ftmp.xmax || ftmp.ymin >= ftmp.ymax) continue;

            reg[reg_count].bound = ftmp;
            reg[reg_count].reference = cnt;
            reg_count++;

            // determine the maxumum bounding box that encloses all valid lights
            light_bound.xmin = std::min(light_bound.xmin, ftmp.xmin);
            light_bound.xmax = std::max(light_bound.xmax, ftmp.xmax);
            light_bound.ymin = std::min(light_bound.ymin, ftmp.ymin);
            light_bound.ymax = std::max(light_bound.ymax, ftmp.ymax);
        }

        // are there any dynalights visible?
        if (reg_count > 0 && light_bound.xmax >= light_bound.xmin && light_bound.ymax >= light_bound.ymin)
        {
            needs_dynalight = true;
        }
    }
    else
    {
        float dyna_weight = 0.0f;
        float dyna_weight_sum = 0.0f;

        fvec3_t       diff;
        dynalight_data_t * pdyna;

        // evaluate all the lights at the camera position
        for (cnt = 0; cnt < dyl.size; cnt++)
        {
            pdyna = dyl.lst + cnt;

            // evaluate the intensity at the camera
            diff[kX] = pdyna->pos[kX] - cam.getCenter()[kX];
            diff[kY] = pdyna->pos[kY] - cam.getCenter()[kY];
            diff[kZ] = pdyna->pos[kZ] - cam.getCenter()[kZ] - 90.0f;   // evaluated at the "head height" of a character

            dyna_weight = std::abs(dyna_lighting_intensity(pdyna, diff));

            fake_dynalight.distance += dyna_weight * pdyna->distance;
            fake_dynalight.falloff += dyna_weight * pdyna->falloff;
            fake_dynalight.level += dyna_weight * pdyna->level;
            fake_dynalight.pos += (pdyna->pos - cam.getCenter()) * dyna_weight;

            dyna_weight_sum += dyna_weight;
        }

        // use a singel dynalight to represent the sum of all dynalights
        if (dyna_weight_sum > 0.0f)
        {
            float radius;
            ego_frect_t ftmp;

            fake_dynalight.distance /= dyna_weight_sum;
            fake_dynalight.falloff /= dyna_weight_sum;
            fake_dynalight.level /= dyna_weight_sum;
            fake_dynalight.pos = (fake_dynalight.pos * (1.0/dyna_weight_sum)) + cam.getCenter();

            radius = std::sqrt(fake_dynalight.falloff * 765.0f * 0.5f);

            // find the intersection with the frustum boundary
            ftmp.xmin = std::max(fake_dynalight.pos[kX] - radius, mesh_bound.xmin);
            ftmp.xmax = std::min(fake_dynalight.pos[kX] + radius, mesh_bound.xmax);
            ftmp.ymin = std::max(fake_dynalight.pos[kY] - radius, mesh_bound.ymin);
            ftmp.ymax = std::min(fake_dynalight.pos[kY] + radius, mesh_bound.ymax);

            // make a fake light bound
            light_bound = ftmp;

            // register the fake dynalight
            reg[reg_count].bound = ftmp;
            reg[reg_count].reference = -1;
            reg_count++;

            // let the downstream calc know we are coming
            needs_dynalight = true;
        }
    }

    // sum up the lighting from global sources
    sum_global_lighting(global_lighting);

    // make the grids update their lighting every 4 frames
    local_keep = std::pow(DYNALIGHT_KEEP, 4);

    // Add to base light level in normal mode
    for (size_t entry = 0; entry < tl._all.size; entry++)
    {
        bool resist_lighting_calculation = true;

        int                dynalight_count = 0;

        // grab each grid box in the "frustum"
        TileIndex fan = tl._all.lst[entry].index;

        // a valid tile?
        ego_grid_info_t  *pgrid = mesh->get_pgrid(fan);
        if (!pgrid) continue;

        // do not update this more than once a frame
        if (pgrid->cache_frame >= 0 && (Uint32)pgrid->cache_frame >= game_frame_all) continue;

        ix = fan.getI() % pinfo->tiles_x;
        iy = fan.getI() / pinfo->tiles_x;

        // Resist the lighting calculation?
        // This is a speedup for lighting calculations so that
        // not every light-tile calculation is done every single frame
        resist_lighting_calculation = (0 != (((ix + iy) ^ game_frame_all) & 0x03));

        if (resist_lighting_calculation) continue;

        // this is not a "bad" grid box, so grab the lighting info
        lighting_cache_t *pcache_old = &(pgrid->cache);

        lighting_cache_t cache_new;
        lighting_cache_init(&cache_new);

        // copy the global lighting
        for (tnc = 0; tnc < LIGHTING_VEC_SIZE; tnc++)
        {
            cache_new.low.lighting[tnc] = global_lighting[tnc];
            cache_new.hgh.lighting[tnc] = global_lighting[tnc];
        };

        // do we need any dynamic lighting at all?
        if (needs_dynalight)
        {
            // calculate the local lighting

            ego_frect_t fgrid_rect;

            x0 = ix * GRID_FSIZE;
            y0 = iy * GRID_FSIZE;

            // check this grid vertex relative to the measured light_bound
            fgrid_rect.xmin = x0 - GRID_FSIZE * 0.5f;
            fgrid_rect.xmax = x0 + GRID_FSIZE * 0.5f;
            fgrid_rect.ymin = y0 - GRID_FSIZE * 0.5f;
            fgrid_rect.ymax = y0 + GRID_FSIZE * 0.5f;

            // check the bounding box of this grid vs. the bounding box of the lighting
            if (fgrid_rect.xmin <= light_bound.xmax && fgrid_rect.xmax >= light_bound.xmin)
            {
                if (fgrid_rect.ymin <= light_bound.ymax && fgrid_rect.ymax >= light_bound.ymin)
                {
                    // this grid has dynamic lighting. add it.
                    for (cnt = 0; cnt < reg_count; cnt++)
                    {
                        fvec3_t       nrm;
                        dynalight_data_t * pdyna;

                        // does this dynamic light intersects this grid?
                        if (fgrid_rect.xmin > reg[cnt].bound.xmax || fgrid_rect.xmax < reg[cnt].bound.xmin) continue;
                        if (fgrid_rect.ymin > reg[cnt].bound.ymax || fgrid_rect.ymax < reg[cnt].bound.ymin) continue;

                        dynalight_count++;

                        // this should be a valid intersection, so proceed
                        tnc = reg[cnt].reference;
                        if (tnc < 0)
                        {
                            pdyna = &fake_dynalight;
                        }
                        else
                        {
                            pdyna = dyl.lst + tnc;
                        }

                        nrm[kX] = pdyna->pos[kX] - x0;
                        nrm[kY] = pdyna->pos[kY] - y0;
                        nrm[kZ] = pdyna->pos[kZ] - ptmem->bbox.getMin()[ZZ];
                        sum_dyna_lighting(pdyna, cache_new.low.lighting, nrm);

                        nrm[kZ] = pdyna->pos[kZ] - ptmem->bbox.getMax()[ZZ];
                        sum_dyna_lighting(pdyna, cache_new.hgh.lighting, nrm);
                    }
                }
            }
        }
        else if (!gfx.gouraudShading_enable)
        {
            // evaluate the intensity at the camera
        }

        // blend in the global lighting every single time
        // average this in with the existing lighting
        lighting_cache_blend(pcache_old, &cache_new, local_keep);

        // find the max intensity
        lighting_cache_max_light(pcache_old);

        pgrid->cache_frame = game_frame_all;
    }

    return gfx_success;
}
//--------------------------------------------------------------------------------------------
gfx_rv gfx_make_tileList(Ego::Graphics::TileList& tl, Camera& cam)
{
    gfx_rv      retval;
    bool      local_allocation;

    // because the main loop of the program will always flip the
    // page before rendering the 1st frame of the actual game,
    // game_frame_all will always start at 1
    if (1 != (game_frame_all & 3))
    {
        return gfx_success;
    }

    // reset the renderlist
    if (gfx_error == tl.reset())
    {
        return gfx_error;
    }

    // has the colst been allocated?
    local_allocation = false;
    if (0 == _renderlist_colst.capacity())
    {
        // allocate a BSP leaf pointer array to return the detected nodes
        local_allocation = true;
        if (NULL == _renderlist_colst.ctor(Ego::Graphics::renderlist_lst_t::CAPACITY))
        {
            gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "Could not allocate collision list");
            return gfx_error;
        }
    }

    // assume the best
    retval = gfx_success;

    // get the tiles in the center of the view
    _renderlist_colst.clear();
    getMeshBSP()->collide(cam.getFrustum(), _renderlist_colst);

    // transfer valid _renderlist_colst entries to the dolist
    if (gfx_error == tl.add(&_renderlist_colst, cam))
    {
        retval = gfx_error;
        goto gfx_make_renderlist_exit;
    }

gfx_make_renderlist_exit:

    // if there was a local allocation, make sure to deallocate
    if (local_allocation)
    {
        _renderlist_colst.dtor();
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_make_entityList(Ego::Graphics::EntityList& el, Camera& cam)
{
    gfx_rv retval;
    bool local_allocation;

    // assume the best
    retval = gfx_success;

	if (el.getSize() >= Ego::Graphics::EntityList::CAPACITY)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "invalid entity list size");
        return gfx_error;
    }

    // Remove all entities from the entity list.
    el.reset();

    // has the colst been allocated?
    local_allocation = false;
    if (0 == _dolist_colst.capacity())
    {
        // allocate a BSP leaf pointer array to return the detected nodes
        local_allocation = true;
		if (!_dolist_colst.ctor(Ego::Graphics::EntityList::CAPACITY))
        {
            gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "Could not allocate collision list");
            return gfx_error;
        }
    }

    // collide the characters with the frustum
    _dolist_colst.clear();
    getChrBSP()->collide(cam.getFrustum(), chr_BSP_is_visible, _dolist_colst);

    // transfer valid _dolist_colst entries to the dolist
    if (gfx_error == el.add_colst(&_dolist_colst))
    {
        retval = gfx_error;
        goto Exit;
    }

    // collide the particles with the frustum
    _dolist_colst.clear();
    getPrtBSP()->collide(cam.getFrustum(), prt_BSP_is_visible, _dolist_colst);

    // transfer valid _dolist_colst entries to the dolist
    if (gfx_error == el.add_colst(&_dolist_colst))
    {
        retval = gfx_error;
        goto Exit;
    }

Exit:

    // if there was a local allocation, make sure to deallocate
    if (local_allocation)
    {
        _dolist_colst.dtor();
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// UTILITY FUNCTIONS
#if 0
float calc_light_rotation(int rotation, int normal)
{
    /// @author ZZ
    /// @details This function helps make_lighttable
    fvec3_t   nrm, nrm2;
    float sinrot, cosrot;

    nrm[kX] = MD2Model::getMD2Normal(normal, 0);
    nrm[kY] = MD2Model::getMD2Normal(normal, 1);
    nrm[kZ] = MD2Model::getMD2Normal(normal, 2);

    sinrot = sinlut[rotation];
    cosrot = coslut[rotation];

    nrm2[kX] = cosrot * nrm[kX] + sinrot * nrm[kY];
    nrm2[kY] = cosrot * nrm[kY] - sinrot * nrm[kX];
    nrm2[kZ] = nrm[kZ];

    return (nrm2[kX] < 0) ? 0 : (nrm2[kX] * nrm2[kX]);
}
#endif

//--------------------------------------------------------------------------------------------
#if 0
float calc_light_global(int rotation, int normal, float lx, float ly, float lz)
{
    /// @author ZZ
    /// @details This function helps make_lighttable
    float fTmp;
    fvec3_t   nrm, nrm2;
    float sinrot, cosrot;

    nrm[kX] = MD2Model::getMD2Normal(normal, 0);
    nrm[kY] = MD2Model::getMD2Normal(normal, 1);
    nrm[kZ] = MD2Model::getMD2Normal(normal, 2);

    sinrot = sinlut[rotation];
    cosrot = coslut[rotation];

    nrm2[kX] = cosrot * nrm[kX] + sinrot * nrm[kY];
    nrm2[kY] = cosrot * nrm[kY] - sinrot * nrm[kX];
    nrm2[kZ] = nrm[kZ];

    fTmp = nrm2[kX] * lx + nrm2[kY] * ly + nrm2[kZ] * lz;
    if (fTmp < 0) fTmp = 0;

    return fTmp * fTmp;
}
#endif

//--------------------------------------------------------------------------------------------
gfx_rv gfx_update_flashing(Ego::Graphics::EntityList& el)
{
    gfx_rv retval;

    if (el.getSize() >= Ego::Graphics::EntityList::CAPACITY)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "invalid entity list size");
        return gfx_error;
    }

    retval = gfx_success;
    for (size_t i = 0, n = el.getSize(); i < n; ++i)
    {
        float tmp_seekurse_level;

        CHR_REF ichr = el.get(i).ichr;

        Object *pchr = _currentModule->getObjectHandler().get(ichr);
        if (!pchr) continue;

        chr_instance_t& pinst = pchr->inst;

        // Do flashing
        if (DONTFLASH != pchr->flashand)
        {
            if (HAS_NO_BITS(game_frame_all, pchr->flashand))
            {
				chr_instance_flash(pinst, 255);
            }
        }

        // Do blacking
        // having one holy player in your party will cause the effect, BUT
        // having some non-holy players will dilute it
        tmp_seekurse_level = std::min(local_stats.seekurse_level, 1.0f);
        if ((local_stats.seekurse_level > 0.0f) && pchr->iskursed && 1.0f != tmp_seekurse_level)
        {
            if (HAS_NO_BITS(game_frame_all, SEEKURSEAND))
            {
				chr_instance_flash(pinst, 255.0f *(1.0f - tmp_seekurse_level));
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_update_all_chr_instance()
{
    gfx_rv retval;
    gfx_rv tmp_rv;

    // assume the best
    retval = gfx_success;

    for (const std::shared_ptr<Object> &pchr : _currentModule->getObjectHandler().iterator())
    {
        //Dont do terminated characters
        if (pchr->isTerminated()) {
            continue;
        }

        if (!ego_mesh_t::grid_is_valid(_currentModule->getMeshPointer(), pchr->getTile())) continue;

        tmp_rv = update_one_chr_instance(pchr.get());

        // deal with return values
        if (gfx_error == tmp_rv)
        {
            retval = gfx_error;
        }
        else if (gfx_success == tmp_rv)
        {
            // the instance has changed, refresh the collision bound
            chr_update_collision_size(pchr.get(), true);
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// Tiled texture "optimizations"
//--------------------------------------------------------------------------------------------

oglx_texture_t *TextureAtlasManager::sml[MESH_IMG_COUNT];
oglx_texture_t *TextureAtlasManager::big[MESH_IMG_COUNT];

int TextureAtlasManager::big_cnt = 0;
int TextureAtlasManager::sml_cnt = 0;

#if TEXTUREATLASMANAGER_VERSION > 3
TextureAtlasManager::TextureAtlasManager() :
big_cnt(0), sml_cnt(0)
{
    for (size_t i = 0; i < MESH_IMG_COUNT; ++i)
    {
        if (!oglx_texture_t::ctor(&(sml[i])))
        {
            while (i > 0)
            {
                --i;
                oglx_texture_t::dtor(&(big[i]));
                oglx_texture_t::dtor(&(sml[i]));
            }
            throw std::runtime_error("unable to initialize texture atlas manager");
        }
        if (!oglx_texture_t::ctor(&(big[i])))
        {
            oglx_texture_t::dtor(&(sml[i]));
            while (i > 0)
            {
                --i;
                oglx_texture_t::dtor(&(big[i]));
                oglx_texture_t::dtor(&(sml[i]));
            }
            throw std::runtime_error("unable to initialize texture atlas manager");
        }
    }
}
TextureAtlasManager::~TextureAtlasManager()
{
    for (size_t i = 0; i < MESH_IMG_COUNT; ++i)
    {
        oglx_texture_t::dtor(&(sml[i]));
        oglx_texture_t::dtor(&(big[i]));
    }
}
TextureAtlasManager *TextureAtlasManager::_singleton = nullptr;
#else
bool TextureAtlasManager::initialized = false;
#endif

#if TEXTUREATLASMANAGER_VERSION > 3
TextureAtlasManager& TextureAtlasManager::get()
{
    if (!_singleton)
    {
        throw std::logic_error("texture atlas manager is not initialized");
    }
    return *_singleton;
}
#endif

void TextureAtlasManager::initialize()
{
#if TEXTUREATLASMANAGER_VERSION > 3
    if (!_singleton)
    {
        _singleton = new TextureAtlasManager();
    }
#else
    if (!initialized)
    {
        for (size_t i = 0; i < MESH_IMG_COUNT; ++i)
        {
            sml[i] = nullptr;
        }
        sml_cnt = 0;
        big_cnt = 0;

        initialized = true;
    }
#endif
}

void TextureAtlasManager::uninitialize()
{
#if TEXTUREATLASMANAGER_VERSION > 3
    if (_singleton)
    {
        delete _singleton;
        _singleton = nullptr;
    }
#else
    if (initialized)
    {
        for (size_t i = 0; i < sml_cnt; ++i)
        {
            delete sml[i];
            sml[i] = nullptr;
        }
        for (size_t i = 0; i < big_cnt; ++i)
        {
            delete big[i];
            big[i] = nullptr;
        }
        sml_cnt = 0;
        big_cnt = 0;

        initialized = false;
    }
#endif
}

void TextureAtlasManager::reinitialize()
{
    for (size_t i = 0; i < sml_cnt; ++i)
    {
        delete sml[i];
        sml[i] = nullptr;
    }
    for (size_t i = 0; i < big_cnt; ++i)
    {
        delete big[i];
        big[i] = nullptr;
    }
    sml_cnt = 0;
    big_cnt = 0;
}

oglx_texture_t *TextureAtlasManager::get_sml(int which)
{
    if (!initialized)
    {
        return nullptr;
    }
    if (which < 0 || which >= sml_cnt || which >= MESH_IMG_COUNT)
    {
        return nullptr;
    }
    return sml[which];
}

oglx_texture_t *TextureAtlasManager::get_big(int which)
{
    if (!initialized)
    {
        return nullptr;
    }
    if (which < 0 || which >= big_cnt || which >= MESH_IMG_COUNT)
    {
        return nullptr;
    }
    return big[which];
}

int TextureAtlasManager::decimate_one_mesh_texture(oglx_texture_t *src_tx, oglx_texture_t *(&tx_lst)[MESH_IMG_COUNT], size_t tx_lst_cnt, int minification)
{
    static const int sub_textures = 8;
    size_t cnt = tx_lst_cnt;
    if (!src_tx || !src_tx->_source)
    {   
        return cnt;
    }

    // make an alias for the texture's SDL_Surface
    auto src_img = src_tx->_source;

    // grab parameters from the mesh
    int src_img_w = src_img->w;
    int src_img_h = src_img->h;

    // how large a step every time through the mesh?
    float step_fx = (float)src_img_w / (float)sub_textures;
    float step_fy = (float)src_img_h / (float)sub_textures;

    SDL_Rect src_img_rect;
    src_img_rect.w = std::ceil(step_fx * minification);
    src_img_rect.w = std::max<Uint16>(1, src_img_rect.w);
    src_img_rect.h = std::ceil(step_fy * minification);
    src_img_rect.h = std::max<Uint16>(1, src_img_rect.h);


    size_t ix, iy;
    float fx, fy;
    // scan across the src_img
    for (iy = 0, fy = 0.0f; iy < sub_textures; iy++, fy += step_fy)
    {
        src_img_rect.y = std::floor(fy);

        for (ix = 0, fx = 0.0f; ix < sub_textures; ix++, fx += step_fx)
        {
            src_img_rect.x = std::floor(fx);

            // grab the destination texture
            oglx_texture_t *dst_tx = new oglx_texture_t();

            // create a blank destination SDL_Surface
            const auto& pfd = Ego::PixelFormatDescriptor::get<Ego::PixelFormat::R8G8B8A8>();
            auto dst_img = ImageManager::get().createImage(src_img_rect.w, src_img_rect.h, pfd);
            if (!dst_img)
            {
                delete dst_tx;
                cnt++;
                continue;
            }
            // Fill the destination surface with opaque white (redundant, but keept it until SDL 2 migration).
            SDL_FillRect(dst_img.get(), nullptr, SDL_MapRGBA(dst_img.get()->format, 0, 255, 255, 255));
            // Copy the pixels.
            for (size_t y = 0; y < src_img_rect.h; ++y)
            {
                if(src_img_rect.y + y >= src_img->h) break;
                for (size_t x = 0; x < src_img_rect.w; ++x)
                {
                    if(src_img_rect.x + x >= src_img->w) break;

                    uint32_t p = SDL_GL_getpixel(src_img.get(), src_img_rect.x + x, src_img_rect.y + y);
                    uint8_t r, g, b, a;
                    SDL_GetRGBA(p, src_img->format, &r, &g, &b, &a);
                    uint32_t q = SDL_MapRGBA(dst_img->format, r, g, b, a);
                    SDL_GL_putpixel(dst_img.get(), x, y, q);
                }
            }

            // upload the SDL_Surface into OpenGL
            dst_tx->load(dst_img);

            tx_lst[cnt] = dst_tx;

            // count the number of textures we're using
            cnt++;
        }
    }

    return cnt;
}

void TextureAtlasManager::decimate_all_mesh_textures()
{
    // Re-initialize the texture atlas manager.
    reinitialize();

    // Do the "small" textures.
    sml_cnt = 0;
    for (size_t i = 0; i < 4; ++i)
    {
        oglx_texture_t *ptx = TextureManager::get().get_valid_ptr(TX_TILE_0 + i);

        sml_cnt = decimate_one_mesh_texture(ptx, sml, sml_cnt, 1);
    }

    // Do the "big" textures.
    big_cnt = 0;
    for (size_t i = 0; i < 4; ++i)
    {
        oglx_texture_t *ptx = TextureManager::get().get_valid_ptr(TX_TILE_0 + i);

        big_cnt = decimate_one_mesh_texture(ptx, big, big_cnt, 2);
    }
}

void TextureAtlasManager::reload_all()
{
    /// @author BB
    /// @details This function re-loads all the current textures back into
    ///               OpenGL texture memory using the cached SDL surfaces

    if (!initialized)
    {
        return;
    }

    for (size_t cnt = 0; cnt < sml_cnt; ++cnt)
    {
        sml[cnt]->load(sml[cnt]->_source);
    }

    for (size_t cnt = 0; cnt < big_cnt; ++cnt)
    {
        big[cnt]->load(big[cnt]->_source);
    }
}

//--------------------------------------------------------------------------------------------
// chr_instance_t FUNCTIONS
//--------------------------------------------------------------------------------------------
gfx_rv update_one_chr_instance(Object *pchr)
{
    if (!ACTIVE_PCHR(pchr))
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, GET_INDEX_PCHR(pchr), "invalid character");
        return gfx_error;
    }
    chr_instance_t& pinst = pchr->inst;

    // only update once per frame
    if (pinst.update_frame >= 0 && (Uint32)pinst.update_frame >= game_frame_all)
    {
        return gfx_success;
    }

    // make sure that the vertices are interpolated
    gfx_rv retval = chr_instance_t::update_vertices(pinst, -1, -1, true);
    if (gfx_error == retval)
    {
        return gfx_error;
    }

    // do the basic lighting
    chr_instance_t::update_lighting_base(pinst, pchr, false);

    // set the update_frame to the current frame
    pinst.update_frame = game_frame_all;

    return retval;
}
