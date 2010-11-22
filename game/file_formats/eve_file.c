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
#include "sound.h"

#include "egoboo_strutil.h"
#include "egoboo_fileutil.h"
#include "egoboo_vfs.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
eve_t * eve_init( eve_t * peve )
{
    if ( NULL == peve ) return peve;

    memset( peve, 0, sizeof( *peve ) );

    peve->endsound_index = INVALID_SOUND;

    return peve;
}

//--------------------------------------------------------------------------------------------
eve_t * load_one_enchant_file_vfs( const char* szLoadName, eve_t * peve )
{
    /// @details ZZ@> This function loads the enchantment associated with an object

    int cnt;
    vfs_FILE* fileread;
    char cTmp;
    IDSZ idsz;

    if ( NULL == peve ) return NULL;

    eve_init( peve );

    fileread = vfs_openRead( szLoadName );
    if ( NULL == fileread ) return NULL;

    // btrue/bfalse values
    peve->retarget = fget_next_bool( fileread );
    peve->override = fget_next_bool( fileread );
    peve->remove_overridden = fget_next_bool( fileread );
    peve->killtargetonend = fget_next_bool( fileread );

    peve->poofonend = fget_next_bool( fileread );

    // More stuff
    peve->lifetime   = fget_next_int( fileread );
    peve->endmessage = fget_next_int( fileread );

    // Drain stuff
    peve->owner_mana    = fget_next_float( fileread ) * 256;
    peve->target_mana   = fget_next_float( fileread ) * 256;
    peve->endifcantpay  = fget_next_bool( fileread );
    peve->owner_life    = fget_next_float( fileread ) * 256;
    peve->target_life   = fget_next_float( fileread ) * 256;

    // Specifics
    peve->required_damagetype = fget_next_damage_type( fileread );
    peve->require_damagetarget_damagetype = fget_next_damage_type( fileread );
    peve->removedbyidsz   = fget_next_idsz( fileread );

    // Now the set values
    peve->setyesno[SETDAMAGETYPE] = fget_next_bool( fileread );
    peve->setvalue[SETDAMAGETYPE] = fget_damage_type( fileread );

    peve->setyesno[SETNUMBEROFJUMPS] = fget_next_bool( fileread );
    peve->setvalue[SETNUMBEROFJUMPS] = fget_int( fileread );

    peve->setyesno[SETLIFEBARCOLOR] = fget_next_bool( fileread );
    peve->setvalue[SETLIFEBARCOLOR] = fget_int( fileread );

    peve->setyesno[SETMANABARCOLOR] = fget_next_bool( fileread );
    peve->setvalue[SETMANABARCOLOR] = fget_int( fileread );

    peve->setyesno[SETSLASHMODIFIER] = fget_next_bool( fileread );
    peve->setvalue[SETSLASHMODIFIER] = fget_damage_modifier( fileread );

    peve->setyesno[SETCRUSHMODIFIER] = fget_next_bool( fileread );
    peve->setvalue[SETCRUSHMODIFIER] = fget_damage_modifier( fileread );

    peve->setyesno[SETPOKEMODIFIER] = fget_next_bool( fileread );
    peve->setvalue[SETPOKEMODIFIER] = fget_damage_modifier( fileread );

    peve->setyesno[SETHOLYMODIFIER] = fget_next_bool( fileread );
    peve->setvalue[SETHOLYMODIFIER] = fget_damage_modifier( fileread );

    peve->setyesno[SETEVILMODIFIER] = fget_next_bool( fileread );
    peve->setvalue[SETEVILMODIFIER] = fget_damage_modifier( fileread );

    peve->setyesno[SETFIREMODIFIER] = fget_next_bool( fileread );
    peve->setvalue[SETFIREMODIFIER] = fget_damage_modifier( fileread );

    peve->setyesno[SETICEMODIFIER] = fget_next_bool( fileread );
    peve->setvalue[SETICEMODIFIER] = fget_damage_modifier( fileread );

    peve->setyesno[SETZAPMODIFIER] = fget_next_bool( fileread );
    peve->setvalue[SETZAPMODIFIER] = fget_damage_modifier( fileread );

    peve->setyesno[SETFLASHINGAND] = fget_next_bool( fileread );
    peve->setvalue[SETFLASHINGAND] = fget_int( fileread );

    peve->setyesno[SETLIGHTBLEND] = fget_next_bool( fileread );
    peve->setvalue[SETLIGHTBLEND] = fget_int( fileread );

    peve->setyesno[SETALPHABLEND] = fget_next_bool( fileread );
    peve->setvalue[SETALPHABLEND] = fget_int( fileread );

    peve->setyesno[SETSHEEN] = fget_next_bool( fileread );
    peve->setvalue[SETSHEEN] = fget_int( fileread );

    peve->setyesno[SETFLYTOHEIGHT] = fget_next_bool( fileread );
    peve->setvalue[SETFLYTOHEIGHT] = fget_int( fileread );

    peve->setyesno[SETWALKONWATER] = fget_next_bool( fileread );
    peve->setvalue[SETWALKONWATER] = fget_bool( fileread );

    peve->setyesno[SETCANSEEINVISIBLE] = fget_next_bool( fileread );
    peve->setvalue[SETCANSEEINVISIBLE] = fget_bool( fileread );

    peve->setyesno[SETMISSILETREATMENT] = fget_next_bool( fileread );
    cTmp = fget_first_letter( fileread );
    if ( 'R' == toupper( cTmp ) )  peve->setvalue[SETMISSILETREATMENT] = MISSILE_REFLECT;
    else if ( 'D' == toupper( cTmp ) )  peve->setvalue[SETMISSILETREATMENT] = MISSILE_DEFLECT;
    else                                peve->setvalue[SETMISSILETREATMENT] = MISSILE_NORMAL;

    peve->setyesno[SETCOSTFOREACHMISSILE] = fget_next_bool( fileread );
    peve->setvalue[SETCOSTFOREACHMISSILE] = fget_float( fileread );

    peve->setyesno[SETMORPH] = fget_next_bool( fileread );
    peve->setvalue[SETMORPH] = btrue;  // fget_bool( fileread );

    peve->setyesno[SETCHANNEL] = fget_next_bool( fileread );
    peve->setvalue[SETCHANNEL] = btrue;  // fget_bool( fileread );

    // Now read in the add values
    peve->addvalue[ADDJUMPPOWER]    = fget_next_float( fileread );
    peve->addvalue[ADDBUMPDAMPEN]   = fget_next_float( fileread ) / 256.0f;    // Stored as 8.8-fixed, used as float
    peve->addvalue[ADDBOUNCINESS]   = fget_next_float( fileread ) / 256.0f;    // Stored as 8.8-fixed, used as float
    peve->addvalue[ADDDAMAGE]       = fget_next_float( fileread ) * 256.0f;    // Stored as float, used as 8.8-fixed
    peve->addvalue[ADDSIZE]         = fget_next_float( fileread );             // Stored as float, used as float
    peve->addvalue[ADDACCEL]        = fget_next_int( fileread )   / 80.0f;     // Stored as int, used as float
    peve->addvalue[ADDRED]          = fget_next_int( fileread );
    peve->addvalue[ADDGRN]          = fget_next_int( fileread );
    peve->addvalue[ADDBLU]          = fget_next_int( fileread );
    peve->addvalue[ADDDEFENSE]      = -fget_next_int( fileread );              // Defense is backwards
    peve->addvalue[ADDMANA]         = fget_next_float( fileread ) * 256.0f;    // Stored as float, used as 8.8-fixed
    peve->addvalue[ADDLIFE]         = fget_next_float( fileread ) * 256.0f;    // Stored as float, used as 8.8-fixed
    peve->addvalue[ADDSTRENGTH]     = fget_next_float( fileread ) * 256.0f;    // Stored as float, used as 8.8-fixed
    peve->addvalue[ADDWISDOM]       = fget_next_float( fileread ) * 256.0f;    // Stored as float, used as 8.8-fixed
    peve->addvalue[ADDINTELLIGENCE] = fget_next_float( fileread ) * 256.0f;    // Stored as float, used as 8.8-fixed
    peve->addvalue[ADDDEXTERITY]    = fget_next_float( fileread ) * 256.0f;    // Stored as float, used as 8.8-fixed

    // Determine which entries are not important
    for ( cnt = 0; cnt < MAX_ENCHANT_ADD; cnt++ )
    {
        peve->addyesno[cnt] = ( 0.0f != peve->addvalue[cnt] );
    }

    // Read expansions
    while ( goto_colon( NULL, fileread, btrue ) )
    {
        idsz = fget_idsz( fileread );

        if ( idsz == MAKE_IDSZ( 'A', 'M', 'O', 'U' ) ) peve->contspawn_amount = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'T', 'Y', 'P', 'E' ) ) peve->contspawn_lpip = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'T', 'I', 'M', 'E' ) ) peve->contspawn_delay = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'F', 'A', 'C', 'E' ) ) peve->contspawn_facingadd = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'S', 'E', 'N', 'D' ) ) peve->endsound_index = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'S', 'T', 'A', 'Y' ) ) peve->stayifnoowner = ( 0 != fget_int( fileread ) );
        else if ( idsz == MAKE_IDSZ( 'O', 'V', 'E', 'R' ) ) peve->spawn_overlay = ( 0 != fget_int( fileread ) );
        else if ( idsz == MAKE_IDSZ( 'D', 'E', 'A', 'D' ) ) peve->stayiftargetdead = ( 0 != fget_int( fileread ) );

        else if ( idsz == MAKE_IDSZ( 'C', 'K', 'U', 'R' ) ) peve->seekurse = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'D', 'A', 'R', 'K' ) ) peve->darkvision = fget_int( fileread );
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
    /// @details ZZ@> This function loads the enchantment associated with an object
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

    template_put_bool( filetemp, filewrite, peve->setyesno[SETCRUSHMODIFIER] );
    template_put_damage_modifier( filetemp, filewrite, peve->setvalue[SETCRUSHMODIFIER] );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETPOKEMODIFIER] );
    template_put_damage_modifier( filetemp, filewrite, peve->setvalue[SETPOKEMODIFIER] );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETHOLYMODIFIER] );
    template_put_damage_modifier( filetemp, filewrite, peve->setvalue[SETHOLYMODIFIER] );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETEVILMODIFIER] );
    template_put_damage_modifier( filetemp, filewrite, peve->setvalue[SETEVILMODIFIER] );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETFIREMODIFIER] );
    template_put_damage_modifier( filetemp, filewrite, peve->setvalue[SETFIREMODIFIER] );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETICEMODIFIER] );
    template_put_damage_modifier( filetemp, filewrite, peve->setvalue[SETICEMODIFIER] );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETZAPMODIFIER] );
    template_put_damage_modifier( filetemp, filewrite, peve->setvalue[SETZAPMODIFIER] );

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
    template_put_float( filetemp, filewrite, peve->addvalue[ADDBUMPDAMPEN] * 256.0f );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDBOUNCINESS] * 256.0f );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDDAMAGE] / 256.0f );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDSIZE] );
    template_put_int( filetemp, filewrite, peve->addvalue[ADDACCEL] * 80.0f );
    template_put_int( filetemp, filewrite, peve->addvalue[ADDRED] );
    template_put_int( filetemp, filewrite, peve->addvalue[ADDGRN] );
    template_put_int( filetemp, filewrite, peve->addvalue[ADDBLU] );
    template_put_int( filetemp, filewrite, -peve->addvalue[ADDDEFENSE] );  // Defense is backwards
    template_put_float( filetemp, filewrite, peve->addvalue[ADDMANA] / 256.0f );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDLIFE] / 256.0f );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDSTRENGTH] / 256.0f );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDWISDOM] / 256.0f );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDINTELLIGENCE] / 256.0f );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDDEXTERITY] / 256.0f );

    // copy the template file to the next free output section
    template_seek_free( filetemp, filewrite );

    if ( peve->contspawn_amount > 0 )
    {
        fput_expansion( filewrite, "", MAKE_IDSZ( 'A', 'M', 'O', 'U' ), peve->contspawn_amount );
    }

    if ( peve->contspawn_lpip > 0 )
    {
        fput_expansion( filewrite, "", MAKE_IDSZ( 'T', 'Y', 'P', 'E' ), peve->contspawn_lpip );
    }

    if ( peve->contspawn_facingadd > 0 )
    {
        fput_expansion( filewrite, "", MAKE_IDSZ( 'T', 'I', 'M', 'E' ), peve->contspawn_facingadd );
    }

    if ( peve->contspawn_delay > 0 )
    {
        fput_expansion( filewrite, "", MAKE_IDSZ( 'F', 'A', 'C', 'E' ), peve->contspawn_delay );
    }

    if ( INVALID_SOUND != peve->endsound_index )
    {
        fput_expansion( filewrite, "", MAKE_IDSZ( 'S', 'E', 'N', 'D' ), peve->endsound_index );
    }

    if ( peve->stayifnoowner )
    {
        fput_expansion( filewrite, "", MAKE_IDSZ( 'S', 'T', 'A', 'Y' ), 1 );
    }

    if ( peve->spawn_overlay )
    {
        fput_expansion( filewrite, "", MAKE_IDSZ( 'O', 'V', 'E', 'R' ), peve->spawn_overlay );
    }

    if ( peve->seekurse )
    {
        fput_expansion( filewrite, "", MAKE_IDSZ( 'C', 'K', 'U', 'R' ), peve->seekurse );
    }

    if ( peve->darkvision )
    {
        fput_expansion( filewrite, "", MAKE_IDSZ( 'D', 'A', 'R', 'K' ), peve->darkvision );
    }

    if ( peve->stayiftargetdead )
    {
        fput_expansion( filewrite, "", MAKE_IDSZ( 'D', 'E', 'A', 'D' ), 1 );
    }

    // dump the rest of the template file
    template_flush( filetemp, filewrite );

    // All done ( finally )
    vfs_close( filewrite );
    template_close_vfs( filetemp );

    return btrue;
}

