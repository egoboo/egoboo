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

/* Egoboo - graphic.c
 * All sorts of stuff related to drawing the game, and all sorts of other stuff
 * (such as data loading) that really should not be in here.
 */

#include "egoboo.h"
#include "graphic.h"
#include "log.h"
#include "script.h"

// Defined in egoboo.h
SDL_Surface *displaySurface = NULL;
bool_t  gTextureOn = bfalse;

void EnableTexturing()
{
    if ( !gTextureOn )
    {
        glEnable( GL_TEXTURE_2D );
        gTextureOn = btrue;
    }
}

void DisableTexturing()
{
    if ( gTextureOn )
    {
        glDisable( GL_TEXTURE_2D );
        gTextureOn = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
void reset_character_alpha( Uint16 character )
{
    // ZZ> This function fixes an item's transparency
    Uint16 enchant, mount;

    if ( character != MAXCHR )
    {
        mount = chrattachedto[character];

        if ( chron[character] && mount != MAXCHR && chrisitem[character] && chrtransferblend[mount] )
        {
            // Okay, reset transparency
            enchant = chrfirstenchant[character];

            while ( enchant < MAXENCHANT )
            {
                unset_enchant_value( enchant, SETALPHABLEND );
                unset_enchant_value( enchant, SETLIGHTBLEND );
                enchant = encnextenchant[enchant];
            }

            chralpha[character] = chrbasealpha[character];
            chrlight[character] = caplight[chrmodel[character]];
            enchant = chrfirstenchant[character];

            while ( enchant < MAXENCHANT )
            {
                set_enchant_value( enchant, SETALPHABLEND, enceve[enchant] );
                set_enchant_value( enchant, SETLIGHTBLEND, enceve[enchant] );
                enchant = encnextenchant[enchant];
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void move_water( void )
{
    // ZZ> This function animates the water overlays
    int layer;

    for ( layer = 0; layer < MAXWATERLAYER; layer++ )
    {
        waterlayeru[layer] += waterlayeruadd[layer];
        waterlayerv[layer] += waterlayervadd[layer];

        if ( waterlayeru[layer] > 1.0f )  waterlayeru[layer] -= 1.0f;

        if ( waterlayerv[layer] > 1.0f )  waterlayerv[layer] -= 1.0f;

        if ( waterlayeru[layer] < -1.0f )  waterlayeru[layer] += 1.0f;

        if ( waterlayerv[layer] < -1.0f )  waterlayerv[layer] += 1.0f;

        waterlayerframe[layer] = ( waterlayerframe[layer] + waterlayerframeadd[layer] ) & WATERFRAMEAND;
    }
}

//--------------------------------------------------------------------------------------------
void load_mesh_fans()
{
    // ZZ> This function loads fan types for the terrain
    int cnt, entry;
    int numfantype, fantype, bigfantype, vertices;
    int numcommand, command, commandsize;
    int itmp;
    float ftmp;
    FILE* fileread;
    float offx, offy;

    // Initialize all mesh types to 0
    entry = 0;

    while ( entry < MAXMESHTYPE )
    {
        meshcommandnumvertices[entry] = 0;
        meshcommands[entry] = 0;
        entry++;
    }

    // Open the file and go to it
    fileread = fopen( "basicdat" SLASH_STR "fans.txt", "r" );

    if ( fileread )
    {
        goto_colon( fileread );
        fscanf( fileread, "%d", &numfantype );
        fantype = 0;
        bigfantype = MAXMESHTYPE / 2; // Duplicate for 64x64 tiles

        while ( fantype < numfantype )
        {
            goto_colon( fileread );
            fscanf( fileread, "%d", &vertices );
            meshcommandnumvertices[fantype] = vertices;
            meshcommandnumvertices[bigfantype] = vertices;  // Dupe
            cnt = 0;

            while ( cnt < vertices )
            {
                goto_colon( fileread );
                fscanf( fileread, "%d", &itmp );
                goto_colon( fileread );
                fscanf( fileread, "%f", &ftmp );
                meshcommandu[fantype][cnt] = ftmp;
                meshcommandu[bigfantype][cnt] = ftmp;  // Dupe
                goto_colon( fileread );
                fscanf( fileread, "%f", &ftmp );
                meshcommandv[fantype][cnt] = ftmp;
                meshcommandv[bigfantype][cnt] = ftmp;  // Dupe
                cnt++;
            }

            goto_colon( fileread );
            fscanf( fileread, "%d", &numcommand );
            meshcommands[fantype] = numcommand;
            meshcommands[bigfantype] = numcommand;  // Dupe
            entry = 0;
            command = 0;

            while ( command < numcommand )
            {
                goto_colon( fileread );
                fscanf( fileread, "%d", &commandsize );
                meshcommandsize[fantype][command] = commandsize;
                meshcommandsize[bigfantype][command] = commandsize;  // Dupe
                cnt = 0;

                while ( cnt < commandsize )
                {
                    goto_colon( fileread );
                    fscanf( fileread, "%d", &itmp );
                    meshcommandvrt[fantype][entry] = itmp;
                    meshcommandvrt[bigfantype][entry] = itmp;  // Dupe
                    entry++;
                    cnt++;
                }

                command++;
            }

            fantype++;
            bigfantype++;  // Dupe
        }

        fclose( fileread );
    }

    // Correct all of them silly texture positions for seamless tiling
    entry = 0;

    while ( entry < MAXMESHTYPE / 2 )
    {
        cnt = 0;

        while ( cnt < meshcommandnumvertices[entry] )
        {
//            meshcommandu[entry][cnt] = ((0.5f/32)+(meshcommandu[entry][cnt]*31/32))/8;
//            meshcommandv[entry][cnt] = ((0.5f/32)+(meshcommandv[entry][cnt]*31/32))/8;
            meshcommandu[entry][cnt] = ( ( 0.6f / 32 ) + ( meshcommandu[entry][cnt] * 30.8f / 32 ) ) / 8;
            meshcommandv[entry][cnt] = ( ( 0.6f / 32 ) + ( meshcommandv[entry][cnt] * 30.8f / 32 ) ) / 8;
            cnt++;
        }

        entry++;
    }

    // Do for big tiles too
    while ( entry < MAXMESHTYPE )
    {
        cnt = 0;

        while ( cnt < meshcommandnumvertices[entry] )
        {
//            meshcommandu[entry][cnt] = ((0.5f/64)+(meshcommandu[entry][cnt]*63/64))/4;
//            meshcommandv[entry][cnt] = ((0.5f/64)+(meshcommandv[entry][cnt]*63/64))/4;
            meshcommandu[entry][cnt] = ( ( 0.6f / 64 ) + ( meshcommandu[entry][cnt] * 62.8f / 64 ) ) / 4;
            meshcommandv[entry][cnt] = ( ( 0.6f / 64 ) + ( meshcommandv[entry][cnt] * 62.8f / 64 ) ) / 4;
            cnt++;
        }

        entry++;
    }

    // Make tile texture offsets
    entry = 0;

    while ( entry < MAXTILETYPE )
    {
        offx = ( entry & 7 ) / 8.0f;
        offy = ( entry >> 3 ) / 8.0f;
        meshtileoffu[entry] = offx;
        meshtileoffv[entry] = offy;
        entry++;
    }
}

//--------------------------------------------------------------------------------------------
void make_fanstart()
{
    // ZZ> This function builds a look up table to ease calculating the
    //     fan number given an x,y pair
    int cnt;

    cnt = 0;

    while ( cnt < meshsizey )
    {
        meshfanstart[cnt] = meshsizex * cnt;
        cnt++;
    }

    cnt = 0;

    while ( cnt < ( meshsizey >> 2 ) )
    {
        meshblockstart[cnt] = ( meshsizex >> 2 ) * cnt;
        cnt++;
    }
}
//---------------------------------------------------------------------------------------------
void make_lighttospek( void )
{
    // ZZ> This function makes a light table to fake directional lighting
    int cnt, tnc;
    Uint8 spek;
    float fTmp, fPow;

    // New routine
    for ( cnt = 0; cnt < MAXSPEKLEVEL; cnt++ )
    {
        for ( tnc = 0; tnc < 256; tnc++ )
        {
            fTmp = tnc / 256.0f;
            fPow = ( fTmp * 4.0f ) + 1;
            fTmp = POW( fTmp, fPow );
            fTmp = fTmp * cnt * 255.0f / MAXSPEKLEVEL;

            if ( fTmp < 0 ) fTmp = 0;

            if ( fTmp > 255 ) fTmp = 255;

            spek = fTmp;
            spek = spek >> 1;
            lighttospek[cnt][tnc] = ( 0xff000000 ) | ( spek << 16 ) | ( spek << 8 ) | ( spek );
        }
    }
}

//--------------------------------------------------------------------------------------------
void show_stat( Uint16 statindex )
{
    // ZZ> This function shows the more specific stats for a character
    int character, level;
    char text[64];
    char gender[8];

    if ( statdelay == 0 )
    {
        if ( statindex < numstat )
        {
            character = statlist[statindex];

            // Name
            sprintf( text, "=%s=", chrname[character] );
            debug_message( text );

            // Level and gender and class
            gender[0] = 0;

            if ( chralive[character] )
            {
                if ( chrgender[character] == GENMALE )
                {
                    sprintf( gender, "male " );
                }

                if ( chrgender[character] == GENFEMALE )
                {
                    sprintf( gender, "female " );
                }

                level = chrexperiencelevel[character];

                if ( level == 0 )
                    sprintf( text, " 1st level %s%s", gender, capclassname[chrmodel[character]] );

                if ( level == 1 )
                    sprintf( text, " 2nd level %s%s", gender, capclassname[chrmodel[character]] );

                if ( level == 2 )
                    sprintf( text, " 3rd level %s%s", gender, capclassname[chrmodel[character]] );

                if ( level >  2 )
                    sprintf( text, " %dth level %s%s", level + 1, gender, capclassname[chrmodel[character]] );
            }
            else
            {
                sprintf( text, " Dead %s", capclassname[chrmodel[character]] );
            }

            // Stats
            debug_message( text );
            sprintf( text, " STR:~%2d~WIS:~%2d~DEF:~%d", chrstrength[character] >> 8, chrwisdom[character] >> 8, 255 - chrdefense[character] );
            debug_message( text );
            sprintf( text, " INT:~%2d~DEX:~%2d~EXP:~%d", chrintelligence[character] >> 8, chrdexterity[character] >> 8, chrexperience[character] );
            debug_message( text );
            statdelay = 10;
        }
    }
}

//--------------------------------------------------------------------------------------------
void show_armor( Uint16 statindex )
{
    // ZF> This function shows detailed armor information for the character
    char text[64], tmps[64];
    short character, skinlevel;

    if ( statdelay == 0 )
    {
        if ( statindex < numstat )
        {
            character = statlist[statindex];
            skinlevel = chrtexture[character] - madskinstart[chrmodel[character]];

            // Armor Name
            sprintf( text, "=%s=", capskinname[chrmodel[character]][skinlevel] );
            debug_message( text );

            // Armor Stats
            sprintf( text, " DEF: %d  SLASH:%3d~CRUSH:%3d POKE:%3d", 255 - capdefense[chrmodel[character]][skinlevel],
                     capdamagemodifier[chrmodel[character]][0][skinlevel]&DAMAGESHIFT,
                     capdamagemodifier[chrmodel[character]][1][skinlevel]&DAMAGESHIFT,
                     capdamagemodifier[chrmodel[character]][2][skinlevel]&DAMAGESHIFT );
            debug_message( text );

            sprintf( text, " HOLY: %i~~EVIL:~%i~FIRE:~%i~ICE:~%i~ZAP: ~%i",
                     capdamagemodifier[chrmodel[character]][3][skinlevel]&DAMAGESHIFT,
                     capdamagemodifier[chrmodel[character]][4][skinlevel]&DAMAGESHIFT,
                     capdamagemodifier[chrmodel[character]][5][skinlevel]&DAMAGESHIFT,
                     capdamagemodifier[chrmodel[character]][6][skinlevel]&DAMAGESHIFT,
                     capdamagemodifier[chrmodel[character]][7][skinlevel]&DAMAGESHIFT );
            debug_message( text );

            if ( capskindressy[chrmodel[character]] ) sprintf( tmps, "Light Armor" );
            else                   sprintf( tmps, "Heavy Armor" );

            sprintf( text, " Type: %s", tmps );

            // Speed and jumps
            if ( chrjumpnumberreset[character] == 0 )  sprintf( text, "None (0)" );

            if ( chrjumpnumberreset[character] == 1 )  sprintf( text, "Novice (1)" );

            if ( chrjumpnumberreset[character] == 2 )  sprintf( text, "Skilled (2)" );

            if ( chrjumpnumberreset[character] == 3 )  sprintf( text, "Master (3)" );

            if ( chrjumpnumberreset[character] > 3 )   sprintf( text, "Inhuman (%i)", chrjumpnumberreset[character] );

            sprintf( tmps, "Jump Skill: %s", text );
            sprintf( text, " Speed:~%3.0f~~%s", capmaxaccel[chrmodel[character]][skinlevel]*80, tmps );
            debug_message( text );

            statdelay = 10;
        }
    }
}

//--------------------------------------------------------------------------------------------
void show_full_status( Uint16 statindex )
{
    // ZF> This function shows detailed armor information for the character including magic
    char text[64], tmps[64];
    short character;
    int i = 0;

    if ( statdelay == 0 )
    {
        if ( statindex < numstat )
        {
            character = statlist[statindex];

            // Enchanted?
            while ( i != MAXENCHANT )
            {
                // Found a active enchantment that is not a skill of the character
                if ( encon[i] && encspawner[i] != character && enctarget[i] == character ) break;

                i++;
            }

            if ( i != MAXENCHANT ) sprintf( text, "=%s is enchanted!=", chrname[character] );
            else sprintf( text, "=%s is unenchanted=", chrname[character] );

            debug_message( text );

            // Armor Stats
            sprintf( text, " DEF: %d  SLASH:%3d~CRUSH:%3d POKE:%3d",
                     255 - chrdefense[character],
                     chrdamagemodifier[character][0]&DAMAGESHIFT,
                     chrdamagemodifier[character][1]&DAMAGESHIFT,
                     chrdamagemodifier[character][2]&DAMAGESHIFT );
            debug_message( text );
            sprintf( text, " HOLY: %i~~EVIL:~%i~FIRE:~%i~ICE:~%i~ZAP: ~%i",
                     chrdamagemodifier[character][3]&DAMAGESHIFT,
                     chrdamagemodifier[character][4]&DAMAGESHIFT,
                     chrdamagemodifier[character][5]&DAMAGESHIFT,
                     chrdamagemodifier[character][6]&DAMAGESHIFT,
                     chrdamagemodifier[character][7]&DAMAGESHIFT );
            debug_message( text );

            // Speed and jumps
            if ( chrjumpnumberreset[character] == 0 )  sprintf( text, "None (0)" );

            if ( chrjumpnumberreset[character] == 1 )  sprintf( text, "Novice (1)" );

            if ( chrjumpnumberreset[character] == 2 )  sprintf( text, "Skilled (2)" );

            if ( chrjumpnumberreset[character] == 3 )  sprintf( text, "Master (3)" );

            if ( chrjumpnumberreset[character] > 3 )   sprintf( text, "Inhuman (4+)" );

            sprintf( tmps, "Jump Skill: %s", text );
            sprintf( text, " Speed:~%3.0f~~%s", chrmaxaccel[character]*80, tmps );
            debug_message( text );
            statdelay = 10;
        }
    }
}

//--------------------------------------------------------------------------------------------
void show_magic_status( Uint16 statindex )
{
    // ZF> Displays special enchantment effects for the character
    char text[64], tmpa[64], tmpb[64];
    short character;
    int i = 0;

    if ( statdelay == 0 )
    {
        if ( statindex < numstat )
        {
            character = statlist[statindex];

            // Enchanted?
            while ( i != MAXENCHANT )
            {
                // Found a active enchantment that is not a skill of the character
                if ( encon[i] && encspawner[i] != character && enctarget[i] == character ) break;

                i++;
            }

            if ( i != MAXENCHANT ) sprintf( text, "=%s is enchanted!=", chrname[character] );
            else sprintf( text, "=%s is unenchanted=", chrname[character] );

            debug_message( text );

            // Enchantment status
            if ( chrcanseeinvisible[character] )  sprintf( tmpa, "Yes" );
            else                 sprintf( tmpa, "No" );

            if ( chrcanseekurse[character] )      sprintf( tmpb, "Yes" );
            else                 sprintf( tmpb, "No" );

            sprintf( text, " See Invisible: %s~~See Kurses: %s", tmpa, tmpb );
            debug_message( text );

            if ( chrcanchannel[character] )     sprintf( tmpa, "Yes" );
            else                 sprintf( tmpa, "No" );

            if ( chrwaterwalk[character] )        sprintf( tmpb, "Yes" );
            else                 sprintf( tmpb, "No" );

            sprintf( text, " Channel Life: %s~~Waterwalking: %s", tmpa, tmpb );
            debug_message( text );

            if ( chrflyheight[character] > 0 )    sprintf( tmpa, "Yes" );
            else                 sprintf( tmpa, "No" );

            if ( chrmissiletreatment[character] == MISREFLECT )       sprintf( tmpb, "Reflect" );
            else if ( chrmissiletreatment[character] == MISREFLECT )  sprintf( tmpb, "Deflect" );
            else                           sprintf( tmpb, "None" );

            sprintf( text, " Flying: %s~~Missile Protection: %s", tmpa, tmpb );
            debug_message( text );

            statdelay = 10;
        }
    }
}

//---------------------------------------------------------------------------------------------
void make_lightdirectionlookup()
{
    // ZZ> This function builds the lighting direction table
    //     The table is used to find which direction the light is coming
    //     from, based on the four corner vertices of a mesh tile.
    Uint32 cnt;
    Uint16 tl, tr, br, bl;
    int x, y;

    for ( cnt = 0; cnt < 65536; cnt++ )
    {
        tl = ( cnt & 0xf000 ) >> 12;
        tr = ( cnt & 0x0f00 ) >> 8;
        br = ( cnt & 0x00f0 ) >> 4;
        bl = ( cnt & 0x000f );
        x = br + tr - bl - tl;
        y = br + bl - tl - tr;
        lightdirectionlookup[cnt] = ( ATAN2( -y, x ) + PI ) * 256 / ( TWO_PI );
    }
}

//--------------------------------------------------------------------------------------------
void display_message( int message, Uint16 character )
{
    // ZZ> This function sticks a message in the display queue and sets its timer
    int slot, read, write, cnt;
    char *eread;
    char szTmp[256];
    char cTmp, lTmp;

    Uint16 target = chraitarget[character];
    Uint16 owner = chraiowner[character];

    if ( message < msgtotal )
    {
        slot = get_free_message();
        msgtime[slot] = messagetime;
        // Copy the message
        read = msgindex[message];
        cnt = 0;
        write = 0;
        cTmp = msgtext[read];  read++;

        while ( cTmp != 0 )
        {
            if ( cTmp == '%' )
            {
                // Escape sequence
                eread = szTmp;
                szTmp[0] = 0;
                cTmp = msgtext[read];  read++;

                if ( cTmp == 'n' )  // Name
                {
                    if ( chrnameknown[character] )
                        sprintf( szTmp, "%s", chrname[character] );
                    else
                    {
                        lTmp = capclassname[chrmodel[character]][0];

                        if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
                            sprintf( szTmp, "an %s", capclassname[chrmodel[character]] );
                        else
                            sprintf( szTmp, "a %s", capclassname[chrmodel[character]] );
                    }

                    if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
                }

                if ( cTmp == 'c' )  // Class name
                {
                    eread = capclassname[chrmodel[character]];
                }

                if ( cTmp == 't' )  // Target name
                {
                    if ( chrnameknown[target] )
                        sprintf( szTmp, "%s", chrname[target] );
                    else
                    {
                        lTmp = capclassname[chrmodel[target]][0];

                        if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
                            sprintf( szTmp, "an %s", capclassname[chrmodel[target]] );
                        else
                            sprintf( szTmp, "a %s", capclassname[chrmodel[target]] );
                    }

                    if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
                }

                if ( cTmp == 'o' )  // Owner name
                {
                    if ( chrnameknown[owner] )
                        sprintf( szTmp, "%s", chrname[owner] );
                    else
                    {
                        lTmp = capclassname[chrmodel[owner]][0];

                        if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
                            sprintf( szTmp, "an %s", capclassname[chrmodel[owner]] );
                        else
                            sprintf( szTmp, "a %s", capclassname[chrmodel[owner]] );
                    }

                    if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
                }

                if ( cTmp == 's' )  // Target class name
                {
                    eread = capclassname[chrmodel[target]];
                }

                if ( cTmp >= '0' && cTmp <= '3' )  // Target's skin name
                {
                    eread = capskinname[chrmodel[target]][cTmp-'0'];
                }

                if ( cTmp == 'd' )  // tmpdistance value
                {
                    sprintf( szTmp, "%d", valuetmpdistance );
                }

                if ( cTmp == 'x' )  // tmpx value
                {
                    sprintf( szTmp, "%d", valuetmpx );
                }

                if ( cTmp == 'y' )  // tmpy value
                {
                    sprintf( szTmp, "%d", valuetmpy );
                }

                if ( cTmp == 'D' )  // tmpdistance value
                {
                    sprintf( szTmp, "%2d", valuetmpdistance );
                }

                if ( cTmp == 'X' )  // tmpx value
                {
                    sprintf( szTmp, "%2d", valuetmpx );
                }

                if ( cTmp == 'Y' )  // tmpy value
                {
                    sprintf( szTmp, "%2d", valuetmpy );
                }

                if ( cTmp == 'a' )  // Character's ammo
                {
                    if ( chrammoknown[character] )
                        sprintf( szTmp, "%d", chrammo[character] );
                    else
                        sprintf( szTmp, "?" );
                }

                if ( cTmp == 'k' )  // Kurse state
                {
                    if ( chriskursed[character] )
                        sprintf( szTmp, "kursed" );
                    else
                        sprintf( szTmp, "unkursed" );
                }

                if ( cTmp == 'p' )  // Character's possessive
                {
                    if ( chrgender[character] == GENFEMALE )
                    {
                        sprintf( szTmp, "her" );
                    }
                    else
                    {
                        if ( chrgender[character] == GENMALE )
                        {
                            sprintf( szTmp, "his" );
                        }
                        else
                        {
                            sprintf( szTmp, "its" );
                        }
                    }
                }

                if ( cTmp == 'm' )  // Character's gender
                {
                    if ( chrgender[character] == GENFEMALE )
                    {
                        sprintf( szTmp, "female " );
                    }
                    else
                    {
                        if ( chrgender[character] == GENMALE )
                        {
                            sprintf( szTmp, "male " );
                        }
                        else
                        {
                            sprintf( szTmp, " " );
                        }
                    }
                }

                if ( cTmp == 'g' )  // Target's possessive
                {
                    if ( chrgender[target] == GENFEMALE )
                    {
                        sprintf( szTmp, "her" );
                    }
                    else
                    {
                        if ( chrgender[target] == GENMALE )
                        {
                            sprintf( szTmp, "his" );
                        }
                        else
                        {
                            sprintf( szTmp, "its" );
                        }
                    }
                }

                cTmp = *eread;  eread++;

                while ( cTmp != 0 && write < MESSAGESIZE - 1 )
                {
                    msgtextdisplay[slot][write] = cTmp;
                    cTmp = *eread;  eread++;
                    write++;
                }
            }
            else
            {
                // Copy the letter
                if ( write < MESSAGESIZE - 1 )
                {
                    msgtextdisplay[slot][write] = cTmp;
                    write++;
                }
            }

            cTmp = msgtext[read];  read++;
            cnt++;
        }

        msgtextdisplay[slot][write] = 0;
    }
}

//--------------------------------------------------------------------------------------------
// This needs work
void Begin3DMode()
{
    glMatrixMode( GL_PROJECTION );
    glLoadMatrixf( mProjection.v );

    glMatrixMode( GL_MODELVIEW );
    glLoadMatrixf( mView.v );
    glMultMatrixf( mWorld.v );
}

void End3DMode()
{

}

/********************> Begin2DMode() <*****/
void Begin2DMode( void )
{
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();                  // Reset The Projection Matrix
    glOrtho( 0, scrx, 0, scry, 1, -1 );        // Set up an orthogonal projection

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_CULL_FACE );
}

/********************> End2DMode() <*****/
void End2DMode( void )
{
    glEnable( GL_CULL_FACE );
    glEnable( GL_DEPTH_TEST );
}

//---------------------------------------------------------------------------------------------
int get_level( Uint8 x, Uint8 y, Uint32 fan, Uint8 waterwalk )
{
    // ZZ> This function returns the height of a point within a mesh fan, precise
    //     If waterwalk is nonzero and the fan is watery, then the level returned is the
    //     level of the water.
    int z0, z1, z2, z3;         // Height of each fan corner
    int zleft, zright, zdone;   // Weighted height of each side

    x = x & 127;
    y = y & 127;
    z0 = meshvrtz[meshvrtstart[fan] + 0];
    z1 = meshvrtz[meshvrtstart[fan] + 1];
    z2 = meshvrtz[meshvrtstart[fan] + 2];
    z3 = meshvrtz[meshvrtstart[fan] + 3];

    zleft = ( z0 * ( 128 - y ) + z3 * y ) >> 7;
    zright = ( z1 * ( 128 - y ) + z2 * y ) >> 7;
    zdone = ( zleft * ( 128 - x ) + zright * x ) >> 7;

    if ( waterwalk )
    {
        if ( watersurfacelevel > zdone && ( meshfx[fan]&MESHFXWATER ) && wateriswater )
        {
            return (int)watersurfacelevel;
        }
    }

    return zdone;
}



//--------------------------------------------------------------------------------------------
// ZF> Load all the global icons used in all modules
bool_t load_all_global_icons()
{
    //Setup
    bool_t result = bfalse;
    globalnumicon = 0;

    //Now load every icon
    nullicon = globalnumicon;
    result = load_one_icon( "basicdat" SLASH_STR "nullicon" );
    keybicon = globalnumicon;
    result = load_one_icon( "basicdat" SLASH_STR "keybicon" );
    mousicon = globalnumicon;
    result = load_one_icon( "basicdat" SLASH_STR "mousicon" );
    joyaicon = globalnumicon;
    result = load_one_icon( "basicdat" SLASH_STR "joyaicon" );
    joybicon = globalnumicon;
    result = load_one_icon( "basicdat" SLASH_STR "joybicon" );

    return result;
}

//--------------------------------------------------------------------------------------------
bool_t load_one_icon( char *szLoadName )
{
    // ZZ> This function is used to load an icon.  Most icons are loaded
    //     without this function though...

    if ( INVALID_TX_ID == GLTexture_Load( GL_TEXTURE_2D,  TxIcon + globalnumicon,  szLoadName, INVALID_KEY ) )
    {
        return bfalse;
    }

    globalnumicon++;
    return btrue;
}

//---------------------------------------------------------------------------------------------
void init_all_icons()
{
    // ZZ> This function sets the icon pointers to NULL
    int cnt;

    for ( cnt = 0; cnt < MAXTEXTURE + 1; cnt++ )
    {
        GLTexture_new( TxIcon + cnt );
    }

    iconrect.left = 0;
    iconrect.right = 32;
    iconrect.top = 0;
    iconrect.bottom = 32;

    release_all_icons();
}

//---------------------------------------------------------------------------------------------
void init_all_titleimages()
{
    // ZZ> This function clears out all of the title images
    int cnt;

    for ( cnt = 0; cnt < MAXMODULE; cnt++ )
    {
        GLTexture_new( TxTitleImage + cnt );
    }
}

//---------------------------------------------------------------------------------------------
void init_bars()
{
    int cnt;

    GLTexture_new( &TxBars );

    // Make the blit rectangles
    for ( cnt = 0; cnt < NUMBAR; cnt++ )
    {
        tabrect[cnt].left = 0;
        tabrect[cnt].right = TABX;
        tabrect[cnt].top = cnt * BARY;
        tabrect[cnt].bottom = ( cnt + 1 ) * BARY;

        barrect[cnt].left = TABX;
        barrect[cnt].right = BARX;  // This is reset whenever a bar is drawn
        barrect[cnt].top = tabrect[cnt].top;
        barrect[cnt].bottom = tabrect[cnt].bottom;
    }
}

//---------------------------------------------------------------------------------------------
void init_blip()
{
    int cnt;

    GLTexture_new( &TxBlip );

    // Set up the rectangles
    for ( cnt = 0; cnt < NUMBAR; cnt++ )
    {
        bliprect[cnt].left   = cnt * BLIPSIZE;
        bliprect[cnt].right  = ( cnt * BLIPSIZE ) + BLIPSIZE;
        bliprect[cnt].top    = 0;
        bliprect[cnt].bottom = BLIPSIZE;
    }

}

//---------------------------------------------------------------------------------------------
void init_map()
{
    // ZZ> This function releases all the map images
    GLTexture_new( &TxMap );

    // Set up the rectangles
    maprect.left   = 0;
    maprect.right  = MAPSIZE;
    maprect.top    = 0;
    maprect.bottom = MAPSIZE;
}

//---------------------------------------------------------------------------------------------
void init_all_textures()
{
    // ZZ> This function clears out all of the textures
    int cnt;

    for ( cnt = 0; cnt < MAXTEXTURE; cnt++ )
    {
        GLTexture_new( txTexture + cnt );
    }
}


//---------------------------------------------------------------------------------------------
void init_all_models()
{
    // ZZ> This function initializes all of the models

    Uint16 cnt;

    for ( cnt = 0; cnt < MAXMODEL; cnt++ )
    {
        capclassname[cnt][0] = 0;

        madused[cnt] = bfalse;
        strncpy( madname[cnt], "*NONE*", sizeof(madname[cnt]) );
    }

    madloadframe = 0;
}

//---------------------------------------------------------------------------------------------
void release_all_icons()
{
    // ZZ> This function clears out all of the icons
    int cnt;

    for ( cnt = 0; cnt < MAXTEXTURE + 1; cnt++ )
    {
        GLTexture_Release( TxIcon + cnt );
        madskintoicon[cnt] = 0;
    }

    globalnumicon = 0;
}

//---------------------------------------------------------------------------------------------
void release_all_titleimages()
{
    // ZZ> This function clears out all of the title images
    int cnt;

    for ( cnt = 0; cnt < MAXMODULE; cnt++ )
    {
        GLTexture_Release( TxTitleImage + cnt );
    }
}

//---------------------------------------------------------------------------------------------
void release_bars()
{
    GLTexture_Release( &TxBars );
}

//---------------------------------------------------------------------------------------------
void release_blip()
{
    GLTexture_Release( &TxBlip );

    youarehereon = bfalse;
    numblip      = 0;
}

//---------------------------------------------------------------------------------------------
void release_map()
{
    GLTexture_Release( &TxMap );

    mapvalid = bfalse;
    mapon    = bfalse;
}

//---------------------------------------------------------------------------------------------
void release_all_textures()
{
    // ZZ> This function releases all of the textures
    int cnt;

    for ( cnt = 0; cnt < MAXTEXTURE; cnt++ )
    {
        GLTexture_Release( txTexture + cnt );
    }
}


//---------------------------------------------------------------------------------------------
void release_all_models()
{
    // ZZ> This function clears out all of the models
    Uint16 cnt;

    for ( cnt = 0; cnt < MAXMODEL; cnt++ )
    {
        capclassname[cnt][0] = 0;

        madused[cnt] = bfalse;
        strncpy( madname[cnt], "*NONE*", sizeof(madname[cnt]) );
    }

    madloadframe = 0;
}


//--------------------------------------------------------------------------------------------
void debug_message( char *text )
{
    // ZZ> This function sticks a message in the display queue and sets its timer
    int slot = get_free_message();
    // Copy the message
    int write = 0;
    int read = 0;
    char cTmp = text[read];  read++;
    msgtime[slot] = messagetime;

    while ( cTmp != 0 )
    {
        msgtextdisplay[slot][write] = cTmp;
        write++;
        cTmp = text[read];  read++;
    }

    msgtextdisplay[slot][write] = 0;
}

//--------------------------------------------------------------------------------------------
void reset_end_text()
{
    // ZZ> This function resets the end-module text
    if ( numpla > 1 )
    {
        sprintf( endtext, "Sadly, they were never heard from again..." );
        endtextwrite = 42;  // Where to append further text
    }
    else
    {
        if ( numpla == 0 )
        {
            // No players???
            sprintf( endtext, "The game has ended..." );
            endtextwrite = 21;
        }
        else
        {
            // One player
            sprintf( endtext, "Sadly, no trace was ever found..." );
            endtextwrite = 33;  // Where to append further text
        }
    }
}

//--------------------------------------------------------------------------------------------
void append_end_text( int message, Uint16 character )
{
    // ZZ> This function appends a message to the end-module text
    int read, cnt;
    char *eread;
    char szTmp[256];
    char cTmp, lTmp;
    Uint16 target, owner;

    target = chraitarget[character];
    owner = chraiowner[character];

    if ( message < msgtotal )
    {
        // Copy the message
        read = msgindex[message];
        cnt = 0;
        cTmp = msgtext[read];  read++;

        while ( cTmp != 0 )
        {
            if ( cTmp == '%' )
            {
                // Escape sequence
                eread = szTmp;
                szTmp[0] = 0;
                cTmp = msgtext[read];  read++;

                if ( cTmp == 'n' )  // Name
                {
                    if ( chrnameknown[character] )
                        sprintf( szTmp, "%s", chrname[character] );
                    else
                    {
                        lTmp = capclassname[chrmodel[character]][0];

                        if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
                            sprintf( szTmp, "an %s", capclassname[chrmodel[character]] );
                        else
                            sprintf( szTmp, "a %s", capclassname[chrmodel[character]] );
                    }

                    if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
                }

                if ( cTmp == 'c' )  // Class name
                {
                    eread = capclassname[chrmodel[character]];
                }

                if ( cTmp == 't' )  // Target name
                {
                    if ( chrnameknown[target] )
                        sprintf( szTmp, "%s", chrname[target] );
                    else
                    {
                        lTmp = capclassname[chrmodel[target]][0];

                        if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
                            sprintf( szTmp, "an %s", capclassname[chrmodel[target]] );
                        else
                            sprintf( szTmp, "a %s", capclassname[chrmodel[target]] );
                    }

                    if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
                }

                if ( cTmp == 'o' )  // Owner name
                {
                    if ( chrnameknown[owner] )
                        sprintf( szTmp, "%s", chrname[owner] );
                    else
                    {
                        lTmp = capclassname[chrmodel[owner]][0];

                        if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
                            sprintf( szTmp, "an %s", capclassname[chrmodel[owner]] );
                        else
                            sprintf( szTmp, "a %s", capclassname[chrmodel[owner]] );
                    }

                    if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
                }

                if ( cTmp == 's' )  // Target class name
                {
                    eread = capclassname[chrmodel[target]];
                }

                if ( cTmp >= '0' && cTmp <= '3' )  // Target's skin name
                {
                    eread = capskinname[chrmodel[target]][cTmp-'0'];
                }

                if ( cTmp == 'd' )  // tmpdistance value
                {
                    sprintf( szTmp, "%d", valuetmpdistance );
                }

                if ( cTmp == 'x' )  // tmpx value
                {
                    sprintf( szTmp, "%d", valuetmpx );
                }

                if ( cTmp == 'y' )  // tmpy value
                {
                    sprintf( szTmp, "%d", valuetmpy );
                }

                if ( cTmp == 'D' )  // tmpdistance value
                {
                    sprintf( szTmp, "%2d", valuetmpdistance );
                }

                if ( cTmp == 'X' )  // tmpx value
                {
                    sprintf( szTmp, "%2d", valuetmpx );
                }

                if ( cTmp == 'Y' )  // tmpy value
                {
                    sprintf( szTmp, "%2d", valuetmpy );
                }

                if ( cTmp == 'a' )  // Character's ammo
                {
                    if ( chrammoknown[character] )
                        sprintf( szTmp, "%d", chrammo[character] );
                    else
                        sprintf( szTmp, "?" );
                }

                if ( cTmp == 'k' )  // Kurse state
                {
                    if ( chriskursed[character] )
                        sprintf( szTmp, "kursed" );
                    else
                        sprintf( szTmp, "unkursed" );
                }

                if ( cTmp == 'p' )  // Character's possessive
                {
                    if ( chrgender[character] == GENFEMALE )
                    {
                        sprintf( szTmp, "her" );
                    }
                    else
                    {
                        if ( chrgender[character] == GENMALE )
                        {
                            sprintf( szTmp, "his" );
                        }
                        else
                        {
                            sprintf( szTmp, "its" );
                        }
                    }
                }

                if ( cTmp == 'm' )  // Character's gender
                {
                    if ( chrgender[character] == GENFEMALE )
                    {
                        sprintf( szTmp, "female " );
                    }
                    else
                    {
                        if ( chrgender[character] == GENMALE )
                        {
                            sprintf( szTmp, "male " );
                        }
                        else
                        {
                            sprintf( szTmp, " " );
                        }
                    }
                }

                if ( cTmp == 'g' )  // Target's possessive
                {
                    if ( chrgender[target] == GENFEMALE )
                    {
                        sprintf( szTmp, "her" );
                    }
                    else
                    {
                        if ( chrgender[target] == GENMALE )
                        {
                            sprintf( szTmp, "his" );
                        }
                        else
                        {
                            sprintf( szTmp, "its" );
                        }
                    }
                }

                // Copy the generated text
                cTmp = *eread;  eread++;

                while ( cTmp != 0 && endtextwrite < MAXENDTEXT - 1 )
                {
                    endtext[endtextwrite] = cTmp;
                    cTmp = *eread;  eread++;
                    endtextwrite++;
                }
            }
            else
            {
                // Copy the letter
                if ( endtextwrite < MAXENDTEXT - 1 )
                {
                    endtext[endtextwrite] = cTmp;
                    endtextwrite++;
                }
            }

            cTmp = msgtext[read];  read++;
            cnt++;
        }
    }

    endtext[endtextwrite] = 0;
}

//--------------------------------------------------------------------------------------------
void make_textureoffset( void )
{
    // ZZ> This function sets up for moving textures
    int cnt;

    for ( cnt = 0; cnt < 256; cnt++ )
    {
        textureoffset[cnt] = cnt / 256.0f;
    }
}

//--------------------------------------------------------------------------------------------
void create_szfpstext( int frames )
{
    // ZZ> This function fills in the number of frames in "000 Frames per Second"
    frames = frames & 511;
    szfpstext[0] = '0' + ( frames / 100 );
    szfpstext[1] = '0' + ( ( frames / 10 ) % 10 );
    szfpstext[2] = '0' + ( frames % 10 );
}

//--------------------------------------------------------------------------------------------
void make_renderlist()
{
    // ZZ> This function figures out which mesh fans to draw
    int cnt, fan, fanx, fany;
    int row, run, numrow;
    int xlist[4], ylist[4];
    int leftnum, leftlist[4];
    int rightnum, rightlist[4];
    int fanrowstart[128], fanrowrun[128];
    int x, stepx, divx, basex;
    int from, to;

    // Clear old render lists
    for ( cnt = 0; cnt < nummeshrenderlist; cnt++ )
    {
        fan = meshrenderlist[cnt];
        meshinrenderlist[fan] = btrue;
    }

    nummeshrenderlist = 0;
    nummeshrenderlistref = 0;
    nummeshrenderlistsha = 0;

    // Make sure it doesn't die ugly !!!BAD!!!

    // It works better this way...
    cornery[cornerlistlowtohighy[3]] += 256;

    // Make life simpler
    xlist[0] = cornerx[cornerlistlowtohighy[0]];
    xlist[1] = cornerx[cornerlistlowtohighy[1]];
    xlist[2] = cornerx[cornerlistlowtohighy[2]];
    xlist[3] = cornerx[cornerlistlowtohighy[3]];
    ylist[0] = cornery[cornerlistlowtohighy[0]];
    ylist[1] = cornery[cornerlistlowtohighy[1]];
    ylist[2] = cornery[cornerlistlowtohighy[2]];
    ylist[3] = cornery[cornerlistlowtohighy[3]];

    // Find the center line

    divx = ylist[3] - ylist[0]; if ( divx < 1 ) return;

    stepx = xlist[3] - xlist[0];
    basex = xlist[0];

    // Find the points in each edge
    leftlist[0] = 0;  leftnum = 1;
    rightlist[0] = 0;  rightnum = 1;

    if ( xlist[1] < ( stepx*( ylist[1] - ylist[0] ) / divx ) + basex )
    {
        leftlist[leftnum] = 1;  leftnum++;
        cornerx[1] -= 512;
    }
    else
    {
        rightlist[rightnum] = 1;  rightnum++;
        cornerx[1] += 512;
    }

    if ( xlist[2] < ( stepx*( ylist[2] - ylist[0] ) / divx ) + basex )
    {
        leftlist[leftnum] = 2;  leftnum++;
        cornerx[2] -= 512;
    }
    else
    {
        rightlist[rightnum] = 2;  rightnum++;
        cornerx[2] += 512;
    }

    leftlist[leftnum] = 3;  leftnum++;
    rightlist[rightnum] = 3;  rightnum++;

    // Make the left edge ( rowstart )
    fany = ylist[0] >> 7;
    row = 0;
    cnt = 1;

    while ( cnt < leftnum )
    {
        from = leftlist[cnt-1];  to = leftlist[cnt];
        x = xlist[from];
        divx = ylist[to] - ylist[from];
        stepx = 0;

        if ( divx > 0 )
        {
            stepx = ( ( xlist[to] - xlist[from] ) << 7 ) / divx;
        }

        x -= 256;
        run = ylist[to] >> 7;

        while ( fany < run )
        {
            if ( fany >= 0 && fany < meshsizey )
            {
                fanx = x >> 7;

                if ( fanx < 0 )  fanx = 0;

                if ( fanx >= meshsizex )  fanx = meshsizex - 1;

                fanrowstart[row] = fanx;
                row++;
            }

            x += stepx;
            fany++;
        }

        cnt++;
    }

    numrow = row;

    // Make the right edge ( rowrun )
    fany = ylist[0] >> 7;
    row = 0;
    cnt = 1;

    while ( cnt < rightnum )
    {
        from = rightlist[cnt-1];  to = rightlist[cnt];
        x = xlist[from];
        // x+=128;
        divx = ylist[to] - ylist[from];
        stepx = 0;

        if ( divx > 0 )
        {
            stepx = ( ( xlist[to] - xlist[from] ) << 7 ) / divx;
        }

        run = ylist[to] >> 7;

        while ( fany < run )
        {
            if ( fany >= 0 && fany < meshsizey )
            {
                fanx = x >> 7;

                if ( fanx < 0 )  fanx = 0;

                if ( fanx >= meshsizex - 1 )  fanx = meshsizex - 1;//-2

                fanrowrun[row] = ABS( fanx - fanrowstart[row] ) + 1;
                row++;
            }

            x += stepx;
            fany++;
        }

        cnt++;
    }

    if ( numrow != row )
    {
        log_error( "ROW error (%i, %i)\n", numrow, row );
    }

    // Fill 'em up again
    fany = ylist[0] >> 7;

    if ( fany < 0 ) fany = 0;

    if ( fany >= meshsizey ) fany = meshsizey - 1;

    row = 0;

    while ( row < numrow )
    {
        cnt = meshfanstart[fany] + fanrowstart[row];
        run = fanrowrun[row];
        fanx = 0;

        while ( fanx < run )
        {
            if ( nummeshrenderlist < MAXMESHRENDER )
            {
                // Put each tile in basic list
                meshinrenderlist[cnt] = btrue;
                meshrenderlist[nummeshrenderlist] = cnt;
                nummeshrenderlist++;

                // Put each tile in one other list, for shadows and relections
                if ( meshfx[cnt]&MESHFXSHA )
                {
                    meshrenderlistsha[nummeshrenderlistsha] = cnt;
                    nummeshrenderlistsha++;
                }
                else
                {
                    meshrenderlistref[nummeshrenderlistref] = cnt;
                    nummeshrenderlistref++;
                }
            }

            cnt++;
            fanx++;
        }

        row++;
        fany++;
    }
}

//--------------------------------------------------------------------------------------------
void figure_out_what_to_draw()
{
    // ZZ> This function determines the things that need to be drawn

    // Find the render area corners
    project_view();
    // Make the render list for the mesh
    make_renderlist();

    camturnleftrightone = ( camturnleftright ) / ( TWO_PI );
    camturnleftrightshort = camturnleftrightone * 65536;

    // Request matrices needed for local machine
    make_dolist();
    order_dolist();
}

//--------------------------------------------------------------------------------------------
void animate_tiles()
{
    // This function changes the animated tile frame
    if ( ( wldframe & animtileupdateand ) == 0 )
    {
        animtileframeadd = ( animtileframeadd + 1 ) & animtileframeand;
    }
}

//--------------------------------------------------------------------------------------------
void load_basic_textures( char *modname )
{
    // ZZ> This function loads the standard textures for a module
    char newloadname[256];

    // Particle sprites
    GLTexture_Load(GL_TEXTURE_2D, txTexture + TX_PARTICLE, "basicdat" SLASH_STR "globalparticles" SLASH_STR "particle", TRANSCOLOR );

    // Module background tiles
    make_newloadname( modname, "gamedat" SLASH_STR "tile0", newloadname );
    GLTexture_Load(GL_TEXTURE_2D, txTexture + TX_TILE_0, newloadname, INVALID_KEY );

    make_newloadname( modname, "gamedat" SLASH_STR "tile1", newloadname );
    GLTexture_Load(GL_TEXTURE_2D,  txTexture + TX_TILE_1, newloadname, INVALID_KEY );

    make_newloadname( modname, "gamedat" SLASH_STR "tile2", newloadname );
    GLTexture_Load(GL_TEXTURE_2D, txTexture + TX_TILE_2, newloadname, INVALID_KEY);

    make_newloadname( modname, "gamedat" SLASH_STR "tile3", newloadname );
    GLTexture_Load(GL_TEXTURE_2D, txTexture + TX_TILE_3, newloadname, INVALID_KEY );

    // Water textures
    make_newloadname( modname, "gamedat" SLASH_STR "watertop", newloadname );
    GLTexture_Load( GL_TEXTURE_2D,  txTexture + TX_WATER_TOP, newloadname, TRANSCOLOR );
    make_newloadname( modname, "gamedat" SLASH_STR "waterlow", newloadname );
    GLTexture_Load( GL_TEXTURE_2D,  txTexture + TX_WATER_LOW, newloadname, TRANSCOLOR);

    // Texture 7 is the phong map
    GLTexture_Load( GL_TEXTURE_2D,  txTexture + TX_PHONG, "basicdat" SLASH_STR "phong", TRANSCOLOR );

}

//--------------------------------------------------------------------------------------------
Uint16 action_number()
{
    // ZZ> This function returns the number of the action in cFrameName, or
    //     it returns NOACTION if it could not find a match
    int cnt;
    char first, second;

    first = cFrameName[0];
    second = cFrameName[1];

    for ( cnt = 0; cnt < MAXACTION; cnt++ )
    {
        if ( first == cActionName[cnt][0] && second == cActionName[cnt][1] )
        {
            return cnt;
        }
    }

    return NOACTION;
}

//--------------------------------------------------------------------------------------------
Uint16 action_frame()
{
    // ZZ> This function returns the frame number in the third and fourth characters
    //     of cFrameName
    int number;
    sscanf( &cFrameName[2], "%d", &number );
    return number;
}

//--------------------------------------------------------------------------------------------
Uint16 test_frame_name( char letter )
{
    // ZZ> This function returns btrue if the 4th, 5th, 6th, or 7th letters
    //     of the frame name matches the input argument
    if ( cFrameName[4] == letter ) return btrue;

    if ( cFrameName[4] == 0 ) return bfalse;

    if ( cFrameName[5] == letter ) return btrue;

    if ( cFrameName[5] == 0 ) return bfalse;

    if ( cFrameName[6] == letter ) return btrue;

    if ( cFrameName[6] == 0 ) return bfalse;

    if ( cFrameName[7] == letter ) return btrue;

    return bfalse;
}

//--------------------------------------------------------------------------------------------
void action_copy_correct( int object, Uint16 actiona, Uint16 actionb )
{
    // ZZ> This function makes sure both actions are valid if either of them
    //     are valid.  It will copy start and ends to mirror the valid action.
    if ( madactionvalid[object][actiona] == madactionvalid[object][actionb] )
    {
        // They are either both valid or both invalid, in either case we can't help
    }
    else
    {
        // Fix the invalid one
        if ( !madactionvalid[object][actiona] )
        {
            // Fix actiona
            madactionvalid[object][actiona] = btrue;
            madactionstart[object][actiona] = madactionstart[object][actionb];
            madactionend[object][actiona] = madactionend[object][actionb];
        }
        else
        {
            // Fix actionb
            madactionvalid[object][actionb] = btrue;
            madactionstart[object][actionb] = madactionstart[object][actiona];
            madactionend[object][actionb] = madactionend[object][actiona];
        }
    }
}

//--------------------------------------------------------------------------------------------
void get_walk_frame( int object, int lip, int action )
{
    // ZZ> This helps make walking look right
    int frame = 0;
    int framesinaction = madactionend[object][action] - madactionstart[object][action];

    while ( frame < 16 )
    {
        int framealong = 0;

        if ( framesinaction > 0 )
        {
            framealong = ( ( frame * framesinaction / 16 ) + 2 ) % framesinaction;
        }

        madframeliptowalkframe[object][lip][frame] = madactionstart[object][action] + framealong;
        frame++;
    }
}

//--------------------------------------------------------------------------------------------
void get_framefx( int frame )
{
    // ZZ> This function figures out the IFrame invulnerability, and Attack, Grab, and
    //     Drop timings
    Uint16 fx = 0;

    if ( test_frame_name( 'I' ) )
        fx = fx | MADFXINVICTUS;

    if ( test_frame_name( 'L' ) )
    {
        if ( test_frame_name( 'A' ) )
            fx = fx | MADFXACTLEFT;

        if ( test_frame_name( 'G' ) )
            fx = fx | MADFXGRABLEFT;

        if ( test_frame_name( 'D' ) )
            fx = fx | MADFXDROPLEFT;

        if ( test_frame_name( 'C' ) )
            fx = fx | MADFXCHARLEFT;
    }

    if ( test_frame_name( 'R' ) )
    {
        if ( test_frame_name( 'A' ) )
            fx = fx | MADFXACTRIGHT;

        if ( test_frame_name( 'G' ) )
            fx = fx | MADFXGRABRIGHT;

        if ( test_frame_name( 'D' ) )
            fx = fx | MADFXDROPRIGHT;

        if ( test_frame_name( 'C' ) )
            fx = fx | MADFXCHARRIGHT;
    }

    if ( test_frame_name( 'S' ) )
        fx = fx | MADFXSTOP;

    if ( test_frame_name( 'F' ) )
        fx = fx | MADFXFOOTFALL;

    if ( test_frame_name( 'P' ) )
        fx = fx | MADFXPOOF;

    madframefx[frame] = fx;
}

//--------------------------------------------------------------------------------------------
void make_framelip( int object, int action )
{
    // ZZ> This helps make walking look right
    int frame, framesinaction;

    if ( madactionvalid[object][action] )
    {
        framesinaction = madactionend[object][action] - madactionstart[object][action];
        frame = madactionstart[object][action];

        while ( frame < madactionend[object][action] )
        {
            madframelip[frame] = ( frame - madactionstart[object][action] ) * 15 / framesinaction;
            madframelip[frame] = ( madframelip[frame] ) & 15;
            frame++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void get_actions( int object )
{
    // ZZ> This function creates the frame lists for each action based on the
    //     name of each md2 frame in the model
    int frame, framesinaction;
    int action, lastaction;

    // Clear out all actions and reset to invalid
    action = 0;

    while ( action < MAXACTION )
    {
        madactionvalid[object][action] = bfalse;
        action++;
    }

    // Set the primary dance action to be the first frame, just as a default
    madactionvalid[object][ACTIONDA] = btrue;
    madactionstart[object][ACTIONDA] = madframestart[object];
    madactionend[object][ACTIONDA] = madframestart[object] + 1;

    // Now go huntin' to see what each frame is, look for runs of same action
    rip_md2_frame_name( 0 );
    lastaction = action_number();  framesinaction = 0;
    frame = 0;

    while ( frame < madframes[object] )
    {
        rip_md2_frame_name( frame );
        action = action_number();

        if ( lastaction == action )
        {
            framesinaction++;
        }
        else
        {
            // Write the old action
            if ( lastaction < MAXACTION )
            {
                madactionvalid[object][lastaction] = btrue;
                madactionstart[object][lastaction] = madframestart[object] + frame - framesinaction;
                madactionend[object][lastaction] = madframestart[object] + frame;
            }

            framesinaction = 1;
            lastaction = action;
        }

        get_framefx( madframestart[object] + frame );
        frame++;
    }

    // Write the old action
    if ( lastaction < MAXACTION )
    {
        madactionvalid[object][lastaction] = btrue;
        madactionstart[object][lastaction] = madframestart[object] + frame - framesinaction;
        madactionend[object][lastaction]   = madframestart[object] + frame;
    }

    // Make sure actions are made valid if a similar one exists
    action_copy_correct( object, ACTIONDA, ACTIONDB );  // All dances should be safe
    action_copy_correct( object, ACTIONDB, ACTIONDC );
    action_copy_correct( object, ACTIONDC, ACTIONDD );
    action_copy_correct( object, ACTIONDB, ACTIONDC );
    action_copy_correct( object, ACTIONDA, ACTIONDB );
    action_copy_correct( object, ACTIONUA, ACTIONUB );
    action_copy_correct( object, ACTIONUB, ACTIONUC );
    action_copy_correct( object, ACTIONUC, ACTIONUD );
    action_copy_correct( object, ACTIONTA, ACTIONTB );
    action_copy_correct( object, ACTIONTC, ACTIONTD );
    action_copy_correct( object, ACTIONCA, ACTIONCB );
    action_copy_correct( object, ACTIONCC, ACTIONCD );
    action_copy_correct( object, ACTIONSA, ACTIONSB );
    action_copy_correct( object, ACTIONSC, ACTIONSD );
    action_copy_correct( object, ACTIONBA, ACTIONBB );
    action_copy_correct( object, ACTIONBC, ACTIONBD );
    action_copy_correct( object, ACTIONLA, ACTIONLB );
    action_copy_correct( object, ACTIONLC, ACTIONLD );
    action_copy_correct( object, ACTIONXA, ACTIONXB );
    action_copy_correct( object, ACTIONXC, ACTIONXD );
    action_copy_correct( object, ACTIONFA, ACTIONFB );
    action_copy_correct( object, ACTIONFC, ACTIONFD );
    action_copy_correct( object, ACTIONPA, ACTIONPB );
    action_copy_correct( object, ACTIONPC, ACTIONPD );
    action_copy_correct( object, ACTIONZA, ACTIONZB );
    action_copy_correct( object, ACTIONZC, ACTIONZD );
    action_copy_correct( object, ACTIONWA, ACTIONWB );
    action_copy_correct( object, ACTIONWB, ACTIONWC );
    action_copy_correct( object, ACTIONWC, ACTIONWD );
    action_copy_correct( object, ACTIONDA, ACTIONWD );  // All walks should be safe
    action_copy_correct( object, ACTIONWC, ACTIONWD );
    action_copy_correct( object, ACTIONWB, ACTIONWC );
    action_copy_correct( object, ACTIONWA, ACTIONWB );
    action_copy_correct( object, ACTIONJA, ACTIONJB );
    action_copy_correct( object, ACTIONJB, ACTIONJC );
    action_copy_correct( object, ACTIONDA, ACTIONJC );  // All jumps should be safe
    action_copy_correct( object, ACTIONJB, ACTIONJC );
    action_copy_correct( object, ACTIONJA, ACTIONJB );
    action_copy_correct( object, ACTIONHA, ACTIONHB );
    action_copy_correct( object, ACTIONHB, ACTIONHC );
    action_copy_correct( object, ACTIONHC, ACTIONHD );
    action_copy_correct( object, ACTIONHB, ACTIONHC );
    action_copy_correct( object, ACTIONHA, ACTIONHB );
    action_copy_correct( object, ACTIONKA, ACTIONKB );
    action_copy_correct( object, ACTIONKB, ACTIONKC );
    action_copy_correct( object, ACTIONKC, ACTIONKD );
    action_copy_correct( object, ACTIONKB, ACTIONKC );
    action_copy_correct( object, ACTIONKA, ACTIONKB );
    action_copy_correct( object, ACTIONMH, ACTIONMI );
    action_copy_correct( object, ACTIONDA, ACTIONMM );
    action_copy_correct( object, ACTIONMM, ACTIONMN );

    // Create table for doing transition from one type of walk to another...
    // Clear 'em all to start
    for ( frame = 0; frame < madframes[object]; frame++ )
    {
        madframelip[frame+madframestart[object]] = 0;
    }

    // Need to figure out how far into action each frame is
    make_framelip( object, ACTIONWA );
    make_framelip( object, ACTIONWB );
    make_framelip( object, ACTIONWC );

    // Now do the same, in reverse, for walking animations
    get_walk_frame( object, LIPDA, ACTIONDA );
    get_walk_frame( object, LIPWA, ACTIONWA );
    get_walk_frame( object, LIPWB, ACTIONWB );
    get_walk_frame( object, LIPWC, ACTIONWC );
}

//--------------------------------------------------------------------------------------------
void make_mad_equally_lit( int model )
{
    // ZZ> This function makes ultra low poly models look better
    int frame, cnt, vert;

    if ( madused[model] )
    {
        frame = madframestart[model];

        for ( cnt = 0; cnt < madframes[model]; cnt++ )
        {
            vert = 0;

            while ( vert < MAXVERTICES )
            {
                madvrta[frame][vert] = EQUALLIGHTINDEX;
                vert++;
            }

            frame++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void check_copy( char* loadname, int object )
{
    // ZZ> This function copies a model's actions
    FILE *fileread;
    int actiona, actionb;
    char szOne[16], szTwo[16];

    madmsgstart[object] = 0;
    fileread = fopen( loadname, "r" );

    if ( fileread )
    {
        while ( goto_colon_yesno( fileread ) )
        {
            fscanf( fileread, "%s%s", szOne, szTwo );
            actiona = what_action( szOne[0] );
            actionb = what_action( szTwo[0] );
            action_copy_correct( object, actiona, actionb );
            action_copy_correct( object, actiona + 1, actionb + 1 );
            action_copy_correct( object, actiona + 2, actionb + 2 );
            action_copy_correct( object, actiona + 3, actionb + 3 );
        }

        fclose( fileread );
    }
}

//--------------------------------------------------------------------------------------------
int load_one_object( int skin, char* tmploadname )
{
    // ZZ> This function loads one object and returns the number of skins
    int object;
    int numskins, numicon;
    char newloadname[256];
    char wavename[256];
    int cnt;

    // Load the object data file and get the object number
    make_newloadname( tmploadname, "/data.txt", newloadname );
    object = load_one_character_profile( newloadname );

    //Don't override it if it's already there
    if (!overrideslots && object == -1) return 0;

    // Make up a name for the model...  IMPORT\TEMP0000.OBJ
    strncpy( madname[object], tmploadname, sizeof(madname[object]) / sizeof(*madname[object]) );
    // Make sure the string is null-terminated (strncpy doesn't do that if it's too long)
    madname[object][ sizeof(madname[object]) / sizeof(*madname[object]) ] = '\0';

    // Append a slash to the tmploadname
    sprintf( newloadname, "%s", tmploadname );
    sprintf( tmploadname, "%s" SLASH_STR, newloadname );

    // Load the AI script for this object
    make_newloadname( tmploadname, "script.txt", newloadname );

    if ( load_ai_script( newloadname ) )
    {
        // Create a reference to the one we just loaded
        madai[object] = iNumAis - 1;
    }

    // Load the object model
    make_newloadname( tmploadname, "tris.md2", newloadname );

#ifdef __unix__

    // unix is case sensitive, but sometimes this file is called tris.MD2
    if ( access( newloadname, R_OK ) )
    {
        make_newloadname( tmploadname, "tris.MD2", newloadname );

        // still no luck !
        if ( access( newloadname, R_OK ) )
        {
            log_warning( "Cannot open: %s\n", newloadname );
        }
    }

#endif

    load_one_md2( newloadname, object );
    md2_models[object] = md2_loadFromFile( newloadname );

    // Fix lighting if need be
    if ( capuniformlit[object] )
    {
        make_mad_equally_lit( object );
    }

    // Create the actions table for this object
    get_actions( object );

    // Copy entire actions to save frame space COPY.TXT
    make_newloadname( tmploadname, "copy.txt", newloadname );
    check_copy( newloadname, object );

    // Load the messages for this object
    make_newloadname( tmploadname, "message.txt", newloadname );
    load_all_messages( newloadname, object );

    // Load the random naming table for this object
    make_newloadname( tmploadname, "naming.txt", newloadname );
    read_naming( object, newloadname );

    // Load the particles for this object
    for ( cnt = 0; cnt < MAXPRTPIPPEROBJECT; cnt++ )
    {
        sprintf( newloadname, "%spart%d.txt", tmploadname, cnt );
        load_one_particle( newloadname, object, cnt );
    }

    // Load the waves for this object
    for ( cnt = 0; cnt < MAXWAVE; cnt++ )
    {
        sprintf( wavename, "sound%d", cnt );
        make_newloadname( tmploadname, wavename, newloadname );
        load_sound( capwaveindex[object] + cnt, newloadname );
    }

    // Load the enchantment for this object
    make_newloadname( tmploadname, "enchant.txt", newloadname );
    load_one_enchant_type( newloadname, object );

    // Load the skins and icons
    madskinstart[object] = skin;
    numskins = 0;
    numicon = 0;
    make_newloadname( tmploadname, "tris0", newloadname );

    if ( INVALID_TX_ID != GLTexture_Load(GL_TEXTURE_2D, txTexture + (skin + numskins), newloadname, TRANSCOLOR ) )
    {
        numskins++;
        make_newloadname( tmploadname, "icon0", newloadname );
        if ( INVALID_TX_ID != GLTexture_Load(GL_TEXTURE_2D, TxIcon + globalnumicon, newloadname, INVALID_KEY ) )
        {
            while ( numicon < numskins )
            {
                madskintoicon[skin+numicon] = globalnumicon;

                if ( object == SPELLBOOK )  bookicon = globalnumicon;

                numicon++;
            }

            globalnumicon++;
        }
    }

    make_newloadname( tmploadname, "tris1", newloadname );
    if ( INVALID_TX_ID != GLTexture_Load(GL_TEXTURE_2D, txTexture + (skin + numskins), newloadname, TRANSCOLOR ) )
    {
        numskins++;
        make_newloadname( tmploadname, "icon1", newloadname );
        if ( INVALID_TX_ID != GLTexture_Load(GL_TEXTURE_2D, TxIcon + globalnumicon, newloadname, INVALID_KEY ) )
        {
            while ( numicon < numskins )
            {
                madskintoicon[skin+numicon] = globalnumicon;
                numicon++;
            }

            globalnumicon++;
        }
    }

    make_newloadname( tmploadname, "tris2", newloadname );
    if ( INVALID_TX_ID != GLTexture_Load(GL_TEXTURE_2D, txTexture + (skin + numskins), newloadname, TRANSCOLOR ) )
    {
        numskins++;
        make_newloadname( tmploadname, "icon2", newloadname );
        if ( INVALID_TX_ID != GLTexture_Load(GL_TEXTURE_2D, TxIcon + globalnumicon, newloadname, INVALID_KEY ) )
        {
            while ( numicon < numskins )
            {
                madskintoicon[skin+numicon] = globalnumicon;
                numicon++;
            }

            globalnumicon++;
        }
    }

    make_newloadname( tmploadname, "tris3", newloadname );
    if ( INVALID_TX_ID != GLTexture_Load(GL_TEXTURE_2D, txTexture + (skin + numskins), newloadname, TRANSCOLOR ) )
    {
        numskins++;
        make_newloadname( tmploadname, "icon3", newloadname );
        if ( INVALID_TX_ID != GLTexture_Load(GL_TEXTURE_2D, TxIcon + globalnumicon, newloadname, INVALID_KEY ) )
        {
            while ( numicon < numskins )
            {
                madskintoicon[skin+numicon] = globalnumicon;
                numicon++;
            }

            globalnumicon++;
        }
    }

    if ( numskins == 0 )
    {
        // If we didn't get a skin, set it to the water texture
        madskinstart[object] = TX_WATER_TOP;
        numskins = 1;

        if (gDevMode) log_message( "NOTE: Object is missing an skin (%s)!\n", tmploadname );
    }

    madskins[object] = numskins;
    return numskins;
}
//--------------------------------------------------------------------------------------------
void load_bars( char* szBitmap )
{
    // ZZ> This function loads the status bar bitmap

    if ( INVALID_TX_ID == GLTexture_Load(GL_TEXTURE_2D, &TxBars, szBitmap, TRANSCOLOR ) )
    {
        log_warning( "Cannot load file! (basicdat" SLASH_STR "bars.bmp)\n" );
    }
}

//--------------------------------------------------------------------------------------------
void load_map( char* szModule )
{
    // ZZ> This function loads the map bitmap
    char szMap[256];

    // Turn it all off
    mapvalid = bfalse;
    mapon = bfalse;
    youarehereon = bfalse;
    numblip = 0;

    // Load the images
    sprintf( szMap, "%sgamedat" SLASH_STR "plan", szModule );

    if ( INVALID_TX_ID != GLTexture_Load(GL_TEXTURE_2D, &TxMap, szMap, INVALID_KEY ) )
    {
        log_warning( "Cannot load file! (basicdat" SLASH_STR "plan.bmp)\n" );
    }
    else
    {
        mapvalid = btrue;
    }

}

//--------------------------------------------------------------------------------------------
void font_init()
{
    GLTexture_new( &TxFont );

    font_release();
}

//--------------------------------------------------------------------------------------------
void font_release()
{
    // BB > fill in default values

    int i, ix, iy, cnt;
    float dx, dy;

    GLTexture_Release( &TxFont );

    // Mark all as unused
    for ( cnt = 0; cnt < 256; cnt++ )
    {
        asciitofont[cnt] = 255;
    }

    dx = 256 / NUMFONTX;
    dy = 256 / NUMFONTY;
    for ( i = 0; i < NUMFONT; i++ )
    {
        ix = i % NUMFONTX;
        iy = i / NUMFONTX;

        fontrect[cnt].x = ix * dx;
        fontrect[cnt].w = dx;
        fontrect[cnt].y = iy * dy;
        fontrect[cnt].h = dy;
        fontxspacing[cnt] = 0;
    }
    fontyspacing = dy;

};


//--------------------------------------------------------------------------------------------
void font_load( char* szBitmap, char* szSpacing )
{
    // ZZ> This function loads the font bitmap and sets up the coordinates
    //     of each font on that bitmap...  Bitmap must have 16x6 fonts
    int cnt, y, xsize, ysize, xdiv, ydiv;
    int xstt, ystt;
    int xspacing, yspacing;
    char cTmp;
    FILE *fileread;

    font_release();

    if ( INVALID_TX_ID == GLTexture_Load( GL_TEXTURE_2D, &TxFont, szBitmap, TRANSCOLOR ) )
    {
        log_error( "Cannot load file! (basicdat" SLASH_STR "fonts.bmp)\n" );
    }

    // Get the size of the bitmap
    xsize = GLTexture_GetImageWidth( &TxFont );
    ysize = GLTexture_GetImageHeight( &TxFont );

    if ( xsize == 0 || ysize == 0 )
    {
        log_error( "Bad font size! (%i, %i)\n", xsize, ysize );
    }

    // Figure out the general size of each font
    ydiv = ysize / NUMFONTY;
    xdiv = xsize / NUMFONTX;

    // Figure out where each font is and its spacing
    fileread = fopen( szSpacing, "r" );
    if ( fileread == NULL )
    {
        log_error( "Font spacing not avalible! (%i, %i)\n", xsize, ysize );
    }

    parse_filename = szSpacing;
    y = 0;
    xstt = 0;
    ystt = 0;

    // Uniform font height is at the top
    goto_colon( fileread );
    fscanf( fileread, "%d", &yspacing );
    fontoffset = scry - yspacing;

    for ( cnt = 0; cnt < NUMFONT && goto_colon_yesno( fileread ); cnt++ )
    {
        fscanf( fileread, "%c%d", &cTmp, &xspacing );

        if ( asciitofont[(Uint8)cTmp] == 255 ) asciitofont[(Uint8)cTmp] = (Uint8) cnt;

        if ( xstt + xspacing + 1 > 255 )
        {
            xstt = 0;
            ystt += yspacing;
        }

        fontrect[cnt].x = xstt;
        fontrect[cnt].w = xspacing;
        fontrect[cnt].y = ystt;
        fontrect[cnt].h = yspacing - 2;
        fontxspacing[cnt] = xspacing + 1;

        xstt += xspacing + 1;
    }
    fclose( fileread );

    // Space between lines
    fontyspacing = ( yspacing >> 1 ) + FONTADD;
}

//--------------------------------------------------------------------------------------------
void make_water()
{
    // ZZ> This function sets up water movements
    int layer, frame, point, mode, cnt;
    float temp;
    Uint8 spek;

    layer = 0;

    while ( layer < numwaterlayer )
    {
        if ( waterlight )  waterlayeralpha[layer] = 255;  // Some cards don't support alpha lights...

        waterlayeru[layer] = 0;
        waterlayerv[layer] = 0;
        frame = 0;

        while ( frame < MAXWATERFRAME )
        {
            // Do first mode
            mode = 0;

            for ( point = 0; point < WATERPOINTS; point++ )
            {
                temp = SIN( ( frame * TWO_PI / MAXWATERFRAME ) + ( TWO_PI * point / WATERPOINTS ) + ( TWO_PI * layer / MAXWATERLAYER ) );
                waterlayerzadd[layer][frame][mode][point] = temp * waterlayeramp[layer];
                waterlayercolor[layer][frame][mode][point] = ( waterlightlevel[layer] * ( temp + 1.0f ) ) + waterlightadd[layer];
            }

            // Now mirror and copy data to other three modes
            mode++;
            waterlayerzadd[layer][frame][mode][0] = waterlayerzadd[layer][frame][0][1];
            waterlayercolor[layer][frame][mode][0] = waterlayercolor[layer][frame][0][1];
            waterlayerzadd[layer][frame][mode][1] = waterlayerzadd[layer][frame][0][0];
            waterlayercolor[layer][frame][mode][1] = waterlayercolor[layer][frame][0][0];
            waterlayerzadd[layer][frame][mode][2] = waterlayerzadd[layer][frame][0][3];
            waterlayercolor[layer][frame][mode][2] = waterlayercolor[layer][frame][0][3];
            waterlayerzadd[layer][frame][mode][3] = waterlayerzadd[layer][frame][0][2];
            waterlayercolor[layer][frame][mode][3] = waterlayercolor[layer][frame][0][2];
            mode++;
            waterlayerzadd[layer][frame][mode][0] = waterlayerzadd[layer][frame][0][3];
            waterlayercolor[layer][frame][mode][0] = waterlayercolor[layer][frame][0][3];
            waterlayerzadd[layer][frame][mode][1] = waterlayerzadd[layer][frame][0][2];
            waterlayercolor[layer][frame][mode][1] = waterlayercolor[layer][frame][0][2];
            waterlayerzadd[layer][frame][mode][2] = waterlayerzadd[layer][frame][0][1];
            waterlayercolor[layer][frame][mode][2] = waterlayercolor[layer][frame][0][1];
            waterlayerzadd[layer][frame][mode][3] = waterlayerzadd[layer][frame][0][0];
            waterlayercolor[layer][frame][mode][3] = waterlayercolor[layer][frame][0][0];
            mode++;
            waterlayerzadd[layer][frame][mode][0] = waterlayerzadd[layer][frame][0][2];
            waterlayercolor[layer][frame][mode][0] = waterlayercolor[layer][frame][0][2];
            waterlayerzadd[layer][frame][mode][1] = waterlayerzadd[layer][frame][0][3];
            waterlayercolor[layer][frame][mode][1] = waterlayercolor[layer][frame][0][3];
            waterlayerzadd[layer][frame][mode][2] = waterlayerzadd[layer][frame][0][0];
            waterlayercolor[layer][frame][mode][2] = waterlayercolor[layer][frame][0][0];
            waterlayerzadd[layer][frame][mode][3] = waterlayerzadd[layer][frame][0][1];
            waterlayercolor[layer][frame][mode][3] = waterlayercolor[layer][frame][0][1];
            frame++;
        }

        layer++;
    }

    // Calculate specular highlights
    spek = 0;

    for ( cnt = 0; cnt < 256; cnt++ )
    {
        spek = 0;

        if ( cnt > waterspekstart )
        {
            temp = cnt - waterspekstart;
            temp = temp / ( 256 - waterspekstart );
            temp = temp * temp;
            spek = temp * waterspeklevel;
        }

        // [claforte] Probably need to replace this with a
        //            glColor4f(spek/256.0f, spek/256.0f, spek/256.0f, 1.0f) call:

        if ( shading == GL_FLAT )
            waterspek[cnt] = 0;
        else
            waterspek[cnt] = spek;
    }
}

//--------------------------------------------------------------------------------------------
void read_wawalite( char *modname )
{
    // ZZ> This function sets up water and lighting for the module
    char newloadname[256];
    FILE* fileread;
    float lx, ly, lz, la;
    float fTmp;
    char cTmp;
    int iTmp;

    make_newloadname( modname, "gamedat" SLASH_STR "wawalite.txt", newloadname );
    fileread = fopen( newloadname, "r" );

    if ( fileread )
    {
        goto_colon( fileread );
        //  !!!BAD!!!
        //  Random map...
        //  If someone else wants to handle this, here are some thoughts for approaching
        //  it.  The .MPD file for the level should give the basic size of the map.  Use
        //  a standard tile set like the Palace modules.  Only use objects that are in
        //  the module's object directory, and only use some of them.  Imagine several Rock
        //  Moles eating through a stone filled level to make a path from the entrance to
        //  the exit.  Door placement will be difficult.
        //  !!!BAD!!!

        // Read water data first
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  numwaterlayer = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterspekstart = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterspeklevel = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterdouselevel = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  watersurfacelevel = iTmp;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );

        if ( cTmp == 'T' || cTmp == 't' )  waterlight = btrue;
        else waterlight = bfalse;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        wateriswater = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  wateriswater = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );

        if ( ( cTmp == 'T' || cTmp == 't' ) && overlayvalid )  overlayon = btrue;
        else overlayon = bfalse;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );

        if ( ( cTmp == 'T' || cTmp == 't' ) && backgroundvalid )  clearson = bfalse;
        else clearson = btrue;

        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayerdistx[0] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayerdisty[0] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayerdistx[1] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayerdisty[1] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  foregroundrepeat = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  backgroundrepeat = iTmp;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlayerz[0] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlayeralpha[0] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlayerframeadd[0] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlightlevel[0] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlightadd[0] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayeramp[0] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayeruadd[0] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayervadd[0] = fTmp;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlayerz[1] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlayeralpha[1] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlayerframeadd[1] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlightlevel[1] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlightadd[1] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayeramp[1] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayeruadd[1] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayervadd[1] = fTmp;

        waterlayeru[0] = 0;
        waterlayerv[0] = 0;
        waterlayeru[1] = 0;
        waterlayerv[1] = 0;
        waterlayerframe[0] = rand() & WATERFRAMEAND;
        waterlayerframe[1] = rand() & WATERFRAMEAND;
        // Read light data second
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  lx = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  ly = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  lz = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  la = fTmp;
        // Read tile data third
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  hillslide = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  slippyfriction = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  airfriction = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterfriction = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  noslipfriction = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  gravity = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  animtileupdateand = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  animtileframeand = iTmp;
        biganimtileframeand = ( iTmp << 1 ) + 1;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  damagetileamount = iTmp;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );

        if ( cTmp == 'S' || cTmp == 's' )  damagetiletype = DAMAGE_SLASH;

        if ( cTmp == 'C' || cTmp == 'c' )  damagetiletype = DAMAGE_CRUSH;

        if ( cTmp == 'P' || cTmp == 'p' )  damagetiletype = DAMAGE_POKE;

        if ( cTmp == 'H' || cTmp == 'h' )  damagetiletype = DAMAGE_HOLY;

        if ( cTmp == 'E' || cTmp == 'e' )  damagetiletype = DAMAGE_EVIL;

        if ( cTmp == 'F' || cTmp == 'f' )  damagetiletype = DAMAGE_FIRE;

        if ( cTmp == 'I' || cTmp == 'i' )  damagetiletype = DAMAGE_ICE;

        if ( cTmp == 'Z' || cTmp == 'z' )  damagetiletype = DAMAGE_ZAP;

        // Read weather data fourth
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        weatheroverwater = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  weatheroverwater = btrue;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  weathertimereset = iTmp;
        weathertime = weathertimereset;
        weatherplayer = 0;
        // Read extra data
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        meshexploremode = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  meshexploremode = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        usefaredge = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  usefaredge = btrue;

        camswing = 0;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  camswingrate = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  camswingamp = fTmp;

        // Read unnecessary data...  Only read if it exists...
        fogon = bfalse;
        fogaffectswater = btrue;
        fogtop = 100;
        fogbottom = 0;
        fogdistance = 100;
        fogred = 255;
        foggrn = 255;
        fogblu = 255;
        damagetileparttype = -1;
        damagetilepartand = 255;
        damagetilesound = -1;
        damagetilesoundtime = TILESOUNDTIME;
        damagetilemindistance = 9999;

        if ( goto_colon_yesno( fileread ) )
        {
            fogon = fogallowed;
            fscanf( fileread, "%f", &fTmp );  fogtop = fTmp;
            goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  fogbottom = fTmp;
            goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  fogred = fTmp * 255;
            goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  foggrn = fTmp * 255;
            goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  fogblu = fTmp * 255;
            goto_colon( fileread );  cTmp = get_first_letter( fileread );

            if ( cTmp == 'F' || cTmp == 'f' )  fogaffectswater = bfalse;

            fogdistance = ( fogtop - fogbottom );

            if ( fogdistance < 1.0f )  fogon = bfalse;

            // Read extra stuff for damage tile particles...
            if ( goto_colon_yesno( fileread ) )
            {
                fscanf( fileread, "%d", &iTmp );  damagetileparttype = iTmp;
                goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
                damagetilepartand = iTmp;
                goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
                damagetilesound = CLIP(iTmp, -1, MAXWAVE);
            }
        }

        // Allow slow machines to ignore the fancy stuff
        if ( !twolayerwateron && numwaterlayer > 1 )
        {
            numwaterlayer = 1;
            iTmp = waterlayeralpha[0];
            iTmp = ( ( waterlayeralpha[1] * iTmp ) >> 8 ) + iTmp;

            if ( iTmp > 255 ) iTmp = 255;

            waterlayeralpha[0] = iTmp;
        }

        fclose( fileread );
        // Do it
        make_lighttable( lx, ly, lz, la );
        make_lighttospek();
        make_water();
    }
    else
    {
        log_error( "Could not read file! (wawalite.txt)\n" );
    }
}

//--------------------------------------------------------------------------------------------
void render_background( Uint16 texture )
{
    // ZZ> This function draws the large background
    GLVERTEX vtlist[4];
    float size;
    float sinsize, cossize;
    float x, y, z, u, v;
    float loc_backgroundrepeat;
    Uint8 i;

    // Figure out the coordinates of its corners
    x = scrx << 6;
    y = scry << 6;
    z = 0.99999f;
    size = x + y + 1;
    sinsize = turntosin[( 3*2047 ) & TRIG_TABLE_MASK] * size;   // why 3/8 of a turn???
    cossize = turntocos[( 3*2047 ) & TRIG_TABLE_MASK] * size;   // why 3/8 of a turn???
    u = waterlayeru[1];
    v = waterlayerv[1];
    loc_backgroundrepeat = backgroundrepeat * MIN( x / scrx, y / scrx );

    vtlist[0].x = x + cossize;
    vtlist[0].y = y - sinsize;
    vtlist[0].z = z;
    vtlist[0].s = 0 + u;
    vtlist[0].t = 0 + v;

    vtlist[1].x = x + sinsize;
    vtlist[1].y = y + cossize;
    vtlist[1].z = z;
    vtlist[1].s = loc_backgroundrepeat + u;
    vtlist[1].t = 0 + v;

    vtlist[2].x = x - cossize;
    vtlist[2].y = y + sinsize;
    vtlist[2].z = z;
    vtlist[2].s = loc_backgroundrepeat + u;
    vtlist[2].t = loc_backgroundrepeat + v;

    vtlist[3].x = x - sinsize;
    vtlist[3].y = y - cossize;
    vtlist[3].z = z;
    vtlist[3].s = 0 + u;
    vtlist[3].t = loc_backgroundrepeat + v;

    {
        GLint shading_save, depthfunc_save;
        GLboolean depthmask_save, cullface_save;

        glGetIntegerv( GL_SHADE_MODEL, &shading_save );
        glShadeModel( GL_FLAT );  // Flat shade this

        depthmask_save = glIsEnabled( GL_DEPTH_WRITEMASK );
        glDepthMask( GL_FALSE );

        glGetIntegerv( GL_DEPTH_FUNC, &depthfunc_save );
        glDepthFunc( GL_ALWAYS );

        cullface_save = glIsEnabled( GL_CULL_FACE );
        glDisable( GL_CULL_FACE );

        GLTexture_Bind( txTexture + texture );

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        glBegin ( GL_TRIANGLE_FAN );

        for ( i = 0; i < 4; i++ )
        {
            glTexCoord2f ( vtlist[i].s, vtlist[i].t );
            glVertex3f ( vtlist[i].x, vtlist[i].y, vtlist[i].z );
        }

        glEnd ();

        glDepthFunc( depthfunc_save );
        glDepthMask( depthmask_save );
        glShadeModel(shading_save);
        if (cullface_save) glEnable( GL_CULL_FACE ); else glDisable( GL_CULL_FACE );
    }
}

//--------------------------------------------------------------------------------------------
void render_foreground_overlay( Uint16 texture )
{
    // ZZ> This function draws the large foreground
    GLVERTEX vtlist[4];
    int i;
    float size;
    float sinsize, cossize;
    float x, y, z;
    float u, v;
    float loc_foregroundrepeat;

    // Figure out the screen coordinates of its corners
    x = scrx << 6;
    y = scry << 6;
    z = 0;
    u = waterlayeru[1];
    v = waterlayerv[1];
    size = x + y + 1;
    sinsize = turntosin[( 3*2047 ) & TRIG_TABLE_MASK] * size;
    cossize = turntocos[( 3*2047 ) & TRIG_TABLE_MASK] * size;
    loc_foregroundrepeat = foregroundrepeat * MIN( x / scrx, y / scrx );

    vtlist[0].x = x + cossize;
    vtlist[0].y = y - sinsize;
    vtlist[0].z = z;
    vtlist[0].s = 0 + u;
    vtlist[0].t = 0 + v;

    vtlist[1].x = x + sinsize;
    vtlist[1].y = y + cossize;
    vtlist[1].z = z;
    vtlist[1].s = loc_foregroundrepeat + u;
    vtlist[1].t = v;

    vtlist[2].x = x - cossize;
    vtlist[2].y = y + sinsize;
    vtlist[2].z = z;
    vtlist[2].s = loc_foregroundrepeat + u;
    vtlist[2].t = loc_foregroundrepeat + v;

    vtlist[3].x = x - sinsize;
    vtlist[3].y = y - cossize;
    vtlist[3].z = z;
    vtlist[3].s = 0 + u;
    vtlist[3].t = loc_foregroundrepeat + v;

    {
        GLint shading_save, depthfunc_save, smoothhint_save;
        GLboolean depthmask_save, cullface_save, alphatest_save;

        GLint alphatestfunc_save, alphatestref_save, alphablendsrc_save, alphablenddst_save;
        GLboolean alphablend_save;

        glGetIntegerv(GL_POLYGON_SMOOTH_HINT, &smoothhint_save);
        glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );             // make sure that the texture is as smooth as possible

        glGetIntegerv( GL_SHADE_MODEL, &shading_save );
        glShadeModel( GL_FLAT );  // Flat shade this

        depthmask_save = glIsEnabled( GL_DEPTH_WRITEMASK );
        glDepthMask( GL_FALSE );

        glGetIntegerv( GL_DEPTH_FUNC, &depthfunc_save );
        glDepthFunc( GL_ALWAYS );

        cullface_save = glIsEnabled( GL_CULL_FACE );
        glDisable( GL_CULL_FACE );

        alphatest_save = glIsEnabled( GL_ALPHA_TEST );
        glEnable( GL_ALPHA_TEST );

        glGetIntegerv( GL_ALPHA_TEST_FUNC, &alphatestfunc_save );
        glGetIntegerv( GL_ALPHA_TEST_REF, &alphatestref_save );
        glAlphaFunc( GL_GREATER, 0 );

        alphablend_save = glIsEnabled( GL_BLEND );
        glEnable( GL_BLEND );

        glGetIntegerv( GL_BLEND_SRC, &alphablendsrc_save );
        glGetIntegerv( GL_BLEND_DST, &alphablenddst_save );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );  // make the texture a filter

        GLTexture_Bind(txTexture + texture);

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        glBegin ( GL_TRIANGLE_FAN );

        for ( i = 0; i < 4; i++ )
        {
            glTexCoord2f ( vtlist[i].s, vtlist[i].t );
            glVertex3f ( vtlist[i].x, vtlist[i].y, vtlist[i].z );
        }

        glEnd ();

        glHint( GL_POLYGON_SMOOTH_HINT, smoothhint_save );
        glShadeModel( shading_save );
        glDepthMask( depthmask_save );
        glDepthFunc( depthfunc_save );

        if (cullface_save) glEnable( GL_CULL_FACE ); else glDisable( GL_CULL_FACE );
        if (alphatest_save) glEnable( GL_ALPHA_TEST ); else glDisable( GL_ALPHA_TEST );

        glAlphaFunc( alphatestfunc_save, alphatestref_save );

        if (alphablend_save) glEnable( GL_BLEND ); else glDisable( GL_BLEND );

        glBlendFunc( alphablendsrc_save, alphablenddst_save );
    }
}

//--------------------------------------------------------------------------------------------
void render_shadow( int character )
{
    // ZZ> This function draws a NIFTY shadow
    GLVERTEX v[4];

    float x, y;
    float level;
    float height, size_umbra, size_penumbra;
    float alpha_umbra, alpha_penumbra;
    Sint8 hide;
    int i;

    hide = caphidestate[chrmodel[character]];

    if ( hide == NOHIDE || hide != chraistate[character] )
    {
        // Original points
        level = chrlevel[character];
        level += SHADOWRAISE;
        height = chrmatrix[character].CNV( 3, 2 ) - level;

        if ( height < 0 ) height = 0;

        size_umbra    = 1.5f * ( chrbumpsize[character] - height / 30.0f );
        size_penumbra = 1.5f * ( chrbumpsize[character] + height / 30.0f );

        if ( height > 0 )
        {
            float factor_penumbra = ( 1.5f ) * ( ( chrbumpsize[character] ) / size_penumbra );
            float factor_umbra = ( 1.5f ) * ( ( chrbumpsize[character] ) / size_umbra );
            alpha_umbra = 0.3f / factor_umbra / factor_umbra / 1.5f;
            alpha_penumbra = 0.3f / factor_penumbra / factor_penumbra / 1.5f;
        }
        else
        {
            alpha_umbra    = 0.3f;
            alpha_penumbra = 0.3f;
        };

        x = chrmatrix[character].CNV( 3, 0 );

        y = chrmatrix[character].CNV( 3, 1 );

        // Choose texture.
        GLTexture_Bind( txTexture + particletexture );

        // GOOD SHADOW
        v[0].s = particleimageu[238][0];

        v[0].t = particleimagev[238][0];

        v[1].s = particleimageu[255][1];

        v[1].t = particleimagev[238][0];

        v[2].s = particleimageu[255][1];

        v[2].t = particleimagev[255][1];

        v[3].s = particleimageu[238][0];

        v[3].t = particleimagev[255][1];

        glEnable( GL_BLEND );

        glBlendFunc( GL_ZERO, GL_ONE_MINUS_SRC_COLOR );

        glDepthMask( GL_FALSE );

        if ( size_penumbra > 0 )
        {
            v[0].x = x + size_penumbra;
            v[0].y = y - size_penumbra;
            v[0].z = level;

            v[1].x = x + size_penumbra;
            v[1].y = y + size_penumbra;
            v[1].z = level;

            v[2].x = x - size_penumbra;
            v[2].y = y + size_penumbra;
            v[2].z = level;

            v[3].x = x - size_penumbra;
            v[3].y = y - size_penumbra;
            v[3].z = level;

            glBegin( GL_TRIANGLE_FAN );
            glColor4f( alpha_penumbra, alpha_penumbra, alpha_penumbra, 1.0f );

            for ( i = 0; i < 4; i++ )
            {
                glTexCoord2f ( v[i].s, v[i].t );
                glVertex3f ( v[i].x, v[i].y, v[i].z );
            }

            glEnd();
        };

        if ( size_umbra > 0 )
        {
            v[0].x = x + size_umbra;
            v[0].y = y - size_umbra;
            v[0].z = level + 0.1f;

            v[1].x = x + size_umbra;
            v[1].y = y + size_umbra;
            v[1].z = level + 0.1f;

            v[2].x = x - size_umbra;
            v[2].y = y + size_umbra;
            v[2].z = level + 0.1f;

            v[3].x = x - size_umbra;
            v[3].y = y - size_umbra;
            v[3].z = level + 0.1f;

            glBegin( GL_TRIANGLE_FAN );
            glColor4f( alpha_umbra, alpha_umbra, alpha_umbra, 1.0f );

            for ( i = 0; i < 4; i++ )
            {
                glTexCoord2f ( v[i].s, v[i].t );
                glVertex3f ( v[i].x, v[i].y, v[i].z );
            }

            glEnd();
        };

        glDisable( GL_BLEND );

        glDepthMask( GL_TRUE );
    };
}

//--------------------------------------------------------------------------------------------
void render_bad_shadow( int character )
{
    // ZZ> This function draws a sprite shadow
    GLVERTEX v[4];
    float size, x, y;
    Uint8 ambi;
    float level;
    int height;
    Sint8 hide;
    Uint8 trans;
    int i;

    hide = caphidestate[chrmodel[character]];

    if ( hide == NOHIDE || hide != chraistate[character] )
    {
        // Original points
        level = chrlevel[character];
        level += SHADOWRAISE;
        height = chrmatrix[character].CNV( 3, 2 ) - level;

        if ( height > 255 )  return;

        if ( height < 0 ) height = 0;

        size = chrshadowsize[character] - ( ( height * chrshadowsize[character] ) >> 8 );

        if ( size < 1 ) return;

        ambi = chrlightlevel[character] >> 4;
        trans = ( ( 255 - height ) >> 1 ) + 64;

        glColor4f( ambi / 255.0f, ambi / 255.0f, ambi / 255.0f, trans / 255.0f );

        x = chrmatrix[character].CNV( 3, 0 );
        y = chrmatrix[character].CNV( 3, 1 );
        v[0].x = ( float ) x + size;
        v[0].y = ( float ) y - size;
        v[0].z = ( float ) level;

        v[1].x = ( float ) x + size;
        v[1].y = ( float ) y + size;
        v[1].z = ( float ) level;

        v[2].x = ( float ) x - size;
        v[2].y = ( float ) y + size;
        v[2].z = ( float ) level;

        v[3].x = ( float ) x - size;
        v[3].y = ( float ) y - size;
        v[3].z = ( float ) level;

        // Choose texture and matrix
        GLTexture_Bind( txTexture + particletexture );

        v[0].s = particleimageu[236][0];
        v[0].t = particleimagev[236][0];

        v[1].s = particleimageu[253][1];
        v[1].t = particleimagev[236][0];

        v[2].s = particleimageu[253][1];
        v[2].t = particleimagev[253][1];

        v[3].s = particleimageu[236][0];
        v[3].t = particleimagev[253][1];

        glBegin( GL_TRIANGLE_FAN );

        for ( i = 0; i < 4; i++ )
        {
            glTexCoord2f ( v[i].s, v[i].t );
            glVertex3f ( v[i].x, v[i].y, v[i].z );
        }

        glEnd();
    }
}

//--------------------------------------------------------------------------------------------
void light_characters()
{
    // ZZ> This function figures out character lighting
    int cnt, tnc, x, y;
    Uint16 tl, tr, bl, br;
    Uint16 light;

    cnt = 0;

    while ( cnt < numdolist )
    {
        tnc = dolist[cnt];
        x = chrxpos[tnc];
        y = chrypos[tnc];
        x = ( x & 127 ) >> 5;  // From 0 to 3
        y = ( y & 127 ) >> 5;  // From 0 to 3
        light = 0;
        tl = meshvrtl[meshvrtstart[chronwhichfan[tnc]] + 0];
        tr = meshvrtl[meshvrtstart[chronwhichfan[tnc]] + 1];
        br = meshvrtl[meshvrtstart[chronwhichfan[tnc]] + 2];
        bl = meshvrtl[meshvrtstart[chronwhichfan[tnc]] + 3];

        // Interpolate lighting level using tile corners
        switch ( x )
        {
            case 0:
                light += tl << 1;
                light += bl << 1;
                break;
            case 1:
            case 2:
                light += tl;
                light += tr;
                light += bl;
                light += br;
                break;
            case 3:
                light += tr << 1;
                light += br << 1;
                break;
        }

        switch ( y )
        {
            case 0:
                light += tl << 1;
                light += tr << 1;
                break;
            case 1:
            case 2:
                light += tl;
                light += tr;
                light += bl;
                light += br;
                break;
            case 3:
                light += bl << 1;
                light += br << 1;
                break;
        }

        light = light >> 3;
        chrlightlevel[tnc] = light;

        if ( !meshexploremode )
        {
            // Look up light direction using corners again
            tl = ( tl << 8 ) & 0xf000;
            tr = ( tr << 4 ) & 0x0f00;
            br = ( br ) & 0x00f0;
            bl = bl >> 4;
            tl = tl | tr | br | bl;
            chrlightturnleftright[tnc] = ( lightdirectionlookup[tl] << 8 );
        }
        else
        {
            chrlightturnleftright[tnc] = 0;
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void light_particles()
{
    // ZZ> This function figures out particle lighting
    int cnt;
    int character;

    cnt = 0;

    while ( cnt < maxparticles )
    {
        if ( prton[cnt] )
        {
            character = prtattachedtocharacter[cnt];

            if ( character != MAXCHR )
            {
                prtlight[cnt] = chrlightlevel[character];
            }
            else
            {
                prtlight[cnt] = meshvrtl[meshvrtstart[prtonwhichfan[cnt]]];
            }
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void set_fan_light( int fanx, int fany, Uint16 particle )
{
    // ZZ> This function is a little helper, lighting the selected fan
    //     with the chosen particle
    float x, y;
    int fan, vertex, lastvertex;
    float level;
    float light;

    if ( fanx >= 0 && fanx < meshsizex && fany >= 0 && fany < meshsizey )
    {
        fan = fanx + meshfanstart[fany];
        vertex = meshvrtstart[fan];
        lastvertex = vertex + meshcommandnumvertices[meshtype[fan]];

        while ( vertex < lastvertex )
        {
            light = meshvrta[vertex];
            x = prtxpos[particle] - meshvrtx[vertex];
            y = prtypos[particle] - meshvrty[vertex];
            level = ( x * x + y * y ) / prtdynalightfalloff[particle];
            level = 255 - level;
            level = level * prtdynalightlevel[particle];

            if ( level > light )
            {
                if ( level > 255 ) level = 255;

                meshvrtl[vertex] = level;
                meshvrta[vertex] = level;
            }

            vertex++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void do_dynalight()
{
    // ZZ> This function does dynamic lighting of visible fans

    int cnt, lastvertex, vertex, fan, entry, fanx, fany, addx, addy;
    float x, y;
    float level;
    float light;

    // Do each floor tile
    if ( meshexploremode )
    {
        // Set base light level in explore mode...  Don't need to do every frame
        if ( ( allframe & 7 ) == 0 )
        {
            cnt = 0;

            while ( cnt < maxparticles )
            {
                if ( prton[cnt] && prtdynalighton[cnt] )
                {
                    fanx = prtxpos[cnt];
                    fany = prtypos[cnt];
                    fanx = fanx >> 7;
                    fany = fany >> 7;
                    addy = -DYNAFANS;

                    while ( addy <= DYNAFANS )
                    {
                        addx = -DYNAFANS;

                        while ( addx <= DYNAFANS )
                        {
                            set_fan_light( fanx + addx, fany + addy, cnt );
                            addx++;
                        }

                        addy++;
                    }
                }

                cnt++;
            }
        }
    }
    else
    {
        if ( shading != GL_FLAT )
        {
            // Add to base light level in normal mode
            entry = 0;

            while ( entry < nummeshrenderlist )
            {
                fan = meshrenderlist[entry];
                vertex = meshvrtstart[fan];
                lastvertex = vertex + meshcommandnumvertices[meshtype[fan]];

                while ( vertex < lastvertex )
                {
                    // Do light particles
                    light = meshvrta[vertex];
                    cnt = 0;

                    while ( cnt < numdynalight )
                    {
                        x = dynalightlistx[cnt] - meshvrtx[vertex];
                        y = dynalightlisty[cnt] - meshvrty[vertex];
                        level = ( x * x + y * y ) / dynalightfalloff[cnt];
                        level = 255 - level;

                        if ( level > 0 )
                        {
                            light += level * dynalightlevel[cnt];
                        }

                        cnt++;
                    }

                    if ( light > 255 ) light = 255;

                    if ( light < 0 ) light = 0;

                    meshvrtl[vertex] = light;
                    vertex++;
                }

                entry++;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void render_water()
{
    // ZZ> This function draws all of the water fans

    int cnt;

    // Set the transformation thing
    glLoadMatrixf( mView.v );
    glMultMatrixf( mWorld.v );

    // Bottom layer first
    if ( numwaterlayer > 1 && waterlayerz[1] > -waterlayeramp[1] )
    {
        cnt = 0;

        while ( cnt < nummeshrenderlist )
        {
            if ( meshfx[meshrenderlist[cnt]]&MESHFXWATER )
            {
                // !!!BAD!!! Water will get screwed up if meshsizex is odd
                render_water_fan( meshrenderlist[cnt], 1 );
            }

            cnt++;
        }
    }

    // Top layer second
    if ( numwaterlayer > 0 && waterlayerz[0] > -waterlayeramp[0] )
    {
        cnt = 0;

        while ( cnt < nummeshrenderlist )
        {
            if ( meshfx[meshrenderlist[cnt]]&MESHFXWATER )
            {
                // !!!BAD!!! Water will get screwed up if meshsizex is odd
                render_water_fan( meshrenderlist[cnt], 0 );
            }

            cnt++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void draw_scene_sadreflection()
{
    // ZZ> This function draws 3D objects
    Uint16 cnt, tnc;
    Uint8 trans;
    rect_t rect;// = {0, 0, scrx, scry};  // Don't know why this isn't working on the Mac, it should

    rect.left = 0;
    rect.right = 0;
    rect.top = scrx;
    rect.bottom = scry;

    // ZB> Clear the z-buffer
    glClear( GL_DEPTH_BUFFER_BIT );

    // Render the reflective floors
    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CW );
    meshlasttexture = 0;

    for ( cnt = 0; cnt < nummeshrenderlistref; cnt++ )
    {
        render_fan( meshrenderlistref[cnt] );
    }

    if ( refon )
    {
        // Render reflections of characters

        glEnable( GL_CULL_FACE );
        glFrontFace( GL_CCW );

        glDisable( GL_DEPTH_TEST );
        glDepthMask( GL_FALSE );

        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );

        for ( cnt = 0; cnt < numdolist; cnt++ )
        {
            tnc = dolist[cnt];

            if ( ( meshfx[chronwhichfan[tnc]]&MESHFXDRAWREF ) )
            {
                render_refmad( tnc, (chralpha[tnc] * chrlight[tnc]) >> 8 );
            }
        }

        // Render the reflected sprites
        glFrontFace( GL_CW );
        render_refprt();

        glDisable( GL_BLEND );
        glEnable( GL_DEPTH_TEST );
        glDepthMask( GL_TRUE );
    }

    // Render the shadow floors
    meshlasttexture = 0;

    for ( cnt = 0; cnt < nummeshrenderlistsha; cnt++ )
    {
        render_fan( meshrenderlistsha[cnt] );
    }

    // Render the shadows
    if ( shaon )
    {
        if ( shasprite )
        {
            // Bad shadows
            glDepthMask( GL_FALSE );
            glEnable( GL_BLEND );
            glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            for ( cnt = 0; cnt < numdolist; cnt++ )
            {
                tnc = dolist[cnt];

                if ( chrattachedto[tnc] == MAXCHR )
                {
                    if ( ( ( chrlight[tnc] == 255 && chralpha[tnc] == 255 ) || capforceshadow[chrmodel[tnc]] ) && chrshadowsize[tnc] != 0 )
                    {
                        render_bad_shadow( tnc );
                    }
                }
            }

            glDisable( GL_BLEND );
            glDepthMask( GL_TRUE );
        }
        else
        {
            // Good shadows for me
            glDepthMask( GL_FALSE );
            glEnable( GL_BLEND );
            glBlendFunc( GL_SRC_COLOR, GL_ZERO );

            for ( cnt = 0; cnt < numdolist; cnt++ )
            {
                tnc = dolist[cnt];

                if ( chrattachedto[tnc] == MAXCHR )
                {
                    if ( ( ( chrlight[tnc] == 255 && chralpha[tnc] == 255 ) || capforceshadow[chrmodel[tnc]] ) && chrshadowsize[tnc] != 0 )
                    {
                        render_shadow( tnc );
                    }
                }
            }

            glDisable( GL_BLEND );
            glDepthMask( GL_TRUE );
        }
    }

    glAlphaFunc( GL_GREATER, 0 );
    glEnable( GL_ALPHA_TEST );
    glDisable( GL_CULL_FACE );

    // Render the normal characters
    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
        tnc = dolist[cnt];

        if ( chralpha[tnc] == 255 && chrlight[tnc] == 255 )
            render_mad( tnc, 255 );
    }

    // Render the sprites
    glDepthMask( GL_FALSE );
    glEnable( GL_BLEND );

    // Now render the transparent characters
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
        tnc = dolist[cnt];

        if ( chralpha[tnc] != 255 && chrlight[tnc] == 255 )
        {
            trans = chralpha[tnc];

            if ( trans < SEEINVISIBLE && ( local_seeinvisible || chrislocalplayer[tnc] ) )  trans = SEEINVISIBLE;

            render_mad( tnc, trans );
        }
    }

    // Alpha water
    if ( !waterlight )  render_water();

    // Then do the light characters
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
        tnc = dolist[cnt];

        if ( chrlight[tnc] != 255 )
        {
            trans = chrlight[tnc];

            if ( trans < SEEINVISIBLE && ( local_seeinvisible || chrislocalplayer[tnc] ) )  trans = SEEINVISIBLE;

            render_mad( tnc, trans );
        }

        // Do phong highlights
        if ( phongon && chralpha[tnc] == 255 && chrlight[tnc] == 255 && !chrenviro[tnc] && chrsheen[tnc] > 0 )
        {
            Uint16 texturesave;
            chrenviro[tnc] = btrue;
            texturesave = chrtexture[tnc];
            chrtexture[tnc] = 7;  // The phong map texture...
            render_mad( tnc, chrsheen[tnc] << 4 );
            chrtexture[tnc] = texturesave;
            chrenviro[tnc] = bfalse;
        }
    }

    // Light water
    if ( waterlight )  render_water();

    // Turn Z buffer back on, alphablend off
    glDepthMask( GL_TRUE );
    glDisable( GL_BLEND );
    glEnable( GL_ALPHA_TEST );
    render_prt();
    glDisable( GL_ALPHA_TEST );

    glDepthMask( GL_TRUE );
    glDisable( GL_BLEND );

    // Done rendering
}

//--------------------------------------------------------------------------------------------
void draw_scene_zreflection()
{
    // ZZ> This function draws 3D objects
    Uint16 cnt, tnc;
    Uint8 trans;

    // Clear the image if need be
    // PORT: I don't think this is needed if(clearson) { clear_surface(lpDDSBack); }
    // Zbuffer is cleared later

    // Render the reflective floors
    glDisable( GL_DEPTH_TEST );
    glDepthMask( GL_FALSE );

    meshlasttexture = 0;
    for ( cnt = 0; cnt < nummeshrenderlistref; cnt++ )
    {
        render_fan( meshrenderlistref[cnt] );
    }

    // BAD: DRAW SHADOW STUFF TOO
    for ( cnt = 0; cnt < nummeshrenderlistsha; cnt++ )
    {
        render_fan( meshrenderlistsha[cnt] );
    }

    glEnable( GL_DEPTH_TEST );
    glDepthMask( GL_TRUE );

    if ( refon )
    {
        // Render reflections of characters
        glFrontFace( GL_CCW );
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        glDepthFunc( GL_LEQUAL );

        for ( cnt = 0; cnt < numdolist; cnt++ )
        {
            tnc = dolist[cnt];

            if ( ( meshfx[chronwhichfan[tnc]]&MESHFXDRAWREF ) )
                render_refmad( tnc, (chralpha[tnc] * chrlight[tnc]) >> 8 );
        }

        // [claforte] I think this is wrong... I think we should choose some other depth func.
        glDepthFunc( GL_ALWAYS );

        // Render the reflected sprites
        glDisable( GL_DEPTH_TEST );
        glDepthMask( GL_FALSE );
        glFrontFace( GL_CW );
        render_refprt();

        glDisable( GL_BLEND );
        glDepthFunc( GL_LEQUAL );
        glEnable( GL_DEPTH_TEST );
        glDepthMask( GL_TRUE );
    }

    // Clear the Zbuffer at a bad time...  But hey, reflections work with Voodoo
    // lpD3DVViewport->Clear(1, &rect, D3DCLEAR_ZBUFFER);
    // Not sure if this is cool or not - DDOI
    // glClear ( GL_DEPTH_BUFFER_BIT );

    // Render the shadow floors
    meshlasttexture = 0;

    for ( cnt = 0; cnt < nummeshrenderlistsha; cnt++ )
    {
        render_fan( meshrenderlistsha[cnt] );
    }

    // Render the shadows
    if ( shaon )
    {
        if ( shasprite )
        {
            // Bad shadows
            glDepthMask( GL_FALSE );
            glEnable( GL_BLEND );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            for ( cnt = 0; cnt < numdolist; cnt++ )
            {
                tnc = dolist[cnt];

                if ( chrattachedto[tnc] == MAXCHR )
                {
                    if ( ( ( chrlight[tnc] == 255 && chralpha[tnc] == 255 ) || capforceshadow[chrmodel[tnc]] ) && chrshadowsize[tnc] != 0 )
                        render_bad_shadow( tnc );
                }
            }

            glDisable( GL_BLEND );
            glDepthMask( GL_TRUE );
        }
        else
        {
            // Good shadows for me
            glDepthMask( GL_FALSE );
            glEnable( GL_BLEND );
            glBlendFunc( GL_SRC_COLOR, GL_ZERO );

            for ( cnt = 0; cnt < numdolist; cnt++ )
            {
                tnc = dolist[cnt];

                if ( chrattachedto[tnc] == MAXCHR )
                {
                    if ( ( ( chrlight[tnc] == 255 && chralpha[tnc] == 255 ) || capforceshadow[chrmodel[tnc]] ) && chrshadowsize[tnc] != 0 )
                        render_shadow( tnc );
                }
            }

            glDisable( GL_BLEND );
            glDepthMask ( GL_TRUE );
        }
    }

    glAlphaFunc( GL_GREATER, 0 );
    glEnable( GL_ALPHA_TEST );

    // Render the normal characters
    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
        tnc = dolist[cnt];

        if ( chralpha[tnc] == 255 && chrlight[tnc] == 255 )
            render_mad( tnc, 255 );
    }

    // Render the sprites
    glDepthMask ( GL_FALSE );
    glEnable( GL_BLEND );

    // Now render the transparent characters
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
        tnc = dolist[cnt];

        if ( chralpha[tnc] != 255 && chrlight[tnc] == 255 )
        {
            trans = chralpha[tnc];

            if ( trans < SEEINVISIBLE && ( local_seeinvisible || chrislocalplayer[tnc] ) )  trans = SEEINVISIBLE;

            render_mad( tnc, trans );
        }
    }

    // And alpha water floors
    if ( !waterlight )
        render_water();

    // Then do the light characters
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
        tnc = dolist[cnt];

        if ( chrlight[tnc] != 255 )
        {
            trans = chrlight[tnc];

            if ( trans < SEEINVISIBLE && ( local_seeinvisible || chrislocalplayer[tnc] ) )  trans = SEEINVISIBLE;

            render_mad( tnc, trans );
        }

        // Do phong highlights
        if ( phongon && chralpha[tnc] == 255 && chrlight[tnc] == 255 && !chrenviro[tnc] && chrsheen[tnc] > 0 )
        {
            Uint16 texturesave;
            chrenviro[tnc] = btrue;
            texturesave = chrtexture[tnc];
            chrtexture[tnc] = 7;  // The phong map texture...
            render_mad( tnc, chrsheen[tnc] << 4 );
            chrtexture[tnc] = texturesave;
            chrenviro[tnc] = bfalse;
        }
    }

    // Do light water
    if ( waterlight )
        render_water();

    // Turn Z buffer back on, alphablend off
    glDepthMask( GL_TRUE );
    glDisable( GL_BLEND );
    glEnable( GL_ALPHA_TEST );
    render_prt();
    glDisable( GL_ALPHA_TEST );

    glDepthMask( GL_TRUE );

    glDisable( GL_BLEND );

    // Done rendering
}

//--------------------------------------------------------------------------------------------
bool_t get_mesh_memory()
{
    // ZZ> This function gets a load of memory for the terrain mesh
    floatmemory = ( float * ) malloc( maxtotalmeshvertices * BYTESFOREACHVERTEX );

    if ( floatmemory == NULL ) return bfalse;

    meshvrtx = floatmemory;
    meshvrty = meshvrtx + maxtotalmeshvertices;
    meshvrtz = meshvrty + maxtotalmeshvertices;
    meshvrta = ( Uint8 * ) (meshvrtz + maxtotalmeshvertices);
    meshvrtl = meshvrta + maxtotalmeshvertices;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void draw_blip( float sizeFactor, Uint8 color, int x, int y )

{
    float xl, xr, yt, yb;
    int width, height;

    // ZZ> This function draws a blip
    if ( x > 0 && y > 0 )
    {
        EnableTexturing();
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        glNormal3f( 0.0f, 0.0f, 1.0f );

        GLTexture_Bind( &TxBlip );

        xl = ( ( float )bliprect[color].left ) / (float)TxBlip.txW;
        xr = ( ( float )bliprect[color].right ) / (float)TxBlip.txW;
        yt = ( ( float )bliprect[color].top ) / (float)TxBlip.txH;
        yb = ( ( float )bliprect[color].bottom ) / (float)TxBlip.txH;
        width = bliprect[color].right - bliprect[color].left;
        height = bliprect[color].bottom - bliprect[color].top;

        width *= sizeFactor; height *= sizeFactor;
        glBegin( GL_QUADS );
        glTexCoord2f( xl, yb ); glVertex2i( x - 1 - (width / 2), scry - y - 1 - (height / 2) );
        glTexCoord2f( xr, yb ); glVertex2i( x - 1 + (width / 2), scry - y - 1 - (height / 2) );
        glTexCoord2f( xr, yt ); glVertex2i( x - 1 + (width / 2), scry - y - 1 + (height / 2) );
        glTexCoord2f( xl, yt ); glVertex2i( x - 1 - (width / 2), scry - y - 1 + (height / 2) );
        glEnd();

    }
}

//--------------------------------------------------------------------------------------------
void draw_one_icon( int icontype, int x, int y, Uint8 sparkle )
{
    // ZZ> This function draws an icon
    int position, blipx, blipy;
    float xl, xr, yt, yb;
    int width, height;

    if ( TxIcon[icontype].textureID >= 0 ) // if(lpDDSIcon[icontype])
    {
        EnableTexturing();    // Enable texture mapping
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

        GLTexture_Bind( TxIcon + icontype );

        xl = ( ( float )iconrect.left ) / 32;
        xr = ( ( float )iconrect.right ) / 32;
        yt = ( ( float )iconrect.top ) / 32;
        yb = ( ( float )iconrect.bottom ) / 32;
        width = iconrect.right - iconrect.left; height = iconrect.bottom - iconrect.top;
        glBegin( GL_QUADS );
        glTexCoord2f( xl, yb );   glVertex2i( x,       scry - y - height );
        glTexCoord2f( xr, yb );   glVertex2i( x + width, scry - y - height );
        glTexCoord2f( xr, yt );   glVertex2i( x + width, scry - y );
        glTexCoord2f( xl, yt );   glVertex2i( x,       scry - y );
        glEnd();
    }

    if ( sparkle != NOSPARKLE )
    {
        position = wldframe & 31;
        position = ( SPARKLESIZE * position >> 5 );

        blipx = x + SPARKLEADD + position;
        blipy = y + SPARKLEADD;
        draw_blip(0.5f, sparkle, blipx, blipy );

        blipx = x + SPARKLEADD + SPARKLESIZE;
        blipy = y + SPARKLEADD + position;
        draw_blip(0.5f, sparkle, blipx, blipy );

        blipx = blipx - position;
        blipy = y + SPARKLEADD + SPARKLESIZE;
        draw_blip(0.5f, sparkle, blipx, blipy );

        blipx = x + SPARKLEADD;
        blipy = blipy - position;
        draw_blip(0.5f, sparkle, blipx, blipy );
    }
}

//--------------------------------------------------------------------------------------------
void draw_one_font( int fonttype, int x, int y )
{
    // ZZ> This function draws a letter or number
    // GAC> Very nasty version for starters.  Lots of room for improvement.
    GLfloat dx, dy, fx1, fx2, fy1, fy2, border;
    GLuint x2, y2;

    y = fontoffset - y;
    x2 = x + fontrect[fonttype].w;
    y2 = y + fontrect[fonttype].h;

    dx = 2.0f / 512.0f;
    dy = 1.0f / 256.0f;
    border = 1.0f / 512.0f;

    fx1 = fontrect[fonttype].x * dx + border;
    fx2 = ( fontrect[fonttype].x + fontrect[fonttype].w ) * dx - border;
    fy1 = fontrect[fonttype].y * dy + border;
    fy2 = ( fontrect[fonttype].y + fontrect[fonttype].h ) * dy - border;

    glBegin( GL_QUADS );
    glTexCoord2f( fx1, fy2 );   glVertex2i( x, y );
    glTexCoord2f( fx2, fy2 );   glVertex2i( x2, y );
    glTexCoord2f( fx2, fy1 );   glVertex2i( x2, y2 );
    glTexCoord2f( fx1, fy1 );   glVertex2i( x, y2 );
    glEnd();
}

//--------------------------------------------------------------------------------------------
void draw_map( int x, int y )
{
    // ZZ> This function draws the map
    EnableTexturing();
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    //glNormal3f( 0.0f, 0.0f, 1.0f );

    GLTexture_Bind( &TxMap );

    glBegin( GL_QUADS );
    glTexCoord2f ( 0.0f, 1.0f ); glVertex2i ( x,   scry - y - MAPSIZE );
    glTexCoord2f ( 1.0f, 1.0f ); glVertex2i ( x + MAPSIZE, scry - y - MAPSIZE );
    glTexCoord2f ( 1.0f, 0.0f ); glVertex2i ( x + MAPSIZE, scry - y );
    glTexCoord2f ( 0.0f, 0.0f ); glVertex2i ( x,   scry - y );
    glEnd();
}

//--------------------------------------------------------------------------------------------
int draw_one_bar( int bartype, int x, int y, int ticks, int maxticks )
{
    // ZZ> This function draws a bar and returns the y position for the next one
    int noticks;
    float xl, xr, yt, yb;
    int width, height;

    EnableTexturing();               // Enable texture mapping
    glColor4f( 1, 1, 1, 1 );

    if ( maxticks > 0 && ticks >= 0 )
    {
        // Draw the tab
        GLTexture_Bind( &TxBars );

        xl = ( ( float )tabrect[bartype].left ) / 128;
        xr = ( ( float )tabrect[bartype].right ) / 128;
        yt = ( ( float )tabrect[bartype].top ) / 128;
        yb = ( ( float )tabrect[bartype].bottom ) / 128;
        width = tabrect[bartype].right - tabrect[bartype].left; height = tabrect[bartype].bottom - tabrect[bartype].top;
        glBegin( GL_QUADS );
        glTexCoord2f( xl, yb );   glVertex2i( x,       scry - y - height );
        glTexCoord2f( xr, yb );   glVertex2i( x + width, scry - y - height );
        glTexCoord2f( xr, yt );   glVertex2i( x + width, scry - y );
        glTexCoord2f( xl, yt );   glVertex2i( x,       scry - y );
        glEnd();

        // Error check
        if ( maxticks > MAXTICK ) maxticks = MAXTICK;

        if ( ticks > maxticks ) ticks = maxticks;

        // Draw the full rows of ticks
        x += TABX;

        while ( ticks >= NUMTICK )
        {
            barrect[bartype].right = BARX;
            GLTexture_Bind( &TxBars );

            xl = ( ( float )barrect[bartype].left ) / 128;
            xr = ( ( float )barrect[bartype].right ) / 128;
            yt = ( ( float )barrect[bartype].top ) / 128;
            yb = ( ( float )barrect[bartype].bottom ) / 128;
            width = barrect[bartype].right - barrect[bartype].left; height = barrect[bartype].bottom - barrect[bartype].top;
            glBegin( GL_QUADS );
            glTexCoord2f( xl, yb );   glVertex2i( x,       scry - y - height );
            glTexCoord2f( xr, yb );   glVertex2i( x + width, scry - y - height );
            glTexCoord2f( xr, yt );   glVertex2i( x + width, scry - y );
            glTexCoord2f( xl, yt );   glVertex2i( x,       scry - y );
            glEnd();
            y += BARY;
            ticks -= NUMTICK;
            maxticks -= NUMTICK;
        }

        // Draw any partial rows of ticks
        if ( maxticks > 0 )
        {
            // Draw the filled ones
            barrect[bartype].right = ( ticks << 3 ) + TABX;
            GLTexture_Bind( &TxBars );

            xl = ( ( float )barrect[bartype].left ) / 128;
            xr = ( ( float )barrect[bartype].right ) / 128;
            yt = ( ( float )barrect[bartype].top ) / 128;
            yb = ( ( float )barrect[bartype].bottom ) / 128;
            width = barrect[bartype].right - barrect[bartype].left; height = barrect[bartype].bottom - barrect[bartype].top;
            glBegin( GL_QUADS );
            glTexCoord2f( xl, yb );   glVertex2i( x,       scry - y - height );
            glTexCoord2f( xr, yb );   glVertex2i( x + width, scry - y - height );
            glTexCoord2f( xr, yt );   glVertex2i( x + width, scry - y );
            glTexCoord2f( xl, yt );   glVertex2i( x,       scry - y );
            glEnd();

            // Draw the empty ones
            noticks = maxticks - ticks;

            if ( noticks > ( NUMTICK - ticks ) ) noticks = ( NUMTICK - ticks );

            barrect[0].right = ( noticks << 3 ) + TABX;
            GLTexture_Bind( &TxBars );

            xl = ( ( float )barrect[0].left ) / 128;
            xr = ( ( float )barrect[0].right ) / 128;
            yt = ( ( float )barrect[0].top ) / 128;
            yb = ( ( float )barrect[0].bottom ) / 128;
            width = barrect[0].right - barrect[0].left; height = barrect[0].bottom - barrect[0].top;
            glBegin( GL_QUADS );
            glTexCoord2f( xl, yb );   glVertex2i( ( ticks << 3 ) + x,       scry - y - height );
            glTexCoord2f( xr, yb );   glVertex2i( ( ticks << 3 ) + x + width, scry - y - height );
            glTexCoord2f( xr, yt );   glVertex2i( ( ticks << 3 ) + x + width, scry - y );
            glTexCoord2f( xl, yt );   glVertex2i( ( ticks << 3 ) + x,       scry - y );
            glEnd();
            maxticks -= NUMTICK;
            y += BARY;
        }

        // Draw full rows of empty ticks
        while ( maxticks >= NUMTICK )
        {
            barrect[0].right = BARX;
            GLTexture_Bind( &TxBars );

            xl = ( ( float )barrect[0].left ) / 128;
            xr = ( ( float )barrect[0].right ) / 128;
            yt = ( ( float )barrect[0].top ) / 128;
            yb = ( ( float )barrect[0].bottom ) / 128;
            width = barrect[0].right - barrect[0].left; height = barrect[0].bottom - barrect[0].top;
            glBegin( GL_QUADS );
            glTexCoord2f( xl, yb );   glVertex2i( x,       scry - y - height );
            glTexCoord2f( xr, yb );   glVertex2i( x + width, scry - y - height );
            glTexCoord2f( xr, yt );   glVertex2i( x + width, scry - y );
            glTexCoord2f( xl, yt );   glVertex2i( x,       scry - y );
            glEnd();
            y += BARY;
            maxticks -= NUMTICK;
        }

        // Draw the last of the empty ones
        if ( maxticks > 0 )
        {
            barrect[0].right = ( maxticks << 3 ) + TABX;
            GLTexture_Bind( &TxBars );

            xl = ( ( float )barrect[0].left ) / 128;
            xr = ( ( float )barrect[0].right ) / 128;
            yt = ( ( float )barrect[0].top ) / 128;
            yb = ( ( float )barrect[0].bottom ) / 128;
            width = barrect[0].right - barrect[0].left; height = barrect[0].bottom - barrect[0].top;
            glBegin( GL_QUADS );
            glTexCoord2f( xl, yb );   glVertex2i( x,       scry - y - height );
            glTexCoord2f( xr, yb );   glVertex2i( x + width, scry - y - height );
            glTexCoord2f( xr, yt );   glVertex2i( x + width, scry - y );
            glTexCoord2f( xl, yt );   glVertex2i( x,       scry - y );
            glEnd();
            y += BARY;
        }
    }

    return y;
}

//--------------------------------------------------------------------------------------------
void BeginText()
{
    EnableTexturing();    // Enable texture mapping

    GLTexture_Bind( &TxFont );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_CULL_FACE );

    glColor4f( 1, 1, 1, 1 );
}

//--------------------------------------------------------------------------------------------
void EndText()
{
    glDisable( GL_BLEND );
    glDisable( GL_ALPHA_TEST );
}

//--------------------------------------------------------------------------------------------
void draw_string( char *szText, int x, int y )
{
    // ZZ> This function spits a line of null terminated text onto the backbuffer
    Uint8 cTmp = szText[0];
    int cnt = 1;
    int x_stt;

    BeginText();

    x_stt = x;
    cnt = 0;
    cTmp = szText[cnt];
    while ( '\0' != cTmp )
    {
        // Convert ASCII to our own little font
        if ( cTmp == '~' )
        {
            // Use squiggle for tab
            x = (x + TABAND) & ( ~TABAND );
        }
        else if ( cTmp == '\n' )
        {
            x  = x_stt;
            y += fontyspacing;
        }
        else
        {
            // Normal letter
            cTmp = asciitofont[cTmp];
            draw_one_font( cTmp, x, y );
            x += fontxspacing[cTmp];
        }

        cnt++;
        cTmp = szText[cnt];
    }

    EndText();
}

//--------------------------------------------------------------------------------------------
int length_of_word( char *szText )
{
    // ZZ> This function returns the number of pixels the
    //     next word will take on screen in the x direction

    // Count all preceeding spaces
    int x = 0;
    int cnt = 0;
    Uint8 cTmp = szText[cnt];

    while ( cTmp == ' ' || cTmp == '~' || cTmp == '\n' )
    {
        if ( cTmp == ' ' )
        {
            x += fontxspacing[asciitofont[cTmp]];
        }
        else if ( cTmp == '~' )
        {
            x += TABAND + 1;
        }

        cnt++;
        cTmp = szText[cnt];
    }

    while ( cTmp != ' ' && cTmp != '~' && cTmp != '\n' && cTmp != 0 )
    {
        x += fontxspacing[asciitofont[cTmp]];
        cnt++;
        cTmp = szText[cnt];
    }

    return x;
}

//--------------------------------------------------------------------------------------------
int draw_wrap_string( char *szText, int x, int y, int maxx )
{
    // ZZ> This function spits a line of null terminated text onto the backbuffer,
    //     wrapping over the right side and returning the new y value
    int sttx = x;
    Uint8 cTmp = szText[0];
    int newy = y + fontyspacing;
    Uint8 newword = btrue;
    int cnt = 1;

    BeginText();

    maxx = maxx + sttx;

    while ( cTmp != 0 )
    {
        // Check each new word for wrapping
        if ( newword )
        {
            int endx = x + length_of_word( szText + cnt - 1 );

            newword = bfalse;

            if ( endx > maxx )
            {
                // Wrap the end and cut off spaces and tabs
                x = sttx + fontyspacing;
                y += fontyspacing;
                newy += fontyspacing;

                while ( cTmp == ' ' || cTmp == '~' )
                {
                    cTmp = szText[cnt];
                    cnt++;
                }
            }
        }
        else
        {
            if ( cTmp == '~' )
            {
                // Use squiggle for tab
                x = x & ( ~TABAND );
                x += TABAND + 1;
            }
            else if ( cTmp == '\n' )
            {
                x = sttx;
                y += fontyspacing;
                newy += fontyspacing;
            }
            else
            {
                // Normal letter
                cTmp = asciitofont[cTmp];
                draw_one_font( cTmp, x, y );
                x += fontxspacing[cTmp];
            }

            cTmp = szText[cnt];

            if ( cTmp == '~' || cTmp == ' ' )
            {
                newword = btrue;
            }

            cnt++;
        }
    }

    EndText();
    return newy;
}

//--------------------------------------------------------------------------------------------
int draw_status( Uint16 character, int x, int y )
{
    // ZZ> This function shows a character's icon, status and inventory
    //     The x,y coordinates are the top left point of the image to draw
    Uint16 item;
    char cTmp;
    char *readtext;

    int life = chrlife[character] >> 8;
    int lifemax = chrlifemax[character] >> 8;
    int mana = chrmana[character] >> 8;
    int manamax = chrmanamax[character] >> 8;
    int cnt = lifemax;

    /* [claforte] This can be removed
      Note: This implies that the status line assumes that the max life and mana
          representable is 50.

    if(cnt > 50) cnt = 50;
      if(cnt == 0) cnt = -9;
      cnt = manamax;
      if(cnt > 50) cnt = 50;
      if(cnt == 0) cnt = -9;
    */

    // Write the character's first name
    if ( chrnameknown[character] )
        readtext = chrname[character];
    else
        readtext = capclassname[chrmodel[character]];

    for ( cnt = 0; cnt < 6; cnt++ )
    {
        cTmp = readtext[cnt];

        if ( cTmp == ' ' || cTmp == 0 )
        {
            generictext[cnt] = 0;
            break;
        }
        else
            generictext[cnt] = cTmp;
    }

    generictext[6] = 0;
    draw_string( generictext, x + 8, y ); y += fontyspacing;

    // Write the character's money
    sprintf( generictext, "$%4d", chrmoney[character] );
    draw_string( generictext, x + 8, y ); y += fontyspacing + 8;

    // Draw the icons
    draw_one_icon( madskintoicon[chrtexture[character]], x + 40, y, chrsparkle[character] );
    item = chrholdingwhich[character][0];

    if ( item != MAXCHR )
    {
        if ( chricon[item] )
        {
            draw_one_icon( madskintoicon[chrtexture[item]], x + 8, y, chrsparkle[item] );

            if ( chrammomax[item] != 0 && chrammoknown[item] )
            {
                if ( !capisstackable[chrmodel[item]] || chrammo[item] > 1 )
                {
                    // Show amount of ammo left
                    sprintf( generictext, "%2d", chrammo[item] );
                    draw_string( generictext, x + 8, y - 8 );
                }
            }
        }
        else
            draw_one_icon( bookicon + ( chrmoney[item]&3 ), x + 8, y, chrsparkle[item] );
    }
    else
        draw_one_icon( nullicon, x + 8, y, NOSPARKLE );

    item = chrholdingwhich[character][1];

    if ( item != MAXCHR )
    {
        if ( chricon[item] )
        {
            draw_one_icon( madskintoicon[chrtexture[item]], x + 72, y, chrsparkle[item] );

            if ( chrammomax[item] != 0 && chrammoknown[item] )
            {
                if ( !capisstackable[chrmodel[item]] || chrammo[item] > 1 )
                {
                    // Show amount of ammo left
                    sprintf( generictext, "%2d", chrammo[item] );
                    draw_string( generictext, x + 72, y - 8 );
                }
            }
        }
        else
            draw_one_icon( bookicon + ( chrmoney[item]&3 ), x + 72, y, chrsparkle[item] );
    }
    else
        draw_one_icon( nullicon, x + 72, y, NOSPARKLE );

    y += 32;

    // Draw the bars
    if ( chralive[character] )
        y = draw_one_bar( chrlifecolor[character], x, y, life, lifemax );
    else
        y = draw_one_bar( 0, x, y, 0, lifemax );  // Draw a black bar

    y = draw_one_bar( chrmanacolor[character], x, y, mana, manamax );
    return y;
}

//--------------------------------------------------------------------------------------------
void draw_text()
{
    // ZZ> This function spits out some words
    char text[512];
    int y, cnt, tnc, fifties, seconds, minutes;

    Begin2DMode();
    // Status bars
    y = 0;

    if ( staton )
    {
        for ( cnt = 0; cnt < numstat && y < scry; cnt++ )
        {
            y = draw_status( statlist[cnt], scrx - BARX, y );
        }
    }

    // Map display
    if ( mapvalid && mapon )
    {
        draw_map( 0, scry - MAPSIZE );

        //If one of the players can sense enemies via EMP, draw them as blips on the map
        if ( local_senseenemies != MAXCHR )
        {
            Uint16 iTmp = 0;

            while ( numblip < MAXBLIP && iTmp < MAXCHR)
            {
                //Show only hated team
                if (chron[iTmp] && teamhatesteam[chrteam[local_senseenemies]][chrteam[iTmp]])
                {
                    //Only if they match the required IDSZ ([NONE] always works)
                    if ( local_senseenemiesID == Make_IDSZ("NONE")
                            || capidsz[iTmp][IDSZ_PARENT] == local_senseenemiesID
                            || capidsz[iTmp][IDSZ_TYPE] == local_senseenemiesID)
                    {
                        //Inside the map?
                        if ( chrxpos[iTmp] < meshedgex && chrypos[iTmp] < meshedgey )
                        {
                            //Valid colors only
                            if ( valuetmpargument < NUMBLIP )
                            {
                                blipx[numblip] = chrxpos[iTmp] * MAPSIZE / meshedgex;
                                blipy[numblip] = chrypos[iTmp] * MAPSIZE / meshedgey;
                                blipc[numblip] = 0; //Red blips
                                numblip++;
                            }
                        }
                    }
                }

                iTmp++;
            }
        }

        for ( cnt = 0; cnt < numblip; cnt++ )
        {
            draw_blip(0.75f, blipc[cnt], blipx[cnt], blipy[cnt] + scry - MAPSIZE );
        }

        if ( youarehereon && ( wldframe&8 ) )
        {
            for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
            {
                if ( plavalid[cnt] && pladevice[cnt] != INPUT_BITS_NONE )
                {
                    tnc = plaindex[cnt];

                    if ( chralive[tnc] )
                        draw_blip( 0.75f, 0, chrxpos[tnc]*MAPSIZE / meshedgex, ( chrypos[tnc]*MAPSIZE / meshedgey ) + scry - MAPSIZE );
                }
            }
        }
    }

    // FPS text
    y = 0;

    if ( outofsync )
    {
        sprintf( text, "OUT OF SYNC" );
        draw_string( text, 0, y );  y += fontyspacing;
    }

    if ( parseerror )
    {
        sprintf( text, "SCRIPT ERROR ( SEE LOG.TXT )" );
        draw_string( text, 0, y );
        y += fontyspacing;
    }

    if ( fpson )
    {
        draw_string( szfpstext, 0, y );
        y += fontyspacing;
    }

    if ( SDLKEYDOWN( SDLK_F1 ) )
    {
        // In-Game help
        sprintf( text, "!!!MOUSE HELP!!!" );
        draw_string( text, 0, y );  y += fontyspacing;

        if ( rtscontrol )
        {
            sprintf( text, "  Left Drag to select units" );
            draw_string( text, 0, y );  y += fontyspacing;
            sprintf( text, "  Left Click to order them" );
            draw_string( text, 0, y );  y += fontyspacing;

        }
        else
        {
            sprintf( text, "  Edit CONTROLS.TXT to change" );
            draw_string( text, 0, y );  y += fontyspacing;
            sprintf( text, "  Left Click to use an item" );
            draw_string( text, 0, y );  y += fontyspacing;
            sprintf( text, "  Left and Right Click to grab" );
            draw_string( text, 0, y );  y += fontyspacing;
            sprintf( text, "  Middle Click to jump" );
            draw_string( text, 0, y );  y += fontyspacing;
            sprintf( text, "  A and S keys do stuff" );
            draw_string( text, 0, y );  y += fontyspacing;
        }

        sprintf( text, "  Right Drag to move camera" );
        draw_string( text, 0, y );  y += fontyspacing;
    }

    if ( SDLKEYDOWN( SDLK_F2 ) )
    {
        // In-Game help
        sprintf( text, "!!!JOYSTICK HELP!!!" );
        draw_string( text, 0, y );  y += fontyspacing;

        if ( rtscontrol )
        {
            sprintf( text, "  Joystick not available" );
            draw_string( text, 0, y );  y += fontyspacing;
        }
        else
        {
            sprintf( text, "  Edit CONTROLS.TXT to change" );
            draw_string( text, 0, y );  y += fontyspacing;
            sprintf( text, "  Hit the buttons" );
            draw_string( text, 0, y );  y += fontyspacing;
            sprintf( text, "  You'll figure it out" );
            draw_string( text, 0, y );  y += fontyspacing;
        }
    }

    if ( SDLKEYDOWN( SDLK_F3 ) )
    {
        // In-Game help
        sprintf( text, "!!!KEYBOARD HELP!!!" );
        draw_string( text, 0, y );  y += fontyspacing;

        if ( rtscontrol )
        {
            sprintf( text, "  Keyboard not available" );
            draw_string( text, 0, y );  y += fontyspacing;
        }
        else
        {
            sprintf( text, "  Edit CONTROLS.TXT to change" );
            draw_string( text, 0, y );  y += fontyspacing;
            sprintf( text, "  TGB control one hand" );
            draw_string( text, 0, y );  y += fontyspacing;
            sprintf( text, "  YHN control the other" );
            draw_string( text, 0, y );  y += fontyspacing;
            sprintf( text, "  Keypad to move and jump" );
            draw_string( text, 0, y );  y += fontyspacing;
            sprintf( text, "  Number keys for stats" );
            draw_string( text, 0, y );  y += fontyspacing;
        }
    }

    if ( gDevMode && SDLKEYDOWN( SDLK_F5 ) )
    {
        // Debug information
        sprintf( text, "!!!DEBUG MODE-5!!!" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  CAM %f %f", camx, camy );
        draw_string( text, 0, y );  y += fontyspacing;
        tnc = plaindex[0];
        sprintf( text, "  PLA0DEF %d %d %d %d %d %d %d %d",
                 chrdamagemodifier[tnc][0]&3,
                 chrdamagemodifier[tnc][1]&3,
                 chrdamagemodifier[tnc][2]&3,
                 chrdamagemodifier[tnc][3]&3,
                 chrdamagemodifier[tnc][4]&3,
                 chrdamagemodifier[tnc][5]&3,
                 chrdamagemodifier[tnc][6]&3,
                 chrdamagemodifier[tnc][7]&3 );
        draw_string( text, 0, y );  y += fontyspacing;
        tnc = plaindex[0];
        sprintf( text, "  PLA0 %5.1f %5.1f", chrxpos[tnc] / 128.0f, chrypos[tnc] / 128.0f );
        draw_string( text, 0, y );  y += fontyspacing;
        tnc = plaindex[1];
        sprintf( text, "  PLA1 %5.1f %5.1f", chrxpos[tnc] / 128.0f, chrypos[tnc] / 128.0f );
        draw_string( text, 0, y );  y += fontyspacing;
    }

    if ( gDevMode &&  SDLKEYDOWN( SDLK_F6 ) )
    {
        // More debug information
        sprintf( text, "!!!DEBUG MODE-6!!!" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  FREEPRT %d", numfreeprt );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  FREECHR %d",  numfreechr );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  MACHINE %d", localmachine );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  EXPORT %d", exportvalid );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  FOGAFF %d", fogaffectswater );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  PASS %d/%d", numshoppassage, numpassage );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  NETPLAYERS %d", numplayer );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  DAMAGEPART %d", damagetileparttype );
        draw_string( text, 0, y );  y += fontyspacing;
    }

    if ( gDevMode && SDLKEYDOWN( SDLK_F7 ) )
    {
        // White debug mode
        sprintf( text, "!!!DEBUG MODE-7!!!" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "CAM %f %f %f %f", mView.CNV( 0, 0 ), mView.CNV( 1, 0 ), mView.CNV( 2, 0 ), mView.CNV( 3, 0 ) );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "CAM %f %f %f %f", mView.CNV( 0, 1 ), mView.CNV( 1, 1 ), mView.CNV( 2, 1 ), mView.CNV( 3, 1 ) );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "CAM %f %f %f %f", mView.CNV( 0, 2 ), mView.CNV( 1, 2 ), mView.CNV( 2, 2 ), mView.CNV( 3, 2 ) );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "CAM %f %f %f %f", mView.CNV( 0, 3 ), mView.CNV( 1, 3 ), mView.CNV( 2, 3 ), mView.CNV( 3, 3 ) );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "x %f", camcenterx );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "y %f", camcentery );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "turn %d %d", autoturncamera, doturntime );
        draw_string( text, 0, y );  y += fontyspacing;
    }

    // Draw paused text
    if ( gamepaused && !SDLKEYDOWN( SDLK_F11 ) )
    {
        sprintf( text, "GAME PAUSED" );
        draw_string( text, -90 + scrx / 2, 0 + scry / 2 );
    }

    // Pressed panic button
    if ( SDLKEYDOWN( SDLK_q ) && SDLKEYDOWN( SDLK_LCTRL ) )
    {
        gameactive = bfalse;
        moduleactive = bfalse;
    }

    if ( timeron )
    {
        fifties = ( timervalue % 50 ) << 1;
        seconds = ( ( timervalue / 50 ) % 60 );
        minutes = ( timervalue / 3000 );
        sprintf( text, "=%d:%02d:%02d=", minutes, seconds, fifties );
        draw_string( text, 0, y );
        y += fontyspacing;
    }

    if ( waitingforplayers )
    {
        sprintf( text, "Waiting for players... " );
        draw_string( text, 0, y );
        y += fontyspacing;
    }

    if ( alllocalpladead || respawnanytime )
    {
        if ( respawnvalid )
        {
            draw_string( "PRESS SPACE TO RESPAWN", 0, y );
            y += fontyspacing;
        }
        else
        {
            draw_string( "PRESS ESCAPE TO QUIT", 0, y );
            y += fontyspacing;
        }
    }
    else if ( beatmodule )
    {
        sprintf( text, "VICTORY!  PRESS ESCAPE" );
        draw_string( text, 0, y );
        y += fontyspacing;
    }

    // Network message input
    if ( console_mode )
    {
        char buffer[KEYB_BUFFER_SIZE + 128];

        snprintf( buffer, sizeof(buffer), "%s > %s%s", netmessagename, keyb.buffer, (0 == (wldframe & 8)) ? "x" : "+" );

        y = draw_wrap_string( buffer, 0, y, scrx - wraptolerance );
    }

    // Messages
    if ( messageon )
    {
        // Display the messages
        tnc = msgstart;

        for ( cnt = 0; cnt < maxmessage; cnt++ )
        {
            if ( msgtime[tnc] > 0 )
            {
                y = draw_wrap_string( msgtextdisplay[tnc], 0, y, scrx - wraptolerance );

                if (msgtime[tnc] > msgtimechange)
                {
                    msgtime[tnc] -= msgtimechange;
                }
                else
                {
                    msgtime[tnc] = 0;
                }
            }

            tnc++;
            tnc = tnc % maxmessage;
        }

        msgtimechange = 0;
    }

    End2DMode();
}

//--------------------------------------------------------------------------------------------
void flip_pages()
{
    SDL_GL_SwapBuffers();
}

//--------------------------------------------------------------------------------------------
void draw_scene()
{
    Begin3DMode();

    make_prtlist();
    do_dynalight();
    light_characters();
    light_particles();

    // clear the depth buffer
    glClear( GL_DEPTH_BUFFER_BIT );

    // Clear the image if need be
    if ( clearson )
    {
        glClear( GL_COLOR_BUFFER_BIT );
    }
    else
    {
        // Render the background
        render_background( TX_WATER_LOW );  // 6 is the texture for waterlow.bmp
    }

    if ( zreflect ) // DO REFLECTIONS
        draw_scene_zreflection();
    else
        draw_scene_sadreflection();

    // clear the depth buffer
    glClear( GL_DEPTH_BUFFER_BIT );

    // Foreground overlay
    if ( overlayon )
    {
        render_foreground_overlay( TX_WATER_TOP );  // Texture 5 is watertop.bmp
    }

    End3DMode();
}

//--------------------------------------------------------------------------------------------
void draw_main()
{
    // ZZ> This function does all the drawing stuff
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    draw_scene();
    draw_text();
    flip_pages();
    allframe++;
    fpsframe++;
}

//--------------------------------------------------------------------------------------------
int load_one_title_image( int titleimage, char *szLoadName )
{
    // ZZ> This function loads a title in the specified image slot, forcing it into
    //     system memory.  Returns btrue if it worked

    Uint32 tx_id;

    tx_id = GLTexture_Load(GL_TEXTURE_2D, TxTitleImage + titleimage, szLoadName, INVALID_KEY );

    return INVALID_TX_ID != tx_id;

}

//--------------------------------------------------------------------------------------------
void load_all_menu_images()
{
    // ZZ> This function loads the title image for each module.  Modules without a
    //     title are marked as invalid

    char searchname[15];
    char loadname[256];
    const char *FileName;
    FILE* filesave;

    // Convert searchname
    strcpy( searchname, "modules" SLASH_STR "*.mod" );

    // Log a directory list
    filesave = fopen( "modules.txt", "w" );

    if ( filesave != NULL )
    {
        fprintf( filesave, "This file logs all of the modules found\n" );
        fprintf( filesave, "** Denotes an invalid module (Or unlockable)\n\n" );
    }

    // Search for .mod directories
    globalnummodule = 0;
    FileName = fs_findFirstFile( "modules", "mod" );
    while ( NULL != FileName && globalnummodule < MAXMODULE )
    {
        sprintf( modloadname[globalnummodule], "%s", FileName );

        sprintf( loadname, "modules" SLASH_STR "%s" SLASH_STR "gamedat" SLASH_STR "menu.txt", FileName );
        if ( get_module_data( globalnummodule, loadname ) )
        {
            // NOTE: just because we can't load the ttle image DOES NOT mean that we ignore the module
            sprintf( loadname, "modules" SLASH_STR "%s" SLASH_STR "gamedat" SLASH_STR "title", FileName );
            load_one_title_image( globalnummodule, loadname );

            fprintf( filesave, "%02d.  %s\n", globalnummodule, modlongname[globalnummodule] );

            globalnummodule++;
        }
        else
        {
            fprintf( filesave, "**.  %s\n", FileName );
        }

        FileName = fs_findNextFile();
    }

    fs_findClose();

    modlongname[globalnummodule][0] = '\0';

    if ( filesave != NULL ) fclose( filesave );
}

//--------------------------------------------------------------------------------------------
bool_t load_blip_bitmap()
{
    // This function loads the blip bitmaps

    if ( INVALID_TX_ID == GLTexture_Load(GL_TEXTURE_2D, &TxBlip, "basicdat" SLASH_STR "blip", INVALID_KEY ) )
    {
        log_warning( "Blip bitmap not loaded. Missing file = \"basicdat" SLASH_STR "blip.bmp\"\n" );
        return bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//void draw_titleimage( int image, int x, int y )
//{
//  // ZZ> This function draws a title image on the backbuffer
//  GLfloat  txWidth, txHeight;
//
//  if ( INVALID_TX_ID != GLTexture_GetTextureID( TxTitleImage + image ) )
//  {
//    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
//    Begin2DMode();
//    glNormal3f( 0, 0, 1 );  // glNormal3f( 0, 1, 0 );
//
//    /* Calculate the texture width & height */
//    txWidth = ( GLfloat )( GLTexture_GetImageWidth( TxTitleImage + image ) / GLTexture_GetDimensions( TxTitleImage + image ) );
//    txHeight = ( GLfloat )( GLTexture_GetImageHeight( TxTitleImage + image ) / GLTexture_GetDimensions( TxTitleImage + image ) );
//
//    /* Bind the texture */
//    GLTexture_Bind( TxTitleImage + image );
//
//    /* Draw the quad */
//    glBegin( GL_QUADS );
//    glTexCoord2f( 0, 1 );  glVertex2f( x, scry - y - GLTexture_GetImageHeight( TxTitleImage + image ) );
//    glTexCoord2f( txWidth, 1 );  glVertex2f( x + GLTexture_GetImageWidth( TxTitleImage + image ), scry - y - GLTexture_GetImageHeight( TxTitleImage + image ) );
//    glTexCoord2f( txWidth, 1 - txHeight );  glVertex2f( x + GLTexture_GetImageWidth( TxTitleImage + image ), scry - y );
//    glTexCoord2f( 0, 1 - txHeight );  glVertex2f( x, scry - y );
//    glEnd();
//
//    End2DMode();
//  }
//}

//--------------------------------------------------------------------------------------------
void do_cursor()
{
    // This function implements a mouse cursor
    input_read();

    cursorx = mous.x;  if ( cursorx < 6 )  cursorx = 6;  if ( cursorx > scrx - 16 )  cursorx = scrx - 16;

    cursory = mous.y;  if ( cursory < 8 )  cursory = 8;  if ( cursory > scry - 24 )  cursory = scry - 24;

    clicked = bfalse;

    if ( mous.button[0] && !pressed )
    {
        clicked = btrue;
    }

    pressed = mous.button[0];
    BeginText();  // Needed to setup text mode
    // draw_one_font(11, cursorx-5, cursory-7);
    draw_one_font( 95, cursorx - 5, cursory - 7 );
    EndText();    // Needed when done with text mode
}

/********************> Reshape3D() <*****/
void Reshape3D( int w, int h )
{
    glViewport( 0, 0, w, h );
}

int glinit( int argc, char **argv )
{
    /* Depth testing stuff */
    glClearDepth( 1.0f );
    glDepthFunc( GL_LESS );
    glEnable( GL_DEPTH_TEST );

    // Load the current graphical settings
    load_graphics();

    // fill mode
    glPolygonMode( GL_FRONT, GL_FILL );
    glPolygonMode( GL_BACK,  GL_FILL );

    /* Enable a single OpenGL light. */
    // glLightfv(GL_LIGHT0, GL_SPECULAR, light_diffuse);
    // glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    // glEnable(GL_LIGHT0);
    glDisable( GL_LIGHTING );
    // glLightModelfv(GL_LIGHT_MODEL_AMBIENT,intensity);

    /* Backface culling */
    glEnable ( GL_CULL_FACE ); // This seems implied - DDOI
    glCullFace( GL_BACK );

    glEnable( GL_COLOR_MATERIAL );  // Need this for color + lighting
    EnableTexturing();    // Enable texture mapping

    return 1;
}

void sdlinit( int argc, char **argv )
{
    int     colordepth;

    log_info ( "Initializing SDL version %d.%d.%d... ", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL );
    if ( SDL_Init(0) < 0 )
    {
        log_message( "Failure!\n" );
        log_error( "Unable to initialize SDL: %s\n", SDL_GetError() );
    }
    else
    {
        log_message( "Success!\n" );
        atexit( SDL_Quit );
    }

    log_info( "Intializing SDL Video... " );
    if ( SDL_InitSubSystem( SDL_INIT_VIDEO ) < 0 )
    {
        log_message( "Failed!\n" );
        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
    }
    else
    {
        log_message( "Succeess!\n" );
    }

    log_info( "Intializing SDL Timing Services... " );
    if ( SDL_InitSubSystem( SDL_INIT_TIMER ) < 0 )
    {
        log_message( "Failed!\n" );
        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
    }
    else
    {
        log_message( "Succeess!\n" );
    }

    log_info( "Intializing SDL Event Threading... " );
    if ( SDL_InitSubSystem( SDL_INIT_EVENTTHREAD ) < 0 )
    {
        log_message( "Failed!\n" );
        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
    }
    else
    {
        log_message( "Succeess!\n" );
    }

#ifndef __APPLE__
    {
        SDL_Surface *theSurface;

        /* Setup the cute windows manager icon */
        theSurface = IMG_Load( "basicdat" SLASH_STR "icon.bmp" );

        if ( theSurface == NULL )
        {
            log_error( "Unable to load icon (basicdat" SLASH_STR "icon.bmp)\n" );
        }

        SDL_WM_SetIcon( theSurface, NULL );
    }
#endif

#ifdef __unix__

    // GLX doesn't differentiate between 24 and 32 bpp, asking for 32 bpp
    // will cause SDL_SetVideoMode to fail with:
    // Unable to set video mode: Couldn't find matching GLX visual

    if ( scrd == 32 ) scrd = 24;

#endif

    colordepth = scrd / 3;

    /* Set the OpenGL Attributes */
#ifndef __unix__
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE,   colordepth );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, colordepth  );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE,  colordepth );
    if (scrd > colordepth * 3)
    {
        SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, scrd - colordepth * 3 );
    }
#endif
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, scrz );

    log_info("Opening SDL Video Mode... ");
    displaySurface = SDL_SetVideoMode( scrx, scry, scrd, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_OPENGL | ( fullscreen ? SDL_FULLSCREEN : 0 ) );
    if ( displaySurface == NULL )
    {
        log_message( "Failure!\n" );
        log_error( "Unable to set video mode: %s\n", SDL_GetError() );
    }
    else
    {
        int i;

        log_message( "Success!\n" );

        SDL_GL_GetAttribute( SDL_GL_DOUBLEBUFFER, &i );
        log_message("\tSDL_GL_DOUBLEBUFFER == %d\n", i);

        SDL_GL_GetAttribute( SDL_GL_RED_SIZE, &i );
        log_message("\tSDL_GL_RED_SIZE     == %d\n", i);

        SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE, &i );
        log_message("\tSDL_GL_GREEN_SIZE   == %d\n", i);

        SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE, &i );
        log_message("\tSDL_GL_BLUE_SIZE    == %d\n", i);

        SDL_GL_GetAttribute( SDL_GL_ALPHA_SIZE, &i );
        log_message("\tSDL_GL_ALPHA_SIZE   == %d\n", i);

        SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &i );
        log_message("\tSDL_GL_DEPTH_SIZE   == %d  (z-buffer depth)\n", i);
    }

    // Set the window name
    SDL_WM_SetCaption( "Egoboo", "Egoboo" );

    if ( gGrabMouse )
    {
        SDL_WM_GrabInput ( SDL_GRAB_ON );
    }

    if ( gHideMouse )
    {
        SDL_ShowCursor( 0 );  // Hide the mouse cursor
    }

    input_init();
}

struct s_packing_test
{
    Uint8 val1;
    Uint8 val2;

    Uint8 ary1[3];
    Uint8 ary2[3];
};

static struct s_packing_test packing_test;

//---------------------------------------------------------------------------------------------
bool_t dump_screenshot()
{
    // dumps the current screen (GL context) to a new bitmap file
    // right now it dumps it to whatever the current directory is

    // returns btrue if successful, bfalse otherwise

    int i;
    FILE *test;
    SDL_Surface *screen;
    bool_t savefound = bfalse;
    char szFilename[100];

    // find a valid file name
    savefound = bfalse;
    i = 0;
    while ( !savefound && ( i < 100 ) )
    {
        sprintf( szFilename, "ego%02d.bmp", i );

        // lame way of checking if the file already exists...
        test = fopen( szFilename, "rb" );

        if ( test != NULL )
        {
            fclose( test );
            i++;
        }
        else
        {
            savefound = btrue;
        }
    }

    if ( !savefound ) return bfalse;

    // grab the current screen
    screen = SDL_GetVideoSurface();

    // if we are not using OpenGl, jsut dump the screen
    if ( 0 == (screen->flags & SDL_OPENGL) )
    {
        SDL_SaveBMP(screen, szFilename);
        return bfalse;
    }

    // we ARE using OpenGL
    glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT ) ;
    {
        SDL_Surface *temp;
        Uint8 *pixels;
        char buff2[100];

        // tell OpenGL about our prefered packing
        glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

        // create a SDL surface
        temp = SDL_CreateRGBSurface( SDL_SWSURFACE, screen->w, screen->h, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                                     0x000000FF, 0x0000FF00, 0x00FF0000, 0
#else
                                     0x00FF0000, 0x0000FF00, 0x000000FF, 0
#endif
                                   );

        if ( temp == NULL ) return bfalse;

        // allocate the pixel store to receive the data from OpenGL
        pixels = malloc( 3 * screen->w * screen->h );
        if ( pixels == NULL )
        {
            SDL_FreeSurface( temp );
            return bfalse;
        }

        // actually grab the OpenGL buffer
        glReadPixels( 0, 0, screen->w, screen->h, GL_RGB, GL_UNSIGNED_BYTE, pixels );

        // copy it into the SDL screen
        for ( i = 0; i < screen->h; i++ )
        {
            memcpy( ( ( char * ) temp->pixels ) + temp->pitch * i, pixels + 3 * screen->w * ( screen->h - i - 1 ), screen->w * 3 );
        }

        // free the pixel data
        free( pixels );

        // Save the file as a .bmp
        SDL_SaveBMP( temp, szFilename );

        // free the SDL surface
        SDL_FreeSurface( temp );

        // tell the user what we did
        sprintf( buff2, "Saved to %s", szFilename );
        debug_message( buff2 );
    }
    glPopClientAttrib();

    return savefound;
}

//--------------------------------------------------------------------------------------------
void check_stats()
{
    // ZZ> This function lets the players check character stats

    static int stat_check_timer = 0;
    static int stat_check_delay = 0;
    int ticks;

    if ( console_mode ) return;

    ticks = SDL_GetTicks();
    if ( ticks > stat_check_timer + 20 )
    {
        stat_check_timer = ticks;
    }

    stat_check_delay -= 20;
    if ( stat_check_delay > 0 )
        return;

    // !!!BAD!!!  XP CHEAT
    if ( SDLKEYDOWN( SDLK_x ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) && plaindex[0] < MAXCHR )  { give_experience( plaindex[0], 25, XPDIRECT ); stat_check_delay = 500; }
        if ( SDLKEYDOWN( SDLK_2 ) && plaindex[1] < MAXCHR )  { give_experience( plaindex[1], 25, XPDIRECT ); stat_check_delay = 500; }
        if ( SDLKEYDOWN( SDLK_3 ) && plaindex[2] < MAXCHR )  { give_experience( plaindex[2], 25, XPDIRECT ); stat_check_delay = 500; }
        if ( SDLKEYDOWN( SDLK_4 ) && plaindex[3] < MAXCHR )  { give_experience( plaindex[3], 25, XPDIRECT ); stat_check_delay = 500; }

        statdelay = 0;
    }

    // !!!BAD!!!  LIFE CHEAT
    if ( SDLKEYDOWN( SDLK_z ) && SDLKEYDOWN( SDLK_1 ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) && plaindex[0] < MAXCHR )  { chrlife[plaindex[0]] += 128; chrlife[plaindex[0]] = MIN(chrlife[plaindex[0]], PERFECTBIG); stat_check_delay = 500; }
        if ( SDLKEYDOWN( SDLK_2 ) && plaindex[1] < MAXCHR )  { chrlife[plaindex[1]] += 128; chrlife[plaindex[0]] = MIN(chrlife[plaindex[1]], PERFECTBIG); stat_check_delay = 500; }
        if ( SDLKEYDOWN( SDLK_3 ) && plaindex[2] < MAXCHR )  { chrlife[plaindex[2]] += 128; chrlife[plaindex[0]] = MIN(chrlife[plaindex[2]], PERFECTBIG); stat_check_delay = 500; }
        if ( SDLKEYDOWN( SDLK_4 ) && plaindex[3] < MAXCHR )  { chrlife[plaindex[3]] += 128; chrlife[plaindex[0]] = MIN(chrlife[plaindex[3]], PERFECTBIG); stat_check_delay = 500; }
    }

    // Display armor stats?
    if ( SDLKEYDOWN( SDLK_LSHIFT ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) )  { show_armor( 1 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_2 ) )  { show_armor( 2 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_3 ) )  { show_armor( 3 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_4 ) )  { show_armor( 4 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_5 ) )  { show_armor( 5 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_6 ) )  { show_armor( 6 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_7 ) )  { show_armor( 7 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_8 ) )  { show_armor( 8 ); stat_check_delay = 1000; }
    }

    // Display enchantment stats?
    else if (  SDLKEYDOWN( SDLK_LCTRL ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) )  { show_full_status( 0 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_2 ) )  { show_full_status( 1 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_3 ) )  { show_full_status( 2 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_4 ) )  { show_full_status( 3 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_5 ) )  { show_full_status( 4 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_6 ) )  { show_full_status( 5 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_7 ) )  { show_full_status( 6 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_8 ) )  { show_full_status( 7 ); stat_check_delay = 1000; }
    }

    // Display character stats?
    else if ( SDLKEYDOWN( SDLK_LALT ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) )  { show_magic_status( 0 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_2 ) )  { show_magic_status( 1 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_3 ) )  { show_magic_status( 2 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_4 ) )  { show_magic_status( 3 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_5 ) )  { show_magic_status( 4 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_6 ) )  { show_magic_status( 5 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_7 ) )  { show_magic_status( 6 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_8 ) )  { show_magic_status( 7 ); stat_check_delay = 1000; }
    }

    // Display character stats?
    else
    {
        if ( SDLKEYDOWN( SDLK_1 ) )  { show_stat( 0 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_2 ) )  { show_stat( 1 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_3 ) )  { show_stat( 2 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_4 ) )  { show_stat( 3 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_5 ) )  { show_stat( 4 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_6 ) )  { show_stat( 5 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_7 ) )  { show_stat( 6 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_8 ) )  { show_stat( 7 ); stat_check_delay = 1000; }
    }




    // handle the map
    if ( SDLKEYDOWN( SDLK_m ) && SDLKEYDOWN( SDLK_LSHIFT ) )
    {
        mapon = mapvalid;
        youarehereon = btrue;
        stat_check_delay = 1000;
    }
}

//---------------------------------------------------------------------------------------------------
void load_graphics()
{
    // ZF> This function loads all the graphics based on the game settings
    GLenum quality;

    // Check if the computer graphic driver supports anisotropic filtering
    if ( texturefilter == 4 )
    {
        glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy );

        if ( !strstr( ( char* )glGetString( GL_EXTENSIONS ), "GL_EXT_texture_filter_anisotropic" ) )
        {
            log_warning( "Your graphics driver does not support anisotropic filtering.\n" );
            texturefilter = TX_TRILINEAR_2; // Set filtering to trillienar instead
        }
    }

    // Enable prespective correction?
    if ( perspective ) quality = GL_NICEST;
    else quality = GL_FASTEST;

    glHint( GL_PERSPECTIVE_CORRECTION_HINT, quality );

    // Enable dithering? (This actually reduces quality but increases preformance)
    if ( dither ) glEnable( GL_DITHER );
    else glDisable( GL_DITHER );

    // Enable gourad shading? (Important!)
    glShadeModel( shading );

    // Enable antialiasing?
    if ( antialiasing )
    {
        glEnable(GL_MULTISAMPLE_ARB);

        glEnable( GL_LINE_SMOOTH );
        glHint( GL_LINE_SMOOTH_HINT,    GL_NICEST );

        glEnable( GL_POINT_SMOOTH );
        glHint( GL_POINT_SMOOTH_HINT,   GL_NICEST );

        glDisable( GL_POLYGON_SMOOTH );
        glHint( GL_POLYGON_SMOOTH_HINT,    GL_FASTEST );

        // PLEASE do not turn this on unless you use
        //  glEnable (GL_BLEND);
        //  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // before every single draw command
        //
        //glEnable(GL_POLYGON_SMOOTH);
        //glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    }
    else
    {
        glDisable(GL_MULTISAMPLE_ARB);
        glDisable( GL_POINT_SMOOTH );
        glDisable( GL_LINE_SMOOTH );
        glDisable( GL_POLYGON_SMOOTH );
    }

}
