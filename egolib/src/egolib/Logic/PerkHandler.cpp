#include "PerkHandler.hpp"
#include "egolib/Graphics/TextureManager.hpp"
#include "egolib/fileutil.h"

namespace Ego
{
namespace Perks
{

PerkHandler::PerkHandler() :
    _perkList()
{
    //Initialize all perks
    initializePerk(TOUGHNESS, Attribute::MIGHT, "mp_data/perks/toughness",
        "Toughness", "+2 extra Life.");
    initializePerk(WEAPON_PROFICIENCY, Attribute::MIGHT, "mp_data/perks/weapon_proficiency",
        "Weapon Proficiency", "Unlocks use of all normal melee and ranged weapons.");
    initializePerk(GIGANTISM, Attribute::MIGHT, "mp_data/perks/gigantism", 
        "Gigantism", "+10% size gain\n+2 Might\n-2 Agility");
    initializePerk(SOLDIERS_FORTITUDE, Attribute::MIGHT, "mp_data/perks/soldiers_fortitude", 
        "Soldiers Fortitude", "+0.15 Life Regeneration");
    initializePerk(BRUTE, Attribute::MIGHT, "mp_data/perks/brute", 
        "Brute", "+10% Damage with melee weapons.\n+1 Might\n-2 Intellect");
    initializePerk(DEFENDER, Attribute::MIGHT, "mp_data/perks/defender", 
        "Defender", "+100% Block Chance with shields");
    initializePerk(MOBILE_DEFENCE, Attribute::MIGHT, "mp_data/perks/mobile_defence", 
        "Mobile Defence", "Allows up to 50% movement while blocking with shield.", DEFENDER);
    initializePerk(HOLD_THE_LINE, Attribute::MIGHT, "mp_data/perks/hold_the_line", 
        "Hold the Line", "Immune to knockback.", DEFENDER);
    initializePerk(STALWART, Attribute::MIGHT, "mp_data/perks/stalwart", 
        "Stalwart", "+1 Poke, Slash and Crush resistance.", DEFENDER);

    //Intellectual
    initializePerk(CARTOGRAPHY, Attribute::INTELLECT, "mp_data/perks/cartography",
        "Cartography", "Always reveals the map, even though you haven't found it yet.");
    initializePerk(NAVIGATION, Attribute::INTELLECT, "mp_data/perks/navigation",
        "Navigation", "Reveals your position on the minimap.", CARTOGRAPHY);
    initializePerk(SENSE_KURSES, Attribute::INTELLECT, "mp_data/perks/sense_kurses",
        "Sense Kurses", "Warns of nearby items that are Kursed by flashing black.");
    initializePerk(KURSE_IMMUNITY, Attribute::INTELLECT, "mp_data/perks/kurse_immunity",
        "Protection from Kurses", "Character is not affected by Kursed items.", SENSE_KURSES);
    initializePerk(DRAGON_BLOOD, Attribute::INTELLECT, "mp_data/perks/dragonblood",
        "Dragon Blood", "+0.25 Mana Regeneration");
    initializePerk(FAST_LEARNER, Attribute::INTELLECT, "mp_data/perks/fast_learner",
        "Fast Learner", "+10% Experience Gain");
    initializePerk(NIGHT_VISION, Attribute::INTELLECT, "mp_data/perks/night_vision",
        "Night Vision", "+25% minimum ambient light.");
    initializePerk(LITERACY, Attribute::INTELLECT, "mp_data/perks/literacy",
        "Literacy", "Can read and write. Allows use of books, scrolls and signs.");
    initializePerk(READ_GNOMISH, Attribute::INTELLECT, "mp_data/perks/use_technological_items",
        "Read G'nomish", "Learn to read G'nomish inscriptions.", LITERACY);
    initializePerk(ARCANE_MAGIC, Attribute::INTELLECT, "mp_data/perks/arcane_magic",
        "Arcane Magic", "Can spend mana to cast Arcane magic through Spellbooks.", LITERACY);
    initializePerk(DIVINE_MAGIC, Attribute::INTELLECT, "mp_data/perks/divine_magic",
        "Divine Magic", "Can spend mana to cast Divine magic through Runes.");
    initializePerk(POISONRY, Attribute::INTELLECT, "mp_data/perks/poisonry",
        "Poisonry", "Always identify Poisoned items.\nCan safely use poisoned items without hurting yourself.\nCan throw bottles of poison.");
    initializePerk(TRAP_LORE, Attribute::INTELLECT, "mp_data/perks/trap_lore",
        "Trap Lore", "Can disarm traps and pick locks.");
    initializePerk(USE_TECHNOLOGICAL_ITEMS, Attribute::INTELLECT, "mp_data/perks/use_technological_items",
        "Use Technological Items", "Can use technological items like Guns, G'nomish tinker, Medkit and Camera.");
    initializePerk(SENSE_UNDEAD, Attribute::INTELLECT, "mp_data/perks/sense_undead",
        "Sense Undead", "Reveals undead monsters on the minimap.", DIVINE_MAGIC);
    initializePerk(PERCEPTIVE, Attribute::INTELLECT, "mp_data/perks/perceptive",
        "Sense Undead", "Can find traps.\nCan find hidden treasure.\n+100% listening range\n10% minimum ambient light.");
    initializePerk(DANGER_SENSE, Attribute::INTELLECT, "mp_data/perks/danger_sense",
        "Danger Sense", "Reveals enemies on the minimap.", PERCEPTIVE);

    //Agility
    initializePerk(ACROBATIC, Attribute::AGILITY, "mp_data/perks/acrobatics",
        "Acrobatic", "Allows double jumping.");
    initializePerk(MASTER_ACROBAT, Attribute::AGILITY, "mp_data/perks/master_acrobat",
        "Master Acrobat", "Allows triple jumping.", ACROBATIC);
    initializePerk(SPRINT, Attribute::AGILITY, "mp_data/perks/sprint",
        "Sprint", "+10% movement speed if at 75% life or more.");
    initializePerk(MOBILITY, Attribute::AGILITY, "mp_data/perks/mobility",
        "Mobility", "Allows up to 50% movement speed while attacking.", SPRINT);
    initializePerk(DASH, Attribute::AGILITY, "mp_data/perks/dash",
        "Dash", "Additional +10% movement speed if uninjured (total +20% with Sprint).", SPRINT);
    initializePerk(DODGE, Attribute::AGILITY, "mp_data/perks/dodge",
        "Dodge", "1% chance per Agility to dodge incoming attacks.");
    initializePerk(MASTERFUL_DODGE, Attribute::AGILITY, "mp_data/perks/masterful_dodge",
        "Masterful Dodge", "+10% Dodge chance.", DODGE);
    initializePerk(BACKSTAB, Attribute::AGILITY, "mp_data/perks/backstab",
        "Backstab", "Attacking enemies from behind deals +10% damage per Agility. Instantly kills sleeping creatures.");


    //Make sure all perks have been initialized properly
    for(size_t i = 0; i < _perkList.size(); ++i) {
        assert(_perkList[i]._id != NR_OF_PERKS);
    }
}

PerkHandler::~PerkHandler()
{
    //dtor
}

void PerkHandler::initializePerk(const PerkID id, const Ego::Attribute::AttributeType type, const std::string &iconPath,
        const std::string &name, const std::string &description, const PerkID perkRequirement)
{
    Perk& perk = _perkList[id];

    //Initialize data
    perk._id = id;
    perk._perkType = type;
    perk._name = name;
    perk._description = description;
    perk._perkRequirement = perkRequirement;
    perk._icon = Ego::DeferredOpenGLTexture(iconPath);

    if(!vfs_exists(iconPath)) {
        log_warning("No icon for perk %s: %s\n", name.c_str(), iconPath.c_str());
    }
}

PerkID PerkHandler::fromString(const std::string &name) const
{
    for(const Perk &perk : _perkList)
    {
        if(perk.getName() == name)
        {
            return perk._id;
        }
    }

    //Failed
    return NR_OF_PERKS;
}

const Perk& PerkHandler::getPerk(const PerkID id) const
{
    return _perkList[id];
}

} //Perk
} //Ego