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

#include "egoboo_typedef.h"

#if defined(__cplusplus)
extern "C"
{
#endif

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
    typedef enum e_feedback
    {
        FEEDBACK_OFF = 0,           //None
        FEEDBACK_TEXT,              //Descriptive text
        FEEDBACK_NUMBER             //Show the damage as a number
    } FEEDBACK_TYPE;

//--------------------------------------------------------------------------------------------
// struct s_egoboo_config
//--------------------------------------------------------------------------------------------

/// The internal representation of the data in "settings.txt"
    struct s_egoboo_config
    {
        // {GRAPHIC}
        bool_t                  fullscreen_req;            ///< Start in fullscreen?
        int                     scrd_req;                  ///< Screen bit depth
        int                     scrz_req;                  ///< Screen z-buffer depth ( 8 unsupported )
        int                     scrx_req;                  ///< Screen X size
        int                     scry_req;                  ///< Screen Y size
        bool_t                  use_perspective;               ///< Perspective correct textures?
        bool_t                  use_dither;                    ///< Dithering?
        bool_t                  reflect_allowed;           ///< Reflections?
        Uint8                   reflect_fade;              ///< 255 = Don't fade reflections
        bool_t                  reflect_prt;               ///< Reflect particles?
        bool_t                  shadow_allowed;            ///< Shadows?
        bool_t                  shadow_sprite;             ///< Shadow sprites?
        bool_t                  use_phong;                 ///< Do phong overlay?
        bool_t                  twolayerwater_allowed;     ///< Two layer water?
        bool_t                  overlay_allowed;           ///< Allow large overlay?
        bool_t                  background_allowed;        ///< Allow large background?
        bool_t                  fog_allowed;
        bool_t                  gouraud_req;               ///< Gouraud shading?
        Uint8                   multisamples;          ///< Antialiasing?
        Uint8                   texturefilter_req;             ///< Texture filtering?
        int                     dyna_count_req;            ///< Max number of lights to draw
        Sint32                  framelimit;
        Uint16                  particle_count_req;                              ///< max number of particles

        // {SOUND}
        bool_t                  sound_allowed;
        bool_t                  music_allowed;
        Uint8                   music_volume;               ///< The sound volume of music
        Uint8                   sound_volume;               ///< Volume of sounds played
        Uint16                  sound_channel_count;        ///< Max number of sounds playing at the same time
        Uint16                  sound_buffer_size;
        bool_t                  sound_highquality;
        bool_t                  sound_highquality_base;
        bool_t                  sound_footfall;

        // {NETWORK}
        bool_t                  network_allowed;            ///< Try to connect?
        int                     network_lag;                ///< Lag tolerance
        char                    network_hostname[64];                            ///< Name for hosting session
        char                    network_messagename[64];                         ///< Name for messages

        // {GAME}
        int                     message_count_req;
        Uint16                  message_duration;               ///< Time to keep the message alive
        bool_t                  StatusList_on;                    ///< Draw the status bars?
        Uint8                   autoturncamera;             ///< Type of camera control...
        bool_t                  fps_allowed;             ///< FPS displayed?
        FEEDBACK_TYPE           feedback;                ///< Feedback type
        Uint8                   difficulty;              ///< What is the current game difficulty

        // {DEBUG}
        bool_t                  hide_mouse;
        bool_t                  grab_mouse;
        bool_t                  dev_mode;
        bool_t                  sdl_image_allowed;       ///< Allow advanced SDL_Image functions?

    };
    typedef struct s_egoboo_config egoboo_config_t;

    extern egoboo_config_t cfg;

//--------------------------------------------------------------------------------------------
// EXTERNAL FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

    bool_t setup_read_vfs();
    bool_t setup_write();
    bool_t setup_quit();

    bool_t setup_download( egoboo_config_t * pcfg );
    bool_t setup_upload( egoboo_config_t * pcfg );
    bool_t setup_synch( egoboo_config_t * pcfg );

    bool_t input_settings_save_vfs( const char* whichfile );
    bool_t input_settings_load_vfs( const char *szFilename );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _egoboo_setup_h
