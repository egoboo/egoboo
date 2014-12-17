#pragma once

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

/// @file egolib/egoboo_setup.h

#include "egolib/typedef.h"
#include "egolib/tx_filters.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct s_camera_options;

    struct s_egoboo_config;
    typedef struct s_egoboo_config egoboo_config_t;

//--------------------------------------------------------------------------------------------
// CONSTANTS
//--------------------------------------------------------------------------------------------

// The possible levels of game difficulty
    enum e_game_difficulty
    {
        GAME_EASY  = 0,
        GAME_NORMAL,
        GAME_HARD
    };

//--------------------------------------------------------------------------------------------
// What feedback does the user want
    enum ego_feedback_type
    {
        EGO_FEEDBACK_TYPE_OFF = 0,           //None
        EGO_FEEDBACK_TYPE_TEXT,              //Descriptive text
        EGO_FEEDBACK_TYPE_NUMBER             //Show the damage as a number
    };

    // this typedef must be after the enum definition or gcc has a fit
    typedef enum ego_feedback_type EGO_FEEDBACK_TYPE;

//--------------------------------------------------------------------------------------------
// struct s_egoboo_config
//--------------------------------------------------------------------------------------------

/// The internal representation of the data in "settings.txt"
    struct s_egoboo_config
    {
        // {GRAPHIC}
        bool               fullscreen_req;            ///< Start in fullscreen?
        int                     scrd_req;                  ///< Screen bit depth
        int                     scrz_req;                  ///< Screen z-buffer depth ( 8 unsupported )
        int                     scrx_req;                  ///< Screen X size
        int                     scry_req;                  ///< Screen Y size
        bool               use_perspective;           ///< Perspective correct textures?
        bool               use_dither;                ///< Dithering?
        bool               reflect_allowed;           ///< Reflections?
        Uint8                   reflect_fade;              ///< 255 = Don't fade reflections
        bool               reflect_prt;               ///< Reflect particles?
        bool               shadow_allowed;            ///< Shadows?
        bool               shadow_sprite;             ///< Shadow sprites?
        bool               use_phong;                 ///< Do phong overlay?
        bool               twolayerwater_allowed;     ///< Two layer water?
        bool               overlay_allowed;           ///< Allow large overlay?
        bool               background_allowed;        ///< Allow large background?
        bool               fog_allowed;
        bool               gouraud_req;               ///< Gouraud shading?
        Uint8                   multisamples;              ///< Antialiasing?
        e_tx_filters            texturefilter_req;         ///< Texture filtering?
        int                     dyna_count_req;            ///< Max number of lights to draw
        Sint32                  framelimit;
        Uint16                  particle_count_req;        ///< max number of particles

        // {SOUND}
        bool               sound_allowed;
        bool               music_allowed;
        Uint8                   music_volume;               ///< The sound volume of music
        Uint8                   sound_volume;               ///< Volume of sounds played
        Uint16                  sound_channel_count;        ///< Max number of sounds playing at the same time
        Uint16                  sound_buffer_size;
        bool               sound_highquality;
        bool               sound_highquality_base;
        bool               sound_footfall;

        // {NETWORK}
        bool               network_allowed;            ///< Try to connect?
        int                     network_lag;                ///< Lag tolerance
        char                    network_hostname[64];                            ///< Name for hosting session
        char                    network_messagename[64];                         ///< Name for messages

        // {GAME}
        int                     message_count_req;
        Uint16                  message_duration;        ///< Time to keep the message alive
        bool               show_stats;              ///< Draw the status bars?
        Uint8                   autoturncamera;          ///< Type of camera control...
        EGO_FEEDBACK_TYPE       feedback;                ///< Feedback type
        Uint8                   difficulty;              ///< What is the current game difficulty

        // {DEBUG}
        bool               fps_allowed;             ///< FPS displayed?
        bool               hide_mouse;
        bool               grab_mouse;
        bool               dev_mode;
        bool               sdl_image_allowed;       ///< Allow advanced SDL_Image functions?

        // other values
        bool                  messageon_req;
    };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    extern egoboo_config_t cfg;

//--------------------------------------------------------------------------------------------
// EXTERNAL FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

    /// begin the setup module
    extern bool setup_begin( void );

    /// end the setup module
    extern bool setup_end( void );

    /// Read the "setup.txt" file
    extern bool setup_read_vfs( void );

    /// Write the "setup.txt" file
    bool setup_write_vfs( void );

    /// download the data in "setup.txt" to an egoboo_config_t data structure
    bool setup_download( egoboo_config_t * pcfg );

    /// upload the data in an egoboo_config_t data structure to "setup.txt"
    bool setup_upload( egoboo_config_t * pcfg );

    /// ensure that the program, egoboo_config_t data structure, and "setup.txt" all agree
    bool config_synch( egoboo_config_t * pcfg, bool synch_from_file );

    /// set the basic search paths used by the egoboo virtual file system
    void setup_init_base_vfs_paths( void );

    /// remove the basic search paths used by the egoboo virtual file system
    void setup_clear_base_vfs_paths( void );

    /// initialize a module's vfs mount points
    bool setup_init_module_vfs_paths( const char * mod_path );

    /// initialize a module's vfs mount points
    void   setup_clear_module_vfs_paths( void );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _egoboo_setup_h
