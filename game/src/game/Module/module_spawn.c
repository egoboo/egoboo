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

#include "egolib/egolib.h"
#include "game/game.h"
#include "game/script_compile.h"
#include "game/Module/Module.hpp"
#include "egolib/Entities/_Include.hpp"
#include "egolib/InputControl/InputDevice.hpp"
#include "game/CharacterMatrix.h"

bool activate_spawn_file_load_object( spawn_file_info_t& psp_info )
{
    /// @author BB
    /// @details Try to load a global object named int psp_info->spawn_coment into
    ///               slot psp_info->slot

    std::string filename;
    PRO_REF ipro;

    if ( psp_info.slot < 0 ) return false;

    //Is it already loaded?
    ipro = ( PRO_REF )psp_info.slot;
    if (ProfileSystem::get().isLoaded(ipro)) return false;

    // do the loading
    if ( CSTR_END != psp_info.spawn_comment[0] )
    {
        // we are relying on the virtual mount point "mp_objects", so use
        // the vfs/PHYSFS file naming conventions
        filename = "mp_objects/" + psp_info.spawn_comment;

        if(!vfs_exists(filename)) {
            if(psp_info.slot > MAX_IMPORT_PER_PLAYER * MAX_PLAYER) {
				Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "object ", "`", filename, "`", " does not exist", Log::EndOfEntry);
            }

            return false;
        }

        psp_info.slot = ProfileSystem::get().loadOneProfile(filename, psp_info.slot).get();
    }

    return ProfileSystem::get().isLoaded((PRO_REF)psp_info.slot);
}

void convert_spawn_file_load_name(spawn_file_info_t& psp_info, const Ego::TreasureTables &treasureTables)
{
    /// @author ZF
    /// @details This turns a spawn comment line into an actual folder name we can use to load something with

    // trim any excess spaces off the psp_info->spawn_coment
    psp_info.spawn_comment = Ego::trim_ws(psp_info.spawn_comment);

    //If it is a reference to a random treasure table then get a random object from that table
    if ( '%' == psp_info.spawn_comment[0] )
    {
        std::string treasureTableName = psp_info.spawn_comment;
        std::string treasureName = treasureTables.getRandomTreasure(treasureTableName);
        psp_info.spawn_comment = treasureName;
    }

    // make sure it ends with a .obj extension
    if ( !id::is_suffix(psp_info.spawn_comment, std::string(".obj") ) )
    {
        psp_info.spawn_comment += ".obj";
    }

    // no capital letters
    id::to_lower_in_situ(psp_info.spawn_comment);
}

//--------------------------------------------------------------------------------------------
void game_load_profile_ai()
{
    /// @author ZF
    /// @details load the AI for each profile, done last so that all reserved slot numbers are already set
    /// since AI scripts can dynamically load new objects if they require it
    // ensure that the script parser exists
    parser_state_t& ps = parser_state_t::get();

    for (const auto &element : ProfileSystem::get().getLoadedProfiles())
    {
        const std::shared_ptr<ObjectProfile> &profile = element.second;

        //Guard agains null elements
        if(profile == nullptr) continue;

        // Load the AI script for this iobj
        std::string filePath = profile->getPathname() + "/script.txt";

        load_ai_script_vfs( ps, filePath, profile.get(), profile->getAIScript() );
    }
}
