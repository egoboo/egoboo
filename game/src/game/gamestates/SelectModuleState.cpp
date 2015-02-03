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

/// @file game/gamestates/SelectModuleState.cpp
/// @details The Main Menu of the game, the first screen presented to the players
/// @author Johan Jansen

#include "game/core/GameEngine.hpp"
#include "game/audio/AudioSystem.hpp"
#include "game/gamestates/SelectModuleState.hpp"
#include "egolib/platform.h"
#include "game/ui.h"
#include "game/menu.h"
#include "game/game.h"
#include "game/graphic_texture.h"
#include "game/gui/Button.hpp"
#include "game/gui/Image.hpp"
#include "game/gui/ModuleSelector.hpp"

/// the module data that the menu system needs
class MenuLoadModuleData
{
public:
	MenuLoadModuleData();

	~MenuLoadModuleData();

	bool isModuleUnlocked() const;

private:
    bool _loaded;
    std::string _name;
    mod_file_t _base;                            ///< the data for the "base class" of the module

    oglx_texture_t _icon;                        ///< the index of the module's tile image
    std::string _vfsPath;                        ///< the virtual pathname of the module
    std::string _destPath;                       ///< the path that module data can be written into

    friend class SelectModuleState;
};

SelectModuleState::SelectModuleState() : SelectModuleState( std::list<std::shared_ptr<LoadPlayerElement>>() )
{
	//ctor
}

SelectModuleState::SelectModuleState(const std::list<std::shared_ptr<LoadPlayerElement>> &players) :
	_onlyStarterModules(players.empty()),
	_background(std::make_shared<Image>()),
	_filterButton(std::make_shared<Button>("All Modules", SDLK_TAB)),
	_moduleFilter(FILTER_OFF),
	_validModules(),
	_selectedPlayerList(players),
	_loadedModules()
{
    // Figure out at what offset we want to draw the module menu.
    int moduleMenuOffsetX = ( GFX_WIDTH  - 640 ) / 2;
    moduleMenuOffsetX = std::max(0, moduleMenuOffsetX);

    int moduleMenuOffsetY = ( GFX_HEIGHT - 480 ) / 2;
    moduleMenuOffsetY = std::max(0, moduleMenuOffsetY);

	//Load all modules
	if(_validModules.empty())
	{
		loadAllModules();
	}

	//Set default filter
	if(_onlyStarterModules)
	{
		setModuleFilter(FILTER_STARTER);
	}
	else
	{
		setModuleFilter(FILTER_OFF);
	}
	addComponent(_background);

	//Add the buttons
	std::shared_ptr<Button> chooseModule = std::make_shared<Button>("Choose Module", SDLK_RETURN);
	chooseModule->setPosition(moduleMenuOffsetX + 377, moduleMenuOffsetY + 173);
	chooseModule->setSize(200, 30);
	chooseModule->setVisible(false);
	addComponent(chooseModule);

	std::shared_ptr<Button> backButton = std::make_shared<Button>("Back", SDLK_ESCAPE);
	backButton->setPosition(moduleMenuOffsetX + 377, chooseModule->getY() + 50);
	backButton->setSize(200, 30);
	backButton->setOnClickFunction(
		[this]{
			this->endState();
		});
	addComponent(backButton);

	std::shared_ptr<ModuleSelector> moduleSelector = std::make_shared<ModuleSelector>();
	addComponent(moduleSelector);

	//Only draw module filter for non-starter games
	if(!_onlyStarterModules)
	{
		_filterButton->setPosition(moduleMenuOffsetX + 337, moduleMenuOffsetY - 27);
		_filterButton->setSize(200, 30);
		_filterButton->setOnClickFunction(
			[this]{
				//TODO
			});
		addComponent(_filterButton);		
	}

}

void SelectModuleState::setModuleFilter(const ModuleFilter filter)
{
	_moduleFilter = filter;

	//Load list of valid modules
	_validModules.clear();

	//Build list of valid modules
	for(const std::shared_ptr<MenuLoadModuleData> &module : _validModules)
	{
		// if this module is not valid given the game options and the
		// selected players, skip it
		if (!module->isModuleUnlocked()) continue;

		if (_onlyStarterModules && 0 == module->_base.importamount)
		{
		    // starter module
		    _validModules.push_back(module);
		}
		else
		{
		    if (FILTER_OFF != _moduleFilter && static_cast<ModuleFilter>(module->_base.moduletype) != _moduleFilter ) continue;
		    if ( _selectedPlayerList.size() > module->_base.importamount ) continue;
		    if ( _selectedPlayerList.size() < module->_base.minplayers ) continue;
		    if ( _selectedPlayerList.size() > module->_base.maxplayers ) continue;

		    // regular module
		    _validModules.push_back(module);
		}
	}

	// load background depending on current filter
	switch (_moduleFilter)
	{
		case FILTER_STARTER:	
			_background->setImage("mp_data/menu/menu_advent");
			_filterButton->setText("Starter Modules");
		break;
	    
	    case FILTER_MAIN: 		
	    	_background->setImage("mp_data/menu/menu_draco");
			_filterButton->setText("Main Quest");
		break;
	    
	    case FILTER_SIDE: 		
	    	_background->setImage("mp_data/menu/menu_sidequest");
			_filterButton->setText("Side Quest");
		break;
	    
	    case FILTER_TOWN: 		
	    	_background->setImage("mp_data/menu/menu_town");
			_filterButton->setText("Towns and Cities");
		break;
	    
	    case FILTER_FUN:  		
	    	_background->setImage("mp_data/menu/menu_funquest");
			_filterButton->setText("Fun Modules");
		break;

	    default:
	    case FILTER_OFF: 		
	    	_background->setImage("mp_data/menu/menu_allquest");  
			_filterButton->setText("All Modules");
	    break;
	}

	//Place background in bottom right corner
	_background->setSize(_background->getTextureWidth(), _background->getTextureHeight());
	_background->setPosition((GFX_WIDTH/2) - (_background->getWidth()/2), GFX_HEIGHT - _background->getHeight());
}

void SelectModuleState::update()
{
}

void SelectModuleState::drawContainer()
{
	ui_beginFrame(0);
	{
	    draw_mouse_cursor();
	}
	ui_endFrame();
}

void SelectModuleState::beginState()
{
	// menu settings
    SDL_WM_GrabInput( SDL_GRAB_OFF );
}

void SelectModuleState::loadAllModules()
{
	//Clear any previously loaded first
	_loadedModules.clear();

    // Search for all .mod directories and load the module info
    vfs_search_context_t *ctxt = vfs_findFirst( "mp_modules", "mod", VFS_SEARCH_DIR );
    const char * vfs_ModPath = vfs_search_context_get_current( ctxt );

    while (nullptr != ctxt && VALID_CSTR( vfs_ModPath ))
    {
    	STRING loadname;
    	std::shared_ptr<MenuLoadModuleData> module = std::make_shared<MenuLoadModuleData>();

        // save the filename
        snprintf( loadname, SDL_arraysize( loadname ), "%s/gamedat/menu.txt", vfs_ModPath );
        if ( nullptr != module_load_info_vfs( loadname, &( module->_base ) ) )
        {
            // mark the module data as loaded
            module->_loaded = true;

            // save the module path
            module->_vfsPath = vfs_ModPath;

            /// @note just because we can't load the title image DOES NOT mean that we ignore the module
            // load title image
            snprintf( loadname, SDL_arraysize(loadname), "%s/gamedat/title", module->_vfsPath.c_str() );
			ego_texture_load_vfs(&module->_icon, loadname, INVALID_KEY );

            /// @note This is kinda a cheat since we know that the virtual paths all begin with "mp_" at the moment.
            // If that changes, this line must be changed as well.
            // Save the user data directory version of the module path.
            snprintf(loadname, SDL_arraysize(loadname), "/%s", vfs_ModPath + 3 );
            module->_destPath = loadname;

            // same problem as above
            strncpy(loadname, vfs_ModPath + 11, SDL_arraysize(loadname) );
            module->_name = loadname;

            _loadedModules.push_back(module);
        }
        else
        {
        	log_warning("Unable to load module: %s\n", loadname);
        }

        ctxt = vfs_findNext( &ctxt );
        vfs_ModPath = vfs_search_context_get_current( ctxt );
    }
    vfs_findClose( &ctxt );
}

void SelectModuleState::printDebugModuleList()
{
    // Log a directory list
    vfs_FILE* filesave = vfs_openWrite( "/debug/modules.txt" );

    if(filesave == nullptr) {
    	return;
    }

    vfs_printf( filesave, "This file logs all of the modules found\n" );
    vfs_printf( filesave, "** Denotes an invalid module\n" );
    vfs_printf( filesave, "## Denotes an unlockable module\n\n" );

    for(size_t imod = 0; imod < _loadedModules.size(); ++imod)
    {
    	const std::shared_ptr<MenuLoadModuleData> &module = _loadedModules[imod];

        if (!module->_loaded)
        {
            vfs_printf(filesave, "**.  %s\n", module->_vfsPath.c_str());
        }
        else if ( module->isModuleUnlocked() )
        {
            vfs_printf(filesave, "%02d.  %s\n", REF_TO_INT(imod), module->_vfsPath.c_str());
        }
        else
        {
            vfs_printf(filesave, "##.  %s\n", module->_vfsPath.c_str());
        }
    }


    vfs_close(filesave);
}

MenuLoadModuleData::MenuLoadModuleData() :
	_loaded(false),
	_name(),
	_base(),
	_icon(),
	_vfsPath(),
	_destPath()
{
    mod_file__init(&_base);
}

MenuLoadModuleData::~MenuLoadModuleData()
{
	oglx_texture_Release(&_icon);
}

bool MenuLoadModuleData::isModuleUnlocked() const
{
    // First check if we are in developers mode or that the right module has been beaten before
    if ( cfg.dev_mode )
    {
        return true;
    }

    if (module_has_idsz_vfs(_base.reference, _base.unlockquest.id, 0, nullptr))
    {
        return true;
    }

//ZF> TODO: re-enable
/*
    if (base.importamount > 0)
    {
        // If that did not work, then check all selected players directories, but only if it isn't a starter module
        for(const std::shared_ptr<LoadPlayerElement> &player : _selectedPlayerList)
        {
            // find beaten quests or quests with proper level
            if(!player->hasQuest(base.unlockquest.id, base.unlockquest.level)) {
                return false;
            }
        }
    }
*/

    return true;
}