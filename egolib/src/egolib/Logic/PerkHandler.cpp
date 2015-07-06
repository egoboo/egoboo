#include "PerkHandler.hpp"
#include "egolib/Graphics/TextureManager.hpp"
#include "egolib/fileutil.h"

namespace Ego
{
namespace Perks
{

std::unique_ptr<PerkHandler> PerkHandler::_singleton = std::unique_ptr<PerkHandler>(new PerkHandler());

PerkHandler::PerkHandler() :
    _perkList()
{
    //Initialize all perks
    initializePerk(TOUGHNESS, Attribute::MIGHT, "mp_data/perks/toughness",
        "Toughness", "+2 extra Life.");
    initializePerk(CARTOGRAPHY, Attribute::INTELLECT, "mp_data/perks/cartography",
        "Cartography", "Always reveals the map, even though you haven't found it yet.");
    initializePerk(SENSE_KURSES, Attribute::INTELLECT, "mp_data/perks/sense_kurses",
        "Sense Kurses", "Warns of nearby items that are Kursed by flashing black.");
    initializePerk(KURSE_IMMUNITY, Attribute::INTELLECT, "mp_data/perks/kurse_immunity",
        "Protection from Kurses", "Character is not affected by Kursed items.", SENSE_KURSES);
    initializePerk(ACROBATIC, Attribute::AGILITY, "mp_data/perks/acrobatic",
        "Acrobatic", "Allows double jumping.");

    //Make sure all perks have been initialized properly
    for(size_t i = 0; i < _perkList.size(); ++i) {
        assert(_perkList[i] && _perkList[i]->_id != NR_OF_PERKS);
    }
}

void PerkHandler::initializePerk(const PerkID id, const Ego::Attribute::AttributeType type, const std::string &iconPath,
        const std::string &name, const std::string &description, const PerkID perkRequirement)
{
    //Allocate memory
    std::unique_ptr<Perk> &perk = _perkList[id];
    perk = std::unique_ptr<Perk>(new Perk());

    //Initialize data
    perk->_id = id;
    perk->_perkType = type;
    perk->_name = name;
    perk->_description = description;
    perk->_perkRequirement = perkRequirement;
//    ego_texture_load_vfs(perk->_icon.get(), iconPath.c_str(), TRANSCOLOR);
}

PerkID PerkHandler::fromString(const std::string &name) const
{
    for(const std::unique_ptr<Perk> &perk : _perkList)
    {
        if(perk->getName() == name)
        {
            return perk->_id;
        }
    }

    //Failed
    return NR_OF_PERKS;
}

const Perk& PerkHandler::getPerk(const PerkID id) const
{
    return *_perkList[id];
}

} //Perk
} //Ego