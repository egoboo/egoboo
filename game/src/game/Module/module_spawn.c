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
#include "game/Module/Module.hpp"
#include "game/Entities/_Include.hpp"
#include "egolib/Logic/TreasureTables.hpp"
#include "game/CharacterMatrix.h"

static void tilt_characters_to_terrain();
static std::shared_ptr<Object> activate_spawn_file_spawn( spawn_file_info_t& psp_info, const std::shared_ptr<Object> &parent);
static bool activate_spawn_file_load_object( spawn_file_info_t& psp_info );
static void convert_spawn_file_load_name(spawn_file_info_t& psp_info, const Ego::TreasureTables &treasureTables);

/// @file game/module_spawn.c
/// @brief Logic for spawning objects after loading a module
/// @details

void activate_spawn_file_vfs()
{
    /// @author ZZ
    /// @details This function sets up character data, loaded from "SPAWN.TXT"
    std::unordered_map<int, std::string> reservedSlots; //Keep track of which slot numbers are reserved by their load name
    std::unordered_set<std::string> dynamicObjectList;  //references to slots that need to be dynamically loaded later
    std::vector<spawn_file_info_t> objectsToSpawn;      //The full list of objects to be spawned 

    //First load treasure tables
    Ego::TreasureTables treasureTables("mp_data/randomtreasure.txt");

    // Turn some back on
    ReadContext ctxt("mp_data/spawn.txt");
    if (!ctxt.ensureOpen())
    {
		std::ostringstream os;
		os << "unable to read spawn file `" << ctxt.getLoadName() << "`" << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }

    else
    {
        std::shared_ptr<Object> parent = nullptr;

        // First load spawn data of every object.
        ctxt.next(); /// @todo Remove this hack.
        while(!ctxt.is(ReadContext::Traits::endOfInput()))
        {
            spawn_file_info_t entry;

            // Read next entry
            if(!spawn_file_read(ctxt, entry))
            {
                break; //no more entries
            }

            //Spit out a warning if they break the limit
            if ( objectsToSpawn.size() >= OBJECTS_MAX )
            {
				Log::get().warn("Too many objects in file \"%s\"! Maximum number of objects is %d.\n", ctxt.getLoadName().c_str(), OBJECTS_MAX );
                break;
            }

            // check to see if the slot is valid
            if ( entry.slot >= INVALID_PRO_REF )
            {
				Log::get().warn("Invalid slot %d for \"%s\" in file \"%s\".\n", entry.slot, entry.spawn_comment, ctxt.getLoadName().c_str() );
                continue;
            }

            //convert the spawn name into a format we like
            convert_spawn_file_load_name(entry, treasureTables);

            // If it is a dynamic slot, remember to dynamically allocate it for later
            if ( entry.slot <= -1 )
            {
                dynamicObjectList.insert(entry.spawn_comment);
            }

            //its a static slot number, mark it as reserved if it isnt already
            else if (reservedSlots[entry.slot].empty())
            {
                reservedSlots[entry.slot] = entry.spawn_comment;
            }

            //Finished with this object for now
            objectsToSpawn.push_back(entry);
        }

        //Next we dynamically find slot numbers for each of the objects in the dynamic list
        for(const std::string &spawnName : dynamicObjectList)
        {
            PRO_REF profileSlot;

            //Find first free slot that is not the spellbook slot
            for (profileSlot = 1 + MAX_IMPORT_PER_PLAYER * MAX_PLAYER; profileSlot < INVALID_PRO_REF; ++profileSlot)
            {
                //don't try to grab loaded profiles
                if (ProfileSystem::get().isValidProfileID(profileSlot)) continue;

                //the slot already dynamically loaded by a different spawn object of the same type that we are, no need to reload in a new slot
                if(reservedSlots[profileSlot] == spawnName) {
                     break;
                }

                //found a completely free slot
                if (reservedSlots[profileSlot].empty())
                {
                    //Reserve this one for us
                    reservedSlots[profileSlot] = spawnName;
                    break;
                }
            }

            //If all slots are reserved, spit out a warning (very unlikely unless there is a bug somewhere)
            if ( profileSlot == INVALID_PRO_REF ) {
				Log::get().warn( "Could not allocate free dynamic slot for object (%s). All %d slots in use?\n", spawnName.c_str(), INVALID_PRO_REF );
            }
        }

        //Now spawn each object in order
        for(spawn_file_info_t &spawnInfo : objectsToSpawn)
        {
            //Dynamic slot number? Then figure out what slot number is assigned to us
            if(spawnInfo.slot <= -1) {
                for(const auto &element : reservedSlots)
                {
                    if(element.second == spawnInfo.spawn_comment)
                    {
                        spawnInfo.slot = element.first;
                        break;
                    }
                }
            }

            // If nothing is already in that slot, try to load it.
            if (!ProfileSystem::get().isValidProfileID(spawnInfo.slot))
            {
                bool import_object = spawnInfo.slot > (_currentModule->getImportAmount() * MAX_IMPORT_PER_PLAYER);

                if ( !activate_spawn_file_load_object( spawnInfo ) )
                {
                    // no, give a warning if it is useful
                    if ( import_object )
                    {
						Log::get().warn("%s:%d:%s: the object \"%s\"(slot %d) in file \"%s\" does not exist on this machine\n", \
							            __FILE__, __LINE__, __FUNCTION__, spawnInfo.spawn_comment, spawnInfo.slot, \
							            ctxt.getLoadName().c_str() );
                    }
                    continue;
                }
            }

            // we only reach this if everything was loaded properly
            std::shared_ptr<Object> spawnedObject = activate_spawn_file_spawn(spawnInfo, parent);

            //We might become the new parent
            if (spawnedObject != nullptr && spawnInfo.attach == ATTACH_NONE) {
                parent = spawnedObject;
            }
        }

        ctxt.close();
    }

    // Fix tilting trees problem
    tilt_characters_to_terrain();
}

std::shared_ptr<Object> activate_spawn_file_spawn(spawn_file_info_t& psp_info, const std::shared_ptr<Object> &parent)
{
    if ( !psp_info.do_spawn || psp_info.slot < 0 ) return nullptr;

    PRO_REF iprofile = static_cast<PRO_REF>(psp_info.slot);

    //Require a valid parent?
    if(psp_info.attach != ATTACH_NONE && !parent) {
        Log::get().warn("Failed to spawn %s due to missing parent!\n", psp_info.spawn_name);
        return nullptr;
    }

    // Spawn the character
    std::shared_ptr<Object> pobject = _currentModule->spawnObject(psp_info.pos, iprofile, psp_info.team, psp_info.skin, psp_info.facing, psp_info.pname == nullptr ? "" : psp_info.pname, ObjectRef::Invalid);

    //Failed to spawn?
    if (!pobject) {
        Log::get().warn("Failed to spawn %s!\n", psp_info.spawn_name);
        return nullptr;
    }

    //Add money
    pobject->giveMoney(psp_info.money);

    //Set AI stuff
    pobject->ai.content = psp_info.content;
    pobject->ai.passage = psp_info.passage;

    // determine the attachment
    switch(psp_info.attach)
    {
        case ATTACH_NONE:
            make_one_character_matrix(pobject->getObjRef());
        break;

        case ATTACH_INVENTORY:
            // Inventory character
            Inventory::add_item(parent->getObjRef(), pobject->getObjRef(), parent->getInventory().getFirstFreeSlotNumber(), true);

            //If the character got merged into a stack, then it will be marked as terminated
            if(pobject->isTerminated()) {
                return nullptr;
            }

            // Make spellbooks change
            SET_BIT(pobject->ai.alert, ALERTIF_GRABBED);
        break;

        case ATTACH_LEFT:
        case ATTACH_RIGHT:
            // Wielded character
            grip_offset_t grip_off = (ATTACH_LEFT == psp_info.attach) ? GRIP_LEFT : GRIP_RIGHT;

            if(pobject->getObjectPhysics().attachToObject(parent, grip_off)) {
                // Handle the "grabbed" messages
                //scr_run_chr_script(pobject);
                UNSET_BIT(pobject->ai.alert, ALERTIF_GRABBED);
            }
        break;

    }

    // Set the starting pinfo->level
    if (psp_info.level > 0) {
        if (pobject->experiencelevel < psp_info.level) {
            pobject->experience = pobject->getProfile()->getXPNeededForLevel(psp_info.level);
        }
    }

    // automatically identify and unkurse all player starting equipment? I think yes.
    if (!_currentModule->isImportValid() && nullptr != parent && parent->isPlayer()) {
        pobject->nameknown = true;
        pobject->iskursed = false;
    }

    // Turn on input devices
    if ( psp_info.stat )
    {
        // what we do depends on what kind of module we're loading
        if ( 0 == _currentModule->getImportAmount() && _currentModule->getPlayerList().size() < _currentModule->getPlayerAmount() )
        {
            // a single player module

            bool player_added = _currentModule->addPlayer(pobject, &InputDevices.lst[local_stats.player_count] );

            if ( _currentModule->getImportAmount() == 0 && player_added )
            {
                // !!!! make sure the player is identified !!!!
                pobject->nameknown = true;
            }
        }
        else if ( _currentModule->getPlayerList().size() < _currentModule->getImportAmount() && _currentModule->getPlayerList().size() < _currentModule->getPlayerAmount() && _currentModule->getPlayerList().size() < g_importList.count )
        {
            // A multiplayer module

            int local_index = -1;
            for ( size_t tnc = 0; tnc < g_importList.count; tnc++ )
            {
                if (pobject->getProfileID() <= import_data.max_slot && ProfileSystem::get().isValidProfileID(pobject->getProfileID()))
                {
                    int islot = REF_TO_INT( pobject->getProfileID() );

                    if ( import_data.slot_lst[islot] == g_importList.lst[tnc].slot )
                    {
                        local_index = tnc;
                        break;
                    }
                }
            }

            if ( -1 != local_index )
            {
                // It's a local input
                _currentModule->addPlayer(pobject, &InputDevices.lst[g_importList.lst[local_index].local_player_num]);
            }
            else
            {
                // It's a remote input
                _currentModule->addPlayer(pobject, nullptr);
            }
        }
    }

    return pobject;
}

bool activate_spawn_file_load_object( spawn_file_info_t& psp_info )
{
    /// @author BB
    /// @details Try to load a global object named int psp_info->spawn_coment into
    ///               slot psp_info->slot

    STRING filename;
    PRO_REF ipro;

    if ( psp_info.slot < 0 ) return false;

    //Is it already loaded?
    ipro = ( PRO_REF )psp_info.slot;
    if (ProfileSystem::get().isValidProfileID(ipro)) return false;

    // do the loading
    if ( CSTR_END != psp_info.spawn_comment[0] )
    {
        // we are relying on the virtual mount point "mp_objects", so use
        // the vfs/PHYSFS file naming conventions
        snprintf( filename, SDL_arraysize( filename ), "mp_objects/%s", psp_info.spawn_comment );

        if(!vfs_exists(filename)) {
            if(psp_info.slot > MAX_IMPORT_PER_PLAYER * MAX_PLAYER) {
				Log::get().warn("activate_spawn_file_load_object() - Object does not exist: %s\n", filename);
            }

            return false;
        }

        psp_info.slot = ProfileSystem::get().loadOneProfile(filename, psp_info.slot);
    }

    return ProfileSystem::get().isValidProfileID((PRO_REF)psp_info.slot);
}

void convert_spawn_file_load_name(spawn_file_info_t& psp_info, const Ego::TreasureTables &treasureTables)
{
    /// @author ZF
    /// @details This turns a spawn comment line into an actual folder name we can use to load something with

    // trim any excess spaces off the psp_info->spawn_coment
    str_trim( psp_info.spawn_comment );

    //If it is a reference to a random treasure table then get a random object from that table
    if ( '%' == psp_info.spawn_comment[0] )
    {
        std::string treasureTableName = psp_info.spawn_comment;
        std::string treasureName = treasureTables.getRandomTreasure(treasureTableName);
        strncpy(psp_info.spawn_comment, treasureName.c_str(), SDL_arraysize(psp_info.spawn_comment));
    }

    // make sure it ends with a .obj extension
    if ( NULL == strstr( psp_info.spawn_comment, ".obj" ) )
    {
        strcat( psp_info.spawn_comment, ".obj" );
    }

    // no capital letters
    strlwr( psp_info.spawn_comment );
}

void tilt_characters_to_terrain()
{
    /// @author ZZ
    /// @details This function sets all of the character's starting tilt values

    Uint8 twist;

    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator())
    {
        if ( object->isTerminated() ) continue;

        if ( object->getProfile()->hasStickyButt() )
        {
            twist = _currentModule->getMeshPointer()->get_twist( object->getTile() );
            object->ori.map_twist_facing_y = g_meshLookupTables.twist_facing_y[twist];
            object->ori.map_twist_facing_x = g_meshLookupTables.twist_facing_x[twist];
        }
        else
        {
            object->ori.map_twist_facing_y = orientation_t::MAP_TURN_OFFSET;
            object->ori.map_twist_facing_x = orientation_t::MAP_TURN_OFFSET;
        }
    }
}
