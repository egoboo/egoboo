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
    initializePerk(JOUSTING, Attribute::MIGHT, "mp_data/perks/jousting", 
        "Jousting", "Can use the Lance both on foot and charge while mounted.");
    initializePerk(PERFECTION, Attribute::MIGHT, "mp_data/perks/perfection", 
        "Perfection", "+1 Agility\n+1 Intellect");
    initializePerk(ENDURANCE, Attribute::MIGHT, "mp_data/perks/endurance", 
        "Endurance", "1% per Might to ignore Staggering when injured.");
    initializePerk(ATHLETICS, Attribute::MIGHT, "mp_data/perks/athletics", 
        "Athletics", "Halves movement penality in water.\n+25% jump power.");
    initializePerk(ANCIENT_BLUD, Attribute::MIGHT, "mp_data/perks/ancient_blud", 
        "Ancient Blud", "+0.25 Life Regeneration");
    initializePerk(BLUNT_WEAPONS_MASTERY, Attribute::MIGHT, "mp_data/perks/blunt_weapons_mastery", 
        "Blunt Weapons Mastery", "Melee blunt weapons deal +25% extra CRUSH damage.");
    initializePerk(BRUTAL_STRIKE, Attribute::MIGHT, "mp_data/perks/brutal_strike", 
        "Brutal Strike", "CRUSH weapons have 1% chance per Might\nto inflict Groggy for 2 seconds.", BLUNT_WEAPONS_MASTERY);
    initializePerk(POLEARM_MASTERY, Attribute::MIGHT, "mp_data/perks/polearm_mastery", 
        "Polearm Mastery", "+20% damage with Spears, Tridents and other Polearms.");
    initializePerk(SWORD_MASTERY, Attribute::MIGHT, "mp_data/perks/sword_mastery", 
        "Sword Mastery", "+20% damage with Swords.", WEAPON_PROFICIENCY);
    initializePerk(AXE_MASTERY, Attribute::MIGHT, "mp_data/perks/axe_mastery", 
        "Axe Mastery", "+20% damage with Hatchets and other Axes.", WEAPON_PROFICIENCY);
    initializePerk(GRIM_REAPER, Attribute::MIGHT, "mp_data/perks/grim_reaper", 
        "Grim Reaper", "5% chance to deal +50 EVIL damage with the Scythe.", WEAPON_PROFICIENCY);
    initializePerk(WOLVERINE, Attribute::MIGHT, "mp_data/perks/wolverine", 
        "Wolverine", "+100% damage with Claws.\nGain +0.25 Life Regeneration while holding Claws.");

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
    initializePerk(READ_GNOMISH, Attribute::INTELLECT, "mp_data/perks/read_gnomish",
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
        "Sense Undead", "Reveals undead monsters on the minimap.");
    initializePerk(PERCEPTIVE, Attribute::INTELLECT, "mp_data/perks/perceptive",
        "Perceptive", "Can find traps and hidden treasure.\n+100% listening range\n10% minimum ambient light.");
    initializePerk(DANGER_SENSE, Attribute::INTELLECT, "mp_data/perks/danger_sense",
        "Danger Sense", "Reveals enemies on the minimap.", PERCEPTIVE);
    initializePerk(SENSE_INVISIBLE, Attribute::INTELLECT, "mp_data/perks/sense_invisible",
        "Sense Invisible", "Reveals invisible enemies.", PERCEPTIVE);
    initializePerk(THAUMATURGY, Attribute::INTELLECT, "mp_data/perks/thaumaturgy",
        "Thaumaturgy", "Can use magic staves and wands to cast spells.");
    initializePerk(WAND_MASTERY, Attribute::INTELLECT, "mp_data/perks/wand_mastery",
        "Wand Mastery", "1% chance per Intellect to not consume charge when using wands.", THAUMATURGY);
    initializePerk(ELEMENTAL_RESISTANCE, Attribute::INTELLECT, "mp_data/perks/elemental_resistance",
        "Elemental Resistance", "+1 Fire, Ice and Zap resistance.");
    initializePerk(FIRE_WARD, Attribute::INTELLECT, "mp_data/perks/fire_ward",
        "Fire Ward", "+3 Fire resistance.", ELEMENTAL_RESISTANCE);
    initializePerk(ICE_WARD, Attribute::INTELLECT, "mp_data/perks/ice_ward",
        "Ice Ward", "+3 Ice resistance.", ELEMENTAL_RESISTANCE);
    initializePerk(ZAP_WARD, Attribute::INTELLECT, "mp_data/perks/zap_ward",
        "Zap Ward", "+3 Zap resistance.", ELEMENTAL_RESISTANCE);
    initializePerk(POWER, Attribute::INTELLECT, "mp_data/perks/power",
        "Power", "+2 Mana.", ARCANE_MAGIC);
    initializePerk(JACK_OF_ALL_TRADES, Attribute::INTELLECT, "mp_data/perks/jack_of_all_trades",
        "Jack of All Trades", "Gets 2 additional perks to select from each level.");
    initializePerk(TELEPORT_MASTERY, Attribute::INTELLECT, "mp_data/perks/teleport_mastery",
        "Teleport Mastery", "No chance to botch teleports.\nTeleport Spell costs 33% less mana.", ARCANE_MAGIC);
    initializePerk(DISINTEGRATE, Attribute::INTELLECT, "mp_data/perks/disintegrate",
        "Disintegrate", "+0.025% chance per Intellect to deal +100 extra damage with Zap damage spells.", ARCANE_MAGIC);
    initializePerk(SORCERY, Attribute::INTELLECT, "mp_data/perks/sorcery",
        "Sorcery", "+10% Spell Damage", ARCANE_MAGIC);
    initializePerk(SPELL_MASTERY, Attribute::INTELLECT, "mp_data/perks/spell_mastery",
        "Spell Mastery", "+1 Spell Power", ARCANE_MAGIC);
    initializePerk(MYSTIC_INTELLECT, Attribute::INTELLECT, "mp_data/perks/mystic_intellect",
        "Mystic Intellect", "+1 Mana\n+0.1 Mana Regeneration", ARCANE_MAGIC);
    initializePerk(MEDITATION, Attribute::INTELLECT, "mp_data/perks/meditation",
        "Meditation", "+0.15 Mana Regeneration");
    initializePerk(BOOKWORM, Attribute::INTELLECT, "mp_data/perks/bookworm",
        "Bookworm", "+2 Intellect.\n-2 Might.");
    initializePerk(MERCENARY, Attribute::INTELLECT, "mp_data/perks/mercenary",
        "Mercenary", "Gain +1 Zenny for each monster killed.");
    initializePerk(DARK_ARTS_MASTERY, Attribute::INTELLECT, "mp_data/perks/dark_arts_mastery",
        "Dark Arts Mastery", "+20% Evil damage with magic.");
    initializePerk(MAGIC_ATTUNEMENT, Attribute::INTELLECT, "mp_data/perks/magic_attunement",
        "Magic Attunement", "25% of life healing also refills your mana.");
    initializePerk(CRUSADER, Attribute::INTELLECT, "mp_data/perks/crusader",
        "Crusader", "Regain 1 mana each time you destroy an undead.");
    initializePerk(LORE_MASTER, Attribute::INTELLECT, "mp_data/perks/lore_master",
        "Lore Master", "Automatically identify all items.", BOOKWORM);

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
    initializePerk(CRACKSHOT, Attribute::AGILITY, "mp_data/perks/crackshot",
        "Crackshot", "1% chance per Intellect to Daze enemies for 3 seconds who are hit by your fireweapons.");
    initializePerk(SHARPSHOOTER, Attribute::AGILITY, "mp_data/perks/sharpshooter",
        "Sharpshooter", "+10% ranged attack damage.\n+25% ranged aim.");
    initializePerk(DEADLY_STRIKE, Attribute::AGILITY, "mp_data/perks/deadly_strike",
        "Deadly Strike", "+1% chance per level to deal +0.25 damage per Agility vs non-undead.");
    initializePerk(CRITICAL_HIT, Attribute::AGILITY, "mp_data/perks/critical_hit",
        "Critical Hit", "+0.5% chance per Agility to deal maximum damage.");
    initializePerk(LUCKY, Attribute::AGILITY, "mp_data/perks/lucky",
        "Lucky", "+10% Critical Hit chance.");
    initializePerk(QUICK_STRIKE, Attribute::AGILITY, "mp_data/perks/quick_strike", 
        "Quick Strike", "10% chance to strike twice with melee weapons.");
    initializePerk(BOW_MASTERY, Attribute::AGILITY, "mp_data/perks/bow_mastery", 
        "Bow Mastery", "+20% damage with Longbows.", WEAPON_PROFICIENCY);
    initializePerk(CROSSBOW_MASTERY, Attribute::AGILITY, "mp_data/perks/crossbow_mastery", 
        "Crossbow Mastery", "+30% Attack Speed with Crossbows.");
    initializePerk(WHIP_MASTERY, Attribute::AGILITY, "mp_data/perks/whip_mastery", 
        "Whip Mastery", "+100% damage with Whips.");
    initializePerk(DOUBLE_SHOT, Attribute::AGILITY, "mp_data/perks/double_shot", 
        "Double Shot", "+1% chance per Agility to fire 1 extra arrow with Longbows.", BOW_MASTERY);
    initializePerk(IMPROVISED_WEAPONS, Attribute::AGILITY, "mp_data/perks/improvised_weapons", 
        "Improvised Weapons", "+100% damage with Shovel, Crowbar, Pick, Torch and Toilet Plunger.");


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

    //TODO: basicdat folder is not added to vfs path yet
    //if(!vfs_exists(iconPath)) {
    //    log_warning("No icon for perk %s: %s\n", name.c_str(), iconPath.c_str());
    //}
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
    assert(id != NR_OF_PERKS);
    return _perkList[id];
}

} //Perk
} //Ego