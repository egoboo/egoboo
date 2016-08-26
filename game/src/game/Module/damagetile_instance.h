#pragma once

/// The actual in-game state of the damage tiles
struct damagetile_instance_t
{
    IPair amount;                    ///< Amount of damage
    DamageType damagetype;

    LocalParticleProfileRef part_gpip;
    uint32_t partand;
    int    sound_index;

    damagetile_instance_t() :
        amount(),
        damagetype(DamageType::DAMAGE_DIRECT),
        part_gpip(LocalParticleProfileRef::Invalid),
        partand(0),
        sound_index(INVALID_SOUND_ID)
    {
        //ctor
    }

    void upload(const wawalite_damagetile_t& source)
    {
        this->amount.base = source.amount;
        this->amount.rand = 1;
        this->damagetype = source.damagetype;

        this->part_gpip = source.part_gpip;
        this->partand = source.partand;
        this->sound_index = Ego::Math::constrain(source.sound_index, INVALID_SOUND_ID, MAX_WAVE);
    }
};
