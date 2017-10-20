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

/// @file  egolib/egoboo_setup.h
/// @brief Functions for handling the <tt>setup.txt</tt> file.

#pragma once

#include "egolib/Configuration/Configuration.hpp"
#include "egolib/Logic/GameDifficulty.hpp"
#include "egolib/Logic/FeedbackType.hpp"
#include "egolib/Renderer/TextureFilter.hpp"

//Forward declarations
enum class CameraTurnMode : uint8_t;
struct egoboo_config_t;

//--------------------------------------------------------------------------------------------

/// The internal representation of the data in "settings.txt"
/**
 * @remark
 *  The suffix "_max" usually indicates that the variable provides an inclusive upper bound
 *  of a property, the corresponding suffix "_min" that it provides an inclusive lower bound.
 *  Variables with the "_count", "_size" suffix provide a desired/required value of a countable
 *  or measurable property. For binary choices which desire/require that something is
 *  enabled/disabled, the suffix "_enable" is used.
 */
struct egoboo_config_t : public Ego::Configuration::Configuration
{
protected:
    static egoboo_config_t _singleton;

    /// Utility function to create a tuple with references to variables of a configuration.
    /// @param config the configuration
    /// @return the tuple
    template <typename T>
    static decltype(auto) make_variable_tuple(T& config)
    {
        auto variables =
            std::tie
            (
                config.graphic_fullscreen,
                config.graphic_colorBuffer_bitDepth,
                config.graphic_depthBuffer_bitDepth,
                config.graphic_stencilBuffer_bitDepth,
                config.graphic_accumulationBuffer_bitDepth,
                config.graphic_resolution_horizontal,
                config.graphic_resolution_vertical,
                config.graphic_perspectiveCorrection_enable,
                config.graphic_dithering_enable,
                config.graphic_reflections_enable,
                config.graphic_reflections_particleReflections_enable,
                config.graphic_shadows_enable,
                config.graphic_shadows_highQuality_enable,
                config.graphic_overlay_enable,
                config.graphic_specularHighlights_enable,
                config.graphic_twoLayerWater_enable,
                config.graphic_background_enable,
                config.graphic_fog_enable,
                config.graphic_gouraudShading_enable,
                config.graphic_antialiasing,
                config.graphic_anisotropy_enable,
                config.graphic_anisotropy_levels,
                config.graphic_doubleBuffering_enable,
                config.graphic_textureFilter_minFilter,
                config.graphic_textureFilter_magFilter,
                config.graphic_textureFilter_mipMapFilter,
                config.graphic_simultaneousDynamicLights_max,
                config.graphic_framesPerSecond_max,
                config.graphic_simultaneousParticles_max,
                config.graphic_hd_textures_enable,
                //
                config.graphic_window_borderless,
                config.graphic_window_resizable,
                config.graphic_window_allowHighDpi,
                config.graphic_window_fullscreenDesktop,
                //
                config.sound_effects_enable,
                config.sound_effects_volume,
                config.sound_music_enable,
                config.sound_music_volume,
                config.sound_channel_count,
                config.sound_outputBuffer_size,
                config.sound_highQuality_enable,
                config.sound_footfallEffects_enable,
                //
                config.network_enable,
                config.network_lagTolerance,
                config.network_hostName,
                config.network_playerName,
                //
                config.game_difficulty,
                //
                config.camera_control,
                //
                config.hud_feedback,
                config.hud_simultaneousMessages_max,
                config.hud_messageDuration,
                config.hud_messages_enable,
                config.hud_displayStatusBars,
                config.hud_displayGameTime,
                config.hud_displayFramesPerSecond,
                //
                config.debug_mesh_renderHeightMap,
                config.debug_mesh_renderNormals,
                config.debug_object_renderBoundingBoxes,
                config.debug_object_renderGrips,
                config.debug_hideMouse,
                config.debug_grabMouse,
                config.debug_developerMode_enable,
                config.debug_sdlImage_enable
            );
        return variables;
    }

    template <typename Functor>
    void for_each(Functor f)
    {
        Configuration::for_each(make_variable_tuple(*this), f);
    }

public:
    void load(std::shared_ptr<ConfigFile> file)
    {
        Configuration::for_each(make_variable_tuple(*this), Load(file));
    }

    void store(std::shared_ptr<ConfigFile> file)
    {
        Configuration::for_each(make_variable_tuple(*this), Store(file));
    }

    // Graphic configuration section.

    /// @brief Enable/disable fullscreen mode.
    /// @remark Default value is @a false.
    /// @warning graphic_fullscreen and graphic_window_fullscreenDesktopp are mutually exclusive.
    Ego::Configuration::Variable<bool> graphic_fullscreen;

    /// @brief The horizontal resolution.
    /// @remark Default value is @a 800.
    /// @todo Type should be <tt>uint16_t</tt>.
    Ego::Configuration::Variable<int> graphic_resolution_horizontal;

    /// @brief The vertical resolution.
    /// @remark Default value is @a 600.
    /// @todo Type should be <tt>uint16_t</tt>.
    Ego::Configuration::Variable<int> graphic_resolution_vertical;

    /// @brief The color buffer depth.
    /// @remark Default value is @a 32.
    /// @todo Type should be <tt>uint8_t</tt>.
    Ego::Configuration::Variable<int> graphic_colorBuffer_bitDepth;

    /// @brief The depth buffer depth.
    /// @remark Default value is @a 24.
    /// @todo Type should be <tt>uint8_t</tt>.
    Ego::Configuration::Variable<int> graphic_depthBuffer_bitDepth;

    /// @brief The stencil buffer depth.
    /// @remark Default value is @a 8.
    Ego::Configuration::Variable<int> graphic_stencilBuffer_bitDepth;

    /// @brief The accumulation buffer depth.
    /// @remark Default value is @a 32.
    Ego::Configuration::Variable<int> graphic_accumulationBuffer_bitDepth;

    /// @brief Enable perspective correction.
    /// @remark Default value is @a false.
    /// @todo Enable this by default?
    Ego::Configuration::Variable<bool> graphic_perspectiveCorrection_enable;

    /// @brief Enable dithering.
    /// @remark Default value is @a false.
    /// @todo Enable this by default?
    Ego::Configuration::Variable<bool> graphic_dithering_enable;

    /// @brief Enable/disable reflections.
    /// @remark Default value is @a true.
    Ego::Configuration::Variable<bool> graphic_reflections_enable;

    /// @brief Enable/disable particle reflections.
    /// @remark Default value is @a true.
    Ego::Configuration::Variable<bool> graphic_reflections_particleReflections_enable;

    /// @brief Enable/disable shadows.
    /// @remark Default value is @a true.
    Ego::Configuration::Variable<bool> graphic_shadows_enable;

    /// @brief Enable/disable high-quality shadows.
    /// @remark Default value is @a true (high quality).
    Ego::Configuration::Variable<bool> graphic_shadows_highQuality_enable;

    /// @brief Enable/disable specular highlights.
    /// @remark Default value is @a true.
    Ego::Configuration::Variable<bool> graphic_specularHighlights_enable;

    /// @brief Enable/disable two layer water.
    /// @remark Default value is @a true.
    Ego::Configuration::Variable<bool> graphic_twoLayerWater_enable;

    /// @brief Enable/disable overlay.
    /// @remark Default value is @a true.
    Ego::Configuration::Variable<bool> graphic_overlay_enable;

    /// @brief Enable/disable background.
    /// @remark Default value is @a true.
    Ego::Configuration::Variable<bool> graphic_background_enable;

    /// @brief Enable/disable fog.
    /// @remark Default value is @a false.
    Ego::Configuration::Variable<bool> graphic_fog_enable;

    /// @brief Enable/disable Gouraud shading.
    /// @remark Default value is @a true.
    Ego::Configuration::Variable<bool> graphic_gouraudShading_enable;

    /// @brief Anti-aliasing level.
    /// @remark Default is @ 2 i.e.two samples, the default value range is 0 (disabled), 2 (2x), 4 (4x) and 8 (8x) and 16 (16x).
    /// @remark
    ///  "multisampling" is a specific optimization of "super sampling".
    ///  "super sampling anti-aliasing (SSAA)" and "full scene antialising (FSAA)" are synonyms.
    ///  In general SSAA/FSAA is expensive and is replaced by by "multisample antialiasing (MSAA)".
    Ego::Configuration::Variable<uint8_t> graphic_antialiasing;

    /// @brief Enable anisotropy.
    /// @remark Default is @a false.
    Ego::Configuration::Variable<bool> graphic_anisotropy_enable;

    /// @brief The anisotropy levels.
    /// @remark Default is @a 1 within the range of <tt>[1,16]</tt>
    Ego::Configuration::Variable<float> graphic_anisotropy_levels;

    /// @brief Enable double buffering.
    /// @remark Default is @a true.
    Ego::Configuration::Variable<bool> graphic_doubleBuffering_enable;

    /// @brief The texture filter used for minification.
    /// @remark Default value is Ego::TextureFilter::Linear.
    Ego::Configuration::Variable<Ego::TextureFilter> graphic_textureFilter_minFilter;

    /// @brief The texture filter used for magnification.
    /// @remark Default value is Ego::TextureFilter::Linear.
    Ego::Configuration::Variable<Ego::TextureFilter> graphic_textureFilter_magFilter;

    /// @brief The filter applied used for mip map selection.
    /// @remark Default value is Ego::TextureFilter::Linear.
    Ego::Configuration::Variable<Ego::TextureFilter> graphic_textureFilter_mipMapFilter;

    /// @brief Inclusive upper bound of number of simultaneous dynamic lights.
    /// @remark Default value is @a 32.
    Ego::Configuration::Variable<uint16_t> graphic_simultaneousDynamicLights_max;

    /// @brief Inclusive upper bound of the frames per second.
    /// @remark Default value is @a 30.
    Ego::Configuration::Variable<uint16_t> graphic_framesPerSecond_max;

    /// @brief Inclusive upper bound of the number of simultaneous particles.
    /// @remark Default value is @a 768.
    Ego::Configuration::Variable<uint16_t> graphic_simultaneousParticles_max;

    /// @brief If @a true, the game will try to load HD versions of textures if
    /// they are available and default back to normal version if not.
    /// HD textures are textures with higher resolution and end with
    /// _HD before the file type suffix (e.g myTexture_HD.png).
    /// @remark Default value is @a true.
    Ego::Configuration::Variable<bool> graphic_hd_textures_enable;

    /// @brief If @a true, the window is borderless, otherwise it is not.
    /// @remark A borderless window displays neither a caption nor an edge frame.
    /// @default Default is @a false.
    Ego::Configuration::Variable<bool> graphic_window_borderless;
    /// @brief If @a true the window is resizable, otherwise it is not.
    /// @default Default is @a false.
    Ego::Configuration::Variable<bool> graphic_window_resizable;
    /// @brief If @a true the window supports high-DPI modes (in Apple terminology 'Retina').
    /// @default Default is @a false.
    Ego::Configuration::Variable<bool> graphic_window_allowHighDpi;
    /// @brief If @a true, the window
    /// @remark A fulldesktop window always covers the entire display or is minimized.
    /// @default Default is @a false.
    /// @warning graphic_fullscreen and graphic_window_fullscreenDesktop mutually exclusive.
    Ego::Configuration::Variable<bool> graphic_window_fullscreenDesktop;

    // Sound configuration section.

    /// @brief Enable/disable effects.
    /// @remark Default value is @a true.
    Ego::Configuration::Variable<bool> sound_effects_enable;

    /// @brief The effects volume.
    /// @remark The default value is @a 75, range is @a 0 to @a 128.
    /// @todo The unit should be "percent", the default value should be 60 "percent"?
    /// We require a "percent" type?
    Ego::Configuration::Variable<uint8_t> sound_effects_volume;

    /// @brief Enable/disable music.
    /// @remark Default value is @a true.
    Ego::Configuration::Variable<bool> sound_music_enable;

    /// @brief The music volume.
    /// @remark The default value is @a 50, range is @a 0 to @a 128.
    /// @todo The unit should be "percent", the default value should be 40 "percent"?
    /// We require a "percent" type?
    Ego::Configuration::Variable<uint8_t> sound_music_volume;

    /// @brief Number of audio channels.
    /// @remark The number of audio channels limit the number of sounds playing at the same time.
    /// The default value is @a 32.
    /// @todo
    /// Encode the following constraints
    /// @code
    /// CLIP<uint16_t>(cfg.sound_channel_count, 8, 128);
    /// CLIP<uint16_t>(cfg.sound_buffer_size, 512, 8196);
    /// @endcode
    Ego::Configuration::Variable<uint16_t> sound_channel_count;

    /// @brief Size of audio output buffer.
    /// @remark Default value is @a 4096.
    /// @todo
    /// Encode the following constraints
    /// @code
    /// CLIP<uint16_t>(cfg.sound_buffer_size, 512, 8196);
    /// @endcode
    Ego::Configuration::Variable<uint16_t> sound_outputBuffer_size;

    /// @brief Enable/disable high quality audio.
    /// @remark Default value is @a false.
    Ego::Configuration::Variable<bool> sound_highQuality_enable;

    /// @brief Enable/disable footfall effects.
    Ego::Configuration::Variable<bool> sound_footfallEffects_enable;

    // Network configuration section.

    /// @brief Enable/disable network?
    /// @remark Default value is @a false.
    Ego::Configuration::Variable<bool> network_enable;

    /// @brief Tolerance to lag.
    /// @remark Default value is @a 10.
    /// @todo And what does @a 10 mean?
    Ego::Configuration::Variable<uint16_t> network_lagTolerance;

    /// @brief Name of host to join.
    /// @remark Default value is @a "Egoboo Host".
    Ego::Configuration::Variable<std::string> network_hostName;

    /// @brief Player name in network games.
    /// @remark Default value is @a "Egoboo Player".
    Ego::Configuration::Variable<std::string> network_playerName;

    // Camera configuration section.

    /// @brief Type of camera control.
    /// @remark Default value is @a CameraTurnMode::Auto.
    Ego::Configuration::Variable<CameraTurnMode> camera_control;

    // Game configuration section.

    /// @brief Game difficulty.
    /// @remark Default value is Ego::GameDifficulty::Normal.
    Ego::Configuration::Variable<Ego::GameDifficulty> game_difficulty;

    // HUD configuration section.

    /// @brief Inclusive upper bound of simultaneous messages.
    /// @remark Default value is @a 6.
    /// @todo
    /// Encode the following constraint:
    /// @code
    /// CLIP(cfg->hud_simultaneousMessages_max.getValue(), (uint8_t)EGO_MESSAGE_MIN, (uint8_t)EGO_MESSAGE_MAX);
    /// @endcode
    Ego::Configuration::Variable<uint8_t> hud_simultaneousMessages_max;

    /// @brief Time in seconds to keep a message alive.
    /// @remark Default value is @a 200.
    Ego::Configuration::Variable<uint16_t> hud_messageDuration;

    /// @brief Show/hide status bar.
    /// @remark Default value is @a true.
    Ego::Configuration::Variable<bool> hud_displayStatusBars;

    /// @brief Show/hide game time.
    /// @remark Default value is @a false.
    Ego::Configuration::Variable<bool> hud_displayGameTime;

    /// @brief Feedback given to the player.
    /// @remark Default value is @a Ego::FeedbackType::Text.
    Ego::Configuration::Variable<Ego::FeedbackType> hud_feedback;

    /// @brief Enable/disable messages.
    /// @remark Default value is @a true.
    Ego::Configuration::Variable<bool> hud_messages_enable;

    /// @brief Show/hide frames per second.
    /// @remark Default value is @a false.
    Ego::Configuration::Variable<bool> hud_displayFramesPerSecond;

    // Debug configuration section.

    /// @brief Hide mouse?
    /// @remark Default value is @a true.
    Ego::Configuration::Variable<bool> debug_hideMouse;

    /// @brief Render mesh's height map?
    /// @remark Default value is @a false.
    Ego::Configuration::Variable<bool> debug_mesh_renderHeightMap;

    /// @brief Render mesh's normals?
    /// @remark Default value is @a false.
    Ego::Configuration::Variable<bool> debug_mesh_renderNormals;

    /// @brief Render object's bounding boxes?
    /// @remark Default value is @a false.
    Ego::Configuration::Variable<bool> debug_object_renderBoundingBoxes;

    /// @brief Render object's grips?
    /// @remark Default value is @a false.
    Ego::Configuration::Variable<bool> debug_object_renderGrips;

    /// @brief Grab mouse?
    /// @remark Default value is @a true.
    Ego::Configuration::Variable<bool> debug_grabMouse;

    /// @brief Enable/disable developer mode.
    /// @remark Default value is @a false.
    Ego::Configuration::Variable<bool> debug_developerMode_enable;

    /// @brief Enable/disable SDL image.
    /// @remark Default value is @a true.
    Ego::Configuration::Variable<bool> debug_sdlImage_enable;

public:

    /// @brief Construct this Egoboo configuration with default settings.
    egoboo_config_t();

    /// @brief Destruct this Egoboo configuration.
    virtual ~egoboo_config_t();

    /// @brief Assign this Egoboo configuration the values of another Egoboo configuration.
    /// @param other the other Egoboo configuration
    /// @return this Egoboo configuration
    egoboo_config_t& operator=(const egoboo_config_t& other);

public:

    /// @brief Get the Egoboo configuration singleton.
    /// @return the singleton
    static egoboo_config_t& get();

};

namespace Ego {
struct Setup
{
private:
    /// The configuration file.
    static std::shared_ptr<ConfigFile> file;
    /// The filename of the configuration file.
    static const std::string fileName;
    /// If the setup has started.
    static bool started;

public:
    /// @brief Load the local <tt>"setup.txt"</tt>.
    /// @remark
    /// This will initialize a represention fromload the configuration from the local <tt>"setup.txt"</tt>.
    /// If loading fails, the outcome of this function is equivalent to the case in which that file is empty.
    static bool begin();

    /// @brief Save the local <tt>"setup.txt"</tt>.
    /// @remark This will write the configuration to the local <tt>"setup.txt"</tt>.
    static bool end();

    /// @brief Download the local <tt>"setup.txt"</tt> into an Egoob configuration.
    /// @return @a true on success, @a false on failure
    /// @remark
    ///  If the local <tt>"setup.txt"</tt> is missing values,
    ///  those value will be replaced by values from the default Egoboo configuration.
    static bool download(egoboo_config_t& cfg);

    /// @brief Upload an Egoboo configuration into the local <tt>"setup.txt"</tt>.
    static bool upload(egoboo_config_t& cfg);
};

} // namespace Ego

/// Set in the VFS the basic mount points/search paths.
void setup_init_base_vfs_paths();

/// Remove from the VFS the basic mounts points/search paths.
void setup_clear_base_vfs_paths();

/// Set in the VFS the module specific mount points/search path.
bool setup_init_module_vfs_paths(const std::string& mod_path);

/// Remove from the VFS the module specific mount points/search paths.
void setup_clear_module_vfs_paths();

