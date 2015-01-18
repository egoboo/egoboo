/*
//--------------------------------------------------------------------------------------------
bool chr_upload_cap( chr_t * pchr, cap_t * pcap )
{
    /// @author BB
    /// @details prepare a character profile for exporting, by uploading some special values into the
    ///     cap. Just so that there is no confusion when you export multiple items of the same type,
    ///     DO NOT pass the pointer returned by chr_get_pcap(). Instead, use a custom cap_t declared on the stack,
    ///     or something similar
    ///
    /// @note This has been modified to basically reverse the actions of chr_download_cap().
    ///       If all enchants have been removed, this should export all permanent changes to the
    ///       base character profile.

    int tnc;

    if ( !DEFINED_PCHR( pchr ) || !LOADED_PCAP( pcap ) ) return false;

    // export values that override spawn.txt values
    pcap->content_override   = pchr->ai.content;
    pcap->state_override     = pchr->ai.state;
    pcap->money              = pchr->money;
    pcap->skin_override      = pchr->skin;
    pcap->level_override     = pchr->experiencelevel;

    // export the current experience
    ints_to_range( pchr->experience, 0, &( pcap->experience ) );

    // export the current mana and life
    pcap->life_spawn = CLIP( (UFP8_T)pchr->life, (UFP8_T)0, pchr->life_max );
    pcap->mana_spawn = CLIP( (UFP8_T)pchr->mana, (UFP8_T)0, pchr->mana_max );

    // Movement
    pcap->anim_speed_sneak = pchr->anim_speed_sneak;
    pcap->anim_speed_walk = pchr->anim_speed_walk;
    pcap->anim_speed_run = pchr->anim_speed_run;

    // weight and size
    pcap->size       = pchr->fat_goto;
    pcap->bumpdampen = pchr->phys.bumpdampen;
    if ( CHR_INFINITE_WEIGHT == pchr->phys.weight || 0.0f == pchr->fat )
    {
        pcap->weight = CAP_INFINITE_WEIGHT;
    }
    else
    {
        Uint32 itmp = pchr->phys.weight / pchr->fat / pchr->fat / pchr->fat;
        pcap->weight = std::min( itmp, (Uint32)CAP_MAX_WEIGHT );
    }

    // Other junk
    pcap->flyheight   = pchr->flyheight;
    pcap->alpha       = pchr->alpha_base;
    pcap->light       = pchr->light_base;
    pcap->flashand    = pchr->flashand;
    pcap->dampen      = pchr->phys.dampen;

    // Jumping
    pcap->jump       = pchr->jump_power;
    pcap->jumpnumber = pchr->jumpnumberreset;

    // Flags
    pcap->stickybutt      = TO_C_BOOL( pchr->stickybutt );
    pcap->canopenstuff    = TO_C_BOOL( pchr->openstuff );
    pcap->transferblend   = TO_C_BOOL( pchr->transferblend );
    pcap->waterwalk       = TO_C_BOOL( pchr->waterwalk );
    pcap->platform        = TO_C_BOOL( pchr->platform );
    pcap->canuseplatforms = TO_C_BOOL( pchr->canuseplatforms );
    pcap->isitem          = TO_C_BOOL( pchr->isitem );
    pcap->invictus        = TO_C_BOOL( pchr->invictus );
    pcap->ismount         = TO_C_BOOL( pchr->ismount );
    pcap->cangrabmoney    = TO_C_BOOL( pchr->cangrabmoney );

    // Damage
    pcap->attachedprt_reaffirm_damagetype = pchr->reaffirm_damagetype;
    pcap->damagetarget_damagetype         = pchr->damagetarget_damagetype;

    // SWID
    ints_to_range( pchr->strength    , 0, &( pcap->strength_stat.val ) );
    ints_to_range( pchr->wisdom      , 0, &( pcap->wisdom_stat.val ) );
    ints_to_range( pchr->intelligence, 0, &( pcap->intelligence_stat.val ) );
    ints_to_range( pchr->dexterity   , 0, &( pcap->dexterity_stat.val ) );

    // Life and Mana
    pcap->life_color = pchr->life_color;
    pcap->mana_color = pchr->mana_color;
    ints_to_range( pchr->life_max     , 0, &( pcap->life_stat.val ) );
    ints_to_range( pchr->mana_max     , 0, &( pcap->mana_stat.val ) );
    ints_to_range( pchr->mana_return  , 0, &( pcap->manareturn_stat.val ) );
    ints_to_range( pchr->mana_flow    , 0, &( pcap->manaflow_stat.val ) );

    // Gender
    pcap->gender  = pchr->gender;

    // Ammo
    pcap->ammomax = pchr->ammomax;
    pcap->ammo    = pchr->ammo;

    // update any skills that have been learned
    idsz_map_copy( pchr->skills, SDL_arraysize( pchr->skills ), pcap->skills );

    // Enchant stuff
    pcap->see_invisible_level = pchr->see_invisible_level;

    // base kurse state
    pcap->kursechance = pchr->iskursed ? 100 : 0;

    // Model stuff
    pcap->stoppedby = pchr->stoppedby;
    pcap->nameknown = TO_C_BOOL( pchr->nameknown || pchr->ammoknown );          // make sure that identified items are saved as identified
    pcap->draw_icon = TO_C_BOOL( pchr->draw_icon );

    // sound stuff...
    for ( tnc = 0; tnc < SOUND_COUNT; tnc++ )
    {
        pcap->sound_index[tnc] = pchr->sound_index[tnc];
    }

    return true;
}
*/
