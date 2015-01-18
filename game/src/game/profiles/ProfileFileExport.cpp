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

/*
bool ObjectProfile::exportToDataFile( const std::string &filePath, const char * szTemplateName, cap_t * pcap )
{
    /// @author BB
    /// @details export one cap_t struct to a "data.txt" file
    ///     converted to using the template file
    int damagetype, skin;

    // Open the file
    vfs_FILE* fileWrite = vfs_openWrite( filePath.c_str() );
    if ( NULL == fileWrite ) return false;

    // open the template file
    vfs_FILE* filetemp = nullptr;

    // try the given template file
    if ( VALID_CSTR( szTemplateName ) )
    {
        filetemp = template_open_vfs( szTemplateName );
    }

    // try a default template file
    if ( NULL == filetemp )
    {
        filetemp = template_open_vfs( "mp_data/templates/data.txt" );
    }

    //did we find a template file?
    if ( NULL == filetemp )
    {
        vfs_close( fileWrite );
        return false;
    }

    // Real general data
    template_put_int( filetemp, fileWrite, -1 );     // -1 signals a flexible load thing
    template_put_string_under( filetemp, fileWrite, pcap->classname );
    template_put_bool( filetemp, fileWrite, pcap->uniformlit );
    template_put_int( filetemp, fileWrite, pcap->ammomax );
    template_put_int( filetemp, fileWrite, pcap->ammo );
    template_put_gender( filetemp, fileWrite, pcap->gender );

    // Object stats
    template_put_int( filetemp, fileWrite, pcap->life_color );
    template_put_int( filetemp, fileWrite, pcap->mana_color );
    template_put_range( filetemp, fileWrite, pcap->_startingLife.val );
    template_put_range( filetemp, fileWrite, pcap->_startingLife.perlevel );
    template_put_range( filetemp, fileWrite, pcap->_startingMana.val );
    template_put_range( filetemp, fileWrite, pcap->_startingMana.perlevel );
    template_put_range( filetemp, fileWrite, pcap->_startingManaRegeneration.val );
    template_put_range( filetemp, fileWrite, pcap->_startingManaRegeneration.perlevel );
    template_put_range( filetemp, fileWrite, pcap->_startingManaFlow.val );
    template_put_range( filetemp, fileWrite, pcap->_startingManaFlow.perlevel );
    template_put_range( filetemp, fileWrite, pcap->_startingStrength.val );
    template_put_range( filetemp, fileWrite, pcap->_startingStrength.perlevel );
    template_put_range( filetemp, fileWrite, pcap->_startingWisdom.val );
    template_put_range( filetemp, fileWrite, pcap->_startingWisdom.perlevel );
    template_put_range( filetemp, fileWrite, pcap->_startingIntelligence.val );
    template_put_range( filetemp, fileWrite, pcap->_startingIntelligence.perlevel );
    template_put_range( filetemp, fileWrite, pcap->_startingDexterity.val );
    template_put_range( filetemp, fileWrite, pcap->_startingDexterity.perlevel );

    // More physical attributes
    template_put_float( filetemp, fileWrite, pcap->size );
    template_put_float( filetemp, fileWrite, pcap->_sizeGainPerLevel );
    template_put_int( filetemp, fileWrite, pcap->_shadowSize );
    template_put_int( filetemp, fileWrite, pcap->_bumpSize );
    template_put_int( filetemp, fileWrite, pcap->_bumpHeight );
    template_put_float( filetemp, fileWrite, pcap->_bumpDampen );
    template_put_int( filetemp, fileWrite, pcap->weight );
    template_put_float( filetemp, fileWrite, pcap->jump );
    template_put_int( filetemp, fileWrite, pcap->jumpnumber );
    template_put_float( filetemp, fileWrite, pcap->_animationSpeedSneak );
    template_put_float( filetemp, fileWrite, pcap->_animationSpeedWalk );
    template_put_float( filetemp, fileWrite, pcap->_animationSpeedRun );
    template_put_int( filetemp, fileWrite, pcap->flyheight );
    template_put_int( filetemp, fileWrite, pcap->flashand );
    template_put_int( filetemp, fileWrite, pcap->alpha );
    template_put_int( filetemp, fileWrite, pcap->light );
    template_put_bool( filetemp, fileWrite, pcap->transferblend );
    template_put_int( filetemp, fileWrite, pcap->sheen );
    template_put_bool( filetemp, fileWrite, pcap->enviro );
    template_put_float( filetemp, fileWrite, FFFF_TO_FLOAT( pcap->uoffvel ) );
    template_put_float( filetemp, fileWrite, FFFF_TO_FLOAT( pcap->voffvel ) );
    template_put_bool( filetemp, fileWrite, pcap->_stickyButt );

    // Invulnerability data
    template_put_bool( filetemp, fileWrite, pcap->invictus );
    template_put_int( filetemp, fileWrite, pcap->nframefacing );
    template_put_int( filetemp, fileWrite, pcap->nframeangle );
    template_put_int( filetemp, fileWrite, pcap->iframefacing );
    template_put_int( filetemp, fileWrite, pcap->iframeangle );

    // Skin defenses
    template_put_int( filetemp, fileWrite, 255 - pcap->defense[0] );
    template_put_int( filetemp, fileWrite, 255 - pcap->defense[1] );
    template_put_int( filetemp, fileWrite, 255 - pcap->defense[2] );
    template_put_int( filetemp, fileWrite, 255 - pcap->defense[3] );

    for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        template_put_float( filetemp, fileWrite, pcap->damage_resistance[damagetype][0] );
        template_put_float( filetemp, fileWrite, pcap->damage_resistance[damagetype][1] );
        template_put_float( filetemp, fileWrite, pcap->damage_resistance[damagetype][2] );
        template_put_float( filetemp, fileWrite, pcap->damage_resistance[damagetype][3] );
    }

    for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        char code;

        for ( skin = 0; skin < MAX_SKIN; skin++ )
        {
            if ( HAS_SOME_BITS( pcap->damage_modifier[damagetype][skin], DAMAGEMANA ) )
            {
                code = 'M';
            }
            else if ( HAS_SOME_BITS( pcap->damage_modifier[damagetype][skin], DAMAGECHARGE ) )
            {
                code = 'C';
            }
            else if ( HAS_SOME_BITS( pcap->damage_modifier[damagetype][skin], DAMAGEINVERT ) )
            {
                code = 'T';
            }
            else if ( HAS_SOME_BITS( pcap->damage_modifier[damagetype][skin], DAMAGEINVICTUS ) )
            {
                code = 'I';
            }
            else
            {
                code = 'F';
            }

            template_put_char( filetemp, fileWrite, code );
        }
    }

    template_put_float( filetemp, fileWrite, pcap->skin_info.maxaccel[0]*80 );
    template_put_float( filetemp, fileWrite, pcap->skin_info.maxaccel[1]*80 );
    template_put_float( filetemp, fileWrite, pcap->skin_info.maxaccel[2]*80 );
    template_put_float( filetemp, fileWrite, pcap->skin_info.maxaccel[3]*80 );

    // Experience and level data
    template_put_int( filetemp, fileWrite, pcap->experience_forlevel[1] );
    template_put_int( filetemp, fileWrite, pcap->experience_forlevel[2] );
    template_put_int( filetemp, fileWrite, pcap->experience_forlevel[3] );
    template_put_int( filetemp, fileWrite, pcap->experience_forlevel[4] );
    template_put_int( filetemp, fileWrite, pcap->experience_forlevel[5] );
    template_put_float( filetemp, fileWrite, FLOAT_TO_FP8( pcap->experience.from ) );
    template_put_int( filetemp, fileWrite, pcap->experience_worth );
    template_put_float( filetemp, fileWrite, pcap->experience_exchange );
    template_put_float( filetemp, fileWrite, pcap->experience_rate[0] );
    template_put_float( filetemp, fileWrite, pcap->experience_rate[1] );
    template_put_float( filetemp, fileWrite, pcap->experience_rate[2] );
    template_put_float( filetemp, fileWrite, pcap->experience_rate[3] );
    template_put_float( filetemp, fileWrite, pcap->experience_rate[4] );
    template_put_float( filetemp, fileWrite, pcap->experience_rate[5] );
    template_put_float( filetemp, fileWrite, pcap->experience_rate[6] );
    template_put_float( filetemp, fileWrite, pcap->experience_rate[7] );

    // IDSZ identification tags
    template_put_idsz( filetemp, fileWrite, pcap->idsz[IDSZ_PARENT] );
    template_put_idsz( filetemp, fileWrite, pcap->idsz[IDSZ_TYPE] );
    template_put_idsz( filetemp, fileWrite, pcap->idsz[IDSZ_SKILL] );
    template_put_idsz( filetemp, fileWrite, pcap->idsz[IDSZ_SPECIAL] );
    template_put_idsz( filetemp, fileWrite, pcap->idsz[IDSZ_HATE] );
    template_put_idsz( filetemp, fileWrite, pcap->idsz[IDSZ_VULNERABILITY] );

    // Item and damage flags
    template_put_bool( filetemp, fileWrite, pcap->isitem );
    template_put_bool( filetemp, fileWrite, pcap->ismount );
    template_put_bool( filetemp, fileWrite, pcap->isstackable );
    template_put_bool( filetemp, fileWrite, pcap->_nameIsKnown );
    template_put_bool( filetemp, fileWrite, pcap->_usageIsKnown );
    template_put_bool( filetemp, fileWrite, pcap->_canCarryToNextModule );
    template_put_bool( filetemp, fileWrite, pcap->_needSkillIDToUse );
    template_put_bool( filetemp, fileWrite, pcap->platform );
    template_put_bool( filetemp, fileWrite, pcap->cangrabmoney );
    template_put_bool( filetemp, fileWrite, pcap->canopenstuff );

    // Other item and damage stuff
    template_put_damage_type( filetemp, fileWrite, pcap->_damageTargetDamageType );
    template_put_action( filetemp, fileWrite, pcap->_weaponAction );

    // Particle attachments
    template_put_int( filetemp, fileWrite, pcap->_attachedParticleAmount );
    template_put_damage_type( filetemp, fileWrite, pcap->_attachedParticleReaffirmDamagetype );
    template_put_int( filetemp, fileWrite, pcap->_attachedParticleProfile );

    // Character hands
    template_put_bool( filetemp, fileWrite, pcap->_slotsValid[SLOT_LEFT] );
    template_put_bool( filetemp, fileWrite, pcap->_slotsValid[SLOT_RIGHT] );

    // Particle spawning on attack
    template_put_bool( filetemp, fileWrite, 0 != pcap->_attackAttached );
    template_put_int( filetemp, fileWrite, pcap->_attackParticleProfile );

    // Particle spawning for GoPoof
    template_put_int( filetemp, fileWrite, pcap->_goPoofParticleAmount );
    template_put_int( filetemp, fileWrite, pcap->_goPoofParticleFacingAdd );
    template_put_int( filetemp, fileWrite, pcap->_goPoofParticleProfile );

    // Particle spawning for blud
    template_put_bool( filetemp, fileWrite, 0 != pcap->_bludValid );
    template_put_int( filetemp, fileWrite, pcap->_bludParticleProfile );

    // Extra stuff
    template_put_bool( filetemp, fileWrite, pcap->waterwalk );
    template_put_float( filetemp, fileWrite, pcap->dampen );

    // More stuff
    template_put_float( filetemp, fileWrite, FP8_TO_FLOAT( pcap->_startingLifeRegeneration ) );     // These two are seriously outdated
    template_put_float( filetemp, fileWrite, FP8_TO_FLOAT( pcap->_manaCost ) );     // and shouldnt be used. Use scripts instead.
    template_put_int( filetemp, fileWrite, pcap->_startingLifeRegeneration );
    template_put_int( filetemp, fileWrite, pcap->stoppedby );
    template_put_string_under( filetemp, fileWrite, pcap->skin_info.name[0] );
    template_put_string_under( filetemp, fileWrite, pcap->skin_info.name[1] );
    template_put_string_under( filetemp, fileWrite, pcap->skin_info.name[2] );
    template_put_string_under( filetemp, fileWrite, pcap->skin_info.name[3] );
    template_put_int( filetemp, fileWrite, pcap->skin_info.cost[0] );
    template_put_int( filetemp, fileWrite, pcap->skin_info.cost[1] );
    template_put_int( filetemp, fileWrite, pcap->skin_info.cost[2] );
    template_put_int( filetemp, fileWrite, pcap->skin_info.cost[3] );
    template_put_float( filetemp, fileWrite, pcap->_strengthBonus );

    // Another memory lapse
    template_put_bool( filetemp, fileWrite, !pcap->_riderCanAttack );
    template_put_bool( filetemp, fileWrite, pcap->_canBeDazed );
    template_put_bool( filetemp, fileWrite, pcap->_canBeGrogged );
    template_put_int( filetemp, fileWrite, 0 );
    template_put_int( filetemp, fileWrite, 0 );
    template_put_bool( filetemp, fileWrite, pcap->_seeInvisibleLevel > 0 );
    template_put_int( filetemp, fileWrite, pcap->kursechance );
    template_put_int( filetemp, fileWrite, pcap->sound_index[SOUND_FOOTFALL] );
    template_put_int( filetemp, fileWrite, pcap->sound_index[SOUND_JUMP] );

    vfs_flush( fileWrite );

    // copy the template file to the next free output section
    template_seek_free( filetemp, fileWrite );

    // Expansions
    if ( pcap->skin_info.dressy&1 )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'D', 'R', 'E', 'S' ), 0 );

    if ( pcap->skin_info.dressy&2 )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'D', 'R', 'E', 'S' ), 1 );

    if ( pcap->skin_info.dressy&4 )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'D', 'R', 'E', 'S' ), 2 );

    if ( pcap->skin_info.dressy&8 )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'D', 'R', 'E', 'S' ), 3 );

    if ( pcap->resistbumpspawn )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'S', 'T', 'U', 'K' ), 0 );

    if ( pcap->_isBigItem )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'P', 'A', 'C', 'K' ), 0 );

    if ( !pcap->reflect )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'V', 'A', 'M', 'P' ), 1 );

    if ( pcap->alwaysdraw )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'D', 'R', 'A', 'W' ), 1 );

    if ( pcap->_isRanged )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'R', 'A', 'N', 'G' ), 1 );

    if ( pcap->hidestate != NOHIDE )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'H', 'I', 'D', 'E' ), pcap->hidestate );

    if ( pcap->_isEquipment )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'E', 'Q', 'U', 'I' ), 1 );

    if ( pcap->_bumpSizeBig >= pcap->_bumpSize * 2 )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'S', 'Q', 'U', 'A' ), 1 );

    if ( pcap->draw_icon != pcap->_usageIsKnown )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'I', 'C', 'O', 'N' ), pcap->draw_icon );

    if ( pcap->forceshadow )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'S', 'H', 'A', 'D' ), 1 );

    if ( pcap->ripple == pcap->isitem )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'R', 'I', 'P', 'P' ), pcap->ripple );

    if ( -1 != pcap->isvaluable )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'V', 'A', 'L', 'U' ), pcap->isvaluable );

    if ( pcap->spelleffect_type >= 0 )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'B', 'O', 'O', 'K' ), pcap->spelleffect_type );

    if ( pcap->_attackFast )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'F', 'A', 'S', 'T' ), pcap->_attackFast );

    if ( pcap->_strengthBonus > 0 )
        vfs_put_expansion_float( fileWrite, "", MAKE_IDSZ( 'S', 'T', 'R', 'D' ), pcap->_strengthBonus );

    if ( pcap->_intelligenceBonus > 0 )
        vfs_put_expansion_float( fileWrite, "", MAKE_IDSZ( 'I', 'N', 'T', 'D' ), pcap->_intelligenceBonus );

    if ( pcap->_dexterityBonus > 0 )
        vfs_put_expansion_float( fileWrite, "", MAKE_IDSZ( 'D', 'E', 'X', 'D' ), pcap->_dexterityBonus );

    if ( pcap->_wisdomBonus > 0 )
        vfs_put_expansion_float( fileWrite, "", MAKE_IDSZ( 'W', 'I', 'S', 'D' ), pcap->_wisdomBonus );

    if ( pcap->_bumpOverrideSize || pcap->_bumpOverrideSizeBig ||  pcap->_bumpOverrideHeight )
    {
        STRING sz_tmp = EMPTY_CSTR;

        if ( pcap->_bumpOverrideSize ) strcat( sz_tmp, "S" );
        if ( pcap->_bumpOverrideSizeBig ) strcat( sz_tmp, "B" );
        if ( pcap->_bumpOverrideHeight ) strcat( sz_tmp, "H" );
        if ( pcap->dont_cull_backfaces ) strcat( sz_tmp, "C" );
        if ( pcap->skin_has_transparency ) strcat( sz_tmp, "T" );

        if ( CSTR_END != sz_tmp[0] )
        {
            vfs_put_expansion_string( fileWrite, "", MAKE_IDSZ( 'M', 'O', 'D', 'L' ), sz_tmp );
        }
    }

    // Basic stuff that is always written
    vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'G', 'O', 'L', 'D' ), pcap->money );
    vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'P', 'L', 'A', 'T' ), pcap->canuseplatforms );
    vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'S', 'K', 'I', 'N' ), pcap->skin_override );
    vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'C', 'O', 'N', 'T' ), pcap->content_override );
    vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'S', 'T', 'A', 'T' ), pcap->state_override );
    vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'L', 'E', 'V', 'L' ), pcap->level_override );
    vfs_put_expansion_float( fileWrite, "", MAKE_IDSZ( 'L', 'I', 'F', 'E' ), FP8_TO_FLOAT( pcap->_spawnLife ) );
    vfs_put_expansion_float( fileWrite, "", MAKE_IDSZ( 'M', 'A', 'N', 'A' ), FP8_TO_FLOAT( pcap->_spawnMana ) );

    // Copy all skill expansions
    {
        IDSZ_node_t *pidsz;
        int iterator;

        iterator = 0;
        pidsz = idsz_map_iterate( pcap->skills, SDL_arraysize( pcap->skills ), &iterator );
        while ( pidsz != NULL )
        {
            //Write that skill into the file
            vfs_put_expansion( fileWrite, "", pidsz->id, pidsz->level );

            //Get the next IDSZ from the map
            pidsz = idsz_map_iterate( pcap->skills, SDL_arraysize( pcap->skills ), &iterator );
        }
    }

    // dump the rest of the template file
    template_flush( filetemp, fileWrite );

    // The end
    vfs_close( fileWrite );
    template_close_vfs( filetemp );

    return true;
}
*/
