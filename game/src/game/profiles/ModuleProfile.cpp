#include "game/profiles/ModuleProfile.hpp"

ModuleProfile::ModuleProfile() :
	_loaded(false),
	_name(),
	_base(),
	_icon(),
	_vfsPath()
{
    mod_file__init(&_base);
}

ModuleProfile::~ModuleProfile()
{
	oglx_texture_release(&_icon);
}

bool ModuleProfile::isModuleUnlocked() const
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

ModuleFilter ModuleProfile::getModuleType() const
{
    return _base.moduletype;
}
