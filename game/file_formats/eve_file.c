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

/* Egoboo - eve_file.c
 */

#include "eve_file.h"

#include "template.h"
#include "sound.h"

#include "egoboo_fileutil.h"
#include "egoboo_vfs.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
eve_t * eve_init( eve_t * peve )
{
    if( NULL == peve ) return peve;

    memset( peve, 0, sizeof(eve_t) );

    peve->endsoundindex = INVALID_SOUND;

    return peve;
}

//--------------------------------------------------------------------------------------------
eve_t * load_one_enchant_file( const char* szLoadName, eve_t * peve )
{
    // ZZ> This function loads the enchantment associated with an object
    vfs_FILE* fileread;
    char cTmp;
    IDSZ idsz;

    if( NULL == peve ) return NULL;

    eve_init( peve );

    fileread = vfs_openRead( szLoadName );
    if ( NULL == fileread ) return NULL;

    // btrue/bfalse values
    peve->retarget = fget_next_bool( fileread );
    peve->override = fget_next_bool( fileread );
    peve->removeoverridden = fget_next_bool( fileread );
    peve->killonend = fget_next_bool( fileread );

    peve->poofonend = fget_next_bool( fileread );

    // More stuff
    peve->time = fget_next_int( fileread );
    peve->endmessage = fget_next_int( fileread );

    // Drain stuff
    peve->ownermana    = fget_next_float( fileread ) * 256;
    peve->targetmana   = fget_next_float( fileread ) * 256;
    peve->endifcantpay = fget_next_bool( fileread );
    peve->ownerlife    = fget_next_float( fileread ) * 256;
    peve->targetlife   = fget_next_float( fileread ) * 256;

    // Specifics
    peve->dontdamagetype = fget_next_damage_type( fileread );
    peve->onlydamagetype = fget_next_damage_type( fileread );
    peve->removedbyidsz  = fget_next_idsz( fileread );

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
         if ( 'R' == toupper(cTmp) )  peve->setvalue[SETMISSILETREATMENT] = MISSILE_REFLECT;
    else if ( 'D' == toupper(cTmp) )  peve->setvalue[SETMISSILETREATMENT] = MISSILE_DEFLECT;
    else                              peve->setvalue[SETMISSILETREATMENT] = MISSILE_NORMAL;

    peve->setyesno[SETCOSTFOREACHMISSILE] = fget_next_bool( fileread );
    peve->setvalue[SETCOSTFOREACHMISSILE] = (Uint8) fget_float( fileread ) * 16;

    peve->setyesno[SETMORPH] = fget_next_bool( fileread );
    peve->setvalue[SETMORPH] = btrue;

    peve->setyesno[SETCHANNEL] = fget_next_bool( fileread );
    peve->setvalue[SETCHANNEL] = btrue;

    // Now read in the add values
    peve->addvalue[ADDJUMPPOWER]    = (Sint32) fget_next_float( fileread ) * 16;
    peve->addvalue[ADDBUMPDAMPEN]   = (Sint32) fget_next_float( fileread ) * 127;
    peve->addvalue[ADDBOUNCINESS]   = (Sint32) fget_next_float( fileread ) * 127;
    peve->addvalue[ADDDAMAGE]       = (Sint32) fget_next_float( fileread ) * 4;
    peve->addvalue[ADDSIZE]         = (Sint32) fget_next_float( fileread ) * 127;
    peve->addvalue[ADDACCEL]        = fget_next_int( fileread );
    peve->addvalue[ADDRED]          = fget_next_int( fileread );
    peve->addvalue[ADDGRN]          = fget_next_int( fileread );
    peve->addvalue[ADDBLU]          = fget_next_int( fileread );
    peve->addvalue[ADDDEFENSE]      = -fget_next_int( fileread );  // Defense is backwards
    peve->addvalue[ADDMANA]         = (Sint32) fget_next_float( fileread ) * 4;
    peve->addvalue[ADDLIFE]         = (Sint32) fget_next_float( fileread ) * 4;
    peve->addvalue[ADDSTRENGTH]     = (Sint32) fget_next_float( fileread ) * 4;
    peve->addvalue[ADDWISDOM]       = (Sint32) fget_next_float( fileread ) * 4;
    peve->addvalue[ADDINTELLIGENCE] = (Sint32) fget_next_float( fileread ) * 4;
    peve->addvalue[ADDDEXTERITY]    = (Sint32) fget_next_float( fileread ) * 4;

    // Read expansions
    while ( goto_colon( NULL, fileread, btrue ) )
    {
        idsz = fget_idsz( fileread );

        if ( idsz == MAKE_IDSZ( 'A', 'M', 'O', 'U' ) )  peve->contspawn_amount = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'T', 'Y', 'P', 'E' ) )  peve->contspawn_pip = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'T', 'I', 'M', 'E' ) )  peve->contspawn_time = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'F', 'A', 'C', 'E' ) )  peve->contspawn_facingadd = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'S', 'E', 'N', 'D' ) )
        {
            // This is wrong, it gets stored or loaded incorrectly (Loaded in game.c)
            int itmp = fget_int( fileread );
            peve->endsoundindex = CLIP(itmp, INVALID_SOUND, MAX_WAVE);
        }
        else if ( idsz == MAKE_IDSZ( 'S', 'T', 'A', 'Y' ) ) peve->stayifnoowner = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'O', 'V', 'E', 'R' ) ) peve->spawn_overlay = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'C', 'K', 'U', 'R' ) ) peve->seekurse = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'D', 'E', 'A', 'D' ) ) peve->stayifdead = fget_int( fileread );
    }

    // All done ( finally )
    vfs_close( fileread );

    strncpy( peve->name, szLoadName, SDL_arraysize(peve->name) );
    peve->loaded = btrue;

    return peve;
}

//--------------------------------------------------------------------------------------------
bool_t save_one_enchant_file( const char* szLoadName, eve_t * peve )
{
    // ZZ> This function loads the enchantment associated with an object
    vfs_FILE* filewrite, * filetemp;

    if( NULL == peve ) return bfalse;

    filewrite = vfs_openWrite( szLoadName );
    if ( NULL == filewrite ) return bfalse;

    filetemp = template_open( "/basicdat/templates/enchant.txt" );

    // btrue/bfalse values
    template_put_bool( filetemp, filewrite, peve->retarget );
    template_put_bool( filetemp, filewrite, peve->override );
    template_put_bool( filetemp, filewrite, peve->removeoverridden );
    template_put_bool( filetemp, filewrite, peve->killonend );

    template_put_bool( filetemp, filewrite, peve->poofonend );

    // More stuff
    template_put_int( filetemp, filewrite, peve->time );
    template_put_int( filetemp, filewrite, peve->endmessage );

    // Drain stuff
    template_put_float( filetemp, filewrite, FP8_TO_FLOAT(peve->ownermana) );
    template_put_float( filetemp, filewrite, FP8_TO_FLOAT(peve->targetmana) );
    template_put_bool ( filetemp, filewrite, peve->endifcantpay );
    template_put_float( filetemp, filewrite, FP8_TO_FLOAT(peve->ownerlife) );
    template_put_float( filetemp, filewrite, FP8_TO_FLOAT(peve->targetlife) );

    // Specifics
    template_put_damage_type( filetemp, filewrite, peve->dontdamagetype );
    template_put_damage_type( filetemp, filewrite, peve->onlydamagetype );
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
    template_put_bool( filetemp, filewrite, peve->setvalue[SETWALKONWATER] );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETCANSEEINVISIBLE] );
    template_put_bool( filetemp, filewrite, peve->setvalue[SETCANSEEINVISIBLE] );

    template_put_bool( filetemp, filewrite, peve->setyesno[SETMISSILETREATMENT] );

    switch( peve->setvalue[SETMISSILETREATMENT] )
    {
        case MISSILE_NORMAL : template_put_char( filetemp, filewrite, 'N' ); break;
        case MISSILE_REFLECT: template_put_char( filetemp, filewrite, 'R' ); break;
        case MISSILE_DEFLECT: template_put_char( filetemp, filewrite, 'D' ); break;
    }

    template_put_bool( filetemp, filewrite, peve->setyesno[SETCOSTFOREACHMISSILE] );
    template_put_float( filetemp, filewrite, peve->setvalue[SETCOSTFOREACHMISSILE] / 16.0f );
    template_put_bool( filetemp, filewrite, peve->setyesno[SETMORPH] );
    template_put_bool( filetemp, filewrite, peve->setyesno[SETCHANNEL] );

    // Now read in the add values
    template_put_float( filetemp, filewrite, peve->addvalue[ADDJUMPPOWER]  /  16.0f );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDBUMPDAMPEN] / 127.0f );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDBOUNCINESS] / 127.0f );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDDAMAGE]     /   4.0f );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDSIZE]       / 127.0f );
    template_put_int( filetemp, filewrite, peve->addvalue[ADDACCEL] );
    template_put_int( filetemp, filewrite, peve->addvalue[ADDRED] );
    template_put_int( filetemp, filewrite, peve->addvalue[ADDGRN] );
    template_put_int( filetemp, filewrite, peve->addvalue[ADDBLU] );
    template_put_int( filetemp, filewrite, -peve->addvalue[ADDDEFENSE] );  // Defense is backwards
    template_put_float( filetemp, filewrite, peve->addvalue[ADDMANA]         / 4.0f );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDLIFE]         / 4.0f );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDSTRENGTH]     / 4.0f );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDWISDOM]       / 4.0f );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDINTELLIGENCE] / 4.0f );
    template_put_float( filetemp, filewrite, peve->addvalue[ADDDEXTERITY]    / 4.0f );

    // copy the template file to the next free output section
    template_seek_free( filetemp, filewrite );

    if( peve->contspawn_amount > 0 )
    {
        fput_expansion( filewrite, "", MAKE_IDSZ( 'A', 'M', 'O', 'U' ), peve->contspawn_amount );
    }

    if( peve->contspawn_pip > 0 )
    {
        fput_expansion( filewrite, "", MAKE_IDSZ( 'T', 'Y', 'P', 'E' ), peve->contspawn_pip );
    }

    if( peve->contspawn_facingadd > 0 )
    {
        fput_expansion( filewrite, "", MAKE_IDSZ( 'T', 'I', 'M', 'E' ), peve->contspawn_facingadd );
    }

    if( peve->contspawn_time > 0 )
    {
        fput_expansion( filewrite, "", MAKE_IDSZ( 'F', 'A', 'C', 'E' ), peve->contspawn_time );
    }

    if( peve->endsoundindex != INVALID_SOUND )
    {
        fput_expansion( filewrite, "", MAKE_IDSZ( 'S', 'E', 'N', 'D' ), peve->endsoundindex );
    }

    if( peve->stayifnoowner )
    {
        fput_expansion( filewrite, "", MAKE_IDSZ( 'S', 'T', 'A', 'Y' ), 1 );
    }

    if( peve->spawn_overlay )
    {
        fput_expansion( filewrite, "", MAKE_IDSZ( 'O', 'V', 'E', 'R' ), peve->spawn_overlay );
    }

    if( peve->seekurse )
    {
        fput_expansion( filewrite, "", MAKE_IDSZ( 'C', 'K', 'U', 'R' ), 1 );
    }

    if( peve->stayifdead )
    {
        fput_expansion( filewrite, "", MAKE_IDSZ( 'D', 'E', 'A', 'D' ), 1 );
    }

    // dump the rest of the template file
    template_flush( filetemp, filewrite );

    // All done ( finally )
    vfs_close( filewrite );
    template_close( filetemp );

    return btrue;
}

