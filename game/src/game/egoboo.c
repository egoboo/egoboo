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
#include "game/char.h"
#include "game/collision.h"

#include "game/Entities/EnchantHandler.hpp"
#include "game/Entities/ParticleHandler.hpp"
#include "game/Entities/ObjectHandler.hpp"

#include "game/Module/Module.hpp"
#include "game/Profiles/_Include.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/**
 * @brief
 *  Download the data from the egoboo_config_t data structure to program variables.
 * @param cfg
 *  the configuration
 * @return
 *  @a true on success, @a false on failure
 */
bool config_download(egoboo_config_t *cfg);

/**
 * @brief
 *  Upload the data from program variables into an egoboo_config_t data structure.
 * @param cfg
 *  the configuration
 * @return
 *  @a true on success, @a false on failure
 * @note
 *  This function must be implemented by the user
 */
bool config_upload(egoboo_config_t *cfg);


bool config_synch(egoboo_config_t *cfg, bool fromFile,bool toFile)
{
    if (fromFile)
    {
        if (!setup_download(cfg))   // Download 'setup.txt' into the Egoboo configuration.
        {
            return false;
        }
    }
    if (!config_download(cfg))      // Download the the program variables into the Egoboo configuration.
    {
        return false;
    }
    if (!config_upload(cfg))        // Upload the Egoboo configuration into the program variables.
    {
        return false;
    }
    if (toFile)
    {
        return setup_upload(cfg);   // Upload the Egoboo configuration into 'setup.txt'.
    }
    return true;
}

bool config_download( egoboo_config_t *cfg)
{
    if (!cfg)
    {
        return false;
    }
    // Status display.
    StatusList.on = cfg->hud_displayStatusBars.getValue();
    // Message display.
    DisplayMsg_count = CLIP(cfg->hud_simultaneousMessages_max.getValue(), (uint8_t)EGO_MESSAGE_MIN, (uint8_t)EGO_MESSAGE_MAX);
    DisplayMsg_on = cfg->hud_simultaneousMessages_max.getValue() > 0;

    // Particle display limit.
    ParticleHandler::get().setDisplayLimit(cfg->graphic_simultaneousParticles_max.getValue());

    // Camera options.
    _cameraSystem.getCameraOptions().turnMode = cfg->camera_control.getValue();

    // Sound options.
    _audioSystem.setConfiguration(*cfg);

    // Rendering options.
    gfx_config_download_from_egoboo_config(&gfx, cfg);

    // Texture options.
    oglx_texture_parameters_download_gfx(&tex_params, cfg);

    return true;
}

bool config_upload(egoboo_config_t *cfg)
{
    if (!cfg) return false;

    cfg->camera_control.setValue(_cameraSystem.getCameraOptions().turnMode);

    // Particle limit.
    cfg->graphic_simultaneousParticles_max.setValue(ParticleHandler::get().getDisplayLimit());

    // messages
    cfg->hud_messages_enable.setValue(DisplayMsg_on);
    cfg->hud_simultaneousMessages_max.setValue(!DisplayMsg_on ? 0 : std::max(EGO_MESSAGE_MIN, DisplayMsg_count));

    return true;
}
