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

PerkID Perk::getID() const
{
    return _id;
}

const Ego::DeferredTexture& Perk::getIcon() const
{
    return _icon;
}

Ego::Attribute::AttributeType Perk::getType() const
{
    return _perkType;
}

const Ego::Math::Colour4f& Perk::getColour() const
{
    static const Ego::Math::Colour4f FIRE_BRICK = Ego::Math::Colour4f(0.698f, 0.134f, 0.134f, 1.0f);
    static const Ego::Math::Colour4f FOREST_GREEN = Ego::Math::Colour4f(0.134f, 0.525f, 0.134f, 1.0f);
    static const Ego::Math::Colour4f DARK_VIOLET = Ego::Math::Colour4f(0.580f, 0.000f, 0.827f, 1.0f);
    static const Ego::Math::Colour4f WHITE = Ego::Math::Colour4f::white();

    switch(_perkType)
    {
        case Ego::Attribute::MIGHT:
            return FIRE_BRICK;

        case Ego::Attribute::AGILITY:
            return FOREST_GREEN;

        case Ego::Attribute::INTELLECT:
            return DARK_VIOLET;

        default:
            return WHITE;        
    }
}

}
}