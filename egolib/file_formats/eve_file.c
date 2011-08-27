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

/// @file file_formats/eve_file.c
/// @brief Functions to read and write Egoboo's enchant definition files ( /modules/*.mod/objects/*.obj/enchant.txt )
/// @details

#include "eve_file.h"
#include "template.h"

#include "../strutil.h"
#include "../fileutil.h"
#include "../vfs.h"

#include "../_math.inl"

// includes for egoboo constants
#include "../../game/sound.h"       // for INVALID_SOUND

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
eve_t * eve_init( eve_t * peve )
{
    if ( NULL == peve ) return peve;

    BLANK_STRUCT_PTR( peve )

    peve->endsound_index = INVALID_SOUND;

    return peve;
}

//--------------------------------------------------------------------------------------------
eve_t * load_one_enchant_file_vfs( const char* szLoadName, eve_t * peve )
{
    /// @author ZZ
    /// @details This function loads the enchantment associated with an object

    int cnt;
    vfs_FILE* fileread;
    char cTmp;
    IDSZ idsz;

    if ( NULL == peve ) return NULL;

    eve_init( peve );

    fileread = vfs_openRead( szLoadName );
    if ( NULL == fileread ) return NULL;

    // btrue/bfalse values
    peve->retarget = vfs_get_next_bool( fileread );
    peve->override = vfs_get_next_bool( fileread );
    peve->remove_overridden = vfs_get_next_bool( fileread );
    peve->killtargetonend = vfs_get_next_bool( fileread );

    peve->poofonend = vfs_get_next_bool( fileread );

    // More stuff
    peve->lifetime   = vfs_get_next_int( fileread );
    peve->endmessage = vfs_get_next_int( fileread );

    // Drain stuff
    peve->owner_mana    = vfs_get_next_sfp8( fileread );
    peve->target_mana   = vfs_get_next_sfp8( fileread );
    peve->endifcantpay  = vfs_get_next_bool( fileread );
    peve->owner_life    = vfs_get_next_sfp8( fileread );
    peve->target_life   = vfs_get_next_sfp8( fileread );

    // Specifics
    peve->required_damagetype = vfs_get_next_damage_type( fileread );
    peve->require_damagetarget_damagetype = vfs_get_next_damage_type( fileread );
    peve->removedbyidsz   = vfs_get_next_idsz( fileread );

    // Now the set values
    peve->setyesno[SETDAMAGETYPE] = vfs_get_next_bool( fileread );
    peve->setvalue[SETDAMAGETYPE] = vfs_get_damage_type( fileread );

    peve->setyesno[SETNUMBEROFJUMPS] = vfs_get_next_bool( fileread );
    peve->setvalue[SETNUMBEROFJUMPS] = vfs_get_int( fileread );

    peve->setyesno[SETLIFEBARCOLOR] = vfs_get_next_bool( fileread );
    peve->setvalue[SETLIFEBARCOLOR] = vfs_get_int( fileread );

    peve->setyesno[SETMANABARCOLOR] = vfs_get_next_bool( fileread );
    peve->setvalue[SETMANABARCOLOR] = vfs_get_int( fileread );

    peve->setyesno[SETSLASHMODIFIER] = vfs_get_next_bool( fileread );
    peve->setvalue[SETSLASHMODIFIER] = vfs_get_damage_modifier( fileread );
    peve->addvalue[ADDSLASHRESIST]   = vfs_get_damage_resist( fileread );

    peve->setyesno[SETCRUSHMODIFIER] = vfs_get_next_bool( fileread );
    peve->setvalue[SETCRUSHMODIFIER] = vfs_get_damage_modifier( fileread );
    peve->addvalue[ADDCRUSHRESIST]   = vfs_get_damage_resist( fileread );

    peve->setyesno[SETPOKEMODIFIER] = vfs_get_next_bool( fileread );
    peve->setvalue[SETPOKEMODIFIER] = vfs_get_damage_modifier( fileread );
    peve->addvalue[ADDPOKERESIST]   = vfs_get_damage_resist( fileread );

    peve->setyesno[SETHOLYMODIFIER] = vfs_get_next_bool( fileread );
    peve->setvalue[SETHOLYMODIFIER] = vfs_get_damage_modifier( fileread );
    peve->addvalue[ADDHOLYRESIST]   = vfs_get_damage_resist( fileread );

    peve->setyesno[SETEVILMODIFIER] = vfs_get_next_bool( fileread );
    peve->setvalue[SETEVILMODIFIER] = vfs_get_damage_modifier( fileread );
    peve->addvalue[ADDEVILRESIST]   = vfs_get_damage_resist( fileread );

    peve->setyesno[SETFIREMODIFIER] = vfs_get_next_bool( fileread );
    peve->setvalue[SETFIREMODIFIER] = vfs_get_damage_modifier( fileread );
    peve->addvalue[ADDFIRERESIST]   = vfs_get_damage_resist( fileread );

    peve->setyesno[SETICEMODIFIER] = vfs_get_next_bool( fileread );
    peve->setvalue[SETICEMODIFIER] = vfs_get_damage_modifier( fileread );
    peve->addvalue[ADDICERESIST]   = vfs_get_damage_resist( fileread );

    peve->setyesno[SETZAPMODIFIER] = vfs_get_next_bool( fileread );
    peve->setvalue[SETZAPMODIFIER] = vfs_get_damage_modifier( fileread );
    peve->addvalue[ADDZAPRESIST]   = vfs_get_damage_resist( fileread );

    peve->setyesno[SETFLASHINGAND] = vfs_get_next_bool( fileread );
    peve->setvalue[SETFLASHINGAND] = vfs_get_int( fileread );

    peve->setyesno[SETLIGHTBLEND] = vfs_get_next_bool( fileread );
    peve->setvalue[SETLIGHTBLEND] = vfs_get_int( fileread );

    peve->setyesno[SETALPHABLEND] = vfs_get_next_bool( fileread );
    peve->setvalue[SETALPHABLEND] = vfs_get_int( fileread );

    peve->setyesno[SETSHEEN] = vfs_get_next_bool( fileread );
    peve->setvalue[SETSHEEN] = vfs_get_int( fileread );

    peve->setyesno[SETFLYTOHEIGHT] = vfs_get_next_bool( fileread );
    peve->setvalue[SETFLYTOHEIGHT] = vfs_get_int( fileread );

    peve->setyesno[SETWALKONWATER] = vfs_get_next_bool( fileread );
    peve->setvalue[SETWALKONWATER] = vfs_get_bool( fileread );

    peve->setyesno[SETCANSEEINVISIBLE] = vfs_get_next_bool( fileread );
    peve->setvalue[SETCANSEEINVISIBLE] = vfs_get_bool( fileread );

    peve->setyesno[SETMISSILETREATMENT] = vfs_get_next_bool( fileread );
    cTmp = vfs_get_first_letter( fileread );
    if ( 'R' == toupper( (unsigned)cTmp ) )       peve->setvalue[SETMISSILETREATMENT] = MISSILE_REFLECT;
    else if ( 'D' == toupper( (unsigned)cTmp ) )  peve->setvalue[SETMISSILETREATMENT] = MISSILE_DEFLECT;
    else                                          peve->setvalue[SETMISSILETREATMENT] = MISSILE_NORMAL;

    peve->setyesno[SETCOSTFOREACHMISSILE] = vfs_get_next_bool( fileread );
    peve->setvalue[SETCOSTFOREACHMISSILE] = vfs_get_float( fileread );

    peve->setyesno[SETMORPH] = vfs_get_next_bool( fileread );
    peve->setvalue[SETMORPH] = btrue;  // vfs_get_bool( fileread );        //ZF> huh? why always channel and morph?

    peve->setyesno[SETCHANNEL] = vfs_get_next_bool( fileread );
    peve->setvalue[SETCHANNEL] = btrue;  // vfs_get_bool( fileread );

    // Now read in the add values
    peve->addvalue[ADDJUMPPOWER]    = vfs_get_next_float( fileread );
    peve->addvalue[ADDBUMPDAMPEN]   = vfs_get_next_int( fileread ) / 256.0f;    // Stored as 8.8-fixed, used as float
    peve->addvalue[ADDBOUNCINESS]   = vfs_get_next_int( fileread ) / 256.0f;    // Stored as 8.8-fixed, used as float
    peve->addvalue[ADDDAMAGE]       = vfs_get_next_sfp8( fileread );            // Stored as float, used as 8.8-fixed
    peve->addvalue[ADDSIZE]         = vfs_get_next_float( fileread );           // Stored as float, used as float
    peve->addvalue[ADDACCEL]        = vfs_get_next_int( fileread )   / 80.0f;   // Stored as int, used as float
    peve->addvalue[ADDRED]          = vfs_get_next_int( fileread );
    peve->addvalue[ADDGRN]          = vfs_get_next_int( fileread );
    peve->addvalue[ADDBLU]          = vfs_get_next_int( fileread );
    peve->addvalue[ADDDEFENSE]      = -vfs_get_next_int( fileread );    // Defense is backwards
    peve->addvalue[ADDMANA]         = vfs_get_next_sfp8( fileread );    // Stored as float, used as 8.8-fixed
    peve->addvalue[ADDLIFE]         = vfs_get_next_sfp8( fileread );    // Stored as float, used as 8.8-fixed
    peve->addvalue[ADDSTRENGTH]     = vfs_get_next_sfp8( fileread );    // Stored as float, used as 8.8-fixed
    peve->addvalue[ADDWISDOM]       = vfs_get_next_sfp8( fileread );    // Stored as float, used as 8.8-fixed
    peve->addvalue[ADDINTELLIGENCE] = vfs_get_next_sfp8( fileread );    // Stored as float, used as 8.8-fixed
    peve->addvalue[ADDDEXTERITY]    = vfs_get_next_sfp8( fileread );    // Stored as float, used as 8.8-fixed

    // Determine which entries are not important
    for ( cnt = 0; cnt < MAX_ENCHANT_ADD; cnt++ )
    {
        peve->addyesno[cnt] = ( 0.0f != peve->addvalue[cnt] );
    }

    // Read expansions
    while ( goto_colon_vfs( NULL, fileread, btrue ) )
    {
        idsz = vfs_get_idsz( fileread );

        if ( idsz == MAKE_IDSZ( 'A', 'M', 'O', 'U' ) ) peve->contspawn_amount = vfs_get_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'T', 'Y', 'P', 'E' ) ) peve->contspawn_lpip = vfs_get_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'T', 'I', 'M', 'E' ) ) peve->contspawn_delay = vfs_get_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'F', 'A', 'C', 'E' ) ) peve->contspawn_facingadd = vfs_get_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'S', 'E', 'N', 'D' ) ) peve->endsound_index = vfs_get_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'S', 'T', 'A', 'Y' ) ) peve->stayifnoowner = ( 0 != vfs_get_int( fileread ) );
        else if ( idsz == MAKE_IDSZ( 'O', 'V', 'E', 'R' ) ) peve->spawn_overlay = ( 0 != vfs_get_int( fileread ) );
        else if ( idsz == MAKE_IDSZ( 'D', 'E', 'A', 'D' ) ) peve->stayiftargetdead = ( 0 != vfs_get_int( fileread ) );

        else if ( idsz == MAKE_IDSZ( 'C', 'K', 'U', 'R' ) ) peve->seekurse = vfs_get_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'D', 'A', 'R', 'K' ) ) peve->darkvision = vfs_get_int( fileread );
    }

    // All done ( finally )
    vfs_close( fileread );

    strncpy( peve->name, szLoadName, SDL_arraysize( peve->name ) );
    peve->loaded = btrue;

    return peve;
}

//--------------------------------------------------------------------------------------------
bool_t save_one_enchant_file_vfs( const char* szLoadName, const char * szTemplateName, eve_t * peve )
{
    /// @author ZZ
    /// @details This function loads the enchantment associated with an object
    vfs_FILE* filewrite, * filetemp;

    if ( NULL == peve ) return bfalse;

    filewrite = vfs_openWrite( szLoadName );
    if ( NULL == filewrite ) return bfalse;

    filetemp = NULL;

    // try the given template file
    if ( VALID_CSTR( szTemplateName ) )
    {
        filetemp = template_open_vfs( szTemplateName );
    }

    // try a default template file
    if ( NULL == filetemp )
    {
        filetemp = template_open_vfs( "mp_data/templates/enchant.txt" );
    }

    // btrue/bfalse values
    template_put_bool( filetemp, filewrite, peve->retarget );
    template_put_bool( filetemp, filewrite, peve->override );
    template_put_bool( filetemp, filewrite, peve->remove_overridden );
    template_put_bool( filetemp, filewrite, peve->killtargetonend );

    template_put_bool( filetemp, filewrite, peve->poofonend );

    // More stuff
    template_put_int( filetemp, filewrite, MAX( -1, peve->lifetime ) );
    template_put_int( filetemp, filewrite, peve->endmessage );

    // Drain stuff
    template_put_float( filetemp, filewrite, FP8_TO_FLOAT( peve->owner_mana ) );
    template_put_float( filetemp, filewrite, FP8_TO_FLOAT( peve->target_mana ) );
    template_put_bool( filetemp, filewrite, peve->endifcantpay );
    template_put_float( filetemp, filewrite, FP8_TO_FLOAT( peve->owner_life ) );
    template_put_float( filetemp, filewrite, FP8_TO_FLOAT( peve->target_life ) );

    // Specifics
    template_put_damage_type( filetemp, filewrite, peve->required_damagetype );
    template_put_damage_type( filetemp, filewrite, peve->require_damagetarget_damagetype );
    template_put_idsz( filetemp, filewrite, peve->removedbyidsz );

    // Now the set values
    template_put_bool( filetemp, filewrite, peve->setyesno[SETDAMAGETYPE] );
    template_put_damage_type( filetemp, filewrite, peve->setvalue[SETDAMAGETYPE] );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETNUMBEROFJUMPS] );
    template_put_int( filetemp, filewrite, peve->setvalue[SETNUMBEROFJUMPS] );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETLIFEBARCOLOR] );
    template_put_int( filetemp, filewrite, peve->setvalue[SETLIFEBARCOLOR] );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETMANABARCOLOR] );
    template_put_int( filetemp, filewrite, peve->setvalue[SETMANABARCOLOR] );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETSLASHMODIFIER] );
    template_put_damage_modifier( filetemp, filewrite, peve->setvalue[SETSLASHMODIFIER] );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDSLASHRESIST] * 100.0f );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETCRUSHMODIFIER] );
    template_put_damage_modifier( filetemp, filewrite, peve->setvalue[SETCRUSHMODIFIER] );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDCRUSHRESIST] * 100.0f );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETPOKEMODIFIER] );
    template_put_damage_modifier( filetemp, filewrite, peve->setvalue[SETPOKEMODIFIER] );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDPOKERESIST] * 100.0f );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETHOLYMODIFIER] );
    template_put_damage_modifier( filetemp, filewrite, peve->setvalue[SETHOLYMODIFIER] );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDHOLYRESIST] * 100.0f );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETEVILMODIFIER] );
    template_put_damage_modifier( filetemp, filewrite, peve->setvalue[SETEVILMODIFIER] );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDEVILRESIST] * 100.0f );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETFIREMODIFIER] );
    template_put_damage_modifier( filetemp, filewrite, peve->setvalue[SETFIREMODIFIER] );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDFIRERESIST] * 100.0f );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETICEMODIFIER] );
    template_put_damage_modifier( filetemp, filewrite, peve->setvalue[SETICEMODIFIER] );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDICERESIST] * 100.0f );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETZAPMODIFIER] );
    template_put_damage_modifier( filetemp, filewrite, peve->setvalue[SETZAPMODIFIER] );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDZAPRESIST] * 100.0f );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETFLASHINGAND] );
    template_put_int( filetemp, filewrite, peve->setvalue[SETFLASHINGAND] );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETLIGHTBLEND] );
    template_put_int( filetemp, filewrite, peve->setvalue[SETLIGHTBLEND] );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETALPHABLEND] );
    template_put_int( filetemp, filewrite, peve->setvalue[SETALPHABLEND] );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETSHEEN] );
    template_put_int( filetemp, filewrite, peve->setvalue[SETSHEEN] );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETFLYTOHEIGHT] );
    template_put_int( filetemp, filewrite, peve->setvalue[SETFLYTOHEIGHT] );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETWALKONWATER] );
    template_put_bool( filetemp, filewrite, 0 != peve->setvalue[SETWALKONWATER] );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETCANSEEINVISIBLE] );
    template_put_bool( filetemp, filewrite, 0 != peve->setvalue[SETCANSEEINVISIBLE] );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETMISSILETREATMENT] );

    switch (( int )peve->setvalue[SETMISSILETREATMENT] )
    {
        case MISSILE_NORMAL : template_put_char( filetemp, filewrite, 'N' ); break;
        case MISSILE_DEFLECT: template_put_char( filetemp, filewrite, 'D' ); break;
        case MISSILE_REFLECT: template_put_char( filetemp, filewrite, 'R' ); break;
    }

    template_put_bool( filetemp, filewrite, peve->setyesno[SETCOSTFOREACHMISSILE] );
    template_put_float( filetemp, filewrite, peve->setvalue[SETCOSTFOREACHMISSILE] );
    template_put_bool( filetemp, filewrite, peve->setyesno[SETMORPH] );
    template_put_bool( filetemp, filewrite, peve->setyesno[SETCHANNEL] );

    // Now read in the add values
    template_put_float( filetemp, filewrite, peve->addvalue[ADDJUMPPOWER] );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDBUMPDAMPEN] * 256.0f );  // Used as float, stored as 8.8-fixed
    template_put_float( filetemp, filewrite, peve->addvalue[ADDBOUNCINESS] * 256.0f );  // Used as float, stored as 8.8-fixed
    template_put_sfp8( filetemp, filewrite, peve->addvalue[ADDDAMAGE] );                // Used as 8.8-fixed, stored as float
    template_put_float( filetemp, filewrite, peve->addvalue[ADDSIZE] );
    template_put_int( filetemp, filewrite, peve->addvalue[ADDACCEL] * 80.0f );
    template_put_int( filetemp, filewrite, peve->addvalue[ADDRED] );
    template_put_int( filetemp, filewrite, peve->addvalue[ADDGRN] );
    template_put_int( filetemp, filewrite, peve->addvalue[ADDBLU] );
    template_put_int( filetemp, filewrite, -peve->addvalue[ADDDEFENSE] );                // Defense is backwards
    template_put_sfp8( filetemp, filewrite, peve->addvalue[ADDMANA] );                   // Used as 8.8-fixed, stored as float
    template_put_sfp8( filetemp, filewrite, peve->addvalue[ADDLIFE] );                   // Used as 8.8-fixed, stored as float
    template_put_sfp8( filetemp, filewrite, peve->addvalue[ADDSTRENGTH] );               // Used as 8.8-fixed, stored as float
    template_put_sfp8( filetemp, filewrite, peve->addvalue[ADDWISDOM] );                 // Used as 8.8-fixed, stored as float
    template_put_sfp8( filetemp, filewrite, peve->addvalue[ADDINTELLIGENCE] );           // Used as 8.8-fixed, stored as float
    template_put_sfp8( filetemp, filewrite, peve->addvalue[ADDDEXTERITY] );              // Used as 8.8-fixed, stored as float

    // copy the template file to the next free output section
    template_seek_free( filetemp, filewrite );

    if ( peve->contspawn_amount > 0 )
    {
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'A', 'M', 'O', 'U' ), peve->contspawn_amount );
    }

    if ( peve->contspawn_lpip > 0 )
    {
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'T', 'Y', 'P', 'E' ), peve->contspawn_lpip );
    }

    if ( peve->contspawn_facingadd > 0 )
    {
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'T', 'I', 'M', 'E' ), peve->contspawn_facingadd );
    }

    if ( peve->contspawn_delay > 0 )
    {
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'F', 'A', 'C', 'E' ), peve->contspawn_delay );
    }

    if ( INVALID_SOUND != peve->endsound_index )
    {
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'S', 'E', 'N', 'D' ), peve->endsound_index );
    }

    if ( peve->stayifnoowner )
    {
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'S', 'T', 'A', 'Y' ), 1 );
    }

    if ( peve->spawn_overlay )
    {
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'O', 'V', 'E', 'R' ), peve->spawn_overlay );
    }

    if ( peve->seekurse )
    {
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'C', 'K', 'U', 'R' ), peve->seekurse );
    }

    if ( peve->darkvision )
    {
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'D', 'A', 'R', 'K' ), peve->darkvision );
    }

    if ( peve->stayiftargetdead )
    {
        vfs_put_expansion( filewrite, "", MAKE_IDSZ( 'D', 'E', 'A', 'D' ), 1 );
    }

    // dump the rest of the template file
    template_flush( filetemp, filewrite );

    // All done ( finally )
    vfs_close( filewrite );
    template_close_vfs( filetemp );

    return btrue;
}

