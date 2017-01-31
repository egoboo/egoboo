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

#include "game/graphic.h"

#include "game/Core/GameEngine.hpp"
#include "game/graphic_prt.h"
#include "game/graphic_mad.h"
#include "game/graphic_fan.h"
#include "game/graphic_billboard.h"
#include "game/renderer_3d.h"
#include "egolib/Script/script.h"
#include "game/script_compile.h"
#include "egolib/FileFormats/Globals.hpp"
#include "game/game.h"
#include "game/lighting.h"
#include "game/GUI/UIManager.hpp"
#include "game/Logic/Player.hpp"
#include "game/Module/Module.hpp"
#include "game/Graphics/RenderPasses.hpp"
#include "game/mesh.h"
#include "game/Graphics/CameraSystem.hpp"
#include "game/Module/Module.hpp"
#include "game/Entities/_Include.hpp"
#include "game/Graphics/TextureAtlasManager.hpp"
#include "game/Module/Passage.hpp"
#include "game/GUI/Material.hpp"

//--------------------------------------------------------------------------------------------

#define SPARKLE_SIZE ICON_SIZE
#define SPARKLE_AND  (SPARKLE_SIZE - 1)

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

gfx_config_t     gfx;

float            indextoenvirox[MD2Model::normalCount];

//--------------------------------------------------------------------------------------------

static dynalist_t _dynalist;

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

	Ego::Graphics::g_entityReflections.clock.reinit();
	Ego::Graphics::g_entityShadows.clock.reinit();
	Ego::Graphics::g_solidEntities.clock.reinit();
	Ego::Graphics::g_transparentEntities.clock.reinit();
	Ego::Graphics::g_water.clock.reinit();
	Ego::Graphics::g_reflective0.clock.reinit();
	Ego::Graphics::g_reflective1.clock.reinit();
	Ego::Graphics::g_nonReflective.clock.reinit();
	Ego::Graphics::g_foreground.clock.reinit();
	Ego::Graphics::g_background.clock.reinit();
}

static gfx_rv render_scene_init(Ego::Graphics::TileList& tl, Ego::Graphics::EntityList& el, dynalist_t& dyl, Camera& cam);
static gfx_rv render_scene_mesh(Camera& cam, const Ego::Graphics::TileList& tl, const Ego::Graphics::EntityList& el);
static gfx_rv render_scene(Camera& cam, Ego::Graphics::TileList& tl, Ego::Graphics::EntityList& el);

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

static float draw_fps(float y);
static float draw_help(float y);
static float draw_debug(float y);
static float draw_timer(float y);
static float draw_game_status(float y);


static gfx_rv gfx_update_all_chr_instance();
static gfx_rv gfx_update_flashing(Ego::Graphics::EntityList& el);

static bool sum_global_lighting(std::array<float, LIGHTING_VEC_SIZE> &lighting);

//--------------------------------------------------------------------------------------------
// GFX implementation
//--------------------------------------------------------------------------------------------

GFX::GFX() :
    App<GFX>("Egoboo", GameEngine::GAME_VERSION)
{
    // initialize the dynalist frame
    // otherwise, it will not update until the frame count reaches whatever
    // left over or random value is in this counter
    _dynalist.frame = -1;
    _dynalist.size = 0;

    // Initialize the billboard system.
    try {
        BillboardSystem::initialize();
    } catch (...) {
        std::rethrow_exception(std::current_exception());
    }
    // Initialize the texture atlas manager.
    try {
        Ego::Graphics::TextureAtlasManager::initialize();
    } catch (...) {
        BillboardSystem::uninitialize();
        std::rethrow_exception(std::current_exception());
    }
}

GFX::~GFX()
{
    // Uninitialize the billboard system.
    BillboardSystem::uninitialize();

    // Uninitialize the texture atlas manager.
    Ego::Graphics::TextureAtlasManager::uninitialize();

    // Uninitialize the profiling variables.
	reinitClocks(); // Important: clear out the sliding windows of the clocks.
}

//--------------------------------------------------------------------------------------------
void gfx_system_render_world(std::shared_ptr<Camera> camera, std::shared_ptr<Ego::Graphics::TileList> tileList, std::shared_ptr<Ego::Graphics::EntityList> entityList)
{
    if (!camera)
    {
        throw std::invalid_argument("nullptr == camera");
    }
    if (!tileList) {
        throw std::invalid_argument("nullptr == tileList");
    }

    if (!entityList) {
        throw std::invalid_argument("nullptr == entityList");
    }

    Renderer3D::begin3D(*camera);
    {
		Ego::Graphics::g_background.run(*camera, *tileList, *entityList);
        render_scene(*camera, *tileList, *entityList);
		Ego::Graphics::g_foreground.run(*camera, *tileList, *entityList);

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
    Renderer3D::end3D();

    // Render the billboards
    BillboardSystem::get().render_all(*camera);
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
    font_bmp_init();
	reinitClocks();
}

//--------------------------------------------------------------------------------------------
void gfx_system_release_all_graphics()
{
    BillboardSystem::get().reset();
    Ego::TextureManager::get().release_all();
}

//--------------------------------------------------------------------------------------------
void gfx_system_make_enviro()
{
    /// @author ZZ
    /// @details This function sets up the environment mapping table

    // Find the environment map positions
    for (size_t i = 0; i < MD2Model::normalCount; ++i)
    {
        float x = MD2Model::getMD2Normal(i, 0);
        float y = MD2Model::getMD2Normal(i, 1);
        indextoenvirox[i] = std::atan2(y, x) * Ego::Math::invTwoPi<float>();
    }
}

//--------------------------------------------------------------------------------------------
void gfx_system_reload_all_textures()
{
    /// @author BB
    /// @details function is called when the graphics mode is changed or the program is
    /// restored from a minimized state. Otherwise, all OpenGL bitmaps return to a random state.

    Ego::TextureManager::get().reupload();
    Ego::Graphics::TextureAtlasManager::get().reupload();
}

//--------------------------------------------------------------------------------------------
// 2D RENDERER FUNCTIONS
//--------------------------------------------------------------------------------------------
void draw_blip(float sizeFactor, Uint8 color, float x, float y)
{
    /// @author ZZ
    /// @details This function draws a single blip
    ego_frect_t tx_rect;

    float width, height;

    //Now draw it
    if (x > 0.0f && y > 0.0f)
    {
        std::shared_ptr<const Ego::Texture> ptex = Ego::TextureManager::get().getTexture("mp_data/blip");

        #define BLIPSIZE 6
        Ego::Rectangle<int> bliprect[COLOR_MAX];        // The blip rectangles

        // Set up the rectangles
        for (int cnt = 0; cnt < COLOR_MAX; cnt++) {
            bliprect[cnt]._left = cnt * BLIPSIZE;
            bliprect[cnt]._right = cnt * BLIPSIZE + BLIPSIZE;
            bliprect[cnt]._top = 0;
            bliprect[cnt]._bottom = BLIPSIZE;
        }

        tx_rect.xmin = (float)bliprect[color]._left / (float)ptex->getWidth();
        tx_rect.xmax = (float)bliprect[color]._right / (float)ptex->getWidth();
        tx_rect.ymin = (float)bliprect[color]._top / (float)ptex->getHeight();
        tx_rect.ymax = (float)bliprect[color]._bottom / (float)ptex->getHeight();

        width = sizeFactor * (bliprect[color]._right - bliprect[color]._left);
        height = sizeFactor * (bliprect[color]._bottom - bliprect[color]._top);

        auto sc_rect = Rectangle2f(Point2f(x - (width / 2), y - (height / 2)), Point2f(x + (width / 2), y + (height / 2)));
        _gameEngine->getUIManager()->drawQuad2D(sc_rect, tx_rect, std::make_shared<Ego::GUI::Material>(ptex, Ego::Math::Colour4f::white(), true));
    }
}

//--------------------------------------------------------------------------------------------
float draw_icon_texture(const std::shared_ptr<const Ego::Texture>& ptex, float x, float y, Uint8 sparkle_color, Uint32 sparkle_timer, float size, bool useAlpha)
{
    float       width, height;
    ego_frect_t tx_rect;

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

    auto sc_rect = Rectangle2f(Point2f(x, y), Point2f(x + width, y + height));
    _gameEngine->getUIManager()->drawQuad2D(sc_rect, tx_rect, std::make_shared<const Ego::GUI::Material>(ptex, Ego::Math::Colour4f::white(), true));

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
float draw_game_icon(const std::shared_ptr<const Ego::Texture>& icontype, float x, float y, Uint8 sparkle_color, Uint32 sparkle_timer, float size)
{
    /// @author ZZ
    /// @details This function draws an icon

    return draw_icon_texture(icontype, x, y, sparkle_color, sparkle_timer, size);
}

//--------------------------------------------------------------------------------------------
float draw_fps(float y)
{
    // FPS text

    parser_state_t& ps = parser_state_t::get();

    if (ps.get_error())
    {
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "SCRIPT ERROR ( see \"/debug/log.txt\" )", 0, 1.0f);
    }

    /// @todo Add extra options for UPS and update lag don't display UPS or update lag just because FPS are displayed.
    if (egoboo_config_t::get().hud_displayFramesPerSecond.getValue())
    {
        std::ostringstream os;
        os.setf(std::ios_base::fixed, std::ios_base::floatfield);
        os << std::setw(2) << std::setprecision(2) << _gameEngine->getFPS() << " FPS, " 
           << std::setw(2) << std::setprecision(2) << _gameEngine->getUPS() << " UPS, " 
           << _gameEngine->getFrameSkip() << " update lag";
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), os.str(), 0.0f, 1.0f);

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
    if (Ego::Input::InputSystem::get().isKeyDown(SDLK_F1))
    {
        // In-Game help
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "!!!MOUSE HELP!!!");
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "~~Go to input settings to change");
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "Default settings");
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "~~Left Click to use an item");
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "~~Left and Right Click to grab");
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "~~Middle Click to jump");
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "~~A and S keys do stuff");
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "~~Right Drag to move camera");
    }
    if (Ego::Input::InputSystem::get().isKeyDown(SDLK_F2))
    {
        // In-Game help
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "!!!JOYSTICK HELP!!!");
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "~~Go to input settings to change.");
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "~~Hit the buttons");
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "~~You'll figure it out");
    }
    if (Ego::Input::InputSystem::get().isKeyDown(SDLK_F3))
    {
        // In-Game help
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "!!!KEYBOARD HELP!!!");
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "~~Go to input settings to change.");
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "Default settings");
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "~~TGB control left hand");
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "~~YHN control right hand");
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "~~Keypad to move and jump");
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "~~Number keys for stats");
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

    if (Ego::Input::InputSystem::get().isKeyDown(SDLK_F5))
    {
        // Debug information
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "!!!DEBUG MODE-5!!!");
        std::ostringstream os;
        os << "~~CAM"
           << " " << CameraSystem::get().getMainCamera()->getPosition()[kX]
           << " " << CameraSystem::get().getMainCamera()->getPosition()[kY]
           << " " << CameraSystem::get().getMainCamera()->getPosition()[kZ];
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), os.str(), 0, 1.0f);
        if (_currentModule->getPlayerList().size() > 0)
        {
            std::shared_ptr<Object> pchr = _currentModule->getPlayer(0)->getObject();
            os << "~~PLA0DEF"
               << " " << std::setw(4) << std::setprecision(2) << pchr->getRawDamageResistance(DAMAGE_SLASH)
               << " " << std::setw(4) << std::setprecision(2) << pchr->getRawDamageResistance(DAMAGE_CRUSH)
               << " " << std::setw(4) << std::setprecision(2) << pchr->getRawDamageResistance(DAMAGE_POKE)
               << " " << std::setw(4) << std::setprecision(2) << pchr->getRawDamageResistance(DAMAGE_HOLY)
               << " " << std::setw(4) << std::setprecision(2) << pchr->getRawDamageResistance(DAMAGE_EVIL)
               << " " << std::setw(4) << std::setprecision(2) << pchr->getRawDamageResistance(DAMAGE_FIRE)
               << " " << std::setw(4) << std::setprecision(2) << pchr->getRawDamageResistance(DAMAGE_ICE)
               << " " << std::setw(4) << std::setprecision(2) << pchr->getRawDamageResistance(DAMAGE_ZAP);
            y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), os.str(), 0, 1.0f);
            os.str(std::string());
            os << std::setw(5) << std::setprecision(1) << (pchr->getPosX() / Info<float>::Grid::Size())
               << std::setw(5) << std::setprecision(1) << (pchr->getPosY() / Info<float>::Grid::Size());
            y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), os.str(), 0, 1.0f);
        }

        if (_currentModule->getPlayerList().size() > 1)
        {
            std::shared_ptr<Object> pchr = _currentModule->getPlayer(1)->getObject();
            std::ostringstream os;
            os << "~~PLA1"
                << " " << std::setw(5) << std::setprecision(1) << (pchr->getPosY() / Info<float>::Grid::Size())
                << " " << std::setw(5) << std::setprecision(1) << (pchr->getPosY() / Info<float>::Grid::Size());
            y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), os.str(), 0, 1.0f);
        }
    }

    if (Ego::Input::InputSystem::get().isKeyDown(SDLK_F6))
    {
        std::ostringstream os;
        // More debug information
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "!!!DEBUG MODE-6!!!");
        
        os.str(std::string()); os << "~~FREEPRT: " << ParticleHandler::get().getFreeCount();
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), os.str(), 0, 1.0f);
        
        os.str(std::string()); os << "~~FREECHR: " << OBJECTS_MAX - _currentModule->getObjectHandler().getObjectCount();
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), os.str(), 0, 1.0f);

        os.str(std::string()); os << "~~EXPORT:  " << (_currentModule->isExportValid() ? "TRUE" : "FALSE");
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), os.str(), 0, 1.0f);

        os.str(std::string()); os << "~~PASS:    " << _currentModule->getPassageCount();
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), os.str(), 0, 1.0f);
    }

    if (Ego::Input::InputSystem::get().isKeyDown(SDLK_F7))
    {
        std::shared_ptr<Camera> camera = CameraSystem::get().getMainCamera();

        std::ostringstream os;
        // White debug mode
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "!!!DEBUG MODE-7!!!");

        os.str(std::string()); os << "CAM <"
            << camera->getViewMatrix()(0, 0) << ", "
            << camera->getViewMatrix()(0, 1) << ", "
            << camera->getViewMatrix()(0, 2) << ", "
            << camera->getViewMatrix()(0, 3) << ">";
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), os.str(), 0, 1.0f);
        
        os.str(std::string()); os << "CAM <"
            << camera->getViewMatrix()(1, 0) << ", "
            << camera->getViewMatrix()(1, 1) << ", "
            << camera->getViewMatrix()(1, 2) << ", "
            << camera->getViewMatrix()(1, 3) << ">";
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), os.str(), 0, 1.0f);
        
        os.str(std::string()); os << "CAM <"
            << camera->getViewMatrix()(2, 0) << ", "
            << camera->getViewMatrix()(2, 1) << ", "
            << camera->getViewMatrix()(2, 2) << ", "
            << camera->getViewMatrix()(2, 3) << ">";
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), os.str(), 0, 1.0f);
        
        os.str(std::string()); os << "CAM <"
            << camera->getViewMatrix()(3, 0) << ", "
            << camera->getViewMatrix()(3, 1) << ", "
            << camera->getViewMatrix()(3, 2) << ", "
            << camera->getViewMatrix()(3, 3) << ">";
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), os.str(), 0, 1.0f);
        
        os.str(std::string()); os << "CAM center <"
            << camera->getCenter()[0] << ", "
            << camera->getCenter()[1] << ", "
            << camera->getCenter()[2] << ">";
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), os.str(), 0, 1.0f);
        
        os.str(std::string()); os << "CAM turn " << static_cast<int>(camera->getTurnMode()) << " " << camera->getTurnTime();
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), os.str(), 0, 1.0f);
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
        std::ostringstream os;
        os << "=" << minutes << ":" << std::setw(2) << seconds << ":" << std::setw(2) << fifties << "=";
        y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), os.str(), 0, 1.0f);
    }

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_game_status(float y)
{
#if 0
    if ( egonet_getWaitingForClients() )
    {
        y = _gameEngine->getUIManager()->drawBitmapFontString( 0, y, "Waiting for players... " );
    }
    else if (g_serverState.player_count > 0 )
#endif
    {
        if (local_stats.allpladead || _currentModule->canRespawnAnyTime())
        {
            if (_currentModule->isRespawnValid() && egoboo_config_t::get().game_difficulty.getValue() < Ego::GameDifficulty::Hard)
            {
                y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "PRESS SPACE TO RESPAWN");
            }
            else
            {
                y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "PRESS ESCAPE TO QUIT");
            }
        }
        else if (_currentModule->isBeaten())
        {
            y = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(0, y), "VICTORY!  PRESS ESCAPE");
        }
    }
#if 0
    else
    {
        y = _gameEngine->getUIManager()->drawBitmapFontString( 0, y, "ERROR: MISSING PLAYERS" );
    }
#endif

    return y;
}

//--------------------------------------------------------------------------------------------
void draw_hud()
{
    /// @author ZZ
    /// @details draw in-game heads up display

    _gameEngine->getUIManager()->beginRenderUI();
    {
        int y = draw_fps(0);
        y = draw_help(y);
        y = draw_debug(y);
        y = draw_timer(y);
        y = draw_game_status(y);
    }
    _gameEngine->getUIManager()->endRenderUI();
}

//--------------------------------------------------------------------------------------------
void draw_mouse_cursor()
{
    //if (!mous.on)
    //{
    //    Ego::GraphicsSystem::setCursorVisibility(false);
    //    return;
    //}

    const std::shared_ptr<Ego::Texture> &pcursor = Ego::TextureManager::get().getTexture("mp_data/cursor");

    // Invalid texture?
    if (nullptr == pcursor)
    {
        // Show the system mouse cursor.
        Ego::GraphicsSystemNew::get().setCursorVisibility(true);
    }
    else
    {
        // Hide the system mouse cursor.
        Ego::GraphicsSystemNew::get().setCursorVisibility(false);

        //Get current mouse position
        int x, y;
        SDL_GetMouseState(&x, &y);

        //Draw cursor
        _gameEngine->getUIManager()->beginRenderUI();
        _gameEngine->getUIManager()->drawImage(Point2f(x, y), Vector2f(pcursor->getWidth(), pcursor->getHeight()), std::make_shared<Ego::GUI::Material>(pcursor, Ego::Colour4f::white(), true));
        _gameEngine->getUIManager()->endRenderUI();
    }
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

    auto mesh = tl.getMesh();
    if (!mesh)
    {
		throw Id::RuntimeErrorException(__FILE__, __LINE__, "tile list is not attached to a mesh");
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
		if (gfx_error == GridIllumination::do_grid_lighting(tl, dyl, cam))
        {
            retval = gfx_error;
        }
    }

    {
		ClockScope<ClockPolicy::NonRecursive> scope(light_fans_timer);
        // apply the lighting to the characters and particles
		GridIllumination::light_fans(tl);
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
    animate_all_tiles(*tl.getMesh());

	// Render non-reflective tiles.
	Ego::Graphics::g_nonReflective.run(cam, tl, el);

    //--------------------------------
    // draw the reflective tiles and reflections of entities
    if (gfx.refon)
    {
		// Clear background behind reflective tiles.
		Ego::Graphics::g_reflective0.run(cam, tl, el);

		// Render reflections of entities.
		Ego::Graphics::g_entityReflections.run(cam, tl, el);
    }
    else
    {
    }
    // Render water.
	Ego::Graphics::g_reflective1.run(cam, tl, el);

	if (egoboo_config_t::get().debug_mesh_renderHeightMap.getValue())
	{
		// restart the mesh texture code
		TileRenderer::invalidate();

		// render the heighmap
        Ego::Graphics::Internal::TileListV2::render_heightmap(*tl.getMesh().get(), tl._all);

		// let the mesh texture code know that someone else is in control now
		TileRenderer::invalidate();
	}

    // Render the shadows of entities.
	Ego::Graphics::g_entityShadows.run(cam, tl, el);

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
			ClockScope<ClockPolicy::NonRecursive> clockScope2(sortDoListReflected_timer);
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
        el.sort(cam, false);
	}

    // Render solid entities.
	Ego::Graphics::g_solidEntities.run(cam, tl, el);

	// Render water.
	Ego::Graphics::g_water.run(cam, tl, el);

	// Render transparent entities.
	Ego::Graphics::g_transparentEntities.run(cam, tl, el);

    //Draw all passages
    if(Ego::Input::InputSystem::get().isKeyDown(SDLK_F8)) {
        draw_passages(cam);
    }

#if defined(DRAW_PRT_GRIP_ATTACH)
    render_all_prt_attachment();
#endif

#if defined(DRAW_LISTS)
    // draw some debugging lines
    Renderer3D::lineSegmentList.draw_all(cam);
    Renderer3D::pointList.draw_all(cam);
#endif

#if defined(DRAW_PRT_BBOX)
    render_all_prt_bbox();
#endif
    return retval;
}

void draw_passages(Camera& cam)
{
    /**
    * @brief
    *   Renders all passages using bboxes
    * @todo
    *   this is unoptimized and renders stuff off-screen
    **/

    Ego::Renderer::get().setProjectionMatrix(cam.getProjectionMatrix());
    Ego::Renderer::get().setViewMatrix(cam.getViewMatrix());
    Ego::Renderer::get().setWorldMatrix(Matrix4f4f::identity());

    for(int i = 0; i < _currentModule->getPassageCount(); ++i) {
        const AxisAlignedBox2f& passageABB = _currentModule->getPassageByID(i)->getAxisAlignedBox2f();

        //AABB2f to octagonal collision box
        oct_bb_t bb;
        bb._mins[OCT_X] = passageABB.getMin().x();
        bb._maxs[OCT_X] = passageABB.getMax().x();
        bb._mins[OCT_Y] = passageABB.getMin().y();
        bb._maxs[OCT_Y] = passageABB.getMax().y();

        bb._mins[OCT_XY] = bb._mins[OCT_X];
        bb._maxs[OCT_XY] = bb._maxs[OCT_X];
        bb._mins[OCT_YX] = bb._mins[OCT_Y];
        bb._maxs[OCT_YX] = bb._maxs[OCT_Y];

        //TODO: should be mesh highest elevation at OCT_X and OCT_Y
        bb._mins[OCT_Z] = -100.0f;
        bb._maxs[OCT_Z] = 100.0f;

        Renderer3D::renderOctBB(bb, true, false);
    }    
}

//--------------------------------------------------------------------------------------------
// gfx_config_t FUNCTIONS
//--------------------------------------------------------------------------------------------
void gfx_config_t::download(gfx_config_t& self, egoboo_config_t& cfg)
{
    // Load GFX configuration values, even if no Egoboo configuration is provided.
    init(self);

    self.antialiasing = cfg.graphic_antialiasing.getValue() > 0;

    self.refon = cfg.graphic_reflections_enable.getValue();

    self.shadows_enable = cfg.graphic_shadows_enable.getValue();
    self.shadows_highQuality_enable = !cfg.graphic_shadows_highQuality_enable.getValue();

    self.gouraudShading_enable = cfg.graphic_gouraudShading_enable.getValue();
    self.dither = cfg.graphic_dithering_enable.getValue();
    self.perspective = cfg.graphic_perspectiveCorrection_enable.getValue();
    self.phongon = cfg.graphic_specularHighlights_enable.getValue();

    self.draw_background = cfg.graphic_background_enable.getValue();
    self.draw_overlay = cfg.graphic_overlay_enable.getValue();

    self.dynalist_max = Ego::Math::constrain(cfg.graphic_simultaneousDynamicLights_max.getValue(), (uint16_t)0, (uint16_t)TOTAL_MAX_DYNA);
}

void gfx_config_t::init(gfx_config_t& self)
{
    self.gouraudShading_enable = true;
    self.refon = true;
    self.antialiasing = false;
    self.dither = false;
    self.perspective = false;
    self.phongon = true;
    self.shadows_enable = true;
    self.shadows_highQuality_enable = true;

    self.draw_background = false;
    self.draw_overlay = false;
    self.draw_water_0 = true;
    self.draw_water_1 = true;

    self.dynalist_max = 8;
}

//--------------------------------------------------------------------------------------------
// grid_lighting FUNCTIONS
//--------------------------------------------------------------------------------------------
float GridIllumination::grid_lighting_test(const ego_mesh_t& mesh, GLXvector3f pos, float& low_diff, float& hgh_diff)
{
    const lighting_cache_t *cache_list[4];

    int ix = std::floor(pos[XX] / Info<float>::Grid::Size());
    int iy = std::floor(pos[YY] / Info<float>::Grid::Size());

    Index1D fan[4];
    fan[0] = mesh.getTileIndex(Index2D(ix, iy));
    fan[1] = mesh.getTileIndex(Index2D(ix + 1, iy));
    fan[2] = mesh.getTileIndex(Index2D(ix, iy + 1));
    fan[3] = mesh.getTileIndex(Index2D(ix + 1, iy + 1));

    for (size_t cnt = 0; cnt < 4; cnt++)
    {
        cache_list[cnt] = nullptr;
		if (fan[cnt] == Index1D::Invalid) {
			cache_list[cnt] = nullptr;
		} else {
			cache_list[cnt] = &(mesh.getTileInfo(fan[cnt])._cache);
		}
    }

    float u = pos[XX] / Info<float>::Grid::Size() - ix;
    float v = pos[YY] / Info<float>::Grid::Size() - iy;

    return lighting_cache_test(cache_list, u, v, low_diff, hgh_diff);
}

//--------------------------------------------------------------------------------------------
float GridIllumination::light_corners(ego_mesh_t& mesh, ego_tile_info_t& tile, bool reflective, float mesh_lighting_keep)
{
	// if no update is requested, return an "error value"
	if (!tile._lightingCache.getNeedUpdate())
	{
		return -1.0f;
	}

	// has the lighting already been calculated this frame?
	if (tile._lightingCache.isValid(_gameEngine->getNumberOfFramesRendered()))
	{
		return -1.0f;
	}

	// get the normal and lighting cache for this tile
	tile_mem_t& ptmem = mesh._tmem;
	normal_cache_t& ncache = tile._ncache;
	light_cache_t& lcache = tile._lightingCache._contents;
	light_cache_t& d1_cache = tile._vertexLightingCache._d1_cache;
	light_cache_t& d2_cache = tile._vertexLightingCache._d2_cache;

	float max_delta = 0.0f;
	for (size_t corner = 0; corner < 4; corner++)
	{
		GLXvector3f& pnrm = ncache[corner];
		float& plight = lcache[corner];
		float& pdelta1 = d1_cache[corner];
		float& pdelta2 = d2_cache[corner];
		GLXvector3f& ppos = ptmem._plst[tile._vrtstart + corner];

		float light_old, delta, light_tmp;
		float light_new = 0.0f;
		light_one_corner(mesh, tile, reflective, 
			             Vector3f(ppos[0], ppos[1], ppos[2]),
			             Vector3f(pnrm[0], pnrm[1], pnrm[2]),
			             light_new);

		if (plight != light_new)
		{
			light_old = plight;
			plight = light_old * mesh_lighting_keep + light_new * (1.0f - mesh_lighting_keep);

			// measure the actual delta
			delta = std::abs(light_old - plight);

			// measure the relative change of the lighting
			light_tmp = 0.5f * (std::abs(plight) + std::abs(light_old));
			if (0.0f == light_tmp)
			{
				delta = 10.0f;
			}
			else
			{
				delta /= light_tmp;
				delta = Ego::Math::constrain(delta, 0.0f, 10.0f);
			}

			// add in the actual change this update
			pdelta2 += std::abs(delta);

			// update the estimate to match the actual change
			pdelta1 = pdelta2;
		}

		max_delta = std::max(max_delta, pdelta1);
	}

	// un-mark the lcache
	tile._lightingCache.setNeedUpdate(false);
	tile._lightingCache.setLastFrame(_gameEngine->getNumberOfFramesRendered());

	return max_delta;
}

bool GridIllumination::grid_lighting_interpolate(const ego_mesh_t& mesh, lighting_cache_t& dst, const Vector2f& pos)
{
    // grab this tile's coordinates
    int ix = std::floor(pos[XX] / Info<float>::Grid::Size()),
        iy = std::floor(pos[YY] / Info<float>::Grid::Size());

    // find the tile id for the surrounding tiles
	Index1D fan[4];
    fan[0] = mesh.getTileIndex(Index2D(ix, iy));
    fan[1] = mesh.getTileIndex(Index2D(ix + 1, iy));
    fan[2] = mesh.getTileIndex(Index2D(ix, iy + 1));
    fan[3] = mesh.getTileIndex(Index2D(ix + 1, iy + 1));

	std::array<const lighting_cache_t *,4> cache_list;
    for (size_t cnt = 0; cnt < 4; cnt++)
    {
		if (fan[cnt] == Index1D::Invalid) {
			cache_list[cnt] = nullptr;
		} else {
			cache_list[cnt] = &(mesh.getTileInfo(fan[cnt])._cache);
		}
    }

    // grab the coordinates relative to the parent tile
	float u = pos[XX] / Info<float>::Grid::Size() - ix,
		  v = pos[YY] / Info<float>::Grid::Size() - iy;

    return lighting_cache_t::lighting_cache_interpolate(dst, cache_list, u, v);
}

void GridIllumination::test_one_corner(const ego_mesh_t& mesh, GLXvector3f pos, float& pdelta)
{
	// interpolate the lighting for the given corner of the mesh
	float low_delta, hgh_delta;
	pdelta = grid_lighting_test(mesh, pos, low_delta, hgh_delta);

	// determine the weighting
	float hgh_wt, low_wt;
	hgh_wt = (pos[ZZ] - mesh._tmem._bbox.getMin()[kZ]) / (mesh._tmem._bbox.getMax()[kZ] - mesh._tmem._bbox.getMin()[kZ]);
	hgh_wt = Ego::Math::constrain(hgh_wt, 0.0f, 1.0f);
	low_wt = 1.0f - hgh_wt;

	pdelta = low_wt * low_delta + hgh_wt * hgh_delta;
}

bool GridIllumination::test_corners(const ego_mesh_t& mesh, ego_tile_info_t& tile, float threshold)
{
	if (threshold < 0.0f) threshold = 0.0f;

	// get the lighting and per-vertex lighting cache for this tile
	light_cache_t& lcache = tile._lightingCache._contents;
	light_cache_t& d1_cache = tile._vertexLightingCache._d1_cache;

	bool retval = false;
	for (size_t corner = 0; corner < 4; corner++)
	{
		float& pdelta = d1_cache[corner];
		float& plight = lcache[corner];
		GLXvector3f& ppos = mesh._tmem._plst[tile._vrtstart + corner];

		float delta;
		test_one_corner(mesh, ppos, delta);

		if (0.0f == plight)
		{
			delta = 10.0f;
		}
		else
		{
			delta /= plight;
			delta = Ego::Math::constrain(delta, 0.0f, 10.0f);
		}

		pdelta += delta;

		if (pdelta > threshold)
		{
			retval = true;
		}
	}

	return retval;
}

void GridIllumination::light_one_corner(ego_mesh_t& mesh, ego_tile_info_t& tile, const bool reflective, const Vector3f& pos, const Vector3f& nrm, float& plight)
{
	// interpolate the lighting for the given corner of the mesh
	lighting_cache_t grid_light;
	grid_lighting_interpolate(mesh, grid_light, Vector2f(pos[kX], pos[kY]));

	if (reflective) {
		float light_dir, light_amb;
		lighting_cache_t::lighting_evaluate_cache(grid_light, nrm, pos[ZZ], mesh._tmem._bbox, &light_amb, &light_dir);

		// make ambient light only illuminate 1/2
		plight = light_amb + 0.5f * light_dir;
	} else {
		plight = lighting_cache_t::lighting_evaluate_cache(grid_light, nrm, pos[ZZ], mesh._tmem._bbox, NULL, NULL);
	}
}

bool GridIllumination::light_corner(ego_mesh_t& mesh, const Index1D& fan, float height, float nrm[], float& plight)
{
	ego_tile_info_t& ptile = mesh.getTileInfo(fan);

	// get the grid lighting
	const lighting_cache_t& lighting = ptile._cache;

	bool reflective = (0 != ptile.testFX(MAPFX_REFLECTIVE));

	// evaluate the grid lighting at this node
	if (reflective)
	{
		float light_dir, light_amb;

		lighting_cache_t::lighting_evaluate_cache(lighting, Vector3f(nrm[0], nrm[1], nrm[2]), height, mesh._tmem._bbox, &light_amb, &light_dir);

		// make ambient light only illuminate 1/2
		plight = light_amb + 0.5f * light_dir;
	} else {
		plight = lighting_cache_t::lighting_evaluate_cache(lighting, Vector3f(nrm[0], nrm[1], nrm[2]), height, mesh._tmem._bbox, NULL, NULL);
	}

	// clip the light to a reasonable value
	plight = Ego::Math::constrain(plight, 0.0f, 255.0f);

	return true;
}

//--------------------------------------------------------------------------------------------
// MODE CONTROL
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
void gfx_do_clear_screen()
{
    auto& renderer = Ego::Renderer::get();
    // Clear the depth buffer.
    renderer.setDepthWriteEnabled(true);
	renderer.getDepthBuffer().clear();
    // Clear the colour buffer.
	renderer.getColourBuffer().clear();
}

//--------------------------------------------------------------------------------------------
void gfx_do_flip_pages()
{
    Ego::Core::ConsoleHandler::get().draw_all();
    SDL_GL_SwapWindow(Ego::GraphicsSystem::window->get());
}

//--------------------------------------------------------------------------------------------
// LIGHTING FUNCTIONS
//--------------------------------------------------------------------------------------------
gfx_rv GridIllumination::light_fans_throttle_update(ego_mesh_t * mesh, ego_tile_info_t& tile, const Index1D& tileIndex, float threshold)
{
    bool       retval = false;

    if (!mesh)
    {
		throw Id::RuntimeErrorException(__FILE__, __LINE__, "nullptr == mesh");
    }
	tile_mem_t& tmem = mesh->_tmem;

#if defined(CLIP_LIGHT_FANS) && !defined(CLIP_ALL_LIGHT_FANS)

    // visible fans based on the update "need"
    retval = test_corners(*mesh, tile, threshold);

    // update every 4 fans even if there is no need
    if (!retval)
    {
        // use a kind of checkerboard pattern
        auto i2 = Grid::map<int>(tileIndex, (int)tmem.getInfo().getTileCountX());
        if (0 != (((i2.x() ^ i2.y()) + _gameEngine->getNumberOfFramesRendered()) & 0x03))
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
void GridIllumination::light_fans_update_lcache(Ego::Graphics::TileList& tl)
{
	const int frame_skip = 1 << 2; // 1 << 2 ~ 2^2 ~ 4. 
#if defined(CLIP_ALL_LIGHT_FANS)
	const int frame_mask = frame_skip - 1; // 4 - 1 = 3 = binary(11).
#endif

	/// @note we are measuring the change in the intensity at the corner of a tile (the "delta") as
	/// a fraction of the current intensity. This is because your eye is much more sensitive to
	/// intensity differences when the intensity is low.
	///
	/// @note it is normally assumed that 64 colors of gray can make a smoothly colored black and white picture
	/// which means that the threshold could be set as low as 1/64 = 0.015625.
	const float delta_threshold = 0.05f;
	
	bool is_valid;

	auto mesh = tl.getMesh();
	if (!mesh)
	{
		throw Id::RuntimeErrorException(__FILE__, __LINE__, "tile list not attached to a mesh");
	}

#if defined(CLIP_ALL_LIGHT_FANS)
	// Update all visible fans once every 4 frames.
	if (0 != (_gameEngine->getNumberOfFramesRendered() & frame_mask)) {
		return;
}
#endif

#if !defined(CLIP_LIGHT_FANS)
    // update only every frame
	float local_mesh_lighting_keep = 0.9f;
#else
    // update only every 4 frames
	float local_mesh_lighting_keep = std::pow(0.9f, frame_skip);
#endif

    // cache the grid lighting
    for (size_t entry = 0; entry < tl._all.size(); entry++)
    {
        // which tile?
        Index1D fan = tl._all[entry].getIndex();

        // grab a pointer to the tile
		ego_tile_info_t& ptile = mesh->getTileInfo(fan);

        // Test to see whether the lcache was already updated
        // - ptile->_lcache_frame < 0 means that the cache value is invalid.
        // - ptile->_lcache_frame is updated inside ego_mesh_light_corners()
#if defined(CLIP_LIGHT_FANS)
        // clip the updated on each individual tile
        is_valid = ptile._lightingCache.isValid(_gameEngine->getNumberOfFramesRendered(), frame_skip);
#else
        // let the function clip all tile updates
        is_valid = ptile._lightingCache.isValid(_gameEngine->getNumberOfFramesRendered());
#endif
	if (is_valid)
        {
            continue;
        }

        // If no update was requested ...
        if (!ptile._lightingCache.getNeedUpdate())
        {
			// ... do we need one?
            gfx_rv light_fans_rv = light_fans_throttle_update(mesh.get(), ptile, fan, delta_threshold);
            ptile._lightingCache.setNeedUpdate(gfx_success == light_fans_rv);
        }

		// If there is still no need for an update, go to the next tile.
		if (!ptile._lightingCache.getNeedUpdate()) {
			continue;
		}

        // is the tile reflective?
        bool reflective = (0 != ptile.testFX(MAPFX_REFLECTIVE));

        // light the corners of this tile
        float delta = GridIllumination::light_corners(*mesh, ptile, reflective, local_mesh_lighting_keep);

#if defined(CLIP_LIGHT_FANS)
        // Use the actual maximum change in the intensity at a tile corner to
        // signal whether we need to calculate the next stage.
        ptile._vertexLightingCache.setNeedUpdate(delta > delta_threshold);
#else
        // make sure that ego_mesh_light_corners() did not return an "error value"
        ptile._vertexLightingCache.setNeedUpdate(delta > 0.0f);
#endif
    }
}

//--------------------------------------------------------------------------------------------

float GridIllumination::grid_get_mix(float u0, float u, float v0, float v) {
    // Get the distance of u and v from u0 and v0.
    float du = u - u0,
        dv = v - v0;

    // If the absolute distance du or dv is greater than 1,
    // return 0.
    if (std::abs(du) > 1.0f || std::abs(dv) > 1.0f) {
        return 0.0f;
    }
    // The distances are within the bounds of [-1,+1] at this point.
    // The original formulas are
    // wt_u = (1.0f - du)*(1.0f + du)
    // wt_v = (1.0f - dv)*(1.0f + dv)
    // However, a term of the form
    // y = (1 - x) * (1 + x)
    // can be simplified to
    // y = (1 - x) * 1 + (1 - x) * x
    //   = (1 - x) + (1 - x) * x
    //   = 1 - x + x - x^2
    //   = 1 - x^2
    // Hence the original formulas become
    // wt_u = 1.0f - du * du)
    // wt_v = 1.0f - dv * dv
    float wt_u = 1.0f - du * du,
        wt_v = 1.0f - dv * dv;

    return wt_u * wt_v;
}

float GridIllumination::ego_mesh_interpolate_vertex(const ego_tile_info_t& info, const GLXvector3f& position) {
    const oct_bb_t& boundingBox = info._oct;
    const light_cache_t& lightCache = info._lightingCache._contents;

    // Set the lighting to 0.
    float light = 0.0f;

    // Determine texture coordinates of the specified point.
    float u = (position[XX] - boundingBox._mins[OCT_X]) / (boundingBox._maxs[OCT_X] - boundingBox._mins[OCT_X]);
    float v = (position[YY] - boundingBox._mins[OCT_Y]) / (boundingBox._maxs[OCT_Y] - boundingBox._mins[OCT_Y]);

    // Interpolate the lighting at the four vertices of the tile.
    // to determine the final lighting at the specified point.
    float weightedSum = 0.0f;
    for (size_t i = 0; i < 4; ++i) {
        // Mix the u, v coordinate pairs (0,0),
        // (1,0), (1,1), and (0,1) using the
        // texture coordinates of the specified
        // point.
        static const float ix_off[4] = {0.0f, 1.0f, 1.0f, 0.0f},
            iy_off[4] = {0.0f, 0.0f, 1.0f, 1.0f};
        float mix = grid_get_mix(ix_off[i], u, iy_off[i], v);

        weightedSum += mix;
        light += mix * lightCache[i];
    }

    // Normalize to the weighted sum.
    if (light > 0.0f && weightedSum > 0.0f) {
        light /= weightedSum;
        light = Ego::Math::constrain(light, 0.0f, 255.0f);
    } else {
        light = 0.0f;
    }
    return light;
}

void GridIllumination::light_fans_update_clst(Ego::Graphics::TileList& tl)
{
    /// @author BB
    /// @details update the tile's color list, if needed
    auto mesh = tl.getMesh();
    if (!mesh)
    {
		throw Id::RuntimeErrorException(__FILE__, __LINE__, "tile list is not attached to a mesh");
    }

    // alias the tile memory
	tile_mem_t& ptmem = mesh->_tmem;

    // use the grid to light the tiles
    for (size_t entry = 0; entry < tl._all.size(); entry++)
    {
        Index1D fan = tl._all[entry].getIndex();
        if (Index1D::Invalid == fan) continue;

        // valid tile?
		ego_tile_info_t& ptile = mesh->getTileInfo(fan);

        // Do nothing if this tile does not need an update.
        if (!ptile._vertexLightingCache.getNeedUpdate()) {
            continue;
        }

        // Do nothing if the update was performed in this frame.
        if (ptile._vertexLightingCache.isValid(_gameEngine->getNumberOfFramesRendered())) {
            continue;
        }

		size_t numberOfVertices;
		tile_definition_t *pdef = tile_dict.get(ptile._type);
        if (nullptr != pdef) {
			numberOfVertices = pdef->numvertices;
        } else {
			numberOfVertices = 4;
        }

		size_t index, vertex;
        // copy the 1st 4 vertices
        for (index = 0, vertex = ptile._vrtstart; index < 4; index++, vertex++)
        {
            GLXvector3f& color = ptmem._clst[vertex];
            float light = ptile._lightingCache._contents[index];
			color[RR] = color[GG] = color[BB] 
				= INV_FF<float>() * Ego::Math::constrain(light, 0.0f, 255.0f);
        }

        for ( /* Intentionall left empty. */; index < numberOfVertices; index++, vertex++)
        {
			GLXvector3f& color = ptmem._clst[vertex];
			const GLXvector3f& position = ptmem._plst[vertex];
			float light = ego_mesh_interpolate_vertex(ptile, position);
			color[RR] = color[GG] = color[BB] 
				= INV_FF<float>() * Ego::Math::constrain(light, 0.0f, 255.0f);
        }

        // clear out the deltas
        ptile._vertexLightingCache._d1_cache.fill(0.0f);
        ptile._vertexLightingCache._d2_cache.fill(0.0f);

        // This tile was updated this frame and does not require an update (for some time).
		ptile._vertexLightingCache.setNeedUpdate(false);
		ptile._vertexLightingCache._lastFrame = _gameEngine->getNumberOfFramesRendered();
    }
}

//--------------------------------------------------------------------------------------------
void GridIllumination::light_fans(Ego::Graphics::TileList& tl)
{
	light_fans_update_lcache(tl);
	light_fans_update_clst(tl);
}

//--------------------------------------------------------------------------------------------
float get_ambient_level()
{
    /// @author BB
    /// @details get the actual global ambient level
    float glob_amb = 0.0f;
    float min_amb = 0.0f;
    if (gfx.usefaredge)
    {
        // for outside modules, max light_a means bright sunlight
        // this should be handled with directional lighting, so ambient light is 0
        glob_amb = light_a * 255.0f;
        //glob_amb = 0;
    }
    else
    {
        // for inside modules, max light_a means dingy dungeon lighting
        glob_amb = light_a * 64.0f;
    }

    // determine the minimum ambient, based on darkvision
    min_amb = INVISIBLE / 4;
    if (local_stats.seedark_mag > 0.0f)
    {
        // give a iny boost in the case of no light
        // start with the global light
        min_amb = std::max(glob_amb, min_amb) + 1.0f;

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
void dynalist_t::init(dynalist_t& self) {
    self.size = 0;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_make_dynalist(dynalist_t& dyl, Camera& cam)
{
    /// @author ZZ
    /// @details This function figures out which particles are visible, and it sets up dynamic
    ///    lighting

    size_t    tnc;
	Vector3f  vdist;

    float         distance = 0.0f;
    dynalight_data_t * plight = NULL;

    float         distance_max = 0.0f;
    dynalight_data_t * plight_max = NULL;

    // HACK: if dynalist is ahead of the game by 30 frames or more, reset and force an update
    if ((Uint32)(dyl.frame + 30) >= _gameEngine->getNumberOfFramesRendered())
        dyl.frame = -1;

    // do not update the dynalist more than once a frame
    if (dyl.frame >= 0 && (Uint32)dyl.frame >= _gameEngine->getNumberOfFramesRendered())
    {
        return gfx_success;
    }

    // Don't really make a list, just set to visible or not
    dynalist_t::init(dyl);

    for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
    {
        if(particle->isTerminated()) continue;
        
        dynalight_info_t& pprt_dyna = particle->dynalight;

        // is the light on?
        if (!pprt_dyna.on || 0.0f == pprt_dyna.level) continue;

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
                    distance_max = plight_max->distance;
                }
            }
        }

        if (NULL != plight)
        {
            plight->distance = distance;
            plight->pos = particle->getPosition();
            plight->level = pprt_dyna.level;
            plight->falloff = pprt_dyna.falloff;
        }
    }

    // the list is updated, so update the frame count
    dyl.frame = _gameEngine->getNumberOfFramesRendered();

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv GridIllumination::do_grid_lighting(Ego::Graphics::TileList& tl, dynalist_t& dyl, Camera& cam)
{
    /// @author ZZ
    /// @details Do all tile lighting, dynamic and global

    size_t cnt;

    int    tnc;

    float x0, y0, local_keep;
    bool needs_dynalight;

    std::array<float, LIGHTING_VEC_SIZE> global_lighting = {0};

    size_t               reg_count = 0;
    dynalight_registry_t reg[TOTAL_MAX_DYNA];

    ego_frect_t mesh_bound, light_bound;
    dynalight_data_t fake_dynalight;

	auto mesh = tl.getMesh();
    if (!mesh)
    {
		throw Id::RuntimeErrorException(__FILE__, __LINE__, "tile list not attached to a mesh");
    }

	Ego::MeshInfo& pinfo = mesh->_info;
	tile_mem_t& tmem = mesh->_tmem;

    // find a bounding box for the "frustum"
    mesh_bound.xmin = tmem._edge_x;
    mesh_bound.xmax = 0;
    mesh_bound.ymin = tmem._edge_y;
    mesh_bound.ymax = 0;
    for (size_t entry = 0; entry < tl._all.size(); entry++)
    {
        Index1D fan = tl._all[entry].getIndex();
        if (fan.i() >= pinfo.getTileCount()) continue;

		const oct_bb_t& poct = tmem.get(fan)._oct;

        mesh_bound.xmin = std::min(mesh_bound.xmin, poct._mins[OCT_X]);
        mesh_bound.xmax = std::max(mesh_bound.xmax, poct._maxs[OCT_X]);
        mesh_bound.ymin = std::min(mesh_bound.ymin, poct._mins[OCT_Y]);
        mesh_bound.ymax = std::max(mesh_bound.ymax, poct._maxs[OCT_Y]);
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
    dynalight_data_t::init(fake_dynalight);

    // initialize the light_bound
    light_bound.xmin = tmem._edge_x;
    light_bound.xmax = 0;
    light_bound.ymin = tmem._edge_y;
    light_bound.ymax = 0;

    // make bounding boxes for each dynamic light
    if (gfx.gouraudShading_enable)
    {
        for (cnt = 0; cnt < dyl.size; cnt++)
        {
            float radius;
            ego_frect_t ftmp;

            dynalight_data_t& pdyna = dyl.lst[cnt];

            if (pdyna.falloff <= 0.0f || 0.0f == pdyna.level) continue;

            radius = std::sqrt(pdyna.falloff * 765.0f * 0.5f);

            // find the intersection with the frustum boundary
            ftmp.xmin = std::max(pdyna.pos[kX] - radius, mesh_bound.xmin);
            ftmp.xmax = std::min(pdyna.pos[kX] + radius, mesh_bound.xmax);
            ftmp.ymin = std::max(pdyna.pos[kY] - radius, mesh_bound.ymin);
            ftmp.ymax = std::min(pdyna.pos[kY] + radius, mesh_bound.ymax);

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

        // evaluate all the lights at the camera position
        for (cnt = 0; cnt < dyl.size; cnt++)
        {
			dynalight_data_t& pdyna = dyl.lst[cnt];

            // evaluate the intensity at the camera
			Vector3f diff = pdyna.pos - cam.getCenter() - Vector3f(0.0f, 0.0f, 90.0f); // evaluate at the "head height" of a character

            dyna_weight = std::abs(dyna_lighting_intensity(&pdyna, diff));

            fake_dynalight.distance += dyna_weight * pdyna.distance;
            fake_dynalight.falloff += dyna_weight * pdyna.falloff;
            fake_dynalight.level += dyna_weight * pdyna.level;
            fake_dynalight.pos += (pdyna.pos - cam.getCenter()) * dyna_weight;

            dyna_weight_sum += dyna_weight;
        }

        // use a single dynalight to represent the sum of all dynalights
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
    local_keep = 0.0f; //std::pow(DYNALIGHT_KEEP, 4); //const static float DYNALIGHT_KEEP = 0.9f;

    // Add to base light level in normal mode
    for (size_t entry = 0; entry < tl._all.size(); entry++)
    {
        bool resist_lighting_calculation = true;

        // grab each grid box in the "frustum"
        Index1D fan = tl._all[entry].getIndex();

        // a valid tile?
        ego_tile_info_t& ptile = mesh->getTileInfo(fan);

        // do not update this more than once a frame
        if (ptile._cache_frame >= 0 && (uint32_t)ptile._cache_frame >= _gameEngine->getNumberOfFramesRendered()) continue;
        auto i2 = Grid::map<int>(fan, pinfo.getTileCountX());
        // Resist the lighting calculation?
        // This is a speedup for lighting calculations so that
        // not every light-tile calculation is done every single frame
        resist_lighting_calculation = (0 != (((i2.x() + i2.y()) ^ _gameEngine->getNumberOfFramesRendered()) & 0x03));

        if (resist_lighting_calculation) continue;

        // this is not a "bad" grid box, so grab the lighting info
        lighting_cache_t& pcache_old = ptile._cache;

        lighting_cache_t cache_new;
        cache_new.init(); /// @todo Not needed because of constructor.

        // copy the global lighting
        for (tnc = 0; tnc < LIGHTING_VEC_SIZE; tnc++)
        {
            cache_new.low._lighting[tnc] = global_lighting[tnc];
            cache_new.hgh._lighting[tnc] = global_lighting[tnc];
        };

        // do we need any dynamic lighting at all?
        if (needs_dynalight)
        {
            // calculate the local lighting

            ego_frect_t fgrid_rect;

            x0 = i2.x() * Info<float>::Grid::Size();
            y0 = i2.y() * Info<float>::Grid::Size();

            // check this grid vertex relative to the measured light_bound
            fgrid_rect.xmin = x0 - Info<float>::Grid::Size() * 0.5f;
            fgrid_rect.xmax = x0 + Info<float>::Grid::Size() * 0.5f;
            fgrid_rect.ymin = y0 - Info<float>::Grid::Size() * 0.5f;
            fgrid_rect.ymax = y0 + Info<float>::Grid::Size() * 0.5f;

            // check the bounding box of this grid vs. the bounding box of the lighting
            if (fgrid_rect.xmin <= light_bound.xmax && fgrid_rect.xmax >= light_bound.xmin)
            {
                if (fgrid_rect.ymin <= light_bound.ymax && fgrid_rect.ymax >= light_bound.ymin)
                {
                    // this grid has dynamic lighting. add it.
                    for (cnt = 0; cnt < reg_count; cnt++)
                    {
						Vector3f nrm;
                        dynalight_data_t *pdyna;

                        // does this dynamic light intersects this grid?
                        if (fgrid_rect.xmin > reg[cnt].bound.xmax || fgrid_rect.xmax < reg[cnt].bound.xmin) continue;
                        if (fgrid_rect.ymin > reg[cnt].bound.ymax || fgrid_rect.ymax < reg[cnt].bound.ymin) continue;

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
                        nrm[kZ] = pdyna->pos[kZ] - tmem._bbox.getMin()[ZZ];
                        sum_dyna_lighting(pdyna, cache_new.low._lighting, nrm);

                        nrm[kZ] = pdyna->pos[kZ] - tmem._bbox.getMax()[ZZ];
                        sum_dyna_lighting(pdyna, cache_new.hgh._lighting, nrm);
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
        lighting_cache_t::blend(pcache_old, cache_new, local_keep);

        // find the max intensity
        pcache_old.max_light();

        ptile._cache_frame = _gameEngine->getNumberOfFramesRendered();
    }

    return gfx_success;
}
//--------------------------------------------------------------------------------------------
gfx_rv gfx_make_tileList(Ego::Graphics::TileList& tl, Camera& cam)
{
	// @a true if clipping is enabled, @a false otherwise.
	static const bool clippingEnabled = true;

    // Because the main loop of the program will always flip the
    // page before rendering the 1st frame of the actual game,
    // _gameEngine->getNumberOfFramesRendered() will always start at 1
    if (1 != (_gameEngine->getNumberOfFramesRendered() & 3))
    {
        return gfx_success;
    }

    // reset the renderlist
    tl.reset();

    // get the tiles in the center of the view (TODO: calculate actual tile view from camera frustrum)
	int startX, startY, endX, endY;
	if (clippingEnabled)
	{
		static const float offset = 10;
		float centerX = cam.getTrackPosition()[kX] / Info<float>::Grid::Size();
		float centerY = cam.getTrackPosition()[kY] / Info<float>::Grid::Size();
		startX = Ego::Math::constrain<int>(centerX - offset, 0, _currentModule->getMeshPointer()->_info.getTileCountX());
		startY = Ego::Math::constrain<int>(centerY - offset, 0, _currentModule->getMeshPointer()->_info.getTileCountY());
		endX = Ego::Math::constrain<int>(centerX + offset, 0, _currentModule->getMeshPointer()->_info.getTileCountX());
		endY = Ego::Math::constrain<int>(centerY + offset, 0, _currentModule->getMeshPointer()->_info.getTileCountY());
	}
	else
	{
		startX = 0;
		startY = 0;
		endX = _currentModule->getMeshPointer()->_info.getTileCountX();
		endY = _currentModule->getMeshPointer()->_info.getTileCountY();
	}
    for(size_t x = startX; x < endX; ++x) {
        for(size_t y = startY; y < endY; ++y) {
            if (gfx_error == tl.add(x + y * _currentModule->getMeshPointer()->_info.getTileCountX(), cam))
            {
                return gfx_error;
            }        
        }
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_make_entityList(Ego::Graphics::EntityList& el, Camera& cam)
{
    // Remove all entities from the entity list.
    el.clear();

    // collide the characters with the frustum
    std::vector<std::shared_ptr<Object>> visibleObjects = 
        _currentModule->getObjectHandler().findObjects(
            cam.getCenter()[kX], 
            cam.getCenter()[kY], 
			Info<float>::Grid::Size() * 10,  //@todo: use camera view size here instead
            true);

    for(const std::shared_ptr<Object> object : visibleObjects) {
        el.add(cam, *object.get());
    }

    for(const std::shared_ptr<Ego::Particle> particle : ParticleHandler::get().iterator()) {
        el.add(cam, *particle.get());
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
// UTILITY FUNCTIONS
#if 0
float calc_light_rotation(int rotation, int normal)
{
    /// @author ZZ
    /// @details This function helps make_lighttable
	Vector3f nrm, nrm2;
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
	Vector3f nrm, nrm2;
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
        Log::Entry e(Log::Level::Error, __FILE__, __LINE__);
        e << "invalid entity list size" << Log::EndOfEntry;
        Log::get() << e;
        return gfx_error;
    }

    retval = gfx_success;
    for (size_t i = 0, n = el.getSize(); i < n; ++i)
    {
        float tmp_seekurse_level;

        ObjectRef iobj = el.get(i).iobj;

        const std::shared_ptr<Object> &object = _currentModule->getObjectHandler()[iobj];
        if (!object) continue;

        // Do flashing
        if (DONTFLASH != object->getProfile()->getFlashAND())
        {
            if (HAS_NO_BITS(_gameEngine->getNumberOfFramesRendered(), object->getProfile()->getFlashAND()))
            {
				object->inst.flash(255);
            }
        }

        // Do blacking
        // having one holy player in your party will cause the effect, BUT
        // having some non-holy players will dilute it
        tmp_seekurse_level = std::min(local_stats.seekurse_level, 1.0f);
        if ((local_stats.seekurse_level > 0.0f) && object->iskursed && 1.0f != tmp_seekurse_level)
        {
            if (HAS_NO_BITS(_gameEngine->getNumberOfFramesRendered(), SEEKURSEAND))
            {
				object->inst.flash(255.0f *(1.0f - tmp_seekurse_level));
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_update_all_chr_instance()
{
    gfx_rv retval;

    // assume the best
    retval = gfx_success;

    for (const std::shared_ptr<Object> &pchr : _currentModule->getObjectHandler().iterator())
    {
        //Dont do terminated characters
        if (pchr->isTerminated()) {
            continue;
        }

        //Skip objects outside the map
		auto mesh = _currentModule->getMeshPointer();
        if (!mesh->grid_is_valid(pchr->getTile())) continue;

        // make sure that the vertices are interpolated
        if(pchr->inst.updateVertices(-1, -1, true) == gfx_error) {
            retval = gfx_error;
        }

        // the instance has changed, refresh the collision bound
        else {
            pchr->getObjectPhysics().updateCollisionSize(true);            
        }

        // do the basic lighting
        pchr->inst.updateLighting();
    }

    return retval;
}

// variables to optimize calls to bind the textures
bool TileRenderer::disableTexturing = false;
TX_REF TileRenderer::image = Ego::Graphics::MESH_IMG_COUNT;
uint8_t TileRenderer::size = 0xFF;

std::shared_ptr<Ego::Texture> TileRenderer::get_texture(uint8_t image, uint8_t size)
{
	if (0 == size) {
		return Ego::Graphics::TextureAtlasManager::get().getSmall(image);
	} else if (1 == size) {
		return Ego::Graphics::TextureAtlasManager::get().getBig(image);
	}  else {
        return nullptr;
    }
}

void TileRenderer::invalidate()
{
	image = Ego::Graphics::MESH_IMG_COUNT;
	size = 0xFF;
}

void TileRenderer::bind(const ego_tile_info_t& tile)
{
	uint8_t newImage, newSize;
    std::shared_ptr<Ego::Texture> texture = nullptr;
	bool needsBinding = false;

	// Disable texturing.
	if (disableTexturing)
	{
        needsBinding = true;
		TileRenderer::invalidate();
	}
	else
	{
		newImage = TILE_GET_LOWER_BITS(tile._img);
		newSize = (tile._type < tile_dict.offset) ? 0 : 1;

		if ((image != newImage) || (size != newSize))
		{
			texture = get_texture(newImage, newSize);
            needsBinding = true;

			image = newImage;
			size = newSize;
		}
	}

	if (needsBinding)
	{
		Ego::Renderer::get().getTextureUnit().setActivated(texture.get());
		if (texture && texture->hasAlpha())
		{
			// MH: Enable alpha blending if the texture requires it.
			Ego::Renderer::get().setBlendingEnabled(true);
			Ego::Renderer::get().setBlendFunction(Ego::BlendFunction::One, Ego::BlendFunction::OneMinusSourceAlpha);
		}
	}
}
