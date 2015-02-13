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

/// @file  game/egoboo.c
/// @brief Code for the main program process
/// @details

#define DECLARE_GLOBALS
#include "game/egoboo.h"
#include "game/game.h"
#include "game/renderer_2d.h"
#include "game/PrtList.h"
#include "game/audio/AudioSystem.hpp"

#if defined(__cplusplus)
extern "C"
{
#endif
    extern bool config_download( egoboo_config_t * pcfg, bool synch_from_file );
    extern bool config_upload( egoboo_config_t * pcfg );
#if defined(__cplusplus)
}

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool config_download( egoboo_config_t * pcfg, bool synch_from_file )
{
    bool rv;

    // synchronize settings from a pre-loaded setup.txt? (this will load setup.txt into *pcfg)
    if ( synch_from_file )
    {
        rv = setup_download( pcfg );
        if ( !rv ) return false;
    }

    // status display
    StatusList.on = pcfg->show_stats;

    // fps display
    fpson = pcfg->fps_allowed;

    // message display
    DisplayMsg_count    = CLIP( pcfg->message_count_req, EGO_MESSAGE_MIN, EGO_MESSAGE_MAX );
    DisplayMsg_on       = pcfg->message_count_req > 0;
    wraptolerance = pcfg->show_stats ? 90 : 32;

    // Get the particle limit
    // if the particle limit has changed, make sure to make not of it
    // number of particles
    PrtList.setDisplayLimit(pcfg->particle_count_req);

    // camera options
    _cameraSystem.getCameraOptions().turnMode = pcfg->autoturncamera;

    // sound options
    _audioSystem.setConfiguration(*pcfg);

    // renderer options
    gfx_config_download_from_egoboo_config( &gfx, pcfg );

    // texture options
    oglx_texture_parameters_download_gfx( &tex_params, pcfg );

    return true;
}

//--------------------------------------------------------------------------------------------
bool config_upload( egoboo_config_t * pcfg )
{
    if ( NULL == pcfg ) return false;

    pcfg->autoturncamera = _cameraSystem.getCameraOptions().turnMode;
    pcfg->fps_allowed    = TO_C_BOOL( fpson );

    // number of particles
    pcfg->particle_count_req = CLIP( PrtList.getDisplayLimit(), (size_t)0, (size_t)MAX_PRT );

    // messages
    pcfg->messageon_req     = TO_C_BOOL( DisplayMsg_on );
    pcfg->message_count_req = !DisplayMsg_on ? 0 : std::max( EGO_MESSAGE_MIN, DisplayMsg_count );

    // convert the config values to a setup file
    return setup_upload( pcfg );
}
