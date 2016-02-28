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

/// @file  egolib/egoboo_setup.c
/// @brief Functions for handling the <tt>setup.txt</tt> file.
/// @details

#include "egolib/egoboo_setup.h"

#include "egolib/_math.h"
#include "game/Graphics/Camera.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static bool _setup_started = false;
static std::string _config_filename = "";
std::shared_ptr<ConfigFile> _lpConfigSetup = nullptr;

egoboo_config_t egoboo_config_t::_singleton;

egoboo_config_t& egoboo_config_t::get()
{
    return _singleton;
}

egoboo_config_t::egoboo_config_t() :
    // Graphic configuration section.
    graphic_fullscreen(true,"graphic.fullscreen","enable/disable fullscreen mode"),
    graphic_colorBuffer_bitDepth(24,"graphic.colourBuffer.bitDepth","bit depth of the colour buffer"),
    graphic_depthBuffer_bitDepth(8,"graphic.depthBuffer.bitDepth","bit depth of the depth buffer"),
    graphic_resolution_horizontal(800,"graphic.resolution.horizontal", "horizontal resolution"),
    graphic_resolution_vertical(600,"graphic.resolution.vertical", "vertical resolution"),
    graphic_perspectiveCorrection_enable(false,"graphic.perspectiveCorrection.enable","enable/displable perspective correction"),
    graphic_dithering_enable(false, "graphic.dithering.enable","enable/disable dithering"),
    graphic_reflections_enable(true, "graphic.reflections.enable","enable/disable reflections"),
    graphic_reflections_particleReflections_enable(true, "graphic.reflections.particleReflections.enable", "enable/disable particle reflections"),
    graphic_shadows_enable(true, "graphic.shadows.enable", "enable/disable shadows"),
    graphic_shadows_highQuality_enable(true, "graphic.shadows.highQuality.enable", "enable/disable high quality shadows"),                              // Shadow sprites?
    graphic_overlay_enable(true, "graphic.overlay.enable", "enable/disable overlay"),
    graphic_specularHighlights_enable(true,"graphic.specularHighlights.enable","enable/disable specular highlights"),
    graphic_twoLayerWater_enable(true,"graphic.twoLayerWater.enable","enable/disable two layer water"),
    graphic_background_enable(true, "graphic.background.enable", "enable/disable background"),
    graphic_fog_enable(false, "graphic.fog.enable", "enable/disable fog"),
    graphic_gouraudShading_enable(true, "graphic.gouraudShading.enable", "enable/disable Gouraud shading"),
    graphic_antialiasing(2, "graphic.antialiasing", "set antialiasing level 0 (off), 1 (2x), 2 (4x), 3 (8x), 4 (16x)"),
    graphic_anisotropy_enable(false, "graphic.anisotropy.enable", "enable anisotropic texture filtering"),
    graphic_anisotropy_levels(1.0f, "graphic.anisotropy.levels", "anisotropy levels", 1.0f, 16.0f),
    graphic_textureFilter_minFilter(Ego::TextureFilter::Linear, "graphic.textureFilter.minFilter", "texture filter used for minification",
    {
        { "none", Ego::TextureFilter::None },
        { "nearest", Ego::TextureFilter::Nearest },
        { "linear", Ego::TextureFilter::Linear },
    }),
    graphic_textureFilter_magFilter(Ego::TextureFilter::Linear, "graphic.textureFilter.magFilter", "texture filter used for magnification",
    {
        { "none", Ego::TextureFilter::None },
        { "nearest", Ego::TextureFilter::Nearest },
        { "linear", Ego::TextureFilter::Linear },
    }),
    graphic_textureFilter_mipMapFilter(Ego::TextureFilter::Linear, "graphic.textureFilter.mipMapFilter", "filter used for mip map selection",
    {
        { "none", Ego::TextureFilter::None },
        { "nearest", Ego::TextureFilter::Nearest },
        { "linear", Ego::TextureFilter::Linear }
    }),
    graphic_simultaneousDynamicLights_max(32, "graphic.simultaneousDynamicLights.max", "inclusive upper bound of simultaneous dynamic lights"),
    graphic_framesPerSecond_max(30, "graphic.framesPerSecond.max", "inclusive upper bound of frames per second"),
    graphic_simultaneousParticles_max(768, "graphic.simultaneousParticles.max", "inclusive upper bound of simultaneous particles"),
    graphic_hd_textures_enable(true, "graphic.graphic_hd_textures_enable", "enable/disable HD textures"),

    // Sound configuration section.
    sound_effects_enable(true, "sound.effects.enable", "enable/disable effects"),
    sound_effects_volume(90, "sound.effects.volume", "effects volume"),
    sound_music_enable(true, "sound.music.enable", "enable/disable music"),
    sound_music_volume(70,"sound.music.volume", "music volume"),
    sound_channel_count(32,"sound.channel.count", "number of audio channels.\n"
    "The number of audio channels available limits the number of sounds playing at the same time"),
    sound_outputBuffer_size(4096, "sound.outputBuffer.size", "size of the output buffers in samples.\n"
    "Should be a power of 2, good values seem to range between 512 (inclusive) and 8192 (inclusive).\n"
    "Smaller values yield faster response time, but can lead to underflow if the audio buffer is not filled in time"),
    sound_highQuality_enable(false,"sound.highQuality.enable","enable/disable high quality sound"),
    sound_footfallEffects_enable(true,"sound.footfallEffects.enable","enable/disable footfall effects"),
    // Network configuration section.
    network_enable(false,"network.enable","enable/disable networking"),
    network_lagTolerance(10,"network.lagTolerance","tolerance of lag in seconds"),
    network_hostName("Egoboo host","network.hostName", "name of host to join"),
    network_playerName("Egoboo player", "network.playerName", "player name in network games"),
    // Game configuration section.
    game_difficulty(Ego::GameDifficulty::Normal, "game.difficulty", "game difficulty",
    {
        { "Easy", Ego::GameDifficulty::Easy },
        { "Normal", Ego::GameDifficulty::Normal },
        { "Hard", Ego::GameDifficulty::Hard },
    }),
    // Camera configuration section.
    camera_control(CameraTurnMode::Auto, "camera.control", "type of camera control",
    {
        { "Good", CameraTurnMode::Good },
        { "Auto", CameraTurnMode::Auto },
        { "None", CameraTurnMode::None },
    }),
    // HUD configuration section.
    hud_feedback(Ego::FeedbackType::Text, "hud.feedback", "feed back given to the player",
    {
        { "None",    Ego::FeedbackType::None },
        { "Numeric", Ego::FeedbackType::Number },
        { "Textual", Ego::FeedbackType::Text },
    }),
    hud_simultaneousMessages_max(6, "hud.simultaneousMessages.max", "inclusive upper bound of simultaneous messages"),
    hud_messageDuration(200, "hud.messageDuration", "time in seconds to keep a message alive"),
    hud_messages_enable(true, "hud.messages.enable", "enable/disable messages"),
    hud_displayStatusBars(true, "hud.displayStatusBars", "show/hide status bar"),
    hud_displayGameTime(false, "hud.displayGameTime","show/hide game timer"),
    hud_displayFramesPerSecond(false, "hud.displayFramesPerSecond", "show/hide frames per second"),
    // Debug configuration section.
	debug_mesh_renderHeightMap(false, "debug.mesh.renderHeightMap", "render mesh's height map"),
	debug_mesh_renderNormals(false, "debug.mesh.renderNormals", "render mesh's normals"),
    debug_hideMouse(true,"debug.hideMouse","show/hide mouse"),
    debug_grabMouse(true,"debug.grabMouse","grab/don't grab mouse"),
    debug_developerMode_enable(false,"debug.developerMode.enable","enable/disable developer mode"),
    debug_sdlImage_enable(true,"debug.SDL_Image.enable","enable/disable advanced SDL_image function")
{}

egoboo_config_t::~egoboo_config_t()
{}

egoboo_config_t& egoboo_config_t::operator=(const egoboo_config_t& other)
{
    // Graphic configuration section.
    graphic_fullscreen = other.graphic_fullscreen;
    graphic_resolution_horizontal = other.graphic_resolution_horizontal;
    graphic_resolution_vertical = other.graphic_resolution_vertical;
    graphic_colorBuffer_bitDepth = other.graphic_colorBuffer_bitDepth;
    graphic_depthBuffer_bitDepth = other.graphic_depthBuffer_bitDepth;
    graphic_perspectiveCorrection_enable = other.graphic_perspectiveCorrection_enable;
    graphic_dithering_enable = other.graphic_dithering_enable;
    graphic_reflections_enable = other.graphic_reflections_enable;
    graphic_reflections_particleReflections_enable = other.graphic_reflections_particleReflections_enable;
    graphic_shadows_enable = other.graphic_shadows_enable;
    graphic_shadows_highQuality_enable = other.graphic_shadows_highQuality_enable;
    graphic_specularHighlights_enable = other.graphic_specularHighlights_enable;
    graphic_twoLayerWater_enable = other.graphic_twoLayerWater_enable;
    graphic_overlay_enable = other.graphic_overlay_enable;
    graphic_background_enable = other.graphic_background_enable;
    graphic_fog_enable = other.graphic_fog_enable;
    graphic_gouraudShading_enable = other.graphic_gouraudShading_enable;
    graphic_antialiasing = other.graphic_antialiasing;
    graphic_anisotropy_enable = other.graphic_anisotropy_enable;
    graphic_anisotropy_levels = other.graphic_anisotropy_levels;
    graphic_textureFilter_minFilter = other.graphic_textureFilter_minFilter;
    graphic_textureFilter_magFilter = other.graphic_textureFilter_magFilter;
    graphic_textureFilter_mipMapFilter = other.graphic_textureFilter_mipMapFilter;
    graphic_simultaneousDynamicLights_max = other.graphic_simultaneousDynamicLights_max;
    graphic_framesPerSecond_max = other.graphic_framesPerSecond_max;
    graphic_simultaneousParticles_max = other.graphic_simultaneousParticles_max;
    graphic_hd_textures_enable = other.graphic_hd_textures_enable;

    // Sound configuration section.
    sound_effects_enable = other.sound_effects_enable;
    sound_effects_volume = other.sound_effects_volume;
    sound_music_enable = other.sound_music_enable;
    sound_music_volume = other.sound_music_volume;

    sound_channel_count = other.sound_channel_count;
    sound_outputBuffer_size = other.sound_outputBuffer_size;
    sound_highQuality_enable = other.sound_highQuality_enable;
    sound_footfallEffects_enable = other.sound_footfallEffects_enable;

    // Network configuration section.
    network_enable = other.network_enable;
    network_lagTolerance = other.network_lagTolerance;
    network_hostName = other.network_hostName;
    network_playerName = other.network_playerName;

    // Camera configuration section.
    camera_control = other.camera_control;

    // Game configuration section.
    game_difficulty = other.game_difficulty;
    
    // HUD configuration section.
    hud_displayGameTime = other.hud_displayGameTime;
    hud_messages_enable = other.hud_messages_enable;
    hud_simultaneousMessages_max = other.hud_simultaneousMessages_max;
    hud_messageDuration = other.hud_messageDuration;
    hud_displayStatusBars = other.hud_displayStatusBars;
    hud_feedback = other.hud_feedback;
    hud_displayFramesPerSecond = other.hud_displayFramesPerSecond;

    // Debug configuration section.
	debug_mesh_renderHeightMap = other.debug_mesh_renderHeightMap;
	debug_mesh_renderNormals = other.debug_mesh_renderNormals;
    debug_hideMouse = other.debug_hideMouse;
    debug_grabMouse = other.debug_grabMouse;
    debug_developerMode_enable = other.debug_developerMode_enable;
    debug_sdlImage_enable = other.debug_sdlImage_enable;

    return *this;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool setup_begin()
{
    if (_setup_started)
    {
        return true;
    }

    // Select the local "setup.txt".
    _config_filename = "setup.txt";
    // Parse the local "setup.txt".
    ConfigFileParser parser;
    _lpConfigSetup = parser.parse(_config_filename);
    if (!_lpConfigSetup)
    {
		Log::get().warn("unable to load setup file `%s`\n", _config_filename.c_str());
        try
        {
            _lpConfigSetup = std::make_shared<ConfigFile>(_config_filename);
        }
        catch (...)
        {
            return false;
        }
        _setup_started = true;
    }
    else
    {
		Log::get().info("loaded setup file `%s`\n", _config_filename.c_str());
        _setup_started = true;
    }
    return _setup_started;
}

bool setup_end()
{
    if (ConfigFileUnParser().unparse(_lpConfigSetup))
    {
        _lpConfigSetup = nullptr;
        return true;
    }
    else
    {
		Log::get().warn("unable to save setup file `%s`\n", _lpConfigSetup->getFileName().c_str());
        _lpConfigSetup = nullptr;
        return false;
    }
}

//--------------------------------------------------------------------------------------------
bool setup_download(egoboo_config_t *cfg)
{
    if (!cfg)
    {
        throw std::invalid_argument("nullptr == cfg");
    }
    if (!_lpConfigSetup)
    {
        throw std::logic_error("`setup.txt` not initialized");
    }
    cfg->for_each(egoboo_config_t::Load(_lpConfigSetup));
    return true;
}

bool setup_upload(egoboo_config_t *cfg)
{
    if (!cfg)
    {
        throw std::invalid_argument("nullptr == cfg");
    }
    if (!_lpConfigSetup)
    {
        throw std::logic_error("`setup.txt` not initialized");
    }
    cfg->for_each(egoboo_config_t::Store(_lpConfigSetup));   
    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void setup_init_base_vfs_paths()
{
    //---- tell the vfs to add the basic search paths
    vfs_set_base_search_paths();

    //---- mount all of the default global directories

    // mount the global basicdat directory t the beginning of the list
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat", "mp_data", 1 );

    // Create a mount point for the /user/modules directory
    vfs_add_mount_point( fs_getUserDirectory(), "modules", "mp_modules", 1 );

    // Create a mount point for the /data/modules directory
    vfs_add_mount_point( fs_getDataDirectory(), "modules", "mp_modules", 1 );

    // Create a mount point for the /user/players directory
    vfs_add_mount_point( fs_getUserDirectory(), "players", "mp_players", 1 );

    // Create a mount point for the /data/players directory
    //vfs_add_mount_point( fs_getDataDirectory(), "players", "mp_players", 1 );     //ZF> Let's remove the local players folder since it caused so many problems for people

    // Create a mount point for the /user/remote directory
    vfs_add_mount_point( fs_getUserDirectory(), "import", "mp_import", 1 );

    // Create a mount point for the /user/remote directory
    vfs_add_mount_point( fs_getUserDirectory(), "remote", "mp_remote", 1 );
}

void setup_clear_base_vfs_paths()
{
    vfs_remove_mount_point( "mp_data" );
    vfs_remove_mount_point( "mp_modules" );
    vfs_remove_mount_point( "mp_players" );
    vfs_remove_mount_point( "mp_remote" );
}

//--------------------------------------------------------------------------------------------
bool setup_init_module_vfs_paths(const char *mod_path)
{
    const char * path_seperator_1, * path_seperator_2;
    const char * mod_dir_ptr;
    STRING mod_dir_string;

    STRING tmpDir;

    if ( INVALID_CSTR( mod_path ) ) return false;

    // revert to the program's basic mount points
    setup_clear_module_vfs_paths();

    path_seperator_1 = strrchr( mod_path, SLASH_CHR );
    path_seperator_2 = strrchr( mod_path, NET_SLASH_CHR );
    path_seperator_1 = std::max( path_seperator_1, path_seperator_2 );

    if ( NULL == path_seperator_1 )
    {
        mod_dir_ptr = mod_path;
    }
    else
    {
        mod_dir_ptr = path_seperator_1 + 1;
    }

    strncpy( mod_dir_string, mod_dir_ptr, SDL_arraysize( mod_dir_string ) );

    //==== set the module-dependent mount points

    //---- add the "/modules/*.mod/objects" directories to mp_objects
    snprintf( tmpDir, SDL_arraysize( tmpDir ), "modules" SLASH_STR "%s" SLASH_STR "objects", mod_dir_string );

    // mount the user's module objects directory at the beginning of the mount point list
    vfs_add_mount_point( fs_getDataDirectory(), tmpDir, "mp_objects", 1 );

    // mount the global module objects directory next in the mount point list
    vfs_add_mount_point( fs_getUserDirectory(), tmpDir, "mp_objects", 1 );

    //---- add the "/basicdat/globalobjects/*" directories to mp_objects
    //ZF> TODO: Maybe we should dynamically search for all folders in this directory and add them as valid mount points?
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "items",            "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "magic",            "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "magic_item",       "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "misc",             "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "monsters",         "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "players",          "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "potions",          "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "unique",           "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "weapons",          "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "work_in_progress", "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "traps",            "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "pets",             "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "scrolls",          "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "armor",            "mp_objects", 1 );

    //---- add the "/modules/*.mod/gamedat" directory to mp_data
    snprintf( tmpDir, SDL_arraysize( tmpDir ), "modules" SLASH_STR "%s" SLASH_STR "gamedat",  mod_dir_string );

    // mount the user's module gamedat directory at the beginning of the mount point list
    vfs_add_mount_point( fs_getUserDirectory(), tmpDir, "mp_data", 1 );

    // append the global module gamedat directory
    vfs_add_mount_point( fs_getDataDirectory(), tmpDir, "mp_data", 1 );

    // put the global globalparticles data after the module gamedat data
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalparticles", "mp_data", 1 );

    return true;
}

//--------------------------------------------------------------------------------------------
void setup_clear_module_vfs_paths()
{
    /// @author BB
    /// @details clear out the all mount points

    // clear out the basic mount points
    setup_clear_base_vfs_paths();

    // clear out the module's mount points
    vfs_remove_mount_point( "mp_objects" );

    // set up the basic mount points again
    setup_init_base_vfs_paths();
}
