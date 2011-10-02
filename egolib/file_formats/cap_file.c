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

/// @file file_formats/cap_file.c
/// @brief routines for reading and writing the character profile file data.txt
/// @details

#include "cap_file.h"
#include "map_file.h"

#include "template.h"

#include "../fileutil.h"
#include "../strutil.h"
#include "../vfs.h"
#include "../_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// this function is declared in mad.h
extern int    action_which( char cTmp );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
cap_t * cap_init( cap_t * pcap )
{
    /// @author BB
    /// @details initialize the character profile data to safe values

    int cnt;

    if ( NULL == pcap ) return pcap;

    // clear out all the data
    // all = 0, = false, and = 0.0f statements are redundant
    BLANK_STRUCT_PTR( pcap )

    for ( cnt = 0; cnt < IDSZ_COUNT; cnt++ )
    {
        pcap->idsz[cnt] = IDSZ_NONE;
    }

    // clear out the sounds
    for ( cnt = 0; cnt < SOUND_COUNT; cnt++ )
    {
        pcap->sound_index[cnt] = -1;
    }

    // Clear non-zero, non-false expansions...
    pcap->reflect          = btrue;
    pcap->hidestate        = NOHIDE;
    pcap->spelleffect_type = NO_SKIN_OVERRIDE;
    pcap->skin_override    = NO_SKIN_OVERRIDE;
    pcap->isvaluable       = -1;
    pcap->draw_icon        = btrue;

    // either these will be overridden by data in the data.txt, or
    // they will be limited by the spawning character's max stats
    pcap->life_spawn = PERFECTBIG;
    pcap->mana_spawn = PERFECTBIG;

    // More stuff I forgot
    pcap->stoppedby  = MAPFX_IMPASS;

    // Skills
    idsz_map_init( pcap->skills, SDL_arraysize( pcap->skills ) );

    return pcap;
}

//--------------------------------------------------------------------------------------------
cap_t * load_one_cap_file_vfs( const char * tmploadname, cap_t * pcap )
{
    /// @author ZZ
    /// @details This function fills a character profile with data from data.txt, returning
    ///     the icap slot that the profile was stuck into.  It may cause the program
    ///     to abort if bad things happen.

    vfs_FILE* fileread;

    int   iTmp;
    char  cTmp;
    float fTmp;

    Uint8 damagetype, level, xptype;
    int idsz_cnt;
    IDSZ idsz;
    int cnt;
    STRING szLoadName;

    if ( NULL == pcap ) return NULL;

    make_newloadname( tmploadname, "/data.txt", szLoadName );

    // Open the file
    fileread = vfs_openRead( szLoadName );
    if ( NULL == fileread )
    {
        return NULL;
    }

    // ignore this, since it was already read
    iTmp = vfs_get_next_int( fileread );

    cap_init( pcap );

    // mark the source filename
    strncpy( pcap->name, szLoadName, SDL_arraysize( pcap->name ) );

    // mark it as loaded
    pcap->loaded = btrue;

    // Read in the real general data
    vfs_get_next_name( fileread, pcap->classname, SDL_arraysize( pcap->classname ) );

    // Light cheat
    pcap->uniformlit = vfs_get_next_bool( fileread );

    // Ammo
    pcap->ammomax = vfs_get_next_int( fileread );
    pcap->ammo = vfs_get_next_int( fileread );

    // Gender
    cTmp = vfs_get_next_char( fileread );
    if ( 'F' == char_toupper(( unsigned )cTmp ) )  pcap->gender = GENDER_FEMALE;
    else if ( 'M' == char_toupper(( unsigned )cTmp ) )  pcap->gender = GENDER_MALE;
    else if ( 'R' == char_toupper(( unsigned )cTmp ) )  pcap->gender = GENDER_RANDOM;
    else                              pcap->gender = GENDER_OTHER;

    // Read in the icap stats
    pcap->life_color = vfs_get_next_int( fileread );
    pcap->mana_color = vfs_get_next_int( fileread );

    vfs_get_next_range( fileread, &( pcap->life_stat.val ) );
    vfs_get_next_range( fileread, &( pcap->life_stat.perlevel ) );

    vfs_get_next_range( fileread, &( pcap->mana_stat.val ) );
    vfs_get_next_range( fileread, &( pcap->mana_stat.perlevel ) );

    vfs_get_next_range( fileread, &( pcap->manareturn_stat.val ) );
    vfs_get_next_range( fileread, &( pcap->manareturn_stat.perlevel ) );

    vfs_get_next_range( fileread, &( pcap->manaflow_stat.val ) );
    vfs_get_next_range( fileread, &( pcap->manaflow_stat.perlevel ) );

    vfs_get_next_range( fileread, &( pcap->strength_stat.val ) );
    vfs_get_next_range( fileread, &( pcap->strength_stat.perlevel ) );

    vfs_get_next_range( fileread, &( pcap->wisdom_stat.val ) );
    vfs_get_next_range( fileread, &( pcap->wisdom_stat.perlevel ) );

    vfs_get_next_range( fileread, &( pcap->intelligence_stat.val ) );
    vfs_get_next_range( fileread, &( pcap->intelligence_stat.perlevel ) );

    vfs_get_next_range( fileread, &( pcap->dexterity_stat.val ) );
    vfs_get_next_range( fileread, &( pcap->dexterity_stat.perlevel ) );

    // More physical attributes
    pcap->size = vfs_get_next_float( fileread );
    pcap->size_perlevel = vfs_get_next_float( fileread );
    pcap->shadow_size = vfs_get_next_int( fileread );
    pcap->bump_size = vfs_get_next_int( fileread );
    pcap->bump_height = vfs_get_next_int( fileread );
    pcap->bumpdampen = vfs_get_next_float( fileread );
    pcap->weight = vfs_get_next_int( fileread );
    pcap->jump = vfs_get_next_float( fileread );
    pcap->jumpnumber = vfs_get_next_int( fileread );
    pcap->anim_speed_sneak = vfs_get_next_float( fileread );
    pcap->anim_speed_walk = vfs_get_next_float( fileread );
    pcap->anim_speed_run = vfs_get_next_float( fileread );
    pcap->flyheight = vfs_get_next_int( fileread );
    pcap->flashand = vfs_get_next_int( fileread );
    pcap->alpha = vfs_get_next_int( fileread );
    pcap->light = vfs_get_next_int( fileread );
    pcap->transferblend = vfs_get_next_bool( fileread );

    pcap->sheen = vfs_get_next_int( fileread );
    pcap->enviro = vfs_get_next_bool( fileread );

    fTmp = vfs_get_next_float( fileread );
    pcap->uoffvel    = FLOAT_TO_FFFF( fTmp );
    fTmp = vfs_get_next_float( fileread );
    pcap->voffvel    = FLOAT_TO_FFFF( fTmp );
    pcap->stickybutt = vfs_get_next_bool( fileread );

    // Invulnerability data
    pcap->invictus     = vfs_get_next_bool( fileread );
    pcap->nframefacing = vfs_get_next_int( fileread );
    pcap->nframeangle  = vfs_get_next_int( fileread );
    pcap->iframefacing = vfs_get_next_int( fileread );
    pcap->iframeangle  = vfs_get_next_int( fileread );

    // Resist burning and stuck arrows with nframe angle of 1 or more
    if ( pcap->nframeangle > 0 )
    {
        if ( 1 == pcap->nframeangle )
        {
            pcap->nframeangle = 0;
        }
    }

    // Skin defenses ( 4 skins )
    goto_colon_vfs( NULL, fileread, bfalse );
    for ( cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        iTmp = 0xFF - vfs_get_int( fileread );
        pcap->defense[cnt] = CLIP( iTmp, 0, 0xFF );
    }

    for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        goto_colon_vfs( NULL, fileread, bfalse );
        for ( cnt = 0; cnt < MAX_SKIN; cnt++ )
        {
            pcap->damage_resistance[damagetype][cnt] = vfs_get_damage_resist( fileread );
        }
    }

    for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        goto_colon_vfs( NULL, fileread, bfalse );

        for ( cnt = 0; cnt < MAX_SKIN; cnt++ )
        {
            cTmp = char_toupper(( unsigned )vfs_get_first_letter( fileread ) );
            switch ( cTmp )
            {
                case 'T': pcap->damage_modifier[damagetype][cnt] |= DAMAGEINVERT;   break;
                case 'C': pcap->damage_modifier[damagetype][cnt] |= DAMAGECHARGE;   break;
                case 'M': pcap->damage_modifier[damagetype][cnt] |= DAMAGEMANA;     break;
                case 'I': pcap->damage_modifier[damagetype][cnt] |= DAMAGEINVICTUS; break;

                    //F is nothing
                default: break;
            }
        }
    }

    goto_colon_vfs( NULL, fileread, bfalse );
    for ( cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        pcap->skin_info.maxaccel[cnt] = vfs_get_float( fileread ) / 80.0f;
    }

    // Experience and level data
    pcap->experience_forlevel[0] = 0;
    for ( level = 1; level < MAXBASELEVEL; level++ )
    {
        pcap->experience_forlevel[level] = vfs_get_next_int( fileread );
    }

    vfs_get_next_range( fileread, &( pcap->experience ) );
    pcap->experience.from /= 256.0f;
    pcap->experience.to   /= 256.0f;

    pcap->experience_worth    = vfs_get_next_int( fileread );
    pcap->experience_exchange = vfs_get_next_float( fileread );

    for ( xptype = 0; xptype < XP_COUNT; xptype++ )
    {
        pcap->experience_rate[xptype] = vfs_get_next_float( fileread ) + 0.001f;
    }

    // IDSZ tags
    for ( idsz_cnt = 0; idsz_cnt < IDSZ_COUNT; idsz_cnt++ )
    {
        pcap->idsz[idsz_cnt] = vfs_get_next_idsz( fileread );
    }

    // Item and damage flags
    pcap->isitem               = vfs_get_next_bool( fileread );
    pcap->ismount              = vfs_get_next_bool( fileread );
    pcap->isstackable          = vfs_get_next_bool( fileread );
    pcap->nameknown            = vfs_get_next_bool( fileread );
    pcap->usageknown           = vfs_get_next_bool( fileread );
    pcap->cancarrytonextmodule = vfs_get_next_bool( fileread );
    pcap->needskillidtouse     = vfs_get_next_bool( fileread );
    pcap->platform             = vfs_get_next_bool( fileread );
    pcap->cangrabmoney         = vfs_get_next_bool( fileread );
    pcap->canopenstuff         = vfs_get_next_bool( fileread );

    // More item and damage stuff
    pcap->damagetarget_damagetype = vfs_get_next_damage_type( fileread );
    pcap->weaponaction            = action_which( vfs_get_next_char( fileread ) );

    // Particle attachments
    pcap->attachedprt_amount              = vfs_get_next_int( fileread );
    pcap->attachedprt_reaffirm_damagetype = vfs_get_next_damage_type( fileread );
    pcap->attachedprt_lpip                = vfs_get_next_int( fileread );

    // Character hands
    pcap->slotvalid[SLOT_LEFT]  = vfs_get_next_bool( fileread );
    pcap->slotvalid[SLOT_RIGHT] = vfs_get_next_bool( fileread );

    // Attack order ( weapon )
    pcap->attack_attached = vfs_get_next_bool( fileread );
    pcap->attack_lpip  = vfs_get_next_int( fileread );

    // GoPoof
    pcap->gopoofprt_amount    = vfs_get_next_int( fileread );
    pcap->gopoofprt_facingadd = vfs_get_next_int( fileread );
    pcap->gopoofprt_lpip      = vfs_get_next_int( fileread );

    // Blud
    cTmp = vfs_get_next_char( fileread );
    if ( 'T' == char_toupper(( unsigned )cTmp ) )  pcap->blud_valid = btrue;
    else if ( 'U' == char_toupper(( unsigned )cTmp ) )  pcap->blud_valid = ULTRABLUDY;
    else                              pcap->blud_valid = bfalse;

    pcap->blud_lpip = vfs_get_next_int( fileread );

    // Stuff I forgot
    pcap->waterwalk = vfs_get_next_bool( fileread );
    pcap->dampen    = vfs_get_next_float( fileread );

    // More stuff I forgot
    pcap->life_heal    = vfs_get_next_float( fileread ) * 0xff;
    pcap->manacost     = vfs_get_next_float( fileread ) * 0xff;
    pcap->life_return  = vfs_get_next_int( fileread );
    pcap->stoppedby   |= vfs_get_next_int( fileread );

    for ( cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        vfs_get_next_name( fileread, pcap->skin_info.name[cnt], sizeof( pcap->skin_info.name[cnt] ) );
    }

    for ( cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        pcap->skin_info.cost[cnt] = vfs_get_next_int( fileread );
    }

    pcap->str_bonus = vfs_get_next_float( fileread );          //ZF> Deprecated, but keep here for backwards compatability

    // Another memory lapse
    pcap->ridercanattack = !vfs_get_next_bool( fileread );
    pcap->canbedazed     =  vfs_get_next_bool( fileread );
    pcap->canbegrogged   =  vfs_get_next_bool( fileread );

    goto_colon_vfs( NULL, fileread, bfalse );  // !!!BAD!!! Life add
    goto_colon_vfs( NULL, fileread, bfalse );  // !!!BAD!!! Mana add
    if ( vfs_get_next_bool( fileread ) )   pcap->see_invisible_level = 1;

    pcap->kursechance                 = vfs_get_next_int( fileread );
    pcap->sound_index[SOUND_FOOTFALL] = vfs_get_next_int( fileread );  // Footfall sound
    pcap->sound_index[SOUND_JUMP]     = vfs_get_next_int( fileread );  // Jump sound

    // assume the normal dependence of ripple on isitem
    pcap->ripple = !pcap->isitem;

    // assume a round object
    pcap->bump_sizebig = pcap->bump_size * SQRT_TWO;

    // assume the normal icon usage
    pcap->draw_icon = pcap->usageknown;

    // assume normal platform usage
    pcap->canuseplatforms = !pcap->platform;

    // Read expansions
    while ( goto_colon_vfs( NULL, fileread, btrue ) )
    {
        idsz = vfs_get_idsz( fileread );

        if ( idsz == MAKE_IDSZ( 'D', 'R', 'E', 'S' ) ) SET_BIT( pcap->skin_info.dressy, 1 << vfs_get_int( fileread ) );
        else if ( idsz == MAKE_IDSZ( 'G', 'O', 'L', 'D' ) ) pcap->money = vfs_get_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'S', 'T', 'U', 'K' ) ) pcap->resistbumpspawn = ( 0 != ( 1 - vfs_get_int( fileread ) ) );
        else if ( idsz == MAKE_IDSZ( 'P', 'A', 'C', 'K' ) ) pcap->istoobig = !( 0 != vfs_get_int( fileread ) );
        else if ( idsz == MAKE_IDSZ( 'V', 'A', 'M', 'P' ) ) pcap->reflect = !( 0 != vfs_get_int( fileread ) );
        else if ( idsz == MAKE_IDSZ( 'D', 'R', 'A', 'W' ) ) pcap->alwaysdraw = ( 0 != vfs_get_int( fileread ) );
        else if ( idsz == MAKE_IDSZ( 'R', 'A', 'N', 'G' ) ) pcap->isranged = ( 0 != vfs_get_int( fileread ) );
        else if ( idsz == MAKE_IDSZ( 'H', 'I', 'D', 'E' ) ) pcap->hidestate = vfs_get_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'E', 'Q', 'U', 'I' ) ) pcap->isequipment = ( 0 != vfs_get_int( fileread ) );
        else if ( idsz == MAKE_IDSZ( 'S', 'Q', 'U', 'A' ) ) pcap->bump_sizebig = pcap->bump_size * 2;
        else if ( idsz == MAKE_IDSZ( 'I', 'C', 'O', 'N' ) ) pcap->draw_icon = ( 0 != vfs_get_int( fileread ) );
        else if ( idsz == MAKE_IDSZ( 'S', 'H', 'A', 'D' ) ) pcap->forceshadow = ( 0 != vfs_get_int( fileread ) );
        else if ( idsz == MAKE_IDSZ( 'S', 'K', 'I', 'N' ) ) 
		{
            // ??? ZF, what is the valus of this integer for a random skin ???
			int iTmp = vfs_get_int( fileread );
			pcap->skin_override = (iTmp < 0) ? -1 : iTmp;
		}
        else if ( idsz == MAKE_IDSZ( 'C', 'O', 'N', 'T' ) ) pcap->content_override = vfs_get_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'S', 'T', 'A', 'T' ) ) pcap->state_override = vfs_get_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'L', 'E', 'V', 'L' ) ) pcap->level_override = vfs_get_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'P', 'L', 'A', 'T' ) ) pcap->canuseplatforms = ( 0 != vfs_get_int( fileread ) );
        else if ( idsz == MAKE_IDSZ( 'R', 'I', 'P', 'P' ) ) pcap->ripple = ( 0 != vfs_get_int( fileread ) );
        else if ( idsz == MAKE_IDSZ( 'V', 'A', 'L', 'U' ) ) pcap->isvaluable = vfs_get_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'L', 'I', 'F', 'E' ) ) pcap->life_spawn = 0xff * vfs_get_float( fileread );
        else if ( idsz == MAKE_IDSZ( 'M', 'A', 'N', 'A' ) ) pcap->mana_spawn = 0xff * vfs_get_float( fileread );
        else if ( idsz == MAKE_IDSZ( 'B', 'O', 'O', 'K' ) )
		{
			int iTmp = vfs_get_int( fileread );
			pcap->spelleffect_type = (iTmp < 0) ? -1 : (iTmp % MAX_SKIN);
		}
        else if ( idsz == MAKE_IDSZ( 'F', 'A', 'S', 'T' ) ) pcap->attack_fast = ( 0 != vfs_get_int( fileread ) );

        //Damage bonuses from stats
        else if ( idsz == MAKE_IDSZ( 'S', 'T', 'R', 'D' ) ) pcap->str_bonus = vfs_get_float( fileread );
        else if ( idsz == MAKE_IDSZ( 'I', 'N', 'T', 'D' ) ) pcap->int_bonus = vfs_get_float( fileread );
        else if ( idsz == MAKE_IDSZ( 'W', 'I', 'S', 'D' ) ) pcap->wis_bonus = vfs_get_float( fileread );
        else if ( idsz == MAKE_IDSZ( 'D', 'E', 'X', 'D' ) ) pcap->dex_bonus = vfs_get_float( fileread );

        // get some model customization
        else if ( MAKE_IDSZ( 'M', 'O', 'D', 'L' ) == idsz )
        {
            STRING tmp_buffer;
            if ( vfs_get_string( fileread, tmp_buffer, SDL_arraysize( tmp_buffer ) ) )
            {
                char * ptr;

                ptr = strpbrk( tmp_buffer, "SBHCT" );
                while ( NULL != ptr )
                {
                    if ( 'S' == *ptr )
                    {
                        pcap->bump_override_size = btrue;
                    }
                    else if ( 'B' == *ptr )
                    {
                        pcap->bump_override_sizebig = btrue;
                    }
                    else if ( 'H' == *ptr )
                    {
                        pcap->bump_override_height = btrue;
                    }
                    else if ( 'C' == *ptr )
                    {
                        pcap->dont_cull_backfaces = btrue;
                    }
                    else if ( 'T' == *ptr )
                    {
                        pcap->skin_has_transparency = btrue;
                    }

                    // start on the next character
                    ptr++;
                    ptr = strpbrk( ptr, "SBHCT" );
                }
            }
        }
        //If it is none of the predefined IDSZ extensions then add it as a new skill
        else
        {
            idsz_map_add( pcap->skills, SDL_arraysize( pcap->skills ), idsz, vfs_get_int( fileread ) );
        }
    }
    vfs_close( fileread );

    //log_debug( "CapStack_load_one() - loaded icap %s (%d)\n", pcap->classname, icap );

    return pcap;
}

//--------------------------------------------------------------------------------------------
bool_t save_one_cap_file_vfs( const char * szSaveName, const char * szTemplateName, cap_t * pcap )
{
    /// @author BB
    /// @details export one cap_t struct to a "data.txt" file
    ///     converted to using the template file

    vfs_FILE* filewrite, * filetemp;

    int damagetype, skin;

    if ( NULL == pcap ) return bfalse;

    // Open the file
    filewrite = vfs_openWrite( szSaveName );
    if ( NULL == filewrite ) return bfalse;

    // open the template file
    filetemp = NULL;

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
        vfs_close( filewrite );
        return bfalse;
    }

    // Real general data
    template_put_int( filetemp, filewrite, -1 );     // -1 signals a flexible load thing
    template_put_string_under( filetemp, filewrite, pcap->classname );
    template_put_bool( filetemp, filewrite, pcap->uniformlit );
    template_put_int( filetemp, filewrite, pcap->ammomax );
    template_put_int( filetemp, filewrite, pcap->ammo );
    template_put_gender( filetemp, filewrite, pcap->gender );

    // Object stats
    template_put_int( filetemp, filewrite, pcap->life_color );
    template_put_int( filetemp, filewrite, pcap->mana_color );
    template_put_range( filetemp, filewrite, pcap->life_stat.val );
    template_put_range( filetemp, filewrite, pcap->life_stat.perlevel );
    template_put_range( filetemp, filewrite, pcap->mana_stat.val );
    template_put_range( filetemp, filewrite, pcap->mana_stat.perlevel );
    template_put_range( filetemp, filewrite, pcap->manareturn_stat.val );
    template_put_range( filetemp, filewrite, pcap->manareturn_stat.perlevel );
    template_put_range( filetemp, filewrite, pcap->manaflow_stat.val );
    template_put_range( filetemp, filewrite, pcap->manaflow_stat.perlevel );
    template_put_range( filetemp, filewrite, pcap->strength_stat.val );
    template_put_range( filetemp, filewrite, pcap->strength_stat.perlevel );
    template_put_range( filetemp, filewrite, pcap->wisdom_stat.val );
    template_put_range( filetemp, filewrite, pcap->wisdom_stat.perlevel );
    template_put_range( filetemp, filewrite, pcap->intelligence_stat.val );
    template_put_range( filetemp, filewrite, pcap->intelligence_stat.perlevel );
    template_put_range( filetemp, filewrite, pcap->dexterity_stat.val );
    template_put_range( filetemp, filewrite, pcap->dexterity_stat.perlevel );

    // More physical attributes
    template_put_float( filetemp, filewrite, pcap->size );
    template_put_float( filetemp, filewrite, pcap->size_perlevel );
    template_put_int( filetemp, filewrite, pcap->shadow_size );
    template_put_int( filetemp, filewrite, pcap->bump_size );
    template_put_int( filetemp, filewrite, pcap->bump_height );
    template_put_float( filetemp, filewrite, pcap->bumpdampen );
    template_put_int( filetemp, filewrite, pcap->weight );
    template_put_float( filetemp, filewrite, pcap->jump );
    template_put_int( filetemp, filewrite, pcap->jumpnumber );
    template_put_float( filetemp, filewrite, pcap->anim_speed_sneak );
    template_put_float( filetemp, filewrite, pcap->anim_speed_walk );
    template_put_float( filetemp, filewrite, pcap->anim_speed_run );
    template_put_int( filetemp, filewrite, pcap->flyheight );
    template_put_int( filetemp, filewrite, pcap->flashand );
    template_put_int( filetemp, filewrite, pcap->alpha );
    template_put_int( filetemp, filewrite, pcap->light );
    template_put_bool( filetemp, filewrite, pcap->transferblend );
    template_put_int( filetemp, filewrite, pcap->sheen );
    template_put_bool( filetemp, filewrite, pcap->enviro );
    template_put_float( filetemp, filewrite, FFFF_TO_FLOAT( pcap->uoffvel ) );
    template_put_float( filetemp, filewrite, FFFF_TO_FLOAT( pcap->voffvel ) );
    template_put_bool( filetemp, filewrite, pcap->stickybutt );

    // Invulnerability data
    template_put_bool( filetemp, filewrite, pcap->invictus );
    template_put_int( filetemp, filewrite, pcap->nframefacing );
    template_put_int( filetemp, filewrite, pcap->nframeangle );
    template_put_int( filetemp, filewrite, pcap->iframefacing );
    template_put_int( filetemp, filewrite, pcap->iframeangle );

    // Skin defenses
    template_put_int( filetemp, filewrite, 255 - pcap->defense[0] );
    template_put_int( filetemp, filewrite, 255 - pcap->defense[1] );
    template_put_int( filetemp, filewrite, 255 - pcap->defense[2] );
    template_put_int( filetemp, filewrite, 255 - pcap->defense[3] );

    for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        template_put_float( filetemp, filewrite, pcap->damage_resistance[damagetype][0] );
        template_put_float( filetemp, filewrite, pcap->damage_resistance[damagetype][1] );
        template_put_float( filetemp, filewrite, pcap->damage_resistance[damagetype][2] );
        template_put_float( filetemp, filewrite, pcap->damage_resistance[damagetype][3] );
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

            template_put_char( filetemp, filewrite, code );
        }
    }

    template_put_float( filetemp, filewrite, pcap->skin_info.maxaccel[0]*80 );
    template_put_float( filetemp, filewrite, pcap->skin_info.maxaccel[1]*80 );
    template_put_float( filetemp, filewrite, pcap->skin_info.maxaccel[2]*80 );
    template_put_float( filetemp, filewrite, pcap->skin_info.maxaccel[3]*80 );

    // Experience and level data
    template_put_int( filetemp, filewrite, pcap->experience_forlevel[1] );
    template_put_int( filetemp, filewrite, pcap->experience_forlevel[2] );
    template_put_int( filetemp, filewrite, pcap->experience_forlevel[3] );
    template_put_int( filetemp, filewrite, pcap->experience_forlevel[4] );
    template_put_int( filetemp, filewrite, pcap->experience_forlevel[5] );
    template_put_float( filetemp, filewrite, FLOAT_TO_FP8( pcap->experience.from ) );
    template_put_int( filetemp, filewrite, pcap->experience_worth );
    template_put_float( filetemp, filewrite, pcap->experience_exchange );
    template_put_float( filetemp, filewrite, pcap->experience_rate[0] );
    template_put_float( filetemp, filewrite, pcap->experience_rate[1] );
    template_put_float( filetemp, filewrite, pcap->experience_rate[2] );
    template_put_float( filetemp, filewrite, pcap->experience_rate[3] );
    template_put_float( filetemp, filewrite, pcap->experience_rate[4] );
    template_put_float( filetemp, filewrite, pcap->experience_rate[5] );
    template_put_float( filetemp, filewrite, pcap->experience_rate[6] );
    template_put_float( filetemp, filewrite, pcap->experience_rate[7] );

    // IDSZ identification tags
    template_put_idsz( filetemp, filewrite, pcap->idsz[IDSZ_PARENT] );
    template_put_idsz( filetemp, filewrite, pcap->idsz[IDSZ_TYPE] );
    template_put_idsz( filetemp, filewrite, pcap->idsz[IDSZ_SKILL] );
    template_put_idsz( filetemp, filewrite, pcap->idsz[IDSZ_SPECIAL] );
    template_put_idsz( filetemp, filewrite, pcap->idsz[IDSZ_HATE] );
    template_put_idsz( filetemp, filewrite, pcap->idsz[IDSZ_VULNERABILITY] );

    // Item and damage flags
    template_put_bool( filetemp, filewrite, pcap->isitem );
    template_put_bool( filetemp, filewrite, pcap->ismount );
    template_put_bool( filetemp, filewrite, pcap->isstackable );
    template_put_bool( filetemp, filewrite, pcap->nameknown );
    template_put_bool( filetemp, filewrite, pcap->usageknown );
    template_put_bool( filetemp, filewrite, pcap->cancarrytonextmodule );
    template_put_bool( filetemp, filewrite, pcap->needskillidtouse );
    template_put_bool( filetemp, filewrite, pcap->platform );
    template_put_bool( filetemp, filewrite, pcap->cangrabmoney );
    template_put_bool( filetemp, filewrite, pcap->canopenstuff );

    // Other item and damage stuff
    template_put_damage_type( filetemp, filewrite, pcap->damagetarget_damagetype );
    template_put_action( filetemp, filewrite, pcap->weaponaction );

    // Particle attachments
    template_put_int( filetemp, filewrite, pcap->attachedprt_amount );
    template_put_damage_type( filetemp, filewrite, pcap->attachedprt_reaffirm_damagetype );
    template_put_int( filetemp, filewrite, pcap->attachedprt_lpip );

    // Character hands
    template_put_bool( filetemp, filewrite, pcap->slotvalid[SLOT_LEFT] );
    template_put_bool( filetemp, filewrite, pcap->slotvalid[SLOT_RIGHT] );

    // Particle spawning on attack
    template_put_bool( filetemp, filewrite, 0 != pcap->attack_attached );
    template_put_int( filetemp, filewrite, pcap->attack_lpip );

    // Particle spawning for GoPoof
    template_put_int( filetemp, filewrite, pcap->gopoofprt_amount );
    template_put_int( filetemp, filewrite, pcap->gopoofprt_facingadd );
    template_put_int( filetemp, filewrite, pcap->gopoofprt_lpip );

    // Particle spawning for blud
    template_put_bool( filetemp, filewrite, 0 != pcap->blud_valid );
    template_put_int( filetemp, filewrite, pcap->blud_lpip );

    // Extra stuff
    template_put_bool( filetemp, filewrite, pcap->waterwalk );
    template_put_float( filetemp, filewrite, pcap->dampen );

    // More stuff
    template_put_float( filetemp, filewrite, FP8_TO_FLOAT( pcap->life_heal ) );     // These two are seriously outdated
    template_put_float( filetemp, filewrite, FP8_TO_FLOAT( pcap->manacost ) );     // and shouldnt be used. Use scripts instead.
    template_put_int( filetemp, filewrite, pcap->life_return );
    template_put_int( filetemp, filewrite, pcap->stoppedby );
    template_put_string_under( filetemp, filewrite, pcap->skin_info.name[0] );
    template_put_string_under( filetemp, filewrite, pcap->skin_info.name[1] );
    template_put_string_under( filetemp, filewrite, pcap->skin_info.name[2] );
    template_put_string_under( filetemp, filewrite, pcap->skin_info.name[3] );
    template_put_int( filetemp, filewrite, pcap->skin_info.cost[0] );
    template_put_int( filetemp, filewrite, pcap->skin_info.cost[1] );
    template_put_int( filetemp, filewrite, pcap->skin_info.cost[2] );
    template_put_int( filetemp, filewrite, pcap->skin_info.cost[3] );
    template_put_float( filetemp, filewrite, pcap->str_bonus );

    // Another memory lapse
    template_put_bool( filetemp, filewrite, !pcap->ridercanattack );
    template_put_bool( filetemp, filewrite, pcap->canbedazed );
    template_put_bool( filetemp, filewrite, pcap->canbegrogged );
    template_put_int( filetemp, filewrite, 0 );
    template_put_int( filetemp, filewrite, 0 );
    template_put_bool( filetemp, filewrite, pcap->see_invisible_level > 0 );
    template_put_int( filetemp, filewrite, pcap->kursechance );
    template_put_int( filetemp, filewrite, pcap->sound_index[SOUND_FOOTFALL] );
    template_put_int( filetemp, filewrite, pcap->sound_index[SOUND_JUMP] );

    vfs_flush( filewrite );

    // copy the template file to the next free output section
    template_seek_free( filetemp, filewrite );

    // Expansions
    if ( pcap->skin_info.dressy&1 )
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'D', 'R', 'E', 'S' ), 0 );

    if ( pcap->skin_info.dressy&2 )
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'D', 'R', 'E', 'S' ), 1 );

    if ( pcap->skin_info.dressy&4 )
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'D', 'R', 'E', 'S' ), 2 );

    if ( pcap->skin_info.dressy&8 )
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'D', 'R', 'E', 'S' ), 3 );

    if ( pcap->resistbumpspawn )
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'S', 'T', 'U', 'K' ), 0 );

    if ( pcap->istoobig )
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'P', 'A', 'C', 'K' ), 0 );

    if ( !pcap->reflect )
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'V', 'A', 'M', 'P' ), 1 );

    if ( pcap->alwaysdraw )
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'D', 'R', 'A', 'W' ), 1 );

    if ( pcap->isranged )
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'R', 'A', 'N', 'G' ), 1 );

    if ( pcap->hidestate != NOHIDE )
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'H', 'I', 'D', 'E' ), pcap->hidestate );

    if ( pcap->isequipment )
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'E', 'Q', 'U', 'I' ), 1 );

    if ( pcap->bump_sizebig >= pcap->bump_size * 2 )
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'S', 'Q', 'U', 'A' ), 1 );

    if ( pcap->draw_icon != pcap->usageknown )
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'I', 'C', 'O', 'N' ), pcap->draw_icon );

    if ( pcap->forceshadow )
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'S', 'H', 'A', 'D' ), 1 );

    if ( pcap->ripple == pcap->isitem )
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'R', 'I', 'P', 'P' ), pcap->ripple );

    if ( -1 != pcap->isvaluable )
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'V', 'A', 'L', 'U' ), pcap->isvaluable );

    if ( NO_SKIN_OVERRIDE != pcap->spelleffect_type )
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'B', 'O', 'O', 'K' ), pcap->spelleffect_type );

    if ( pcap->attack_fast )
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'F', 'A', 'S', 'T' ), pcap->attack_fast );

    if ( pcap->str_bonus > 0 )
        vfs_put_expansion_float( filewrite, "", MAKE_IDSZ( 'S', 'T', 'R', 'D' ), pcap->str_bonus );

    if ( pcap->int_bonus > 0 )
        vfs_put_expansion_float( filewrite, "", MAKE_IDSZ( 'I', 'N', 'T', 'D' ), pcap->int_bonus );

    if ( pcap->dex_bonus > 0 )
        vfs_put_expansion_float( filewrite, "", MAKE_IDSZ( 'D', 'E', 'X', 'D' ), pcap->dex_bonus );

    if ( pcap->wis_bonus > 0 )
        vfs_put_expansion_float( filewrite, "", MAKE_IDSZ( 'W', 'I', 'S', 'D' ), pcap->wis_bonus );

    if ( pcap->bump_override_size || pcap->bump_override_sizebig ||  pcap->bump_override_height )
    {
        STRING sz_tmp = EMPTY_CSTR;

        if ( pcap->bump_override_size ) strcat( sz_tmp, "S" );
        if ( pcap->bump_override_sizebig ) strcat( sz_tmp, "B" );
        if ( pcap->bump_override_height ) strcat( sz_tmp, "H" );
        if ( pcap->dont_cull_backfaces ) strcat( sz_tmp, "C" );
        if ( pcap->skin_has_transparency ) strcat( sz_tmp, "T" );

        if ( CSTR_END != sz_tmp[0] )
        {
            vfs_put_expansion_string( filewrite, "", MAKE_IDSZ( 'M', 'O', 'D', 'L' ), sz_tmp );
        }
    }

    // Basic stuff that is always written
    vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'G', 'O', 'L', 'D' ), pcap->money );
    vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'P', 'L', 'A', 'T' ), pcap->canuseplatforms );
    vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'S', 'K', 'I', 'N' ), pcap->skin_override );
    vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'C', 'O', 'N', 'T' ), pcap->content_override );
    vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'S', 'T', 'A', 'T' ), pcap->state_override );
    vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'L', 'E', 'V', 'L' ), pcap->level_override );
    vfs_put_expansion_float( filewrite, "", MAKE_IDSZ( 'L', 'I', 'F', 'E' ), FP8_TO_FLOAT( pcap->life_spawn ) );
    vfs_put_expansion_float( filewrite, "", MAKE_IDSZ( 'M', 'A', 'N', 'A' ), FP8_TO_FLOAT( pcap->mana_spawn ) );

    // Copy all skill expansions
    {
        IDSZ_node_t *pidsz;
        int iterator;

        iterator = 0;
        pidsz = idsz_map_iterate( pcap->skills, SDL_arraysize( pcap->skills ), &iterator );
        while ( pidsz != NULL )
        {
            //Write that skill into the file
            vfs_put_expansion( filewrite, "", pidsz->id, pidsz->level );

            //Get the next IDSZ from the map
            pidsz = idsz_map_iterate( pcap->skills, SDL_arraysize( pcap->skills ), &iterator );
        }
    }

    // dump the rest of the template file
    template_flush( filetemp, filewrite );

    // The end
    vfs_close( filewrite );
    template_close_vfs( filetemp );

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
SKIN_T cap_get_skin( cap_t * pcap )
{
	SKIN_T retval = MAX_SKIN;

	if( NULL == pcap ) return MAX_SKIN;

    if ( NO_SKIN_OVERRIDE != pcap->spelleffect_type )
    {
        retval = pcap->spelleffect_type % MAX_SKIN;
    }
    else if ( NO_SKIN_OVERRIDE != pcap->skin_override )
    {
        retval = pcap->skin_override % MAX_SKIN;
    }

	return retval;
}