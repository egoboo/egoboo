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

/// @brief Routines for reading and writing <tt>"controls.txt"</tt>.

#include "egolib/InputControl/ControlSettingsFile.hpp"
#include "egolib/InputControl/InputDevice.hpp"
#include "egolib/Log/_Include.hpp"

#include "egolib/fileutil.h"

static std::string controlInputToString(const Ego::Input::InputDevice::InputButton &button);


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool input_settings_load_vfs(const char* szFilename)
{
    /// @author ZZ
    /// @details This function reads the controls.txt file, version 3

    ReadContext ctxt(szFilename);

    // Read input for each player
    for (size_t i = 0; i < Ego::Input::InputDevice::DeviceList.size(); i++)
    {
        std::string currenttag;
        Ego::Input::InputDevice &device = Ego::Input::InputDevice::DeviceList[i];

        //Read each input control button
        for (size_t icontrol = 0; icontrol < static_cast<size_t>(Ego::Input::InputDevice::InputButton::COUNT); ++icontrol) {
            if(!ctxt.skipToColon(true)) {
                return false;
            }

            currenttag = ctxt.readToEndOfLine();
            if (!currenttag.empty())
            {
                const SDL_Keycode keyCode = SDL_GetKeyFromName(currenttag.c_str());
                device.setInputMapping(static_cast<Ego::Input::InputDevice::InputButton>(icontrol), keyCode);
            }
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool input_settings_save_vfs( const char* szFilename )
{
    /// @author ZF
    /// @details This function saves all current game settings to "controls.txt"
    vfs_FILE* filewrite = vfs_openWrite( szFilename );
    if ( NULL == filewrite )
    {
        Log::get().warn( "Could not save input settings (%s)!\n", szFilename );
        return false;
    }

    //Add version number
    vfs_put_version(filewrite, 4);

    // Just some information
    vfs_puts( "Controls\n", filewrite );
    vfs_puts( "========\n", filewrite );
    vfs_puts( "This file lets users modify the handling of input devices.\n", filewrite );
    vfs_puts( "See the game manual for a list of settings and more info.\n", filewrite );
    vfs_puts( "Note that you can mix KEY_ type settings with other \n", filewrite );
    vfs_puts( "devices... Write the input after the colons!\n\n", filewrite );

    vfs_puts( "General Controls\n", filewrite );
    vfs_puts( "========\n", filewrite );
    vfs_puts( "These are general control codes and cannot be changed\n", filewrite );
    vfs_puts( "ESC                       - Open ingame menu\n", filewrite );
    vfs_puts( "SPACE                     - Respawn character (if dead and possible)\n", filewrite );
    vfs_puts( "1 to 7                    - Show character detailed stats\n", filewrite );
    vfs_puts( "F11                       - Take screenshot\n", filewrite );
    vfs_puts( "\n", filewrite );

    // The actual settings
    for (size_t i = 0; i < Ego::Input::InputDevice::DeviceList.size(); i++)
    {
        const Ego::Input::InputDevice &device = Ego::Input::InputDevice::DeviceList[i];
        const std::string player = "\nPLAYER " + std::to_string(i + 1) + "\n";

        //which player
        vfs_puts(player.c_str(), filewrite);
        vfs_puts("========\n", filewrite);

        //controller type
        //const std::string controller = "CONTROLLER:         " + translate_input_type_to_string(pdevice.device_type);
        //vfs_puts(controller.c_str(), filewrite);

        for (size_t icontrol = 0; icontrol < static_cast<size_t>(Ego::Input::InputDevice::InputButton::COUNT); ++icontrol) {
            Ego::Input::InputDevice::InputButton button = static_cast<Ego::Input::InputDevice::InputButton>(icontrol);

            std::stringstream output;
            output << controlInputToString(button) << "\t\t" << ": " << device.getMappedInputName(button) << '\n';
            vfs_puts(output.str().c_str(), filewrite);
        }

    }

    // All done
    vfs_close(filewrite);

    return true;
}

std::string controlInputToString(const Ego::Input::InputDevice::InputButton &button)
{
    switch(button)
    {
        case Ego::Input::InputDevice::InputButton::MOVE_UP:           return "Move Up";
        case Ego::Input::InputDevice::InputButton::MOVE_RIGHT:        return "Move Right";
        case Ego::Input::InputDevice::InputButton::MOVE_DOWN:         return "Move Down";
        case Ego::Input::InputDevice::InputButton::MOVE_LEFT:         return "Move Left";
        case Ego::Input::InputDevice::InputButton::JUMP:              return "Jump";
        case Ego::Input::InputDevice::InputButton::USE_LEFT:          return "Use Left";
        case Ego::Input::InputDevice::InputButton::GRAB_LEFT:         return "Grab Left";
        case Ego::Input::InputDevice::InputButton::USE_RIGHT:         return "Use Right";
        case Ego::Input::InputDevice::InputButton::GRAB_RIGHT:        return "Grab Right";
        case Ego::Input::InputDevice::InputButton::INVENTORY:         return "Inventory";
        case Ego::Input::InputDevice::InputButton::STEALTH:           return "Stealth";
        case Ego::Input::InputDevice::InputButton::CAMERA_LEFT:       return "CAMERA_LEFT";
        case Ego::Input::InputDevice::InputButton::CAMERA_RIGHT:      return "CAMERA_RIGHT";
        case Ego::Input::InputDevice::InputButton::CAMERA_ZOOM_IN:    return "CAMERA_ZOOM_IN";
        case Ego::Input::InputDevice::InputButton::CAMERA_ZOOM_OUT:   return "CAMERA_ZOOM_OUT";
        case Ego::Input::InputDevice::InputButton::CAMERA_CONTROL:    return "Camera Control";
        case Ego::Input::InputDevice::InputButton::COUNT:             return "UNKNOWN";
    }

    throw UnhandledSwitchCaseException(__FILE__, __LINE__, "unreachable code reached");
}
