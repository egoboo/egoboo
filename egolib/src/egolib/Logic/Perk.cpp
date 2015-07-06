#include "Perk.hpp"

namespace Ego
{
namespace Perks
{

Perk::Perk() :
    _id(NR_OF_PERKS),
    _perkType(Ego::Attribute::MIGHT),
    _name("INVALID"),
    _description(),
    _perkRequirement(NR_OF_PERKS),
    _icon()
//    _icon(new oglx_texture_t())
{
    //default constructor for uninitialized Perk
}


const std::string& Perk::getName() const
{
    return _name;
}

const std::string& Perk::getDescription() const
{
    return _description;
}

PerkID Perk::getRequirement() const
{
    return _perkRequirement;
}

}
}