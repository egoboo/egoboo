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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - game.c
 */

#define DECLARE_GLOBALS

#include "egoboo.h"
#include "clock.h"
#include "link.h"
#include "ui.h"
#include "font.h"
#include "log.h"

#include "egoboo_endian.h"
#include "egoboo_setup.h"

#include "char.h"
#include "camera.h"
#include "id_md2.h"

#include <time.h>
#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float sinlut[MAXLIGHTROTATION];
float coslut[MAXLIGHTROTATION];

void memory_cleanUp(void);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int what_action( char cTmp )
{
    // ZZ> This function changes a letter into an action code
    int action;
    action = ACTIONDA;

    if ( cTmp == 'U' || cTmp == 'u' )  action = ACTIONUA;

    if ( cTmp == 'T' || cTmp == 't' )  action = ACTIONTA;

    if ( cTmp == 'S' || cTmp == 's' )  action = ACTIONSA;

    if ( cTmp == 'C' || cTmp == 'c' )  action = ACTIONCA;

    if ( cTmp == 'B' || cTmp == 'b' )  action = ACTIONBA;

    if ( cTmp == 'L' || cTmp == 'l' )  action = ACTIONLA;

    if ( cTmp == 'X' || cTmp == 'x' )  action = ACTIONXA;

    if ( cTmp == 'F' || cTmp == 'f' )  action = ACTIONFA;

    if ( cTmp == 'P' || cTmp == 'p' )  action = ACTIONPA;

    if ( cTmp == 'Z' || cTmp == 'z' )  action = ACTIONZA;

    return action;
}

//--------------------------------------------------------------------------------------------
// Random Things-----------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void make_newloadname( char *modname, char *appendname, char *newloadname )
{
    // ZZ> This function takes some names and puts 'em together
    int cnt, tnc;
    char ctmp;

    cnt = 0;
    ctmp = modname[cnt];

    while ( ctmp != 0 )
    {
        newloadname[cnt] = ctmp;
        cnt++;
        ctmp = modname[cnt];
    }

    tnc = 0;
    ctmp = appendname[tnc];

    while ( ctmp != 0 )
    {
        newloadname[cnt] = ctmp;
        cnt++;
        tnc++;
        ctmp = appendname[tnc];
    }

    newloadname[cnt] = 0;
}

//--------------------------------------------------------------------------------------------
char * get_file_path(char *character)
{
    //ZF> This turns a character name into a proper filepath for loading and saving files
    //    also turns all letter to lower case in case of case sensitive OS.
    Uint8 cnt = 0;
    char letter;
    static char pathname[16];

    letter = character[cnt];

    while ( cnt < 8 && letter != 0 )
    {
        letter = character[cnt];

        if ( letter >= 'A' && letter <= 'Z' )  letter -= 'A' - 'a';

        if ( letter != 0 )
        {
            if ( ( letter < 'a' || letter > 'z' ))  letter = '_';

            pathname[cnt] = letter;
            cnt++;
        }
    }

    pathname[cnt] = '.'; cnt++;
    pathname[cnt] = 'o'; cnt++;
    pathname[cnt] = 'b'; cnt++;
    pathname[cnt] = 'j'; cnt++;
    pathname[cnt] = 0;
    return pathname;
}

//--------------------------------------------------------------------------------------------
void export_one_character( int character, int owner, int number, bool_t is_local )
{
    // ZZ> This function exports a character
    int tnc = 0, profile;
    char fromdir[128];
    char todir[128];
    char fromfile[128];
    char tofile[128];
    char todirname[16];
    char todirfullname[64];

    // Don't export enchants
    disenchant_character( character );

    profile = chrmodel[character];

    if ( ( capcancarrytonextmodule[profile] || !capisitem[profile] ) && exportvalid )
    {
        // TWINK_BO.OBJ
        sprintf(todirname, "%s", get_file_path(chrname[owner]) );

        // Is it a character or an item?
        if ( owner != character )
        {
            // Item is a subdirectory of the owner directory...
            sprintf( todirfullname, "%s" SLASH_STR "%d.obj", todirname, number );
        }
        else
        {
            // Character directory
            sprintf( todirfullname, "%s", todirname );
        }

        // players/twink.obj or players/twink.obj/sword.obj
        if ( is_local )
        {
            sprintf( todir, "players" SLASH_STR "%s", todirfullname );
        }
        else
        {
            sprintf( todir, "remote" SLASH_STR "%s", todirfullname );
        }

        // modules/advent.mod/objects/advent.obj
        sprintf( fromdir, "%s", madname[profile] );

        // Delete all the old items
        if ( owner == character )
        {
            tnc = 0;

            while ( tnc < 8 )
            {
                sprintf( tofile, "%s" SLASH_STR "%d.obj", todir, tnc );  /*.OBJ*/
                fs_removeDirectoryAndContents( tofile );
                tnc++;
            }
        }

        // Make the directory
        fs_createDirectory( todir );

        // Build the DATA.TXT file
        sprintf( tofile, "%s" SLASH_STR "data.txt", todir );  /*DATA.TXT*/
        export_one_character_profile( tofile, character );

        // Build the SKIN.TXT file
        sprintf( tofile, "%s" SLASH_STR "skin.txt", todir );  /*SKIN.TXT*/
        export_one_character_skin( tofile, character );

        // Build the NAMING.TXT file
        sprintf( tofile, "%s" SLASH_STR "naming.txt", todir );  /*NAMING.TXT*/
        export_one_character_name( tofile, character );

        // Copy all of the misc. data files
        sprintf( fromfile, "%s" SLASH_STR "message.txt", fromdir );  /*MESSAGE.TXT*/
        sprintf( tofile, "%s" SLASH_STR "message.txt", todir );  /*MESSAGE.TXT*/
        fs_copyFile( fromfile, tofile );
        sprintf( fromfile, "%s" SLASH_STR "tris.md2", fromdir );  /*TRIS.MD2*/
        sprintf( tofile,   "%s" SLASH_STR "tris.md2", todir );  /*TRIS.MD2*/
        fs_copyFile( fromfile, tofile );
        sprintf( fromfile, "%s" SLASH_STR "copy.txt", fromdir );  /*COPY.TXT*/
        sprintf( tofile,   "%s" SLASH_STR "copy.txt", todir );  /*COPY.TXT*/
        fs_copyFile( fromfile, tofile );
        sprintf( fromfile, "%s" SLASH_STR "script.txt", fromdir );
        sprintf( tofile,   "%s" SLASH_STR "script.txt", todir );
        fs_copyFile( fromfile, tofile );
        sprintf( fromfile, "%s" SLASH_STR "enchant.txt", fromdir );
        sprintf( tofile,   "%s" SLASH_STR "enchant.txt", todir );
        fs_copyFile( fromfile, tofile );
        sprintf( fromfile, "%s" SLASH_STR "credits.txt", fromdir );
        sprintf( tofile,   "%s" SLASH_STR "credits.txt", todir );
        fs_copyFile( fromfile, tofile );
        sprintf( fromfile, "%s" SLASH_STR "quest.txt", fromdir );
        sprintf( tofile,   "%s" SLASH_STR "quest.txt", todir );
        fs_copyFile( fromfile, tofile );

        // Copy all of the particle files
        tnc = 0;

        while ( tnc < MAXPRTPIPPEROBJECT )
        {
            sprintf( fromfile, "%s" SLASH_STR "part%d.txt", fromdir, tnc );
            sprintf( tofile,   "%s" SLASH_STR "part%d.txt", todir,   tnc );
            fs_copyFile( fromfile, tofile );
            tnc++;
        }

        // Copy all of the sound files
        tnc = 0;

        while ( tnc < MAXWAVE )
        {
            sprintf( fromfile, "%s" SLASH_STR "sound%d.wav", fromdir, tnc );
            sprintf( tofile,   "%s" SLASH_STR "sound%d.wav", todir,   tnc );
            fs_copyFile( fromfile, tofile );
            sprintf( fromfile, "%s" SLASH_STR "sound%d.ogg", fromdir, tnc );
            sprintf( tofile,   "%s" SLASH_STR "sound%d.ogg", todir,   tnc );
            fs_copyFile( fromfile, tofile );
            tnc++;
        }

        // Copy all of the image files (try to copy all supported formats too)
        tnc = 0;

        while ( tnc < 4 )
        {
            Uint8 type = 0;

            while (type < maxformattypes)
            {
                sprintf( fromfile, "%s" SLASH_STR "tris%d%s", fromdir, tnc, TxFormatSupported[type] );
                sprintf( tofile,   "%s" SLASH_STR "tris%d%s", todir,   tnc, TxFormatSupported[type] );
                fs_copyFile( fromfile, tofile );
                sprintf( fromfile, "%s" SLASH_STR "icon%d%s", fromdir, tnc, TxFormatSupported[type] );
                sprintf( tofile,   "%s" SLASH_STR "icon%d%s", todir,   tnc, TxFormatSupported[type] );
                fs_copyFile( fromfile, tofile );
                type++;
            }

            tnc++;
        }
    }
}

//---------------------------------------------------------------------------------------------
void export_all_players( bool_t require_local )
{
    // ZZ> This function saves all the local players in the
    //     PLAYERS directory
    bool_t is_local;
    int cnt, character, item, number;

    // Check each player
    for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        is_local = ( 0 != pladevice[cnt] );

        if ( require_local && !is_local ) continue;

        if ( !plavalid[cnt] ) continue;

        // Is it alive?
        character = plaindex[cnt];

        if ( !chron[character] || !chralive[character] ) continue;

        // Export the character
        number = 0;
        export_one_character( character, character, number, is_local );

        // Export the left hand item
        number = 0;
        item = chrholdingwhich[character][number];

        if ( item != MAXCHR && chrisitem[item] )  export_one_character( item, character, number, is_local );

        // Export the right hand item
        number = 1;
        item = chrholdingwhich[character][number];

        if ( item != MAXCHR && chrisitem[item] )  export_one_character( item, character, number, is_local );

        // Export the inventory
        number = 2;
        item = chrnextinpack[character];

        while ( item != MAXCHR )
        {
            if ( chrisitem[item] )
            {
                export_one_character( item, character, number++, is_local );
            }

            item = chrnextinpack[item];
        }
    }

}

//---------------------------------------------------------------------------------------------
void export_all_local_players( void )
{
    // ZZ> This function saves all the local players in the
    //     PLAYERS directory

    // Check each player
    if ( exportvalid )
    {
        export_all_players( btrue );
    }
}

//---------------------------------------------------------------------------------------------
void quit_module( void )
{
    // ZZ> This function forces a return to the menu
    moduleactive = bfalse;
    hostactive = bfalse;

    export_all_local_players();
    empty_import_directory();  // Free up that disk space...

    gamepaused = bfalse;

    if ( soundvalid ) Mix_FadeOutChannel( -1, 500 );     // Stop all sounds that are playing
}

//--------------------------------------------------------------------------------------------
void quit_game( void )
{
    // ZZ> This function exits the game entirely

    if ( gameactive )
    {
        gameactive = bfalse;
    }

    if ( moduleactive )
    {
        quit_module();
    }

    if ( floatmemory != NULL )
    {
        free( floatmemory );
        floatmemory = NULL;
    }

    empty_import_directory();
}

//--------------------------------------------------------------------------------------------
void goto_colon( FILE* fileread )
{
    // ZZ> This function moves a file read pointer to the next colon
//    char cTmp;
    Uint32 ch = fgetc( fileread );

//    fscanf(fileread, "%c", &cTmp);
    while ( ch != ':' )
    {
        if ( ch == EOF )
        {
            // not enough colons in file!
            log_error( "There are not enough colons in file! (%s)\n", parse_filename );
        }

        ch = fgetc( fileread );
    }
}

//--------------------------------------------------------------------------------------------
Uint8 goto_colon_yesno( FILE* fileread )
{
    // ZZ> This function moves a file read pointer to the next colon, or it returns
    //     bfalse if there are no more
    char cTmp;

    do
    {
        if ( fscanf( fileread, "%c", &cTmp ) == EOF )
        {
            return bfalse;
        }
    }
    while ( cTmp != ':' );

    return btrue;
}

//--------------------------------------------------------------------------------------------
char get_first_letter( FILE* fileread )
{
    // ZZ> This function returns the next non-whitespace character
    char cTmp;
    fscanf( fileread, "%c", &cTmp );

    while ( isspace( cTmp ) )
    {
        fscanf( fileread, "%c", &cTmp );
    }

    return cTmp;
}

//--------------------------------------------------------------------------------------------
// Tag Reading---------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void reset_tags()
{
    // ZZ> This function resets the tags
    numscantag = 0;
}

//--------------------------------------------------------------------------------------------
int read_tag( FILE *fileread )
{
    // ZZ> This function finds the next tag, returning btrue if it found one
    if ( goto_colon_yesno( fileread ) )
    {
        if ( numscantag < MAXTAG )
        {
            fscanf( fileread, "%s%d", tagname[numscantag], &tagvalue[numscantag] );
            numscantag++;
            return btrue;
        }
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
void read_all_tags( char *szFilename )
{
    // ZZ> This function reads the scancode.txt file
    FILE* fileread;

    reset_tags();
    fileread = fopen( szFilename, "r" );

    if ( fileread )
    {
        while ( read_tag( fileread ) );

        fclose( fileread );
    }
}

//--------------------------------------------------------------------------------------------
int tag_value( char *string )
{
    // ZZ> This function matches the string with its tag, and returns the value...
    //     It will return 255 if there are no matches.
    int cnt;

    cnt = 0;

    while ( cnt < numscantag )
    {
        if ( strcmp( string, tagname[cnt] ) == 0 )
        {
            // They match
            return tagvalue[cnt];
        }

        cnt++;
    }

    // No matches
    return 255;
}

//--------------------------------------------------------------------------------------------
char* tag_to_string( Sint32 tag, bool_t onlykeys )
{
    int cnt;

    cnt = 0;

    while ( cnt < MAXTAG )
    {
        if ( tag == tagvalue[cnt])
        {
            // They match
            if (onlykeys)
            {
                if (tagname[cnt][0] == 'K') return tagname[cnt];
            }
            else
            {
                return tagname[cnt];
            }
        }

        cnt++;
    }

    // No matches
    return "N/A";
}

//--------------------------------------------------------------------------------------------
bool_t control_key_is_pressed( Uint8 control )
{
    // ZZ> This function returns btrue if the given control is pressed...

    bool_t retval = bfalse;

    if ( !netmessagemode && sdlkeybuffer )
    {
        retval = ( sdlkeybuffer[controlvalue[control]] != 0 );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t control_mouse_is_pressed( Uint8 control )
{
    // ZZ> This function returns btrue if the given control is pressed...

    bool_t retval = bfalse;

    if ( controliskey[control] )
    {
        if ( !netmessagemode && sdlkeybuffer )
        {
            retval = ( sdlkeybuffer[controlvalue[control]] != 0 );
        }
    }
    else
    {
        retval = ( msb == controlvalue[control] );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t control_joya_is_pressed( Uint8 control )
{
    // ZZ> This function returns btrue if the given control is pressed...

    bool_t retval = bfalse;

    if ( controliskey[control] )
    {
        if ( !netmessagemode && sdlkeybuffer )
        {
            retval = ( sdlkeybuffer[controlvalue[control]] != 0 );
        }
    }
    else
    {
        retval = ( jab == controlvalue[control] );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t control_joyb_is_pressed( Uint8 control )
{
    // ZZ> This function returns btrue if the given control is pressed...

    bool_t retval = bfalse;

    if ( controliskey[control] )
    {
        if ( !netmessagemode && sdlkeybuffer )
        {
            retval = ( 0 != sdlkeybuffer[controlvalue[control]] );
        }
    }
    else
    {
        retval = ( jbb == controlvalue[control] );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
char * undo_idsz( IDSZ idsz )
{
    // ZZ> This function takes an integer and makes an text IDSZ out of it.
    //     It will set valueidsz to "NONE" if the idsz is 0
    static char value_string[5] = {"NONE"};

    if ( idsz == IDSZNONE )
    {
        sprintf( valueidsz, "NONE" );
        snprintf( value_string, sizeof( value_string ), "NONE" );
    }
    else
    {
        valueidsz[0] = ( ( idsz >> 15 ) & 31 ) + 'A';
        valueidsz[1] = ( ( idsz >> 10 ) & 31 ) + 'A';
        valueidsz[2] = ( ( idsz >> 5 ) & 31 ) + 'A';
        valueidsz[3] = ( ( idsz ) & 31 ) + 'A';
        valueidsz[4] = 0;

        //Bad! both function return and return to global function!
        value_string[0] = (( idsz >> 15 ) & 31 ) + 'A';
        value_string[1] = (( idsz >> 10 ) & 31 ) + 'A';
        value_string[2] = (( idsz >> 5 ) & 31 ) + 'A';
        value_string[3] = (( idsz ) & 31 ) + 'A';
        value_string[4] = 0;
    }

    return value_string;
}

//--------------------------------------------------------------------------------------------
IDSZ get_idsz( FILE* fileread )
{
    // ZZ> This function reads and returns an IDSZ tag, or IDSZNONE if there wasn't one

    IDSZ idsz = IDSZNONE;
    char cTmp = get_first_letter( fileread );

    if ( cTmp == '[' )
    {
        fscanf( fileread, "%c", &cTmp );  cTmp = cTmp - 'A';  idsz = idsz | ( cTmp << 15 );
        fscanf( fileread, "%c", &cTmp );  cTmp = cTmp - 'A';  idsz = idsz | ( cTmp << 10 );
        fscanf( fileread, "%c", &cTmp );  cTmp = cTmp - 'A';  idsz = idsz | ( cTmp << 5 );
        fscanf( fileread, "%c", &cTmp );  cTmp = cTmp - 'A';  idsz = idsz | ( cTmp );
    }

    if ( idsz == IDSZ_NONE )
        idsz = IDSZNONE;

    return idsz;
}
//--------------------------------------------------------------------------------------------
int fget_int( FILE* fileread )
{
    int iTmp = 0;

    if ( feof( fileread ) ) return iTmp;

    fscanf( fileread, "%d", &iTmp );
    return iTmp;
}

//--------------------------------------------------------------------------------------------

bool_t fcopy_line(FILE * fileread, FILE * filewrite)
{
    /// @details BB@> copy a line of arbitrary length, in chunks of length
    ///      sizeof(linebuffer)
    /// @todo This should be moved to file_common.c

    char linebuffer[64];

    if (NULL == fileread || NULL == filewrite) return bfalse;

    if ( feof(fileread) || feof(filewrite) ) return bfalse;

    fgets(linebuffer, sizeof(linebuffer), fileread);
    fputs(linebuffer, filewrite);

    while ( strlen(linebuffer) == sizeof(linebuffer) )
    {
        fgets(linebuffer, sizeof(linebuffer), fileread);
        fputs(linebuffer, filewrite);
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
int get_free_message( void )
{
    // This function finds the best message to use
    // Pick the first one
    int tnc = msgstart;
    msgstart++;
    msgstart = msgstart % maxmessage;
    return tnc;
}

//--------------------------------------------------------------------------------------------
void getadd( int min, int value, int max, int* valuetoadd )
{
    // ZZ> This function figures out what value to add should be in order
    //     to not overflow the min and max bounds
    int newvalue;

    newvalue = value + ( *valuetoadd );

    if ( newvalue < min )
    {
        // Increase valuetoadd to fit
        *valuetoadd = min - value;

        if ( *valuetoadd > 0 )  *valuetoadd = 0;

        return;
    }

    if ( newvalue > max )
    {
        // Decrease valuetoadd to fit
        *valuetoadd = max - value;

        if ( *valuetoadd < 0 )  *valuetoadd = 0;
    }
}

//--------------------------------------------------------------------------------------------
void fgetadd( float min, float value, float max, float* valuetoadd )
{
    // ZZ> This function figures out what value to add should be in order
    //     to not overflow the min and max bounds
    float newvalue;

    newvalue = value + ( *valuetoadd );

    if ( newvalue < min )
    {
        // Increase valuetoadd to fit
        *valuetoadd = min - value;

        if ( *valuetoadd > 0 )  *valuetoadd = 0;

        return;
    }

    if ( newvalue > max )
    {
        // Decrease valuetoadd to fit
        *valuetoadd = max - value;

        if ( *valuetoadd < 0 )  *valuetoadd = 0;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void load_action_names( char* loadname )
{
    // ZZ> This function loads all of the 2 letter action names
    FILE* fileread;
    int cnt;
    char first, second;

    fileread = fopen( loadname, "r" );

    if ( fileread )
    {
        cnt = 0;

        while ( cnt < MAXACTION )
        {
            goto_colon( fileread );
            fscanf( fileread, "%c%c", &first, &second );
            cActionName[cnt][0] = first;
            cActionName[cnt][1] = second;
            cnt++;
        }

        fclose( fileread );
    }
}

//--------------------------------------------------------------------------------------------
void get_name( FILE* fileread, char *szName )
{
    // ZZ> This function loads a string of up to MAXCAPNAMESIZE characters, parsing
    //     it for underscores.  The szName argument is rewritten with the null terminated
    //     string
    int cnt;
    char cTmp;
    char szTmp[256];

    fscanf( fileread, "%s", szTmp );
    cnt = 0;

    while ( cnt < MAXCAPNAMESIZE - 1 )
    {
        cTmp = szTmp[cnt];

        if ( cTmp == '_' )  cTmp = ' ';

        szName[cnt] = cTmp;
        cnt++;
    }

    szName[cnt] = 0;
}

//--------------------------------------------------------------------------------------------
void log_madused( char *savename )
{
    // ZZ> This is a debug function for checking model loads
    FILE* hFileWrite;
    int cnt;

    hFileWrite = fopen( savename, "w" );

    if ( hFileWrite )
    {
        fprintf( hFileWrite, "Slot usage for objects in last module loaded...\n" );
        fprintf( hFileWrite, "%d of %d frames used...\n", madloadframe, MAXFRAME );
        cnt = 0;

        while ( cnt < MAXMODEL )
        {
            fprintf( hFileWrite, "%3d %32s %s\n", cnt, capclassname[cnt], madname[cnt] );
            cnt++;
        }

        fclose( hFileWrite );
    }
}

//---------------------------------------------------------------------------------------------
float light_for_normal( int rotation, int normal, float lx, float ly, float lz, float ambi )
{
    // ZZ> This function helps make_lighttable
    float fTmp;
    float nx, ny, nz;
    float sinrot, cosrot;

    nx = kMd2Normals[normal][0];
    ny = kMd2Normals[normal][1];
    nz = kMd2Normals[normal][2];
    sinrot = sinlut[rotation];
    cosrot = coslut[rotation];
    fTmp = cosrot * nx + sinrot * ny;
    ny = cosrot * ny - sinrot * nx;
    nx = fTmp;
    fTmp = nx * lx + ny * ly + nz * lz + ambi;

    if ( fTmp < ambi ) fTmp = ambi;

    return fTmp;
}

//---------------------------------------------------------------------------------------------
void make_lighttable( float lx, float ly, float lz, float ambi )
{
    // ZZ> This function makes a light table to fake directional lighting
    int lev, cnt, tnc;
    int itmp, itmptwo;

    // Build a lookup table for sin/cos
    for ( cnt = 0; cnt < MAXLIGHTROTATION; cnt++ )
    {
        sinlut[cnt] = SIN( TWO_PI * cnt / MAXLIGHTROTATION );
        coslut[cnt] = COS( TWO_PI * cnt / MAXLIGHTROTATION );
    }

    for ( cnt = 0; cnt < MD2LIGHTINDICES - 1; cnt++ )  // Spikey mace
    {
        for ( tnc = 0; tnc < MAXLIGHTROTATION; tnc++ )
        {
            lev = MAXLIGHTLEVEL - 1;
            itmp = ( 255 * light_for_normal( tnc,
                                             cnt,
                                             lx * lev / MAXLIGHTLEVEL,
                                             ly * lev / MAXLIGHTLEVEL,
                                             lz * lev / MAXLIGHTLEVEL,
                                             ambi ) );

            // This creates the light value for each level entry
            while ( lev >= 0 )
            {
                itmptwo = ( ( ( lev * itmp / ( MAXLIGHTLEVEL - 1 ) ) ) );

                if ( itmptwo > 255 )  itmptwo = 255;

                lighttable[lev][tnc][cnt] = ( Uint8 ) itmptwo;
                lev--;
            }
        }
    }

    // Fill in index number 162 for the spike mace
    for ( tnc = 0; tnc < MAXLIGHTROTATION; tnc++ )
    {
        lev = MAXLIGHTLEVEL - 1;
        itmp = 255;

        // This creates the light value for each level entry
        while ( lev >= 0 )
        {
            itmptwo = ( ( ( lev * itmp / ( MAXLIGHTLEVEL - 1 ) ) ) );

            if ( itmptwo > 255 )  itmptwo = 255;

            lighttable[lev][tnc][cnt] = ( Uint8 ) itmptwo;
            lev--;
        }
    }
}

//---------------------------------------------------------------------------------------------
int vertexconnected( int modelindex, int vertex )
{
    // ZZ> This function returns 1 if the model vertex is connected, 0 otherwise
    int cnt, tnc, entry;

    entry = 0;

    for ( cnt = 0; cnt < madcommands[modelindex]; cnt++ )
    {
        for ( tnc = 0; tnc < madcommandsize[modelindex][cnt]; tnc++ )
        {
            if ( madcommandvrt[modelindex][entry] == vertex )
            {
                // The vertex is used
                return 1;
            }

            entry++;
        }
    }

    // The vertex is not used
    return 0;
}

//---------------------------------------------------------------------------------------------
void get_madtransvertices( int modelindex )
{
    // ZZ> This function gets the number of vertices to transform for a model...
    //     That means every one except the grip ( unconnected ) vertices
    int cnt, trans = 0;

    //if (modelindex == 0)
    //{
    //    for ( cnt = 0; cnt < madvertices[modelindex]; cnt++ )
    //    {
    //        printf("%d-%d\n", cnt, vertexconnected( modelindex, cnt ) );
    //    }
    //}

    madtransvertices[modelindex] = madvertices[modelindex];
}

//---------------------------------------------------------------------------------------------
int rip_md2_header( void )
{
    // ZZ> This function makes sure an md2 is really an md2
    int iTmp;
    int* ipIntPointer;

    // Check the file type
    ipIntPointer = ( int* ) cLoadBuffer;

    iTmp = ENDIAN_INT32( ipIntPointer[0] );

    if ( iTmp != MD2START ) return bfalse;

    return btrue;
}

//---------------------------------------------------------------------------------------------
void fix_md2_normals( Uint16 modelindex )
{
    // ZZ> This function helps light not flicker so much
    int cnt, tnc;
    Uint16 indexofcurrent, indexofnext, indexofnextnext, indexofnextnextnext;
    Uint16 indexofnextnextnextnext;
    Uint32 frame;

    frame = madframestart[modelindex];
    cnt = 0;

    while ( cnt < madvertices[modelindex] )
    {
        tnc = 0;

        while ( tnc < madframes[modelindex] )
        {
            indexofcurrent = madvrta[frame][cnt];
            indexofnext = madvrta[frame+1][cnt];
            indexofnextnext = madvrta[frame+2][cnt];
            indexofnextnextnext = madvrta[frame+3][cnt];
            indexofnextnextnextnext = madvrta[frame+4][cnt];

            if ( indexofcurrent == indexofnextnext && indexofnext != indexofcurrent )
            {
                madvrta[frame+1][cnt] = indexofcurrent;
            }

            if ( indexofcurrent == indexofnextnextnext )
            {
                if ( indexofnext != indexofcurrent )
                {
                    madvrta[frame+1][cnt] = indexofcurrent;
                }

                if ( indexofnextnext != indexofcurrent )
                {
                    madvrta[frame+2][cnt] = indexofcurrent;
                }
            }

            if ( indexofcurrent == indexofnextnextnextnext )
            {
                if ( indexofnext != indexofcurrent )
                {
                    madvrta[frame+1][cnt] = indexofcurrent;
                }

                if ( indexofnextnext != indexofcurrent )
                {
                    madvrta[frame+2][cnt] = indexofcurrent;
                }

                if ( indexofnextnextnext != indexofcurrent )
                {
                    madvrta[frame+3][cnt] = indexofcurrent;
                }
            }

            tnc++;
        }

        cnt++;
    }
}

//---------------------------------------------------------------------------------------------
void rip_md2_commands( Uint16 modelindex )
{
    // ZZ> This function converts an md2's GL commands into our little command list thing
    int iTmp;
    float fTmpu, fTmpv;
    int iNumVertices;
    int tnc;
    bool_t command_error = bfalse, entry_error = bfalse;

    // char* cpCharPointer = (char*) cLoadBuffer;
    int* ipIntPointer = ( int* ) cLoadBuffer;
    float* fpFloatPointer = ( float* ) cLoadBuffer;

    // Number of GL commands in the MD2
    int iCommandWords = ENDIAN_INT32( ipIntPointer[9] );

    // Offset (in DWORDS) from the start of the file to the gl command list.
    int iCommandOffset = ENDIAN_INT32( ipIntPointer[15] ) >> 2;

    // Read in each command
    // iCommandWords is the number of dwords in the command list.
    // iCommandCount is the number of GL commands
    int iCommandCount = 0;
    int entry = 0;

    int cnt = 0;

    while ( cnt < iCommandWords )
    {
        iNumVertices = ENDIAN_INT32( ipIntPointer[iCommandOffset] );  iCommandOffset++;  cnt++;

        if ( iNumVertices != 0 )
        {
            Uint32 command_type;

            if ( iNumVertices < 0 )
            {
                // Fans start with a negative
                iNumVertices = -iNumVertices;
                command_type = GL_TRIANGLE_FAN;
            }
            else
            {
                // Strips start with a positive
                command_type = GL_TRIANGLE_STRIP;
            }

            command_error = (iCommandCount >= MAXCOMMAND);

            if (!command_error)
            {
                madcommandtype[modelindex][iCommandCount] = command_type;
                madcommandsize[modelindex][iCommandCount] = MIN(iNumVertices, MAXCOMMANDENTRIES);
            }

            // Read in vertices for each command
            tnc = 0;
            entry_error = bfalse;

            while ( tnc < iNumVertices )
            {
                fTmpu = ENDIAN_FLOAT( fpFloatPointer[iCommandOffset] );  iCommandOffset++;  cnt++;
                fTmpv = ENDIAN_FLOAT( fpFloatPointer[iCommandOffset] );  iCommandOffset++;  cnt++;
                iTmp  = ENDIAN_INT32( ipIntPointer[iCommandOffset]   );  iCommandOffset++;  cnt++;

                entry_error = entry >= MAXCOMMANDENTRIES;

                if (iTmp > MAXVERTICES)
                {
                    log_warning("rip_md2_commands() - vertex value %d beyond preallocated range %d", iTmp, MAXVERTICES);
                }

                if (!command_error && !entry_error)
                {
                    madcommandu[modelindex][entry] = fTmpu - ( 0.5f / 64 ); // GL doesn't align correctly
                    madcommandv[modelindex][entry] = fTmpv - ( 0.5f / 64 ); // with D3D
                    madcommandvrt[modelindex][entry] = iTmp;
                }

                entry++;
                tnc++;
            }

            if (entry_error)
            {
                log_warning("rip_md2_commands() - Number of OpenGL command %d, entries exceeds preset maximum: %d of %d\n", iCommandCount, iNumVertices, MAXCOMMANDENTRIES );
            }

            iCommandCount++;
        }
    }

    if (entry_error)
    {
        log_warning("rip_md2_commands() - Number of OpenGL commands exceeds preset maximum: %d of %d\n", iCommandCount, MAXCOMMAND );
    }

    madcommands[modelindex] = MIN(MAXCOMMAND, iCommandCount);
}

//---------------------------------------------------------------------------------------------
int rip_md2_frame_name( int frame )
{
    // ZZ> This function gets frame names from the load buffer, it returns
    //     btrue if the name in cFrameName[] is valid
    int iFrameOffset;
    int iNumVertices;
    int iNumFrames;
    int cnt;
    int* ipNamePointer;
    int* ipIntPointer;
    int foundname;

    // Jump to the Frames section of the md2 data
    ipNamePointer = ( int* ) cFrameName;
    ipIntPointer = ( int* ) cLoadBuffer;

    iNumVertices = ENDIAN_INT32( ipIntPointer[6] );
    iNumFrames   = ENDIAN_INT32( ipIntPointer[10] );
    iFrameOffset = ENDIAN_INT32( ipIntPointer[14] ) >> 2;

    // Chug through each frame
    foundname = bfalse;
    cnt = 0;

    while ( cnt < iNumFrames && !foundname )
    {
        iFrameOffset += 6;

        if ( cnt == frame )
        {
            ipNamePointer[0] = ipIntPointer[iFrameOffset]; iFrameOffset++;
            ipNamePointer[1] = ipIntPointer[iFrameOffset]; iFrameOffset++;
            ipNamePointer[2] = ipIntPointer[iFrameOffset]; iFrameOffset++;
            ipNamePointer[3] = ipIntPointer[iFrameOffset]; iFrameOffset++;
            foundname = btrue;
        }
        else
        {
            iFrameOffset += 4;
        }

        iFrameOffset += iNumVertices;
        cnt++;
    }

    cFrameName[15] = 0;  // Make sure it's null terminated
    return foundname;
}

//---------------------------------------------------------------------------------------------
void rip_md2_frames( Uint16 modelindex )
{
    // ZZ> This function gets frames from the load buffer and adds them to
    //     the indexed model
    Uint8 cTmpx, cTmpy, cTmpz;
    Uint8 cTmpa;
    float fRealx, fRealy, fRealz;
    float fScalex, fScaley, fScalez;
    float fTranslatex, fTranslatey, fTranslatez;
    int iFrameOffset;
    int iNumVertices;
    int iNumFrames;
    int cnt, tnc;
    char* cpCharPointer;
    int* ipIntPointer;
    float* fpFloatPointer;

    // Jump to the Frames section of the md2 data
    cpCharPointer  = ( char* ) cLoadBuffer;
    ipIntPointer   = ( int* ) cLoadBuffer;
    fpFloatPointer = ( float* ) cLoadBuffer;

    iNumVertices = ENDIAN_INT32( ipIntPointer[6] );
    iNumFrames   = ENDIAN_INT32( ipIntPointer[10] );
    iFrameOffset = ENDIAN_INT32( ipIntPointer[14] ) >> 2;

    // Read in each frame
    madframestart[modelindex] = madloadframe;
    madframes[modelindex] = iNumFrames;
    madvertices[modelindex] = iNumVertices;
    madscale[modelindex] = ( float ) ( 1.0f / 320.0f );  // Scale each vertex float to fit it in a short
    cnt = 0;

    while ( cnt < iNumFrames && madloadframe < MAXFRAME )
    {
        fScalex     = ENDIAN_FLOAT( fpFloatPointer[iFrameOffset] ); iFrameOffset++;
        fScaley     = ENDIAN_FLOAT( fpFloatPointer[iFrameOffset] ); iFrameOffset++;
        fScalez     = ENDIAN_FLOAT( fpFloatPointer[iFrameOffset] ); iFrameOffset++;
        fTranslatex = ENDIAN_FLOAT( fpFloatPointer[iFrameOffset] ); iFrameOffset++;
        fTranslatey = ENDIAN_FLOAT( fpFloatPointer[iFrameOffset] ); iFrameOffset++;
        fTranslatez = ENDIAN_FLOAT( fpFloatPointer[iFrameOffset] ); iFrameOffset++;

        iFrameOffset += 4;
        tnc = 0;

        while ( tnc < iNumVertices )
        {
            // This should work because it's reading a single character
            cTmpx = cpCharPointer[( iFrameOffset<<2 )+0];
            cTmpy = cpCharPointer[( iFrameOffset<<2 )+1];
            cTmpz = cpCharPointer[( iFrameOffset<<2 )+2];
            cTmpa = cpCharPointer[( iFrameOffset<<2 )+3];

            fRealx = ( cTmpx * fScalex ) + fTranslatex;
            fRealy = ( cTmpy * fScaley ) + fTranslatey;
            fRealz = ( cTmpz * fScalez ) + fTranslatez;

            madvrtx[madloadframe][tnc] = ( Sint16 ) (-fRealx * 256.0f );
            madvrty[madloadframe][tnc] = ( Sint16 ) ( fRealy * 256.0f );
            madvrtz[madloadframe][tnc] = ( Sint16 ) ( fRealz * 256.0f );
            madvrta[madloadframe][tnc] = cTmpa;

            iFrameOffset++;
            tnc++;
        }

        madloadframe++;
        cnt++;
    }
}

//---------------------------------------------------------------------------------------------
int load_one_md2( char* szLoadname, Uint16 modelindex )
{
    // ZZ> This function loads an id md2 file, storing the converted data in the indexed model
//    int iFileHandleRead;
    size_t iBytesRead = 0;
    int iReturnValue;

    // Read the input file
    FILE *file = fopen( szLoadname, "rb" );

    if ( !file )
    {
        log_warning( "Cannot load file! (%s)\n", szLoadname );
        return bfalse;
    }

    // Read up to MD2MAXLOADSIZE bytes from the file into the cLoadBuffer array.
    iBytesRead = fread( cLoadBuffer, 1, MD2MAXLOADSIZE, file );

    if ( iBytesRead == 0 )
        return bfalse;

    // Check the header
    // TODO: Verify that the header's filesize correspond to iBytesRead.
    iReturnValue = rip_md2_header();

    if ( !iReturnValue )
        return bfalse;

    // Get the frame vertices
    rip_md2_frames( modelindex );
    // Get the commands
    rip_md2_commands( modelindex );
    // Fix them normals
    fix_md2_normals( modelindex );
    // Figure out how many vertices to transform
    get_madtransvertices( modelindex );

    fclose( file );

    return btrue;
}

//--------------------------------------------------------------------------------------------
void make_enviro( void )
{
    // ZZ> This function sets up the environment mapping table
    int cnt;
    float z;
    float x, y;

    // Find the environment map positions
    for ( cnt = 0; cnt < MD2LIGHTINDICES; cnt++ )
    {
        x = kMd2Normals[cnt][0];
        y = kMd2Normals[cnt][1];
        x = ( ATAN2( y, x ) + PI ) / ( PI );
        x--;

        if ( x < 0 )
            x--;

        indextoenvirox[cnt] = x;
    }

    for ( cnt = 0; cnt < 256; cnt++ )
    {
        z = cnt / 256.0f;  // Z is between 0 and 1
        lighttoenviroy[cnt] = z;
    }
}

//--------------------------------------------------------------------------------------------
void add_stat( Uint16 character )
{
    // ZZ> This function adds a status display to the do list
    if ( numstat < MAXSTAT )
    {
        statlist[numstat] = character;
        chrstaton[character] = btrue;
        numstat++;
    }
}

//--------------------------------------------------------------------------------------------
void move_to_top( Uint16 character )
{
    // ZZ> This function puts the character on top of the statlist
    int cnt, oldloc;

    // Find where it is
    oldloc = numstat;

    for ( cnt = 0; cnt < numstat; cnt++ )
        if ( statlist[cnt] == character )
        {
            oldloc = cnt;
            cnt = numstat;
        }

    // Change position
    if ( oldloc < numstat )
    {
        // Move all the lower ones up
        while ( oldloc > 0 )
        {
            oldloc--;
            statlist[oldloc+1] = statlist[oldloc];
        }

        // Put the character in the top slot
        statlist[0] = character;
    }
}

//--------------------------------------------------------------------------------------------
void sort_stat()
{
    // ZZ> This function puts all of the local players on top of the statlist
    int cnt;

    for ( cnt = 0; cnt < numpla; cnt++ )
        if ( plavalid[cnt] && pladevice[cnt] != INPUTNONE )
        {
            move_to_top( plaindex[cnt] );
        }
}
//--------------------------------------------------------------------------------------------
void play_action( Uint16 character, Uint16 action, Uint8 actionready )
{
    // ZZ> This function starts a generic action for a character
    if ( madactionvalid[chrmodel[character]][action] )
    {
        chrnextaction[character] = ACTIONDA;
        chraction[character] = action;
        chrlip[character] = 0;
        chrlastframe[character] = chrframe[character];
        chrframe[character] = madactionstart[chrmodel[character]][chraction[character]];
        chractionready[character] = actionready;
    }
}

//--------------------------------------------------------------------------------------------
void set_frame( int character, int frame, Uint16 lip )
{
    // ZZ> This function sets the frame for a character explicitly...  This is used to
    //     rotate Tank turrets
    chrnextaction[character] = ACTIONDA;
    chraction[character] = ACTIONDA;
    chrlip[character] = ( lip << 6 );
    chrlastframe[character] = madactionstart[chrmodel[character]][ACTIONDA] + frame;
    chrframe[character] = madactionstart[chrmodel[character]][ACTIONDA] + frame + 1;
    chractionready[character] = btrue;
}

//--------------------------------------------------------------------------------------------
int generate_number( int numbase, int numrand )
{
    // ZZ> This function generates a random number
    int tmp = 0;

    tmp = numbase;

    if ( numrand > 0 )
    {
        tmp += ( rand() % numrand );
    }
    else
    {
        log_warning( "One of the data pairs is wrong! (%i and %i) Cannot be 0 or less.\n", numbase, numrand );
        numrand = numbase;
    }

    return tmp;
}

//--------------------------------------------------------------------------------------------
void setup_alliances( char *modname )
{
    // ZZ> This function reads the alliance file
    char newloadname[256];
    char szTemp[256];
    Uint8 teama, teamb;
    FILE *fileread;

    // Load the file
    make_newloadname( modname, "gamedat" SLASH_STR "alliance.txt", newloadname );
    fileread = fopen( newloadname, "r" );

    if ( fileread )
    {
        while ( goto_colon_yesno( fileread ) )
        {
            fscanf( fileread, "%s", szTemp );
            teama = ( szTemp[0] - 'A' ) % MAXTEAM;
            fscanf( fileread, "%s", szTemp );
            teamb = ( szTemp[0] - 'A' ) % MAXTEAM;
            teamhatesteam[teama][teamb] = bfalse;
        }

        fclose( fileread );
    }
}

//--------------------------------------------------------------------------------------------
void make_twist()
{
    // ZZ> This function precomputes surface normals and steep hill acceleration for
    //     the mesh
    int cnt;
    int x, y;
    float xslide, yslide;

    cnt = 0;

    while ( cnt < 256 )
    {
        y = cnt >> 4;
        x = cnt & 15;
        y = y - 7;  // -7 to 8
        x = x - 7;  // -7 to 8
        mapudtwist[cnt] = 32768 + y * SLOPE;
        maplrtwist[cnt] = 32768 + x * SLOPE;

        if ( ABS( y ) >= 7 ) y = y << 1;

        if ( ABS( x ) >= 7 ) x = x << 1;

        xslide = x * SLIDE;
        yslide = y * SLIDE;

        if ( xslide < 0 )
        {
            xslide += SLIDEFIX;

            if ( xslide > 0 )
                xslide = 0;
        }
        else
        {
            xslide -= SLIDEFIX;

            if ( xslide < 0 )
                xslide = 0;
        }

        if ( yslide < 0 )
        {
            yslide += SLIDEFIX;

            if ( yslide > 0 )
                yslide = 0;
        }
        else
        {
            yslide -= SLIDEFIX;

            if ( yslide < 0 )
                yslide = 0;
        }

        veludtwist[cnt] = -yslide * hillslide;
        vellrtwist[cnt] = xslide * hillslide;
        flattwist[cnt] = bfalse;

        if ( ABS( veludtwist[cnt] ) + ABS( vellrtwist[cnt] ) < SLIDEFIX*4 )
        {
            flattwist[cnt] = btrue;
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
int load_mesh( char *modname )
{
    // ZZ> This function loads the level.mpd file
    FILE* fileread;
    char newloadname[256];
    int itmp, cnt;
    float ftmp;
    int fan;
    int numvert, numfan;
    int x, y, vert;

    make_newloadname( modname, "gamedat" SLASH_STR "level.mpd", newloadname );
    fileread = fopen( newloadname, "rb" );

    if ( fileread )
    {

        fread( &itmp, 4, 1, fileread );  if ( MAPID != ( Uint32 )ENDIAN_INT32( itmp ) ) return bfalse;

        fread( &itmp, 4, 1, fileread );  numvert   = ( int )ENDIAN_INT32( itmp );
        fread( &itmp, 4, 1, fileread );  meshsizex = ( int )ENDIAN_INT32( itmp );
        fread( &itmp, 4, 1, fileread );  meshsizey = ( int )ENDIAN_INT32( itmp );

        numfan    = meshsizex * meshsizey;
        meshedgex = meshsizex * 128;
        meshedgey = meshsizey * 128;
        numfanblock = ( ( meshsizex >> 2 ) ) * ( ( meshsizey >> 2 ) );  // MESHSIZEX MUST BE MULTIPLE OF 4

        // Load fan data
        fan = 0;

        while ( fan < numfan )
        {
            fread( &itmp, 4, 1, fileread );
            meshtype[fan] = (ENDIAN_INT32( itmp ) >> 24) & 0xFF;
            meshfx[fan]   = (ENDIAN_INT32( itmp ) >> 16) & 0xFF;
            meshtile[fan] = (ENDIAN_INT32( itmp )      ) & 0xFFFF;
            fan++;
        }

        // Load fan data
        fan = 0;

        while ( fan < numfan )
        {
            fread( &itmp, 1, 1, fileread );
            meshtwist[fan] = ENDIAN_INT32( itmp );
            fan++;
        }

        // Load vertex x data
        cnt = 0;

        while ( cnt < numvert )
        {
            fread( &ftmp, 4, 1, fileread );
            meshvrtx[cnt] = ENDIAN_FLOAT( ftmp );
            cnt++;
        }

        // Load vertex y data
        cnt = 0;

        while ( cnt < numvert )
        {
            fread( &ftmp, 4, 1, fileread );
            meshvrty[cnt] = ENDIAN_FLOAT( ftmp );
            cnt++;
        }

        // Load vertex z data
        cnt = 0;

        while ( cnt < numvert )
        {
            fread( &ftmp, 4, 1, fileread );
            meshvrtz[cnt] = ENDIAN_FLOAT( ftmp ) / 16.0f;  // Cartman uses 4 bit fixed point for Z
            cnt++;
        }

        // Load vertex a data
        cnt = 0;

        while ( cnt < numvert )
        {
            fread( &itmp, 1, 1, fileread );
            meshvrta[cnt] = ENDIAN_INT32( itmp );
            meshvrtl[cnt] = 0;
            cnt++;
        }

        fclose( fileread );

        make_fanstart();

        vert = 0;
        y = 0;

        while ( y < meshsizey )
        {
            x = 0;

            while ( x < meshsizex )
            {
                fan = meshfanstart[y] + x;
                meshvrtstart[fan] = vert;
                vert += meshcommandnumvertices[meshtype[fan]];
                x++;
            }

            y++;
        }

        return btrue;
    }
    else log_warning( "Cannot find level.mpd!!\n" );

    return bfalse;
}

//--------------------------------------------------------------------------------------------
void update_game()
{
    // ZZ> This function does several iterations of character movements and such
    //     to keep the game in sync.
    int cnt, numdead;

    // Check for all local players being dead
    alllocalpladead = bfalse;
    localseeinvisible = bfalse;
    localseekurse = bfalse;

    cnt = 0;
    numdead = 0;

    while ( cnt < MAXPLAYER )
    {
        if ( plavalid[cnt] && pladevice[cnt] != INPUTNONE )
        {
            if ( !chralive[plaindex[cnt]] )
            {
                numdead++;

                if ( alllocalpladead && SDLKEYDOWN( SDLK_SPACE ) && respawnvalid && revivetimer == 0 )
                {
                    respawn_character( plaindex[cnt] );
                    chrexperience[cnt] *= EXPKEEP;  // Apply xp Penality
                }
            }
            else
            {
                if ( chrcanseeinvisible[plaindex[cnt]] )
                {
                    localseeinvisible = btrue;
                }

                if ( chrcanseekurse[plaindex[cnt]] )
                {
                    localseekurse = btrue;
                }
            }
        }

        cnt++;
    }

    if ( numdead >= numlocalpla )
    {
        alllocalpladead = btrue;
    }

    // This is the main game loop
    // [claforte Jan 6th 2001]
    // TODO: Put that back in place once networking is functional.
    while ( wldclock < allclock && numplatimes > 0 )
    {
        // Important stuff to keep in sync
        srand( randsave );
        sv_talkToRemotes();
        resize_characters();
        keep_weapons_with_holders();
        let_ai_think();
        do_weather_spawn();
        do_enchant_spawn();
        unbuffer_player_latches();
        move_characters();
        move_particles();
        make_character_matrices();
        attach_particles();
        make_onwhichfan();
        bump_characters();
        stat_return();
        update_pits();

        // Generate the new seed
        randsave += *( ( Uint32* ) & kMd2Normals[wldframe&127][0] );
        randsave += *( ( Uint32* ) & kMd2Normals[randsave&127][1] );

        // Stuff for which sync doesn't matter
        animate_tiles();
        move_water();

        // Timers
        wldclock  += UPDATE_SKIP;
        statclock += UPDATE_SKIP;

        //Reset the respawn timer
        if ( revivetimer > 0 )
        {
            revivetimer -= UPDATE_SKIP;
        }

        wldframe++;
    }

    if ( !rtscontrol )
    {
        if ( numplatimes == 0 )
        {
            // The remote ran out of messages, and is now twiddling its thumbs...
            // Make it go slower so it doesn't happen again
            wldclock += 25;
        }

        if ( numplatimes > 3 && !hostactive )
        {
            // The host has too many messages, and is probably experiencing control
            // lag...  Speed it up so it gets closer to sync
            wldclock -= 5;
        }
    }
}

//--------------------------------------------------------------------------------------------
void update_timers()
{
    // ZZ> This function updates the game timers
    lstclock = allclock;
    allclock = SDL_GetTicks() - sttclock;
    fpsclock += allclock - lstclock;

    if ( fpsclock >= TICKS_PER_SEC )
    {
        create_szfpstext( fpsframe );
        fpsclock = 0;
        fpsframe = 0;
    }
}

//--------------------------------------------------------------------------------------------
void read_pair( FILE* fileread )
{
    // ZZ> This function reads a damage/stat pair ( eg. 5-9 )
    char cTmp;
    float  fBase, fRand;

    fscanf( fileread, "%f", &fBase );  // The first number
    pairbase = fBase * 256;
    cTmp = get_first_letter( fileread );  // The hyphen

    if ( cTmp != '-' )
    {
        // Not in correct format, so fail
        pairrand = 1;
        return;
    }

    fscanf( fileread, "%f", &fRand );  // The second number
    pairrand = fRand * 256;
    pairrand = pairrand - pairbase;

    if ( pairrand < 1 )
        pairrand = 1;
}

//--------------------------------------------------------------------------------------------
void undo_pair( int base, int rand )
{
    // ZZ> This function generates a damage/stat pair ( eg. 3-6.5f )
    //     from the base and random values.  It set pairfrom and
    //     pairto
    pairfrom = base / 256.0f;
    pairto = rand / 256.0f;

    if ( pairfrom < 0.0f )
    {
        pairfrom = 0.0f;
        log_warning( "We got a randomization error again! (Base is less than 0)\n" );
    }

    if ( pairto < 0.0f )
    {
        pairto = 0.0f;
        log_warning( "We got a randomization error again! (Max is less than 0)\n" );
    }

    pairto += pairfrom;
}

//--------------------------------------------------------------------------------------------
void ftruthf( FILE* filewrite, char* text, Uint8 truth )
{
    // ZZ> This function kinda mimics fprintf for the output of
    //     btrue bfalse statements

    fprintf( filewrite, "%s", text );

    if ( truth )
    {
        fprintf( filewrite, "TRUE\n" );
    }
    else
    {
        fprintf( filewrite, "FALSE\n" );
    }
}

//--------------------------------------------------------------------------------------------
void fdamagf( FILE* filewrite, char* text, Uint8 damagetype )
{
    // ZZ> This function kinda mimics fprintf for the output of
    //     SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements
    fprintf( filewrite, "%s", text );

    if ( damagetype == DAMAGESLASH )
        fprintf( filewrite, "SLASH\n" );

    if ( damagetype == DAMAGECRUSH )
        fprintf( filewrite, "CRUSH\n" );

    if ( damagetype == DAMAGEPOKE )
        fprintf( filewrite, "POKE\n" );

    if ( damagetype == DAMAGEHOLY )
        fprintf( filewrite, "HOLY\n" );

    if ( damagetype == DAMAGEEVIL )
        fprintf( filewrite, "EVIL\n" );

    if ( damagetype == DAMAGEFIRE )
        fprintf( filewrite, "FIRE\n" );

    if ( damagetype == DAMAGEICE )
        fprintf( filewrite, "ICE\n" );

    if ( damagetype == DAMAGEZAP )
        fprintf( filewrite, "ZAP\n" );

    if ( damagetype == DAMAGENULL )
        fprintf( filewrite, "NONE\n" );
}

//--------------------------------------------------------------------------------------------
void factiof( FILE* filewrite, char* text, Uint8 action )
{
    // ZZ> This function kinda mimics fprintf for the output of
    //     SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements
    fprintf( filewrite, "%s", text );

    if ( action == ACTIONDA )
        fprintf( filewrite, "WALK\n" );

    if ( action == ACTIONUA )
        fprintf( filewrite, "UNARMED\n" );

    if ( action == ACTIONTA )
        fprintf( filewrite, "THRUST\n" );

    if ( action == ACTIONSA )
        fprintf( filewrite, "SLASH\n" );

    if ( action == ACTIONCA )
        fprintf( filewrite, "CHOP\n" );

    if ( action == ACTIONBA )
        fprintf( filewrite, "BASH\n" );

    if ( action == ACTIONLA )
        fprintf( filewrite, "LONGBOW\n" );

    if ( action == ACTIONXA )
        fprintf( filewrite, "XBOW\n" );

    if ( action == ACTIONFA )
        fprintf( filewrite, "FLING\n" );

    if ( action == ACTIONPA )
        fprintf( filewrite, "PARRY\n" );

    if ( action == ACTIONZA )
        fprintf( filewrite, "ZAP\n" );
}

//--------------------------------------------------------------------------------------------
void fgendef( FILE* filewrite, char* text, Uint8 gender )
{
    // ZZ> This function kinda mimics fprintf for the output of
    //     MALE FEMALE OTHER statements

    fprintf( filewrite, "%s", text );

    if ( gender == GENMALE )
        fprintf( filewrite, "MALE\n" );

    if ( gender == GENFEMALE )
        fprintf( filewrite, "FEMALE\n" );

    if ( gender == GENOTHER )
        fprintf( filewrite, "OTHER\n" );
}

//--------------------------------------------------------------------------------------------
void fpairof( FILE* filewrite, char* text, int base, int rand )
{
    // ZZ> This function mimics fprintf in spitting out
    //     damage/stat pairs
    undo_pair( base, rand );
    fprintf( filewrite, "%s", text );
    fprintf( filewrite, "%4.2f-%4.2f\n", pairfrom, pairto );
}

//--------------------------------------------------------------------------------------------
void funderf( FILE* filewrite, char* text, char* usename )
{
    // ZZ> This function mimics fprintf in spitting out
    //     a name with underscore spaces
    char cTmp;
    int cnt;

    fprintf( filewrite, "%s", text );
    cnt = 0;
    cTmp = usename[0];
    cnt++;

    while ( cTmp != 0 )
    {
        if ( cTmp == ' ' )
        {
            fprintf( filewrite, "_" );
        }
        else
        {
            fprintf( filewrite, "%c", cTmp );
        }

        cTmp = usename[cnt];
        cnt++;
    }

    fprintf( filewrite, "\n" );
}

//--------------------------------------------------------------------------------------------
void get_message( FILE* fileread )
{
    // ZZ> This function loads a string into the message buffer, making sure it
    //     is null terminated.
    int cnt;
    char cTmp;
    char szTmp[256];

    if ( msgtotal < MAXTOTALMESSAGE )
    {
        if ( msgtotalindex >= MESSAGEBUFFERSIZE )
        {
            msgtotalindex = MESSAGEBUFFERSIZE - 1;
        }

        msgindex[msgtotal] = msgtotalindex;
        fscanf( fileread, "%s", szTmp );
        szTmp[255] = 0;
        cTmp = szTmp[0];
        cnt = 1;

        while ( cTmp != 0 && msgtotalindex < MESSAGEBUFFERSIZE - 1 )
        {
            if ( cTmp == '_' )  cTmp = ' ';

            msgtext[msgtotalindex] = cTmp;
            msgtotalindex++;
            cTmp = szTmp[cnt];
            cnt++;
        }

        msgtext[msgtotalindex] = 0;  msgtotalindex++;
        msgtotal++;
    }
}

//--------------------------------------------------------------------------------------------
void load_all_messages( char *loadname, int object )
{
    // ZZ> This function loads all of an objects messages
    FILE *fileread;

    madmsgstart[object] = 0;
    fileread = fopen( loadname, "r" );

    if ( fileread )
    {
        madmsgstart[object] = msgtotal;

        while ( goto_colon_yesno( fileread ) )
        {
            get_message( fileread );
        }

        fclose( fileread );
    }
}

//--------------------------------------------------------------------------------------------
void reset_teams()
{
    // ZZ> This function makes everyone hate everyone else
    int teama, teamb;

    teama = 0;

    while ( teama < MAXTEAM )
    {
        // Make the team hate everyone
        teamb = 0;

        while ( teamb < MAXTEAM )
        {
            teamhatesteam[teama][teamb] = btrue;
            teamb++;
        }

        // Make the team like itself
        teamhatesteam[teama][teama] = bfalse;
        // Set defaults
        teamleader[teama] = NOLEADER;
        teamsissy[teama] = 0;
        teammorale[teama] = 0;
        teama++;
    }

    // Keep the null team neutral
    teama = 0;

    while ( teama < MAXTEAM )
    {
        teamhatesteam[teama][NULLTEAM] = bfalse;
        teamhatesteam[NULLTEAM][teama] = bfalse;
        teama++;
    }
}

//--------------------------------------------------------------------------------------------
void reset_messages()
{
    // ZZ> This makes messages safe to use
    int cnt;

    msgtotal = 0;
    msgtotalindex = 0;
    msgtimechange = 0;
    msgstart = 0;
    cnt = 0;

    while ( cnt < MAXMESSAGE )
    {
        msgtime[cnt] = 0;
        cnt++;
    }

    cnt = 0;

    while ( cnt < MAXTOTALMESSAGE )
    {
        msgindex[cnt] = 0;
        cnt++;
    }

    msgtext[0] = 0;
}

//--------------------------------------------------------------------------------------------
void make_randie()
{
    // ZZ> This function makes the random number table
    int tnc, cnt;

    // Fill in the basic values
    cnt = 0;

    while ( cnt < MAXRAND )
    {
        randie[cnt] = rand() << 1;
        cnt++;
    }

    // Keep adjusting those values
    tnc = 0;

    while ( tnc < 20 )
    {
        cnt = 0;

        while ( cnt < MAXRAND )
        {
            randie[cnt] += rand();
            cnt++;
        }

        tnc++;
    }

    // All done
    randindex = 0;
}

//--------------------------------------------------------------------------------------------
void reset_timers()
{
    // ZZ> This function resets the timers...
    sttclock = SDL_GetTicks();
    allclock = 0;
    lstclock = 0;
    wldclock = 0;
    statclock = 0;
    pitclock = 0;  pitskill = pitsfall = bfalse;
    wldframe = 0;
    allframe = 0;
    fpsframe = 0;
    outofsync = bfalse;
}

void menu_loadInitialMenu();

extern int doMenu( float deltaTime );
extern int initMenus();

//--------------------------------------------------------------------------------------------
int SDL_main( int argc, char **argv )
{
    // ZZ> This is where the program starts and all the high level stuff happens
    glVector t1 = {0, 0, 0};
    glVector t2 = {0, 0, -1};
    glVector t3 = {0, 1, 0};
    double frameDuration;
    int menuActive = 1;
    int menuResult;
    int frame_next = 0, frame_now = 0;

    // Initialize logging first, so that we can use it everywhere.
    log_init();
    log_setLoggingLevel( 2 );

    // start initializing the various subsystems
    log_message( "Starting Egoboo %s...\n", VERSION );

    sys_initialize();
    clock_init();
    fs_init();
    GLSetup_SupportedFormats();

    // read the "setup.txt" file
    if ( !setup_read( "setup.txt" ) )
    {
        log_error( "Could not find Setup.txt\n" );
    }

    // download the "setup.txt" values into game variables
    setup_download();

    read_all_tags( "basicdat" SLASH_STR "scancode.txt" );
    input_settings_load( "controls.txt" );
    reset_ai_script();
    load_ai_codes( "basicdat" SLASH_STR "aicodes.txt" );
    load_action_names( "basicdat" SLASH_STR "actions.txt" );

    sdlinit( argc, argv );
    glinit( argc, argv );
    net_initialize();

    ui_initialize( "basicdat" SLASH_STR "Negatori.ttf", 24 );
    sdlmixer_initialize();

    // register the memory_cleanUp function to automatically run whenever the program exits
    atexit( memory_cleanUp );

    // Linking system
    log_info( "Initializing module linking... " );
    empty_import_directory();

    if ( link_build( "basicdat" SLASH_STR "link.txt", LinkList ) ) log_message( "Success!\n" );
    else log_message( "Failed!\n" );

    if ( !get_mesh_memory() )
    {
        log_error( "Reduce the maximum number of vertices!!!  See SETUP.TXT\n" );
        return bfalse;
    }

    // Matrix init stuff (from remove.c)
    rotmeshtopside = ( ( float )scrx / scry ) * ROTMESHTOPSIDE / ( 1.33333f );
    rotmeshbottomside = ( ( float )scrx / scry ) * ROTMESHBOTTOMSIDE / ( 1.33333f );
    rotmeshup = ( ( float )scrx / scry ) * ROTMESHUP / ( 1.33333f );
    rotmeshdown = ( ( float )scrx / scry ) * ROTMESHDOWN / ( 1.33333f );
    mWorld = IdentityMatrix();
    mViewSave = ViewMatrix( t1, t2, t3, 0 );
    mProjection = ProjectionMatrix( .001f, 2000.0f, ( float )( FOV * PI / 180 ) ); // 60 degree FOV
    mProjection = MatrixMult( Translate( 0, 0, -0.999996f ), mProjection ); // Fix Z value...
    mProjection = MatrixMult( ScaleXYZ( -1, -1, 100000 ), mProjection );  // HUK // ...'cause it needs it

    //[claforte] Fudge the values.
    mProjection.v[10] /= 2.0f;
    mProjection.v[11] /= 2.0f;

    // Load stuff into memory
    prime_icons();
    prime_titleimage();
    make_textureoffset();  // THIS SHOULD WORK
    make_lightdirectionlookup(); // THIS SHOULD WORK
    make_turntosin();  // THIS SHOULD WORK
    make_enviro(); // THIS SHOULD WORK
    load_mesh_fans(); // THIS SHOULD WORK
    load_blip_bitmap();
    load_all_music_sounds();
    initMenus();        // Start the game menu

    // Let the normal OS mouse cursor work
    SDL_WM_GrabInput( SDL_GRAB_OFF );
    SDL_ShowCursor( btrue );

    // Network's temporarily disabled
    clock_frameStep();
    frameDuration = clock_getFrameDuration();
    gameactive = btrue;

    while ( gameactive || menuActive )
    {
        // Clock updates each frame
        clock_frameStep();
        frameDuration = clock_getFrameDuration();

        // Either run through the menus, or jump into the game
        if ( menuActive )
        {
            // Play the menu music
            play_music( 0, 0, -1 );

            // do menus
            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
            read_input();

            // Pressed panic button
            if ( SDLKEYDOWN( SDLK_q ) && SDLKEYDOWN( SDLK_LCTRL ) )
            {
                menuActive = 0;
                gameactive = bfalse;
            }

            ui_beginFrame( frameDuration );

            menuResult = doMenu( ( float )frameDuration );

            switch ( menuResult )
            {
                case 1:
                    // Go ahead and start the game
                    menuActive = 0;
                    gameactive = btrue;
                    networkon = bfalse;
                    hostactive = btrue;

                    if ( gGrabMouse )
                    {
                        SDL_WM_GrabInput ( SDL_GRAB_ON );
                    }

                    if ( gHideMouse )
                    {
                        SDL_ShowCursor( 0 );  // Hide the mouse cursor
                    }
                    break;

                case - 1:
                    // The user selected "Quit"
                    menuActive = 0;
                    gameactive = bfalse;
                    break;
            }

            ui_endFrame();

            SDL_GL_SwapBuffers();
        }
        else
        {
            // Do the game
            // Did we get through all the menus?
            if ( gameactive )
            {
                // Start a new module
                seed = time( NULL );
                srand( seed );

                load_module( pickedmodule );  // :TODO: Seems to be the next part to fix

                pressed = bfalse;
                make_onwhichfan();
                reset_camera();
                reset_timers();
                figure_out_what_to_draw();
                make_character_matrices();
                attach_particles();

                if ( networkon )
                {
                    log_info( "SDL_main: Loading module %s...\n", pickedmodule );
                    netmessagemode = bfalse;
                    netmessagedelay = 20;
                    net_sayHello();
                }

                // Let the game go
                moduleactive = btrue;
                randsave = 0;
                srand( 0 );

                while ( moduleactive )
                {
                    // This is the control loop
                    read_input();
                    // input_net_message();

                    //Check for screenshots
                    if ( !SDLKEYDOWN( SDLK_F11 ) ) screenshotkeyready = btrue;

                    if ( SDLKEYDOWN( SDLK_F11 ) && keyon && screenshotkeyready )
                    {
                        if ( !dump_screenshot() )                // Take the shot, returns bfalse if failed
                        {
                            debug_message( "Error writing screenshot!" );
                            log_warning( "Error writing screenshot\n" );    // Log the error in log.txt
                        }

                        screenshotkeyready = bfalse;
                    }

                    // Check for pause key    // TODO: What to do in network games?
                    if ( !SDLKEYDOWN( SDLK_F8 ) ) pausekeyready = btrue;

                    if ( SDLKEYDOWN( SDLK_F8 ) && keyon && pausekeyready )
                    {
                        pausekeyready = bfalse;

                        if ( gamepaused ) gamepaused = bfalse;
                        else gamepaused = btrue;
                    }

                    // Do important things
                    if ( !gamepaused || networkon )
                    {
                        check_stats();
                        set_local_latches();
                        update_timers();
                        check_passage_music();

                        // NETWORK PORT
                        listen_for_packets();

                        if ( !waitingforplayers )
                        {
                            cl_talkToHost();
                            update_game();
                        }
                        else
                        {

                            wldclock = allclock;
                        }
                    }
                    else
                    {
                        update_timers();
                        wldclock = allclock;
                    }

                    // Do the display stuff
                    frame_now = SDL_GetTicks();

                    if (frame_now > frame_next)
                    {
                        float  frameskip = (float)TICKS_PER_SEC / (float)framelimit;
                        frame_next = frame_now + frameskip; //FPS limit

                        move_camera();
                        figure_out_what_to_draw();
                        // printf("DIAG: doing draw_main\n");
                        draw_main();

                        msgtimechange++;

                        if ( statdelay > 0 )  statdelay--;
                    }

                    // Check for quitters
                    // :TODO: nolocalplayers is not set correctly
                    if ( SDLKEYDOWN( SDLK_ESCAPE ) /*|| nolocalplayers*/ )
                    {
                        quit_module();
                        gameactive = bfalse;
                        menuActive = 1;

                        // Let the normal OS mouse cursor work
                        SDL_WM_GrabInput( SDL_GRAB_OFF );
                        SDL_ShowCursor( 1 );
                    }
                }

                release_module();
                close_session();
            }
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void memory_cleanUp(void)
{
    //ZF> This function releases all loaded things in memory and cleans up everything properly

    log_info("memory_cleanUp() - Attempting to clean up loaded things in memory... ");

    // quit any existing game
    quit_game();

    // make sure that the setup file is written
    setup_upload();
    setup_write();
    setup_quit();

    // make sure that the current control configuration is written
    input_settings_save( "controls.txt" );

    // shut down the ui
    ui_shutdown();

    // shut down all game systems
    if (mixeron) Mix_CloseAudio();

    if (networkon) net_shutDown();

    clock_shutdown();

    log_message("Succeeded!\n");
    log_info( "Exiting Egoboo %s the good way...\n", VERSION );
    log_shutdown();
}
