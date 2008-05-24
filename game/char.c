/* Egoboo - char.c
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void flash_character_height(int character, unsigned char valuelow, signed short low,
    unsigned char valuehigh, signed short high)
{
    // ZZ> This function sets a character's lighting depending on vertex height...
    //     Can make feet dark and head light...
    int cnt;
    unsigned short frame;
    signed short z;


    frame = chrframe[character];
    cnt = 0;
    while(cnt < madtransvertices[chrmodel[character]])
    {
        z = madvrtz[frame][cnt];
        if(z < low)
        {
            chrvrta[character][cnt] = valuelow;
        }
        else
        {
            if(z > high)
            {
                chrvrta[character][cnt] = valuehigh;
            }
            else
            {
                chrvrta[character][cnt] = (valuehigh * (z - low) / (high - low)) +
                                          (valuelow * (high - z) / (high - low));
            }
        }
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void flash_character(int character, unsigned char value)
{
    // ZZ> This function sets a character's lighting
    int cnt;

    cnt = 0;
    while(cnt < madtransvertices[chrmodel[character]])
    {
        chrvrta[character][cnt] = value;
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void flash_select()
{
 /*   // ZZ> This function makes the selected characters blink
    int cnt;
    unsigned char value;

    if((wldframe&31)==0 && allselect==bfalse)
    {
        value = ((wldframe&32)<<3) - ((wldframe&32)>>5);
        cnt = 0;
        while(cnt < numrtsselect)
        {
            flash_character(rtsselect[cnt], value);
            cnt++;
        }
    }*/
}

//--------------------------------------------------------------------------------------------
void add_to_dolist(int cnt)
{
    // This function puts a character in the list
    int fan;


    if(!chrindolist[cnt])
    {
        fan = chronwhichfan[cnt];
        if(meshinrenderlist[fan])
        {
            chrlightlevel[cnt] = meshvrtl[meshvrtstart[fan]];
            dolist[numdolist] = cnt;
            chrindolist[cnt] = btrue;
            numdolist++;


            // Do flashing
            if((allframe&chrflashand[cnt]) == 0 && chrflashand[cnt]!=DONTFLASH)
            {
                flash_character(cnt, 255);
            }
            // Do blacking
            if((allframe&SEEKURSEAND) == 0 && localseekurse && chriskursed[cnt])
            {
                flash_character(cnt, 0);
            }
        }
        else
        {
            // Double check for large/special objects
            if(capalwaysdraw[chrmodel[cnt]])
            {
                dolist[numdolist] = cnt;
                chrindolist[cnt] = btrue;
                numdolist++;
            }
        }
        // Add its weapons too
        if(chrholdingwhich[cnt][0] != MAXCHR)
            add_to_dolist(chrholdingwhich[cnt][0]);
        if(chrholdingwhich[cnt][1] != MAXCHR)
            add_to_dolist(chrholdingwhich[cnt][1]);
    }
}

//--------------------------------------------------------------------------------------------
void order_dolist(void)
{
    // ZZ> This function orders the dolist based on distance from camera,
    //     which is needed for reflections to properly clip themselves.
    //     Order from closest to farthest
    int cnt, tnc, character, order;
    int dist[MAXCHR];
    unsigned short olddolist[MAXCHR];


    // Figure the distance of each
    cnt = 0;
    while(cnt < numdolist)
    {
        character = dolist[cnt];  olddolist[cnt] = character;
        if(chrlight[character] != 255 || chralpha[character] != 255)
        {
            // This makes stuff inside an invisible character visible...
            // A key inside a Jellcube, for example
            dist[cnt] = 0x7fffffff;
        }
        else
        {
            dist[cnt] = ABS(chrxpos[character]-camx)+ABS(chrypos[character]-camy);
        }
        cnt++;
    }


    // Put em in the right order
    cnt = 0;
    while(cnt < numdolist)
    {
        character = olddolist[cnt];
        order = 0;  // Assume this character is closest
        tnc = 0;
        while(tnc < numdolist)
        {
            // For each one closer, increment the order
            order += (dist[cnt] > dist[tnc]);
            order += (dist[cnt] == dist[tnc]) && (cnt < tnc);
            tnc++;
        }
        dolist[order] = character;
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void make_dolist(void)
{
    // ZZ> This function finds the characters that need to be drawn and puts them in the list
    int cnt, character;


    // Remove everyone from the dolist
    cnt = 0;
    while(cnt < numdolist)
    {
        character = dolist[cnt];
        chrindolist[character] = bfalse;
        cnt++;
    }
    numdolist = 0;


    // Now fill it up again
    cnt = 0;
    while(cnt < MAXCHR)
    {
        if(chron[cnt]&&(!chrinpack[cnt]))
        {
            // Add the character
            add_to_dolist(cnt);
        }
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void keep_weapons_with_holders()
{
    // ZZ> This function keeps weapons near their holders
    int cnt, character;


    // !!!BAD!!!  May need to do 3 levels of attachment...
    cnt = 0;
    while(cnt < MAXCHR)
    {
        if(chron[cnt])
        {
            character = chrattachedto[cnt];
            if(character == MAXCHR)
            {
                // Keep inventory with character
                if(chrinpack[cnt] == bfalse)
                {
                    character = chrnextinpack[cnt];
                    while(character != MAXCHR)
                    {
                        chrxpos[character] = chrxpos[cnt];
                        chrypos[character] = chrypos[cnt];
                        chrzpos[character] = chrzpos[cnt];
                        // Copy olds to make SendMessageNear work
                        chroldx[character] = chrxpos[cnt];
                        chroldy[character] = chrypos[cnt];
                        chroldz[character] = chrzpos[cnt];
                        character = chrnextinpack[character];
                    }
                }
            }
            else
            {
                // Keep in hand weapons with character
                if(chrmatrixvalid[character] && chrmatrixvalid[cnt])
                {
                    chrxpos[cnt] = chrmatrix[cnt]_CNV(3,0);
                    chrypos[cnt] = chrmatrix[cnt]_CNV(3,1);
                    chrzpos[cnt] = chrmatrix[cnt]_CNV(3,2);
                }
                else
                {
                    chrxpos[cnt] = chrxpos[character];
                    chrypos[cnt] = chrypos[character];
                    chrzpos[cnt] = chrzpos[character];
                }
                chrturnleftright[cnt] = chrturnleftright[character];
                // Copy this stuff ONLY if it's a weapon, not for mounts
                if(chrtransferblend[character]&&chrisitem[cnt])
                {
                    if(chralpha[character]!=255)
                    {
                        chralpha[cnt] = chralpha[character];
                    }
                    if(chrlight[character]!=255)
                    {
                        chrlight[cnt] = chrlight[character];
                    }
                }
            }
        }
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void make_turntosin(void)
{
    // ZZ> This function makes the lookup table for chrturn...
    int cnt;

    cnt = 0;
    while(cnt < 16384)
    {
        turntosin[cnt] = sin((float) (2*PI*cnt/16384.0));
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void make_one_character_matrix(unsigned short cnt)
{
   // ZZ> This function sets one character's matrix
   unsigned short tnc;
   chrmatrixvalid[cnt] = btrue;
   if(chroverlay[cnt])
   {
       // Overlays are kept with their target...
       tnc = chraitarget[cnt];
       chrxpos[cnt] = chrxpos[tnc];
       chrypos[cnt] = chrypos[tnc];
       chrzpos[cnt] = chrzpos[tnc];
       chrmatrix[cnt]_CNV(0,0) = chrmatrix[tnc]_CNV(0,0);
       chrmatrix[cnt]_CNV(0,1) = chrmatrix[tnc]_CNV(0,1);
       chrmatrix[cnt]_CNV(0,2) = chrmatrix[tnc]_CNV(0,2);
       chrmatrix[cnt]_CNV(0,3) = chrmatrix[tnc]_CNV(0,3);
       chrmatrix[cnt]_CNV(1,0) = chrmatrix[tnc]_CNV(1,0);
       chrmatrix[cnt]_CNV(1,1) = chrmatrix[tnc]_CNV(1,1);
       chrmatrix[cnt]_CNV(1,2) = chrmatrix[tnc]_CNV(1,2);
       chrmatrix[cnt]_CNV(1,3) = chrmatrix[tnc]_CNV(1,3);
       chrmatrix[cnt]_CNV(2,0) = chrmatrix[tnc]_CNV(2,0);
       chrmatrix[cnt]_CNV(2,1) = chrmatrix[tnc]_CNV(2,1);
       chrmatrix[cnt]_CNV(2,2) = chrmatrix[tnc]_CNV(2,2);
       chrmatrix[cnt]_CNV(2,3) = chrmatrix[tnc]_CNV(2,3);
       chrmatrix[cnt]_CNV(3,0) = chrmatrix[tnc]_CNV(3,0);
       chrmatrix[cnt]_CNV(3,1) = chrmatrix[tnc]_CNV(3,1);
       chrmatrix[cnt]_CNV(3,2) = chrmatrix[tnc]_CNV(3,2);
       chrmatrix[cnt]_CNV(3,3) = chrmatrix[tnc]_CNV(3,3);
   }
   else
   {
       chrmatrix[cnt] = ScaleXYZRotateXYZTranslate(chrscale[cnt], chrscale[cnt], chrscale[cnt],
                                    chrturnleftright[cnt]>>2,
                                    ((unsigned short) (chrturnmapud[cnt]+32768))>>2,
                                    ((unsigned short) (chrturnmaplr[cnt]+32768))>>2,
                                    chrxpos[cnt], chrypos[cnt], chrzpos[cnt]);
   }
}

//--------------------------------------------------------------------------------------------
void free_one_character(int character)
{
    // ZZ> This function sticks a character back on the free character stack
    int cnt;

    freechrlist[numfreechr]=character;
    numfreechr++;
    // Remove from stat list
    if(chrstaton[character])
    {
        chrstaton[character] = bfalse;
        cnt = 0;
        while(cnt < numstat)
        {
            if(statlist[cnt] == character)
            {
                cnt++;
                while(cnt < numstat)
                {
                    statlist[cnt-1] = statlist[cnt];
                    cnt++;
                }
                numstat--;
            }
            cnt++;
        }
    }
    // Make sure everyone knows it died
    if(chralive[character] && capinvictus[chrmodel[character]]==bfalse)
    {
        teammorale[chrbaseteam[character]]--;
    }
    cnt = 0;
    while(cnt < MAXCHR)
    {
        if(chron[cnt])
        {
            if(chraitarget[cnt] == character)
            {
                chralert[cnt]|=ALERTIFTARGETKILLED;
                chraitarget[cnt] = cnt;
            }
            if(teamleader[chrteam[cnt]] == character)
            {
                chralert[cnt]|=ALERTIFLEADERKILLED;
            }
        }
        cnt++;
    }
    if(teamleader[chrteam[character]]==character)
    {
        teamleader[chrteam[character]] = NOLEADER;
    }
    chron[character]=bfalse;
    chralive[character]=bfalse;
    chrinpack[character]=bfalse;
}

//--------------------------------------------------------------------------------------------
void free_inventory(int character)
{
    // ZZ> This function frees every item in the character's inventory
    int cnt, next;

    cnt = chrnextinpack[character];
    while(cnt < MAXCHR)
    {
        next = chrnextinpack[cnt];
        free_one_character(cnt);
        cnt = next;
    }
}

//--------------------------------------------------------------------------------------------
void attach_particle_to_character(int particle, int character, int grip)
{
    // ZZ> This function sets one particle's position to be attached to a character.
    //     It will kill the particle if the character is no longer around
    unsigned short vertex, model, frame, lastframe;
    unsigned char lip;
    float pointx;
    float pointy;
    float pointz;
    int temp;


    // Check validity of attachment
    if(chron[character]==bfalse || chrinpack[character])
    {
        prttime[particle] = 1;
        return;
    }


    // Do we have a matrix???
    if(chrmatrixvalid[character])//meshinrenderlist[chronwhichfan[character]])
    {
        // Transform the weapon grip from model to world space
        model = chrmodel[character];
        frame = chrframe[character];
        lastframe = chrlastframe[character];
        lip = chrlip[character]>>6;
        if(grip == SPAWNORIGIN)
        {
            prtxpos[particle] = chrmatrix[character]_CNV(3,0);
            prtypos[particle] = chrmatrix[character]_CNV(3,1);
            prtzpos[particle] = chrmatrix[character]_CNV(3,2);
            return;
        }
        vertex = madvertices[model] - grip;


        // Calculate grip point locations with linear interpolation and other silly things
        switch(lip)
        {
            case 0:  // 25% this frame
                temp = madvrtx[lastframe][vertex];
                temp = temp+temp+temp+madvrtx[frame][vertex]>>2;
                pointx = temp;///chrscale[cnt];
                temp = madvrty[lastframe][vertex];
                temp = temp+temp+temp+madvrty[frame][vertex]>>2;
                pointy = temp;///chrscale[cnt];
                temp = madvrtz[lastframe][vertex];
                temp = temp+temp+temp+madvrtz[frame][vertex]>>2;
                pointz = temp;///chrscale[cnt];
                break;
            case 1:  // 50% this frame
                pointx = (madvrtx[frame][vertex]+madvrtx[lastframe][vertex]>>1);///chrscale[cnt];
                pointy = (madvrty[frame][vertex]+madvrty[lastframe][vertex]>>1);///chrscale[cnt];
                pointz = (madvrtz[frame][vertex]+madvrtz[lastframe][vertex]>>1);///chrscale[cnt];
                break;
            case 2:  // 75% this frame
                temp = madvrtx[frame][vertex];
                temp = temp+temp+temp+madvrtx[lastframe][vertex]>>2;
                pointx = temp;///chrscale[cnt];
                temp = madvrty[frame][vertex];
                temp = temp+temp+temp+madvrty[lastframe][vertex]>>2;
                pointy = temp;///chrscale[cnt];
                temp = madvrtz[frame][vertex];
                temp = temp+temp+temp+madvrtz[lastframe][vertex]>>2;
                pointz = temp;///chrscale[cnt];
                break;
            case 3:  // 100% this frame...  This is the legible one
                pointx = madvrtx[frame][vertex];///chrscale[cnt];
                pointy = madvrty[frame][vertex];///chrscale[cnt];
                pointz = madvrtz[frame][vertex];///chrscale[cnt];
                break;
        }





        // Do the transform
        prtxpos[particle] = (pointx*chrmatrix[character]_CNV(0,0) +
                             pointy*chrmatrix[character]_CNV(1,0) +
                             pointz*chrmatrix[character]_CNV(2,0));
        prtypos[particle] = (pointx*chrmatrix[character]_CNV(0,1) +
                             pointy*chrmatrix[character]_CNV(1,1) +
                             pointz*chrmatrix[character]_CNV(2,1));
        prtzpos[particle] = (pointx*chrmatrix[character]_CNV(0,2) +
                             pointy*chrmatrix[character]_CNV(1,2) +
                             pointz*chrmatrix[character]_CNV(2,2));
        prtxpos[particle] += chrmatrix[character]_CNV(3,0);
        prtypos[particle] += chrmatrix[character]_CNV(3,1);
        prtzpos[particle] += chrmatrix[character]_CNV(3,2);
    }
    else
    {
        // No matrix, so just wing it...
        prtxpos[particle] = chrxpos[character];
        prtypos[particle] = chrypos[character];
        prtzpos[particle] = chrzpos[character];
    }
}

//--------------------------------------------------------------------------------------------
void make_one_weapon_matrix(unsigned short cnt)
{
    // ZZ> This function sets one weapon's matrix, based on who it's attached to
    #define POINTS 4
    int tnc;
    unsigned short character, vertex, model, frame, lastframe;
    unsigned char lip;
    float pointx[POINTS];
    float pointy[POINTS];
    float pointz[POINTS];
    float nupointx[POINTS];
    float nupointy[POINTS];
    float nupointz[POINTS];
    int temp;


    // Transform the weapon grip from model to world space
    character = chrattachedto[cnt];
    model = chrmodel[character];
    frame = chrframe[character];
    lastframe = chrlastframe[character];
    lip = chrlip[character]>>6;
    chrmatrixvalid[cnt] = btrue;


    // Calculate grip point locations with linear interpolation and other silly things
    switch(lip)
    {
        case 0:  // 25% this frame
            vertex = chrweapongrip[cnt][0];
            temp = madvrtx[lastframe][vertex];
            temp = temp+temp+temp+madvrtx[frame][vertex]>>2;
            pointx[0] = temp;
            temp = madvrty[lastframe][vertex];
            temp = temp+temp+temp+madvrty[frame][vertex]>>2;
            pointy[0] = temp;
            temp = madvrtz[lastframe][vertex];
            temp = temp+temp+temp+madvrtz[frame][vertex]>>2;
            pointz[0] = temp;

            vertex = chrweapongrip[cnt][1];
            temp = madvrtx[lastframe][vertex];
            temp = temp+temp+temp+madvrtx[frame][vertex]>>2;
            pointx[1] = temp;
            temp = madvrty[lastframe][vertex];
            temp = temp+temp+temp+madvrty[frame][vertex]>>2;
            pointy[1] = temp;
            temp = madvrtz[lastframe][vertex];
            temp = temp+temp+temp+madvrtz[frame][vertex]>>2;
            pointz[1] = temp;

            vertex = chrweapongrip[cnt][2];
            temp = madvrtx[lastframe][vertex];
            temp = temp+temp+temp+madvrtx[frame][vertex]>>2;
            pointx[2] = temp;
            temp = madvrty[lastframe][vertex];
            temp = temp+temp+temp+madvrty[frame][vertex]>>2;
            pointy[2] = temp;
            temp = madvrtz[lastframe][vertex];
            temp = temp+temp+temp+madvrtz[frame][vertex]>>2;
            pointz[2] = temp;

            vertex = chrweapongrip[cnt][3];
            temp = madvrtx[lastframe][vertex];
            temp = temp+temp+temp+madvrtx[frame][vertex]>>2;
            pointx[3] = temp;
            temp = madvrty[lastframe][vertex];
            temp = temp+temp+temp+madvrty[frame][vertex]>>2;
            pointy[3] = temp;
            temp = madvrtz[lastframe][vertex];
            temp = temp+temp+temp+madvrtz[frame][vertex]>>2;
            pointz[3] = temp;
            break;
        case 1:  // 50% this frame
            vertex = chrweapongrip[cnt][0];
            pointx[0] = (madvrtx[frame][vertex]+madvrtx[lastframe][vertex]>>1);
            pointy[0] = (madvrty[frame][vertex]+madvrty[lastframe][vertex]>>1);
            pointz[0] = (madvrtz[frame][vertex]+madvrtz[lastframe][vertex]>>1);
            vertex = chrweapongrip[cnt][1];
            pointx[1] = (madvrtx[frame][vertex]+madvrtx[lastframe][vertex]>>1);
            pointy[1] = (madvrty[frame][vertex]+madvrty[lastframe][vertex]>>1);
            pointz[1] = (madvrtz[frame][vertex]+madvrtz[lastframe][vertex]>>1);
            vertex = chrweapongrip[cnt][2];
            pointx[2] = (madvrtx[frame][vertex]+madvrtx[lastframe][vertex]>>1);
            pointy[2] = (madvrty[frame][vertex]+madvrty[lastframe][vertex]>>1);
            pointz[2] = (madvrtz[frame][vertex]+madvrtz[lastframe][vertex]>>1);
            vertex = chrweapongrip[cnt][3];
            pointx[3] = (madvrtx[frame][vertex]+madvrtx[lastframe][vertex]>>1);
            pointy[3] = (madvrty[frame][vertex]+madvrty[lastframe][vertex]>>1);
            pointz[3] = (madvrtz[frame][vertex]+madvrtz[lastframe][vertex]>>1);
            break;
        case 2:  // 75% this frame
            vertex = chrweapongrip[cnt][0];
            temp = madvrtx[frame][vertex];
            temp = temp+temp+temp+madvrtx[lastframe][vertex]>>2;
            pointx[0] = temp;
            temp = madvrty[frame][vertex];
            temp = temp+temp+temp+madvrty[lastframe][vertex]>>2;
            pointy[0] = temp;
            temp = madvrtz[frame][vertex];
            temp = temp+temp+temp+madvrtz[lastframe][vertex]>>2;
            pointz[0] = temp;


            vertex = chrweapongrip[cnt][1];
            temp = madvrtx[frame][vertex];
            temp = temp+temp+temp+madvrtx[lastframe][vertex]>>2;
            pointx[1] = temp;
            temp = madvrty[frame][vertex];
            temp = temp+temp+temp+madvrty[lastframe][vertex]>>2;
            pointy[1] = temp;
            temp = madvrtz[frame][vertex];
            temp = temp+temp+temp+madvrtz[lastframe][vertex]>>2;
            pointz[1] = temp;


            vertex = chrweapongrip[cnt][2];
            temp = madvrtx[frame][vertex];
            temp = temp+temp+temp+madvrtx[lastframe][vertex]>>2;
            pointx[2] = temp;
            temp = madvrty[frame][vertex];
            temp = temp+temp+temp+madvrty[lastframe][vertex]>>2;
            pointy[2] = temp;
            temp = madvrtz[frame][vertex];
            temp = temp+temp+temp+madvrtz[lastframe][vertex]>>2;
            pointz[2] = temp;


            vertex = chrweapongrip[cnt][3];
            temp = madvrtx[frame][vertex];
            temp = temp+temp+temp+madvrtx[lastframe][vertex]>>2;
            pointx[3] = temp;
            temp = madvrty[frame][vertex];
            temp = temp+temp+temp+madvrty[lastframe][vertex]>>2;
            pointy[3] = temp;
            temp = madvrtz[frame][vertex];
            temp = temp+temp+temp+madvrtz[lastframe][vertex]>>2;
            pointz[3] = temp;

            break;
        case 3:  // 100% this frame...  This is the legible one
            vertex = chrweapongrip[cnt][0];
            pointx[0] = madvrtx[frame][vertex];
            pointy[0] = madvrty[frame][vertex];
            pointz[0] = madvrtz[frame][vertex];
            vertex = chrweapongrip[cnt][1];
            pointx[1] = madvrtx[frame][vertex];
            pointy[1] = madvrty[frame][vertex];
            pointz[1] = madvrtz[frame][vertex];
            vertex = chrweapongrip[cnt][2];
            pointx[2] = madvrtx[frame][vertex];
            pointy[2] = madvrty[frame][vertex];
            pointz[2] = madvrtz[frame][vertex];
            vertex = chrweapongrip[cnt][3];
            pointx[3] = madvrtx[frame][vertex];
            pointy[3] = madvrty[frame][vertex];
            pointz[3] = madvrtz[frame][vertex];
            break;
    }





    tnc = 0;
    while(tnc < POINTS)
    {
        // Do the transform
        nupointx[tnc] = (pointx[tnc]*chrmatrix[character]_CNV(0,0) +
                       pointy[tnc]*chrmatrix[character]_CNV(1,0) +
                       pointz[tnc]*chrmatrix[character]_CNV(2,0));
        nupointy[tnc] = (pointx[tnc]*chrmatrix[character]_CNV(0,1) +
                       pointy[tnc]*chrmatrix[character]_CNV(1,1) +
                       pointz[tnc]*chrmatrix[character]_CNV(2,1));
        nupointz[tnc] = (pointx[tnc]*chrmatrix[character]_CNV(0,2) +
                       pointy[tnc]*chrmatrix[character]_CNV(1,2) +
                       pointz[tnc]*chrmatrix[character]_CNV(2,2));

        nupointx[tnc] += chrmatrix[character]_CNV(3,0);
        nupointy[tnc] += chrmatrix[character]_CNV(3,1);
        nupointz[tnc] += chrmatrix[character]_CNV(3,2);

        tnc++;
    }




    // Calculate weapon's matrix based on positions of grip points
    // chrscale is recomputed at time of attachment
    chrmatrix[cnt] = FourPoints(nupointx[0], nupointy[0], nupointz[0],
                                nupointx[1], nupointy[1], nupointz[1],
                                nupointx[2], nupointy[2], nupointz[2],
                                nupointx[3], nupointy[3], nupointz[3],
                                chrscale[cnt]);
}

//--------------------------------------------------------------------------------------------
void make_character_matrices()
{
    // ZZ> This function makes all of the character's matrices
    int cnt, tnc;

    // Forget about old matrices
    cnt = 0;
    while(cnt < MAXCHR)
    {
        chrmatrixvalid[cnt] = bfalse;
        cnt++;
    }


    // Do base characters
    tnc = 0;
    while(tnc < MAXCHR)
    {
        if(chrattachedto[tnc] == MAXCHR && chron[tnc])  // Skip weapons for now
        {
            make_one_character_matrix(tnc);
        }
        tnc++;
    }



    // Do first level of attachments
    tnc = 0;
    while(tnc < MAXCHR)
    {
        if(chrattachedto[tnc] != MAXCHR && chron[tnc])
        {
            if(chrattachedto[chrattachedto[tnc]] == MAXCHR)
            {
                make_one_weapon_matrix(tnc);
            }
        }
        tnc++;
    }


    // Do second level of attachments
    tnc = 0;
    while(tnc < MAXCHR)
    {
        if(chrattachedto[tnc] != MAXCHR && chron[tnc])
        {
            if(chrattachedto[chrattachedto[tnc]] != MAXCHR)
            {
                make_one_weapon_matrix(tnc);
            }
        }
        tnc++;
    }
}

//--------------------------------------------------------------------------------------------
int get_free_character()
{
    // ZZ> This function gets an unused character and returns its index
    int character;


    if(numfreechr==0)
    {
        // Return MAXCHR if we can't find one
        return MAXCHR;
    }
    else
    {
        // Just grab the next one
        numfreechr--;
        character=freechrlist[numfreechr];
    }
    return character;
}

//--------------------------------------------------------------------------------------------
unsigned char find_target_in_block(int x, int y, float chrx, float chry, unsigned short facing, 
  unsigned char onlyfriends, unsigned char anyone, unsigned char team,
  unsigned short donttarget, unsigned short oldtarget)
{
    // ZZ> This function helps find a target, returning btrue if it found a decent target
    int cnt;
    unsigned short angle;
    unsigned short charb;
    unsigned char enemies, returncode;
    unsigned int fanblock;
    int distance;



    returncode = bfalse;


    // Current fanblock
    if(x >= 0 && x < (meshsizex>>2) && y >= 0 && y < (meshsizey>>2))
    {
        fanblock = x+meshblockstart[y];


        enemies = bfalse;
        if(onlyfriends==bfalse) enemies = btrue;


        charb = meshbumplistchr[fanblock];
        cnt = 0;
        while(cnt < meshbumplistchrnum[fanblock])
        {
            if(chralive[charb] && chrinvictus[charb]==bfalse && charb != donttarget && charb != oldtarget)
            {
                if(anyone || (chrteam[charb]==team && onlyfriends) || (teamhatesteam[team][chrteam[charb]] && enemies))
                {
                    distance = ABS(chrxpos[charb]-chrx) + ABS(chrypos[charb]-chry);
                    if(distance < globestdistance)
                    {
                        angle = (atan2(chrypos[charb]-chry, chrxpos[charb]-chrx)+PI)*65535/(2*PI);
                        angle = facing-angle;
                        if(angle < globestangle || angle > (65535-globestangle))
                        {
                            returncode = btrue;
                            globesttarget = charb;
                            globestdistance = distance;
                            glouseangle = angle;
                            if(angle  > 32767)
                                globestangle = -angle;
                            else
                                globestangle = angle;
                        }
                    }
                }
            }
            charb = chrbumpnext[charb];
            cnt++;
        }
    }
    return returncode;
}

//--------------------------------------------------------------------------------------------
unsigned short find_target(float chrx, float chry, unsigned short facing, 
  unsigned short targetangle, unsigned char onlyfriends, unsigned char anyone,
  unsigned char team, unsigned short donttarget, unsigned short oldtarget)
{
    // This function finds the best target for the given parameters
    unsigned char done;
    int x, y;

    x = chrx;
    y = chry;
    x = x>>9;
    y = y>>9;
    globestdistance = 9999;
    globestangle = targetangle;
    done = find_target_in_block(x, y, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget);
    done |= find_target_in_block(x+1, y, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget);
    done |= find_target_in_block(x-1, y, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget);
    done |= find_target_in_block(x, y+1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget);
    done |= find_target_in_block(x, y-1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget);
    if(done) return globesttarget;


    done = find_target_in_block(x+1, y+1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget);
    done |= find_target_in_block(x+1, y-1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget);
    done |= find_target_in_block(x-1, y+1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget);
    done |= find_target_in_block(x-1, y-1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget);
    if(done) return globesttarget;


    return MAXCHR;
}

//--------------------------------------------------------------------------------------------
void free_all_characters()
{
    // ZZ> This function resets the character allocation list
    nolocalplayers = btrue;
    numfreechr = 0;
    while(numfreechr<MAXCHR)
    {
        chron[numfreechr] = bfalse;
        chralive[numfreechr] = bfalse;
        chrinpack[numfreechr] = bfalse;
        chrnuminpack[numfreechr] = 0;
        chrnextinpack[numfreechr] = MAXCHR;
        chrstaton[numfreechr] = bfalse;
        chrmatrixvalid[numfreechr] = bfalse;
        freechrlist[numfreechr] = numfreechr;
        numfreechr++;
    }
    numpla = 0;
    numlocalpla = 0;
    numstat = 0;
}

//--------------------------------------------------------------------------------------------
unsigned char __chrhitawall(int character)
{
    // ZZ> This function returns nonzero if the character hit a wall that the
    //     character is not allowed to cross
    unsigned char passtl, passtr, passbr, passbl;
    int x, y, bs;


    y = chrypos[character];  x = chrxpos[character];  bs = chrbumpsize[character]>>1;
    // !!!BAD!!! Should really do bound checking...
    passtl = meshfx[meshfanstart[y-bs>>7]+(x-bs>>7)];
    passtr = meshfx[meshfanstart[y-bs>>7]+(x+bs>>7)];
    passbr = meshfx[meshfanstart[y+bs>>7]+(x+bs>>7)];
    passbl = meshfx[meshfanstart[y+bs>>7]+(x-bs>>7)];
    passtl = (passtl|passtr|passbr|passbl)&chrstoppedby[character];
    return passtl;
}

//--------------------------------------------------------------------------------------------
void reset_character_accel(unsigned short character)
{
    // ZZ> This function fixes a character's max acceleration
    unsigned short enchant;

    if(character != MAXCHR)
    {
        if(chron[character])
        {
            // Okay, remove all acceleration enchants
            enchant = chrfirstenchant[character];
            while(enchant < MAXENCHANT)
            {
                remove_enchant_value(enchant, ADDACCEL);
                enchant = encnextenchant[enchant];
            }
            // Set the starting value
            chrmaxaccel[character] = capmaxaccel[chrmodel[character]][chrtexture[character]-madskinstart[chrmodel[character]]];
            // Put the acceleration enchants back on
            enchant = chrfirstenchant[character];
            while(enchant < MAXENCHANT)
            {
                add_enchant_value(enchant, ADDACCEL, enceve[enchant]);
                enchant = encnextenchant[enchant];
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void detach_character_from_mount(unsigned short character, unsigned char ignorekurse,
  unsigned char doshop)
{
    // ZZ> This function drops an item
    unsigned short mount, hand, enchant, cnt, passage, owner, price;
    unsigned char inshop;
    int loc;


    // Make sure the character is valid
    if(character==MAXCHR)
        return;


    // Make sure the character is mounted
    mount = chrattachedto[character];
    if(mount>=MAXCHR)
        return;


    // Make sure both are still around
    if(!chron[character] || !chron[mount])
        return;


    // Don't allow living characters to drop kursed weapons
    if(ignorekurse==bfalse && chriskursed[character] && chralive[mount] && chrisitem[character])
    {
        chralert[character] = chralert[character]|ALERTIFNOTDROPPED;
        return;
    }


    // Figure out which hand it's in
    hand = 0;
    if(chrinwhichhand[character] == GRIPRIGHT)
    {
        hand = 1;
    }


    // Rip 'em apart
    chrattachedto[character] = MAXCHR;
    if(chrholdingwhich[mount][0] == character)
        chrholdingwhich[mount][0] = MAXCHR;
    if(chrholdingwhich[mount][1] == character)
        chrholdingwhich[mount][1] = MAXCHR;
    chrscale[character] = chrfat[character]*madscale[chrmodel[character]]*4;


    // Run the falling animation...
    play_action(character, ACTIONJB+hand, bfalse);



    // Set the positions
    if(chrmatrixvalid[character])
    {
      chrxpos[character] = chrmatrix[character]_CNV(3,0);
      chrypos[character] = chrmatrix[character]_CNV(3,1);
      chrzpos[character] = chrmatrix[character]_CNV(3,2);
    }
    else
    {
      chrxpos[character] = chrxpos[mount];
      chrypos[character] = chrypos[mount];
      chrzpos[character] = chrzpos[mount];
    }



    // Make sure it's not dropped in a wall...
    if(__chrhitawall(character))
    {
        chrxpos[character] = chrxpos[mount];
        chrypos[character] = chrypos[mount];
    }


    // Check for shop passages
    inshop = bfalse;
    if(chrisitem[character] && numshoppassage != 0 && doshop)
    {
        cnt = 0;
        while(cnt < numshoppassage)
        {
            passage = shoppassage[cnt];
            loc = chrxpos[character];
            loc = loc >> 7;
            if(loc >= passtlx[passage] && loc <= passbrx[passage])
            {
                loc = chrypos[character];
                loc = loc >> 7;
                if(loc >= passtly[passage] && loc <= passbry[passage])
                {
                    inshop = btrue;
                    owner = shopowner[passage];
                    cnt = numshoppassage;  // Finish loop
                    if(owner == NOOWNER)
                    {
                        // The owner has died!!!
                        inshop = bfalse;
                    }
                }
            }
            cnt++;
        }
        if(inshop)
        {
            // Give the mount its money back, alert the shop owner
            price = capskincost[chrmodel[character]][0];
            if(capisstackable[chrmodel[character]])
            {
                price = price * chrammo[character];
            }
            chrmoney[mount] += price;
            chrmoney[owner] -= price;
            if(chrmoney[owner] < 0)  chrmoney[owner] = 0;
            if(chrmoney[mount] > MAXMONEY)  chrmoney[mount] = MAXMONEY;
            chralert[owner] |= ALERTIFORDERED;
            chrorder[owner] = price;  // Tell owner how much...
            chrcounter[owner] = 0;  // 0 for buying an item
        }
    }



    // Make sure it works right
    chrhitready[character]=btrue;
    if(inshop)
    {
      // Drop straight down to avoid theft
      chrxvel[character]=0;
      chryvel[character]=0;
    }
    else
    {
      chrxvel[character]=chrxvel[mount];
      chryvel[character]=chryvel[mount];
    }
    chrzvel[character]=DROPZVEL;


    // Turn looping off
    chrloopaction[character]=bfalse;


    // Reset the team if it is a mount
    if(chrismount[mount])
    {
        chrteam[mount] = chrbaseteam[mount];
        chralert[mount] |= ALERTIFDROPPED;
    }
    chrteam[character] = chrbaseteam[character];
    chralert[character] |= ALERTIFDROPPED;


    // Reset transparency
    if(chrisitem[character] && chrtransferblend[mount])
    {
        // Okay, reset transparency
        enchant = chrfirstenchant[character];
        while(enchant < MAXENCHANT)
        {
            unset_enchant_value(enchant, SETALPHABLEND);
            unset_enchant_value(enchant, SETLIGHTBLEND);
            enchant = encnextenchant[enchant];
        }
        chralpha[character] = capalpha[chrmodel[character]];
        chrlight[character] = caplight[chrmodel[character]];
        enchant = chrfirstenchant[character];
        while(enchant < MAXENCHANT)
        {
            set_enchant_value(enchant, SETALPHABLEND, enceve[enchant]);
            set_enchant_value(enchant, SETLIGHTBLEND, enceve[enchant]);
            enchant = encnextenchant[enchant];
        }
    }


    // Set twist
    chrturnmaplr[character] = 32768;
    chrturnmapud[character] = 32768;
}

//--------------------------------------------------------------------------------------------
void attach_character_to_mount(unsigned short character, unsigned short mount,
    unsigned short grip)
{
    // ZZ> This function attaches one character to another ( the mount )
    //     at either the left or right grip
    int tnc, hand;


    // Make sure both are still around
    if(!chron[character] || !chron[mount] || chrinpack[character] || chrinpack[mount])
        return;

    // Figure out which hand this grip relates to
    hand = 1;
    if(grip == GRIPLEFT)
        hand = 0;


    // Make sure the the hand is valid
    if(capgripvalid[chrmodel[mount]][hand]==bfalse)
        return;


    // Put 'em together
    chrinwhichhand[character] = grip;
    chrattachedto[character] = mount;
    chrholdingwhich[mount][hand] = character;
    tnc = madvertices[chrmodel[mount]] - grip;
    chrweapongrip[character][0] = tnc;
    chrweapongrip[character][1] = tnc+1;
    chrweapongrip[character][2] = tnc+2;
    chrweapongrip[character][3] = tnc+3;
    chrscale[character] = chrfat[character]/(chrfat[mount]*1280);
    chrinwater[character] = bfalse;
    chrjumptime[character]=JUMPDELAY*4;


    // Run the held animation
    if(chrismount[mount] && grip == GRIPONLY)
    {
        // Riding mount
        play_action(character, ACTIONMI, btrue);
        chrloopaction[character]=btrue;
    }
    else
    {
        play_action(character, ACTIONMM+hand, bfalse);
        if(chrisitem[character])
        {
            // Item grab
            chrkeepaction[character]=btrue;
        }
    }




    // Set the team
    if(chrisitem[character])
    {
        chrteam[character]=chrteam[mount];
        // Set the alert
        chralert[character] = chralert[character]|ALERTIFGRABBED;
    }
    if(chrismount[mount])
    {
        chrteam[mount]=chrteam[character];
        // Set the alert
        if(chrisitem[mount]==bfalse)
        {
            chralert[mount] = chralert[mount]|ALERTIFGRABBED;
        }
    }


    // It's not gonna hit the floor
    chrhitready[character]=bfalse;
}

//--------------------------------------------------------------------------------------------
unsigned short stack_in_pack(unsigned short item, unsigned short character)
{
    // ZZ> This function looks in the character's pack for an item similar
    //     to the one given.  If it finds one, it returns the similar item's
    //     index number, otherwise it returns MAXCHR.
    unsigned short inpack, id;
    unsigned char allok;


    if(capisstackable[chrmodel[item]]==btrue)
    {
        inpack = chrnextinpack[character];
        allok = bfalse;
        while(inpack != MAXCHR && allok == bfalse)
        {
            allok = btrue;
            if(chrmodel[inpack]!=chrmodel[item])
            {
                if(capisstackable[chrmodel[inpack]]==bfalse)
                {
                    allok = bfalse;
                }

                if(chrammomax[inpack]!=chrammomax[item])
                {
                    allok = bfalse;
                }

                id = 0;
                while(id < MAXIDSZ && allok == btrue)
                {
                    if(capidsz[chrmodel[inpack]][id]!=capidsz[chrmodel[item]][id])
                    {
                        allok = bfalse;
                    }
                    id++;
                }
            }
            if(allok == bfalse)
            {
                inpack = chrnextinpack[inpack];
            }
        }
        if(allok == btrue)
        {
            return inpack;
        }
    }
    return MAXCHR;
}

//--------------------------------------------------------------------------------------------
void add_item_to_character_pack(unsigned short item, unsigned short character)
{
    // ZZ> This function puts one character inside the other's pack
    unsigned short oldfirstitem, newammo, stack;


    // Make sure everything is hunkydori
    if((!chron[item]) || (!chron[character]) || chrinpack[item] || chrinpack[character] ||
       chrisitem[character])
        return;


    stack = stack_in_pack(item, character);
    if(stack != MAXCHR)
    {
        // We found a similar, stackable item in the pack
        if(chrnameknown[item] || chrnameknown[stack])
        {
            chrnameknown[item] = btrue;
            chrnameknown[stack] = btrue;
        }
        if(capusageknown[chrmodel[item]] || capusageknown[chrmodel[stack]])
        {
            capusageknown[chrmodel[item]] = btrue;
            capusageknown[chrmodel[stack]] = btrue;
        }
        newammo = chrammo[item] + chrammo[stack];
        if(newammo <= chrammomax[stack])
        {
            // All transfered, so kill the in hand item
            chrammo[stack] = newammo;
            if(chrattachedto[item] != MAXCHR)
            {
                detach_character_from_mount(item, btrue, bfalse);
            }
            free_one_character(item);
        }
        else
        {
            // Only some were transfered, 
            chrammo[item] = chrammo[item]+chrammo[stack]-chrammomax[stack];
            chrammo[stack] = chrammomax[stack];
            chralert[character]|=ALERTIFTOOMUCHBAGGAGE;
        }
    }
    else
    {
        // Make sure we have room for another item
        if(chrnuminpack[character] >= MAXNUMINPACK)
        {
            chralert[character]|=ALERTIFTOOMUCHBAGGAGE;
            return;
        }


        // Take the item out of hand
        if(chrattachedto[item] != MAXCHR)
        {
            detach_character_from_mount(item, btrue, bfalse);
            chralert[item] &= (~ALERTIFDROPPED);
        }


        // Remove the item from play
        chrhitready[item] = bfalse;
        chrinpack[item] = btrue;


        // Insert the item into the pack as the first one
        oldfirstitem = chrnextinpack[character];
        chrnextinpack[character] = item;
        chrnextinpack[item] = oldfirstitem;
        chrnuminpack[character]++;
        if(capisequipment[chrmodel[item]])
        {
            // AtLastWaypoint doubles as PutAway
            chralert[item]|=ALERTIFATLASTWAYPOINT;
        }
    }
    return;
}

//--------------------------------------------------------------------------------------------
unsigned short get_item_from_character_pack(unsigned short character, unsigned short grip, unsigned char ignorekurse)
{
    // ZZ> This function takes the last item in the character's pack and puts
    //     it into the designated hand.  It returns the item number or MAXCHR.
    unsigned short item, nexttolastitem;


    // Make sure everything is hunkydori
    if((!chron[character]) || chrinpack[character] || chrisitem[character] || chrnextinpack[character]==MAXCHR)
        return MAXCHR;
    if(chrnuminpack[character] == 0)
        return MAXCHR;


    // Find the last item in the pack
    nexttolastitem = character;
    item = chrnextinpack[character];
    while(chrnextinpack[item] != MAXCHR)
    {
        nexttolastitem = item;
        item = chrnextinpack[item];
    }


    // Figure out what to do with it
    if(chriskursed[item] && chrisequipped[item] && ignorekurse == bfalse)
    {
        // Flag the last item as not removed
        chralert[item] |= ALERTIFNOTPUTAWAY;  // Doubles as IfNotTakenOut
        // Cycle it to the front
        chrnextinpack[item] = chrnextinpack[character];
        chrnextinpack[nexttolastitem] = MAXCHR;
        chrnextinpack[character] = item;
        if(character == nexttolastitem)
        {
            chrnextinpack[item] = MAXCHR;
        }
        return MAXCHR;
    }
    else
    {
        // Remove the last item from the pack
        chrinpack[item] = bfalse;
        chrisequipped[item] = bfalse;
        chrnextinpack[nexttolastitem] = MAXCHR;
        chrnuminpack[character]--;
        chrteam[item]=chrteam[character];


        // Attach the item to the character's hand
        attach_character_to_mount(item, character, grip);
        chralert[item]&=(~ALERTIFGRABBED);
        chralert[item]|=(ALERTIFTAKENOUT);
    }
    return item;
}

//--------------------------------------------------------------------------------------------
void drop_keys(unsigned short character)
{
    // ZZ> This function drops all keys ( [KEYA] to [KEYZ] ) that are in a character's
    //     inventory ( Not hands ).
    unsigned short item, lastitem, nextitem, direction, cosdir;
    int testa, testz;


    if(character < MAXCHR)
    {
        if(chron[character])
        {
            if(chrzpos[character] > -2) // Don't lose keys in pits...
            {
                // The IDSZs to find
                testa = ('K'-'A'<<15)|('E'-'A'<<10)|('Y'-'A'<<5)|('A'-'A');  // [KEYA]
                testz = ('K'-'A'<<15)|('E'-'A'<<10)|('Y'-'A'<<5)|('Z'-'A');  // [KEYZ]


                lastitem = character;
                item = chrnextinpack[character];
                while(item != MAXCHR)
                {
                    nextitem = chrnextinpack[item];
                    if(item != character)  // Should never happen...
                    {
                        if((capidsz[chrmodel[item]][IDSZPARENT]>=(unsigned int) testa &&
                            capidsz[chrmodel[item]][IDSZPARENT]<=(unsigned int) testz) ||
                           (capidsz[chrmodel[item]][IDSZTYPE]>=(unsigned int) testa &&
                            capidsz[chrmodel[item]][IDSZTYPE]<=(unsigned int) testz))
                        {
                            // We found a key...
                            chrinpack[item] = bfalse;
                            chrisequipped[item] = bfalse;
                            chrnextinpack[lastitem] = nextitem;
                            chrnextinpack[item] = MAXCHR;
                            chrnuminpack[character]--;
                            chrattachedto[item] = MAXCHR;
                            chralert[item] |= ALERTIFDROPPED;
                            chrhitready[item]=btrue;


                            direction = RANDIE;
                            chrturnleftright[item] = direction+32768;
                            cosdir = direction + 16384;
                            chrlevel[item] = chrlevel[character];
                            chrxpos[item] = chrxpos[character];
                            chrypos[item] = chrypos[character];
                            chrzpos[item] = chrzpos[character];
                            chrxvel[item] = turntosin[cosdir>>2]*DROPXYVEL;
                            chryvel[item] = turntosin[direction>>2]*DROPXYVEL;
                            chrzvel[item] = DROPZVEL;
                            chrteam[item] = chrbaseteam[item];
                        }
                        else
                        {
                            lastitem = item;
                        }
                    }
                    item = nextitem;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void drop_all_items(unsigned short character)
{
    // ZZ> This function drops all of a character's items
    unsigned short item, direction, cosdir, diradd;


    if(character < MAXCHR)
    {
        if(chron[character])
        {
            detach_character_from_mount(chrholdingwhich[character][0], btrue, bfalse);
            detach_character_from_mount(chrholdingwhich[character][1], btrue, bfalse);
            if(chrnuminpack[character] > 0)
            {
                direction = chrturnleftright[character]+32768;
                diradd = 65535 / chrnuminpack[character];
                while(chrnuminpack[character] > 0)
                {
                    item = get_item_from_character_pack(character, GRIPLEFT, bfalse);
                    if(item < MAXCHR)
                    {
                        detach_character_from_mount(item, btrue, btrue);
                        chrhitready[item]=btrue;
                        chralert[item]|=ALERTIFDROPPED;
                        chrxpos[item] = chrxpos[character];
                        chrypos[item] = chrypos[character];
                        chrzpos[item] = chrzpos[character];
                        chrlevel[item] = chrlevel[character];
                        chrturnleftright[item] = direction+32768;
                        cosdir = direction + 16384;
                        chrxvel[item] = turntosin[cosdir>>2]*DROPXYVEL;
                        chryvel[item] = turntosin[direction>>2]*DROPXYVEL;
                        chrzvel[item] = DROPZVEL;
                        chrteam[item] = chrbaseteam[item];
                    } 
                    direction += diradd;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void character_grab_stuff(int chara, int grip, unsigned char people)
{
    // ZZ> This function makes the character pick up an item if there's one around
    float xa, ya, za, xb, yb, zb, dist;
    int charb, hand;
    unsigned short vertex, model, frame, owner, passage, cnt, price;
    float pointx, pointy, pointz;
    unsigned char inshop;
    int loc;


    // Make life easier
    model = chrmodel[chara];
    hand = (grip-4)>>2;  // 0 is left, 1 is right


    // Make sure the character doesn't have something already, and that it has hands
    if(chrholdingwhich[chara][hand]!=MAXCHR || capgripvalid[model][hand] == bfalse)
        return;


    // Do we have a matrix???
    if(chrmatrixvalid[chara])//meshinrenderlist[chronwhichfan[chara]])
    {
        // Transform the weapon grip from model to world space
        frame = chrframe[chara];
        vertex = madvertices[model] - grip;


        // Calculate grip point locations
        pointx = madvrtx[frame][vertex];///chrscale[cnt];
        pointy = madvrty[frame][vertex];///chrscale[cnt];
        pointz = madvrtz[frame][vertex];///chrscale[cnt];


        // Do the transform
        xa = (pointx*chrmatrix[chara]_CNV(0,0) +
              pointy*chrmatrix[chara]_CNV(1,0) +
              pointz*chrmatrix[chara]_CNV(2,0));
        ya = (pointx*chrmatrix[chara]_CNV(0,1) +
              pointy*chrmatrix[chara]_CNV(1,1) +
              pointz*chrmatrix[chara]_CNV(2,1));
        za = (pointx*chrmatrix[chara]_CNV(0,2) +
              pointy*chrmatrix[chara]_CNV(1,2) +
              pointz*chrmatrix[chara]_CNV(2,2));
        xa += chrmatrix[chara]_CNV(3,0);
        ya += chrmatrix[chara]_CNV(3,1);
        za += chrmatrix[chara]_CNV(3,2);
    }
    else
    {
        // Just wing it
        xa = chrxpos[chara];
        ya = chrypos[chara];
        za = chrzpos[chara];
    }



    // Go through all characters to find the best match
    charb = 0;
    while(charb < MAXCHR)
    {
        if(chron[charb] && (!chrinpack[charb]) && chrweight[charb]<chrweight[chara] && chralive[charb] && chrattachedto[charb]==MAXCHR && ((people==bfalse&&chrisitem[charb]) || (people==btrue&&chrisitem[charb]==bfalse)))
        {
            xb = chrxpos[charb];
            yb = chrypos[charb];
            zb = chrzpos[charb];
            // First check absolute value diamond
            xb = ABS(xa-xb);
            yb = ABS(ya-yb);
            zb = ABS(za-zb);
            dist = xb+yb;
            if(dist < GRABSIZE && zb < GRABSIZE)
            {
                // Don't grab your mount
                if(chrholdingwhich[charb][0]!=chara && chrholdingwhich[charb][1]!=chara)
                {
                    // Check for shop
                    inshop = bfalse;
                    if(chrisitem[charb] && numshoppassage != 0)
                    {
                        cnt = 0;
                        while(cnt < numshoppassage)
                        {
                            passage = shoppassage[cnt];
                            loc = chrxpos[charb];
                            loc = loc >> 7;
                            if(loc >= passtlx[passage] && loc <= passbrx[passage])
                            {
                                loc = chrypos[charb];
                                loc = loc >> 7;
                                if(loc >= passtly[passage] && loc <= passbry[passage])
                                {
                                    inshop = btrue;
                                    owner = shopowner[passage];
                                    cnt = numshoppassage;  // Finish loop
                                    if(owner == NOOWNER)
                                    {
                                        // The owner has died!!!
                                        inshop = bfalse;
                                    }
                                }
                            }
                            cnt++;
                        }
                        if(inshop)
                        {
                            // Pay the shop owner, or don't allow grab...
                            if(chrisitem[chara])
                            {
                                // Pets can shop for free =]
                                inshop = bfalse;
                            }
                            else
                            {
                                chralert[owner] |= ALERTIFORDERED;
                                price = capskincost[chrmodel[charb]][0];
                                if(capisstackable[chrmodel[charb]])
                                {
                                    price = price * chrammo[charb];
                                }
                                chrorder[owner] = price;  // Tell owner how much...
                                if(chrmoney[chara] >= price)
                                {
                                    // Okay to buy
                                    chrcounter[owner] = 1;  // 1 for selling an item
                                    chrmoney[chara] -= price;  // Skin 0 cost is price
                                    chrmoney[owner] += price;
                                    if(chrmoney[owner] > MAXMONEY)  chrmoney[owner] = MAXMONEY;
                                    inshop = bfalse;
                                }
                                else
                                {
                                    // Don't allow purchase
                                    chrcounter[owner] = 2;  // 2 for "you can't afford that"
                                    inshop = btrue;
                                }
                            }
                        }
                    }


                    if(inshop == bfalse)
                    {
                        // Stick 'em together and quit
                        attach_character_to_mount(charb, chara, grip);
                        charb=MAXCHR;
                        if(people==btrue)
                        {
                            // Do a slam animation...  ( Be sure to drop!!! )
                            play_action(chara, ACTIONMC+hand, bfalse);
                        }
                    }
                    else
                    {
                        // Lift the item a little and quit...
                        chrzvel[charb] = DROPZVEL;
                        chrhitready[charb]=btrue;
                        chralert[charb] |= ALERTIFDROPPED;
                        charb=MAXCHR;
                    }
                }
            }
        }
        charb++;
    }
}

//--------------------------------------------------------------------------------------------
void character_swipe(unsigned short cnt, unsigned char grip)
{
    // ZZ> This function spawns an attack particle
    int weapon, particle, spawngrip, thrown;
    unsigned char action;
    unsigned short tTmp;
    float dampen;
    float x, y, z, velocity;


    weapon = chrholdingwhich[cnt][grip];
    spawngrip = SPAWNLAST;
    action = chraction[cnt];
    // See if it's an unarmed attack...
    if(weapon==MAXCHR)
    {
        weapon = cnt;
        spawngrip = 4+(grip<<2);  // 0 = GRIPLEFT, 1 = GRIPRIGHT
    }


    if(weapon!=cnt && ((capisstackable[chrmodel[weapon]] && chrammo[weapon]>1) || (action >= ACTIONFA && action <= ACTIONFD)))
    {
        // Throw the weapon if it's stacked or a hurl animation
        x = chrxpos[cnt];
        y = chrypos[cnt];
        z = chrzpos[cnt];
        thrown = spawn_one_character(x, y, z, chrmodel[weapon], chrteam[cnt], 0, chrturnleftright[cnt], chrname[weapon], MAXCHR);
        if(thrown < MAXCHR)
        {
            chriskursed[thrown] = bfalse;
            chrammo[thrown] = 1;
            chralert[thrown]|=ALERTIFTHROWN;
            velocity = chrstrength[cnt]/(chrweight[thrown]*THROWFIX);
            velocity += MINTHROWVELOCITY;
            if(velocity > MAXTHROWVELOCITY)
            {
                velocity = MAXTHROWVELOCITY;
            }
            tTmp = chrturnleftright[cnt]>>2;
            chrxvel[thrown]+=turntosin[(tTmp+12288)&16383]*velocity;
            chryvel[thrown]+=turntosin[(tTmp+8192)&16383]*velocity;
            chrzvel[thrown] = DROPZVEL;
            if(chrammo[weapon]<=1)
            {
                // Poof the item
                detach_character_from_mount(weapon, btrue, bfalse);
                free_one_character(weapon);
            }
            else
            {
                chrammo[weapon]--;
            }
        }
    }
    else
    {
        // Spawn an attack particle
        if(chrammomax[weapon]==0||chrammo[weapon]!=0)
        {
            if(chrammo[weapon]>0 && capisstackable[chrmodel[weapon]]==bfalse)
            {
                chrammo[weapon]--;  // Ammo usage
            }
			//HERE
            if(capattackprttype[chrmodel[weapon]]!=-1)
            {
                particle = spawn_one_particle(chrxpos[weapon], chrypos[weapon], chrzpos[weapon], chrturnleftright[cnt], chrmodel[weapon], capattackprttype[chrmodel[weapon]], weapon, spawngrip, chrteam[cnt], cnt, 0, MAXCHR);
                if(particle!=MAXPRT)
                {
                    if(capattackattached[chrmodel[weapon]]==bfalse)
                    {
                        // Detach the particle
                        if(pipstartontarget[prtpip[particle]]==bfalse || prttarget[particle]==MAXCHR)
                        {
                            attach_particle_to_character(particle, weapon, spawngrip);
                            // Correct Z spacing base, but nothing else...
                            prtzpos[particle]+=pipzspacingbase[prtpip[particle]];
                        }
                        prtattachedtocharacter[particle]=MAXCHR;
                        // Don't spawn in walls
                        if(__prthitawall(particle))
                        {
                            prtxpos[particle] = chrxpos[weapon];
                            prtypos[particle] = chrypos[weapon];
                            if(__prthitawall(particle))
                            {
                                prtxpos[particle] = chrxpos[cnt];
                                prtypos[particle] = chrypos[cnt];
                            }
                        }
					}
					else
                    {
                        // Attached particles get a strength bonus for reeling...
                        dampen = REELBASE+(chrstrength[cnt]/REEL);
                        prtxvel[particle] = prtxvel[particle]*dampen;
                        prtyvel[particle] = prtyvel[particle]*dampen;
                        prtzvel[particle] = prtzvel[particle]*dampen;
                    }
					
                    // Initial particles get a strength bonus, which may be 0.00
                    prtdamagebase[particle]+=(chrstrength[cnt]*capstrengthdampen[chrmodel[weapon]]);
                    // Initial particles get an enchantment bonus
                    prtdamagebase[particle]+=chrdamageboost[weapon];
                    // Initial particles inherit damage type of weapon
                    prtdamagetype[particle] = chrdamagetargettype[weapon];
                }
            }
        }
        else 
        {
            chrammoknown[weapon] = btrue;
        }
    }
}

//--------------------------------------------------------------------------------------------
void move_characters(void)
{
    // ZZ> This function handles character physics
    int cnt;
    unsigned int mapud, maplr;
    unsigned char twist, actionready;
    unsigned char speed, framelip, allowedtoattack;
    float level, friction;
    float dvx, dvy, dvmax;
    unsigned short action, weapon, mount, item;
    int distance, volume;
    unsigned char watchtarget, grounded;


    // Move every character
    cnt = 0;
    while(cnt < MAXCHR)
    {
        if(chron[cnt] && (!chrinpack[cnt]))
        {
            grounded = bfalse;
            valuegopoof = bfalse;
            // Down that ol' damage timer
            chrdamagetime[cnt]-=(chrdamagetime[cnt] != 0);


            // Character's old location
            chroldx[cnt] = chrxpos[cnt];
            chroldy[cnt] = chrypos[cnt];
            chroldz[cnt] = chrzpos[cnt];
            chroldturn[cnt] = chrturnleftright[cnt];
//            if(chrattachedto[cnt]!=MAXCHR)
//            {
//                chrturnleftright[cnt] = chrturnleftright[chrattachedto[cnt]];
//                if(chrindolist[cnt]==bfalse)
//                {
//                    chrxpos[cnt] = chrxpos[chrattachedto[cnt]];
//                    chrypos[cnt] = chrypos[chrattachedto[cnt]];
//                    chrzpos[cnt] = chrzpos[chrattachedto[cnt]];
//                }
//            }


            // Texture movement
            chruoffset[cnt]+=chruoffvel[cnt];
            chrvoffset[cnt]+=chrvoffvel[cnt];


            if(chralive[cnt])
            {
                if(chrattachedto[cnt]==MAXCHR)
                {
                    // Character latches for generalized movement
                    dvx = chrlatchx[cnt];
                    dvy = chrlatchy[cnt];


                    // Reverse movements for daze
                    if(chrdazetime[cnt] > 0)
                    {
                        dvx=-dvx;
                        dvy=-dvy;
                    }
                    // Switch x and y for daze
                    if(chrgrogtime[cnt] > 0)
                    {
                        dvmax = dvx;
                        dvx=dvy;
                        dvy=dvmax;
                    }



                    // Get direction from the DESIRED change in velocity
                    if(chrturnmode[cnt]==TURNMODEWATCH)
                    {
                        if((ABS(dvx) > WATCHMIN || ABS(dvy) > WATCHMIN))
                        {
                            chrturnleftright[cnt] = terp_dir(chrturnleftright[cnt],(atan2(dvy, dvx)+PI)*65535/(2*PI));
                        }
                    }
                    // Face the target
                    watchtarget = (chrturnmode[cnt]==TURNMODEWATCHTARGET);
                    if(watchtarget)
                    {
                        if(cnt != chraitarget[cnt])
                            chrturnleftright[cnt] = terp_dir(chrturnleftright[cnt],(atan2(chrypos[chraitarget[cnt]]-chrypos[cnt], chrxpos[chraitarget[cnt]]-chrxpos[cnt])+PI)*65535/(2*PI));
                    }



                    if(madframefx[chrframe[cnt]]&MADFXSTOP)
                    {
                        dvx = 0;
                        dvy = 0;
                    }
                    else
                    {
                        // Limit to max acceleration
                        dvmax = chrmaxaccel[cnt];
                        if(dvx < -dvmax) dvx=-dvmax;
                        if(dvx > dvmax) dvx=dvmax;
                        if(dvy < -dvmax) dvy=-dvmax;
                        if(dvy > dvmax) dvy=dvmax;
                        chrxvel[cnt]+=dvx;
                        chryvel[cnt]+=dvy;
                    }


                    // Get direction from ACTUAL change in velocity
                    if(chrturnmode[cnt]==TURNMODEVELOCITY)
                    {
                        if(dvx<-TURNSPD||dvx>TURNSPD||dvy<-TURNSPD||dvy>TURNSPD)
                        {
                            if(chrisplayer[cnt])
                            {
                                // Players turn quickly
                                chrturnleftright[cnt] = terp_dir_fast(chrturnleftright[cnt],(atan2(dvy, dvx)+PI)*65535/(2*PI));
                            }
                            else
                            {
                                // AI turn slowly
                                chrturnleftright[cnt] = terp_dir(chrturnleftright[cnt],(atan2(dvy, dvx)+PI)*65535/(2*PI));
                            }
                        }
                    }
                    // Otherwise make it spin
                    else if(chrturnmode[cnt]==TURNMODESPIN)
                    {
                        chrturnleftright[cnt]+=SPINRATE;
                    }
                }


                // Character latches for generalized buttons
                if(chrlatchbutton[cnt]!=0)
                {
                    if(chrlatchbutton[cnt]&LATCHBUTTONJUMP)
                    {
                        if(chrattachedto[cnt]!=MAXCHR && chrjumptime[cnt]==0)
                        {
                            detach_character_from_mount(cnt, btrue, btrue);
                            chrjumptime[cnt]=JUMPDELAY;
                            chrzvel[cnt]=DISMOUNTZVEL;
                            if(chrflyheight[cnt] != 0)
                                chrzvel[cnt]=DISMOUNTZVELFLY;
                            chrzpos[cnt]+=chrzvel[cnt];
                            if(chrjumpnumberreset[cnt]!=JUMPINFINITE && chrjumpnumber[cnt] != 0)
                              chrjumpnumber[cnt]--;
                            // Play the jump sound
							play_sound(chrxpos[cnt], chrypos[cnt], capwaveindex[chrmodel[cnt]][capwavejump[chrmodel[cnt]]]);

                        }
                        if(chrjumptime[cnt]==0 && chrjumpnumber[cnt]!=0 && chrflyheight[cnt] == 0)
                        {
                            if(chrjumpnumberreset[cnt]!=1 || chrjumpready[cnt])
                            {
                                // Make the character jump
                                chrhitready[cnt]=btrue;
                                if(chrinwater[cnt])
                                {
                                    chrzvel[cnt]=WATERJUMP;
                                }
                                else
                                {
                                    chrzvel[cnt]=chrjump[cnt];
                                }
                                chrjumptime[cnt]=JUMPDELAY;
                                chrjumpready[cnt]=bfalse;
                                if(chrjumpnumberreset[cnt]!=JUMPINFINITE) chrjumpnumber[cnt]--;
                                // Set to jump animation if not doing anything better
                                if(chractionready[cnt])    play_action(cnt, ACTIONJA, btrue);
                                // Play the jump sound (Boing!)
                                distance = ABS(camtrackx-chrxpos[cnt])+ABS(camtracky-chrypos[cnt]);
                                volume = -distance;

                                if(volume > VOLMIN)
								{
                                    play_sound(chrxpos[cnt], chrypos[cnt], capwaveindex[chrmodel[cnt]][capwavejump[chrmodel[cnt]]]);
								}

                            }
                        }
                    }
                    if((chrlatchbutton[cnt]&LATCHBUTTONALTLEFT)&&chractionready[cnt]&&chrreloadtime[cnt]==0)
                    {
                        chrreloadtime[cnt] = GRABDELAY;
                        if(chrholdingwhich[cnt][0]==MAXCHR)
                        {
                            // Grab left
                            play_action(cnt, ACTIONME, bfalse);
                        }
                        else
                        {
                            // Drop left
                            play_action(cnt, ACTIONMA, bfalse);
                        }
                    }
                    if((chrlatchbutton[cnt]&LATCHBUTTONALTRIGHT)&&chractionready[cnt]&&chrreloadtime[cnt]==0)
                    {
                        chrreloadtime[cnt] = GRABDELAY;
                        if(chrholdingwhich[cnt][1]==MAXCHR)
                        {
                            // Grab right
                            play_action(cnt, ACTIONMF, bfalse);
                        }
                        else
                        {
                            // Drop right
                            play_action(cnt, ACTIONMB, bfalse);
                        }
                    }
                    if((chrlatchbutton[cnt]&LATCHBUTTONPACKLEFT)&&chractionready[cnt]&&chrreloadtime[cnt]==0)
                    {
                        chrreloadtime[cnt] = PACKDELAY;
                        item = chrholdingwhich[cnt][0];
                        if(item!=MAXCHR)
                        {
                            if((chriskursed[item] || capistoobig[chrmodel[item]]) && capisequipment[chrmodel[item]]==bfalse)
                            {
                                // The item couldn't be put away
                                chralert[item]|=ALERTIFNOTPUTAWAY;
                            }
                            else
                            {
                                // Put the item into the pack
                                add_item_to_character_pack(item, cnt);
                            }
                        }
                        else
                        {
                            // Get a new one out and put it in hand
                            get_item_from_character_pack(cnt, GRIPLEFT, bfalse);
                        }
                        // Make it take a little time
                        play_action(cnt, ACTIONMG, bfalse);
                    }
                    if((chrlatchbutton[cnt]&LATCHBUTTONPACKRIGHT)&&chractionready[cnt]&&chrreloadtime[cnt]==0)
                    {
                        chrreloadtime[cnt] = PACKDELAY;
                        item = chrholdingwhich[cnt][1];
                        if(item!=MAXCHR)
                        {
                            if((chriskursed[item] || capistoobig[chrmodel[item]]) && capisequipment[chrmodel[item]]==bfalse)
                            {
                                // The item couldn't be put away
                                chralert[item]|=ALERTIFNOTPUTAWAY;
                            }
                            else
                            {
                                // Put the item into the pack
                                add_item_to_character_pack(item, cnt);
                            }
                        }
                        else
                        {
                            // Get a new one out and put it in hand
                            get_item_from_character_pack(cnt, GRIPRIGHT, bfalse);
                        }
                        // Make it take a little time
                        play_action(cnt, ACTIONMG, bfalse);
                    }
                    if(chrlatchbutton[cnt]&LATCHBUTTONLEFT&&chrreloadtime[cnt]==0)
                    {
                        // Which weapon?
                        weapon = chrholdingwhich[cnt][0];
                        if(weapon == MAXCHR)
                        {
                            // Unarmed means character itself is the weapon
                            weapon = cnt;
                        }
                        action = capweaponaction[chrmodel[weapon]];


                        // Can it do it?
                        allowedtoattack = btrue;
                        if(madactionvalid[chrmodel[cnt]][action]==bfalse || chrreloadtime[weapon] > 0 ||
                           (capneedskillidtouse[chrmodel[weapon]] && capidsz[chrmodel[cnt]][IDSZSKILL] != capidsz[chrmodel[weapon]][IDSZSKILL]))
                        {
                            allowedtoattack = bfalse;
                            if(chrreloadtime[weapon] == 0)
                            {
                                // This character can't use this weapon
                                chrreloadtime[weapon] = 50;
                                if(chrstaton[cnt])
                                {
                                    // Tell the player that they can't use this weapon
                                    sprintf(generictext, "%s can't use this item...", chrname[cnt]);
                                    debug_message(generictext);
                                }
                            }
                        }
                        if(action==ACTIONDA)
                        {
                            allowedtoattack = bfalse;
                            if(chrreloadtime[weapon]==0)
                            {
                                chralert[weapon]=chralert[weapon]|ALERTIFUSED;
                            }
                        }


                        if(allowedtoattack)
                        {
                            // Rearing mount
                            mount = chrattachedto[cnt];
                            if(mount!=MAXCHR)
                            {
                                allowedtoattack = capridercanattack[chrmodel[mount]];
                                if(chrismount[mount]&&chralive[mount]&&chrisplayer[mount]==bfalse&&chractionready[mount])
                                {
                                    if((action != ACTIONPA || allowedtoattack == bfalse) && chractionready[cnt])
                                    {
                                        play_action(chrattachedto[cnt], ACTIONUA+(rand()&1), bfalse);
                                        chralert[chrattachedto[cnt]]|=ALERTIFUSED;
                                    }
                                    else
                                    {
                                        allowedtoattack = bfalse;
                                    }
                                }
                            }


                            // Attack button
                            if(allowedtoattack)
                            {
                                if(chractionready[cnt] && madactionvalid[chrmodel[cnt]][action])
                                {
                                    // Check mana cost
                                    if(chrmana[cnt] >= chrmanacost[weapon] || chrcanchannel[cnt])
                                    {
                                        cost_mana(cnt, chrmanacost[weapon], weapon);
                                        // Check life healing
                                        chrlife[cnt]+=chrlifeheal[weapon];
                                        if(chrlife[cnt] > chrlifemax[cnt])  chrlife[cnt] = chrlifemax[cnt];
                                        actionready = bfalse;
                                        if(action == ACTIONPA)
                                            actionready = btrue;
                                        action+=rand()&1;
                                        play_action(cnt, action, actionready);
                                        if(weapon!=cnt)
                                        {
                                            // Make the weapon attack too
                                            play_action(weapon, ACTIONMJ, bfalse);
                                            chralert[weapon]=chralert[weapon]|ALERTIFUSED;
                                        }
                                        else
                                        {
                                            // Flag for unarmed attack
                                            chralert[cnt]|=ALERTIFUSED;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if(chrlatchbutton[cnt]&LATCHBUTTONRIGHT&&chrreloadtime[cnt]==0)
                    {
                        // Which weapon?
                        weapon = chrholdingwhich[cnt][1];
                        if(weapon == MAXCHR)
                        {
                            // Unarmed means character itself is the weapon
                            weapon = cnt;
                        }
                        action = capweaponaction[chrmodel[weapon]]+2;


                        // Can it do it?
                        allowedtoattack = btrue;
                        if(madactionvalid[chrmodel[cnt]][action]==bfalse || chrreloadtime[weapon] > 0 ||
                           (capneedskillidtouse[chrmodel[weapon]] && capidsz[chrmodel[cnt]][IDSZSKILL] != capidsz[chrmodel[weapon]][IDSZSKILL]))
                        {
                            allowedtoattack = bfalse;
                            if(chrreloadtime[weapon] == 0)
                            {
                                // This character can't use this weapon
                                chrreloadtime[weapon] = 50;
                                if(chrstaton[cnt])
                                {
                                    // Tell the player that they can't use this weapon
                                    sprintf(generictext, "%s can't use this item...", chrname[cnt]);
                                    debug_message(generictext);
                                }
                            }
                        }
                        if(action==ACTIONDC)
                        {
                            allowedtoattack = bfalse;
                            if(chrreloadtime[weapon]==0)
                            {
                                chralert[weapon]=chralert[weapon]|ALERTIFUSED;
                            }
                        }


                        if(allowedtoattack)
                        {
                            // Rearing mount
                            mount = chrattachedto[cnt];
                            if(mount!=MAXCHR)
                            {
                                allowedtoattack = capridercanattack[chrmodel[mount]];
                                if(chrismount[mount]&&chralive[mount]&&chrisplayer[mount]==bfalse&&chractionready[mount])
                                {
                                    if((action != ACTIONPC || allowedtoattack == bfalse) && chractionready[cnt])
                                    {
                                        play_action(chrattachedto[cnt], ACTIONUC+(rand()&1), bfalse);
                                        chralert[chrattachedto[cnt]]|=ALERTIFUSED;
                                    }
                                    else
                                    {
                                        allowedtoattack = bfalse;
                                    }
                                }
                            }


                            // Attack button
                            if(allowedtoattack)
                            {
                                if(chractionready[cnt] && madactionvalid[chrmodel[cnt]][action])
                                {
                                    // Check mana cost
                                    if(chrmana[cnt] >= chrmanacost[weapon] || chrcanchannel[cnt])
                                    {
                                        cost_mana(cnt, chrmanacost[weapon], weapon);
                                        // Check life healing
                                        chrlife[cnt]+=chrlifeheal[weapon];
                                        if(chrlife[cnt] > chrlifemax[cnt])  chrlife[cnt] = chrlifemax[cnt];
                                        actionready = bfalse;
                                        if(action == ACTIONPC)
                                            actionready = btrue;
                                        action+=rand()&1;
                                        play_action(cnt, action, actionready);
                                        if(weapon!=cnt)
                                        {
                                            // Make the weapon attack too
                                            play_action(weapon, ACTIONMJ, bfalse);
                                            chralert[weapon]=chralert[weapon]|ALERTIFUSED;
                                        }
                                        else
                                        {
                                            // Flag for unarmed attack
                                            chralert[cnt]|=ALERTIFUSED;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }




            // Is the character in the air?
            level = chrlevel[cnt];
            if(chrflyheight[cnt]==0)
            {
                chrzpos[cnt]+=chrzvel[cnt];
                if(chrzpos[cnt] > level || (chrzvel[cnt] > STOPBOUNCING && chrzpos[cnt] > level-STOPBOUNCING))
                {
                    // Character is in the air
                    chrjumpready[cnt]=bfalse;
                    chrzvel[cnt]+=gravity;


                    // Down jump timers for flapping characters
                    if(chrjumptime[cnt]!=0) chrjumptime[cnt]--;


                    // Airborne characters still get friction to make control easier
                    friction = airfriction;
                }
                else
                {
                    // Character is on the ground
                    chrzpos[cnt] = level;
                    grounded = btrue;
                    if(chrhitready[cnt])
                    {
                        chralert[cnt]|=ALERTIFHITGROUND;
                        chrhitready[cnt] = bfalse;
                    }


                    // Make the characters slide
                    twist = meshtwist[chronwhichfan[cnt]];
                    friction = noslipfriction;
                    if(meshfx[chronwhichfan[cnt]]&MESHFXSLIPPY)
                    {
                        if(wateriswater && (meshfx[chronwhichfan[cnt]]&MESHFXWATER) && chrlevel[cnt] < watersurfacelevel+RAISE+1)
                        {
                            // It says it's slippy, but the water is covering it...
                            // Treat exactly as normal
                            chrjumpready[cnt]=btrue;
                            chrjumpnumber[cnt] = chrjumpnumberreset[cnt];
                            if(chrjumptime[cnt] > 0) chrjumptime[cnt]--;
                            chrzvel[cnt] = -chrzvel[cnt]*chrdampen[cnt];
                            chrzvel[cnt]+=gravity;
                        }
                        else
                        {
                            // It's slippy all right...
                            friction = slippyfriction;
                            chrjumpready[cnt]=btrue;
                            if(chrjumptime[cnt] > 0) chrjumptime[cnt]--;
                            if(chrweight[cnt]!=65535)
                            {
                                // Slippy hills make characters slide
                                chrxvel[cnt]+=vellrtwist[twist];
                                chryvel[cnt]+=veludtwist[twist];
                                chrzvel[cnt]=-SLIDETOLERANCE;
                            }
                            if(flattwist[twist])
                            {
                                // Reset jumping on flat areas of slippiness
                                chrjumpnumber[cnt] = chrjumpnumberreset[cnt];
                            }
                        }
                    }
                    else
                    {
                        // Reset jumping
                        chrjumpready[cnt]=btrue;
                        chrjumpnumber[cnt] = chrjumpnumberreset[cnt];
                        if(chrjumptime[cnt] > 0) chrjumptime[cnt]--;
                        chrzvel[cnt] = -chrzvel[cnt]*chrdampen[cnt];
                        chrzvel[cnt]+=gravity;
                    }




                    // Characters with sticky butts lie on the surface of the mesh
                    if(chrstickybutt[cnt]||chralive[cnt]==bfalse)
                    {
                        maplr = chrturnmaplr[cnt];
                        maplr = (maplr<<6)-maplr+maplrtwist[twist];
                        mapud = chrturnmapud[cnt];
                        mapud = (mapud<<6)-mapud+mapudtwist[twist];
                        chrturnmaplr[cnt] = maplr>>6;
                        chrturnmapud[cnt] = mapud>>6;
                    }
                }
            }
            else
            {
                //  Flying
                chrjumpready[cnt]=bfalse;
                chrzpos[cnt]+=chrzvel[cnt];
                if(level < 0) level = 0;  // Don't fall in pits...
                chrzvel[cnt]+=(level+chrflyheight[cnt]-chrzpos[cnt])*FLYDAMPEN;
                if(chrzpos[cnt] < level)
                {
                    chrzpos[cnt] = level;
                    chrzvel[cnt] = 0;
                }
                // Airborne characters still get friction to make control easier
                friction = airfriction;
            }
            // Move the character
            chrxpos[cnt]+=chrxvel[cnt];
            if(__chrhitawall(cnt)) { chrxpos[cnt]=chroldx[cnt]; chrxvel[cnt]=-chrxvel[cnt]; }
            chrypos[cnt]+=chryvel[cnt];
            if(__chrhitawall(cnt)) { chrypos[cnt]=chroldy[cnt]; chryvel[cnt]=-chryvel[cnt]; }
            // Apply friction for next time
            chrxvel[cnt]=chrxvel[cnt]*friction;
            chryvel[cnt]=chryvel[cnt]*friction;


            // Animate the character
            chrlip[cnt] = (chrlip[cnt] + 64);
            if(chrlip[cnt] == 192)
            {
                // Check frame effects
                if(madframefx[chrframe[cnt]]&MADFXACTLEFT)
                    character_swipe(cnt, 0);
                if(madframefx[chrframe[cnt]]&MADFXACTRIGHT)
                    character_swipe(cnt, 1);
                if(madframefx[chrframe[cnt]]&MADFXGRABLEFT)
                    character_grab_stuff(cnt, GRIPLEFT, bfalse);
                if(madframefx[chrframe[cnt]]&MADFXGRABRIGHT)
                    character_grab_stuff(cnt, GRIPRIGHT, bfalse);
                if(madframefx[chrframe[cnt]]&MADFXCHARLEFT)
                    character_grab_stuff(cnt, GRIPLEFT, btrue);
                if(madframefx[chrframe[cnt]]&MADFXCHARRIGHT)
                    character_grab_stuff(cnt, GRIPRIGHT, btrue);
                if(madframefx[chrframe[cnt]]&MADFXDROPLEFT)
                    detach_character_from_mount(chrholdingwhich[cnt][0], bfalse, btrue);
                if(madframefx[chrframe[cnt]]&MADFXDROPRIGHT)
                    detach_character_from_mount(chrholdingwhich[cnt][1], bfalse, btrue);
                if(madframefx[chrframe[cnt]]&MADFXPOOF && chrisplayer[cnt]==bfalse)
                    valuegopoof = btrue;
                if(madframefx[chrframe[cnt]]&MADFXFOOTFALL)
                {
                    if(capwavefootfall[chrmodel[cnt]] != -1)
                    {
						play_sound(chrxpos[cnt], chrypos[cnt], capwaveindex[chrmodel[cnt]][capwavefootfall[chrmodel[cnt]]]);
                    }
                }
            }
            if(chrlip[cnt] == 0)
            {
                // Change frames
                chrlastframe[cnt] = chrframe[cnt];
                chrframe[cnt]++;
                if(chrframe[cnt] == madactionend[chrmodel[cnt]][chraction[cnt]])
                {
                    // Action finished
                    if(chrkeepaction[cnt])
                    {
                        // Keep the last frame going
                        chrframe[cnt] = chrlastframe[cnt];
                    }
                    else
                    {
                        if(chrloopaction[cnt]==bfalse)
                        {
                            // Go on to the next action
                            chraction[cnt] = chrnextaction[cnt];
                            chrnextaction[cnt] = ACTIONDA;
                        }
                        else
                        {
                            // See if the character is mounted...
                            if(chrattachedto[cnt]!=MAXCHR)
                            {
                                chraction[cnt] = ACTIONMI;
                            }
                        }
                        chrframe[cnt] = madactionstart[chrmodel[cnt]][chraction[cnt]];
                    }
                    chractionready[cnt] = btrue;
                }
            }



            // Do "Be careful!" delay
            if(chrcarefultime[cnt] != 0)
            {
                chrcarefultime[cnt]--;
            }


            // Get running, walking, sneaking, or dancing, from speed
            if((chrkeepaction[cnt]|chrloopaction[cnt])==bfalse)
            {
                framelip = madframelip[chrframe[cnt]];  // 0 - 15...  Way through animation
                if(chractionready[cnt]&&chrlip[cnt]==0&&grounded&&chrflyheight[cnt]==0&&(framelip&7)<2)
                {
                    // Do the motion stuff
                    speed = ABS((int) chrxvel[cnt])+ABS((int) chryvel[cnt]);
                    if(speed < chrsneakspd[cnt])
                    {
//                        chrnextaction[cnt] = ACTIONDA;
                        // Do boredom
                        chrboretime[cnt]--;
                        if(chrboretime[cnt] < 0)
                        {
                            chralert[cnt]|=ALERTIFBORED;
                            chrboretime[cnt] = BORETIME;
                        }
                        else
                        {
                            // Do standstill
                            if(chraction[cnt]>ACTIONDD)
                            {
                                chraction[cnt] = ACTIONDA;
                                chrframe[cnt] = madactionstart[chrmodel[cnt]][chraction[cnt]];
                            }
                        }
                    }
                    else
                    {
                        chrboretime[cnt] = BORETIME;
                        if(speed < chrwalkspd[cnt])
                        {
                            chrnextaction[cnt] = ACTIONWA;
                            if(chraction[cnt]!=ACTIONWA)
                            {
                                chrframe[cnt] = madframeliptowalkframe[chrmodel[cnt]][LIPWA][framelip];
                                chraction[cnt] = ACTIONWA;
                            }
                        }
                        else
                        {
                            if(speed < chrrunspd[cnt])
                            {
                                chrnextaction[cnt] = ACTIONWB;
                                if(chraction[cnt]!=ACTIONWB)
                                {
                                    chrframe[cnt] = madframeliptowalkframe[chrmodel[cnt]][LIPWB][framelip];
                                    chraction[cnt] = ACTIONWB;
                                }
                            }
                            else
                            {
                                chrnextaction[cnt] = ACTIONWC;
                                if(chraction[cnt]!=ACTIONWC)
                                {
                                    chrframe[cnt] = madframeliptowalkframe[chrmodel[cnt]][LIPWC][framelip];
                                    chraction[cnt] = ACTIONWC;
                                }
                            }
                        }
                    }
                }
            }
            // Do poofing
            if(valuegopoof)
            {
                if(chrattachedto[cnt] != MAXCHR)
                    detach_character_from_mount(cnt, btrue, bfalse);
                if(chrholdingwhich[cnt][0] != MAXCHR)
                    detach_character_from_mount(chrholdingwhich[cnt][0], btrue, bfalse);
                if(chrholdingwhich[cnt][1] != MAXCHR)
                    detach_character_from_mount(chrholdingwhich[cnt][1], btrue, bfalse);
                free_inventory(cnt);
                free_one_character(cnt);
            }
        }
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void setup_characters(char *modname)
{
    // ZZ> This function sets up character data, loaded from "SPAWN.TXT"
    int currentcharacter=0, lastcharacter, passage, content, money, level, skin, cnt, tnc, localnumber;
    unsigned char ghost, team, stat, cTmp;
    char *name;
    char itislocal;
    char myname[256], newloadname[256];
    unsigned short facing, grip, attach;
	unsigned int slot;
    float x, y, z;
    FILE *fileread;


    // Turn all characters off
    free_all_characters();


    // Turn some back on
    make_newloadname(modname, "gamedat/spawn.txt", newloadname);
    fileread = fopen(newloadname, "r");
    currentcharacter = MAXCHR;
    if(fileread)
    {
        while(goto_colon_yesno(fileread))
        {
            fscanf(fileread, "%s", myname);
            name = myname;
            if(myname[0] == 'N' && myname[1] == 'O' && myname[2] == 'N' &&
                myname[3] == 'E' && myname[4] == 0)
            {
                // Random name
                name = NULL;
            }
            cnt = 0;
            while(cnt < 256)
            {
                if(myname[cnt]=='_')  myname[cnt] = ' ';
                cnt++;
            }
            fscanf(fileread, "%d", &slot);
            fscanf(fileread, "%f%f%f", &x, &y, &z); x = x*128;  y = y*128;  z = z*128;
            cTmp = get_first_letter(fileread);
                attach = MAXCHR;
                facing = NORTH;
                if(cTmp == 'S' || cTmp == 's')  facing = SOUTH;
                if(cTmp == 'E' || cTmp == 'e')  facing = EAST;
                if(cTmp == 'W' || cTmp == 'w')  facing = WEST;
                if(cTmp == 'L' || cTmp == 'l')  { attach = currentcharacter; grip = GRIPLEFT;   }
                if(cTmp == 'R' || cTmp == 'r')  { attach = currentcharacter; grip = GRIPRIGHT;  }
                if(cTmp == 'I' || cTmp == 'i')  { attach = currentcharacter; grip = INVENTORY;  }
            fscanf(fileread, "%d%d%d%d%d", &money, &skin, &passage, &content, &level);
            cTmp = get_first_letter(fileread);
                stat = bfalse;
                if(cTmp == 'T' || cTmp == 't') stat = btrue;
            cTmp = get_first_letter(fileread);
                ghost = bfalse;
                if(cTmp == 'T' || cTmp == 't') ghost = btrue;
            team = (get_first_letter(fileread)-'A')%MAXTEAM;


            // Spawn the character
            if(team < numplayer || rtscontrol==bfalse || team >= MAXPLAYER)
            {
                lastcharacter = spawn_one_character(x, y, z, slot, team, skin, facing, name, MAXCHR);
                if(lastcharacter < MAXCHR)
                {
                    chrmoney[lastcharacter]+=money;
                    if(chrmoney[lastcharacter] > MAXMONEY)  chrmoney[lastcharacter] = MAXMONEY;
                    if(chrmoney[lastcharacter] < 0)  chrmoney[lastcharacter] = 0;
                    chraicontent[lastcharacter] = content;
                    chrpassage[lastcharacter] = passage;
                    if(attach==MAXCHR)
                    {
                        // Free character
                        currentcharacter = lastcharacter;
                    }
                    else
                    {
                        // Attached character
                        if(grip != INVENTORY)
                        {
                            // Wielded character
                            attach_character_to_mount(lastcharacter, currentcharacter, grip);
                            let_character_think(lastcharacter);  // Empty the grabbed messages
                        }
                        else
                        {
                            // Inventory character
                            add_item_to_character_pack(lastcharacter, currentcharacter);
                            chralert[lastcharacter] |= ALERTIFGRABBED;  // Make spellbooks change
                            chrattachedto[lastcharacter] = currentcharacter;  // Make grab work
                            let_character_think(lastcharacter);  // Empty the grabbed messages
                            chrattachedto[lastcharacter] = MAXCHR;  // Fix grab
                        }
                    }
                    // Turn on player input devices
                    if(stat)
                    {
                        if(importamount == 0)
                        {
                            if(playeramount < 2)
                            {
                                if(numstat == 0)
                                {
                                    // Single player module
                                    add_player(lastcharacter, numstat, INPUTMOUSE|INPUTKEY|INPUTJOYA|INPUTJOYB);
                                }
                            }
                            else
                            {
                                if(networkon == bfalse)
                                {
                                    if(playeramount == 2)
                                    {
                                        // Two player hack
                                        if(numstat == 0)
                                        {
                                            // First player
                                            add_player(lastcharacter, numstat, INPUTMOUSE|INPUTKEY|INPUTJOYB);
                                        }
                                        if(numstat == 1)
                                        {
                                            // Second player
                                            add_player(lastcharacter, numstat, INPUTJOYA);
                                        }
                                    }
                                    else
                                    {
                                        // Three player hack
                                        if(numstat == 0)
                                        {
                                            // First player
                                            add_player(lastcharacter, numstat, INPUTKEY);
                                        }
                                        if(numstat == 1)
                                        {
                                            // Second player
                                            add_player(lastcharacter, numstat, INPUTJOYA);
                                        }
                                        if(numstat == 2)
                                        {
                                            // Third player
                                            add_player(lastcharacter, numstat, INPUTJOYB|INPUTMOUSE);
                                        }
                                    }
                                }
                                else
                                {
                                    // One player per machine hack
                                    if(localmachine == numstat)
                                    {
                                        add_player(lastcharacter, numstat, INPUTMOUSE|INPUTKEY|INPUTJOYA|INPUTJOYB);
                                    }
                                }
                            }
                        }
                        if(numstat < importamount)
                        {
                            // Multiplayer import module
                            itislocal = bfalse;
                            tnc = 0;
                            while(tnc < numimport)
                            {
                                if(capimportslot[chrmodel[lastcharacter]] == localslot[tnc])
                                {
                                    itislocal = btrue;
                                    localnumber = tnc;
                                    tnc = numimport;
                                }
                                tnc++;
                            }


                            if(itislocal)
                            {
                                // It's a local player
                                add_player(lastcharacter, numstat, localcontrol[localnumber]);
                            }
                            else
                            {
                                // It's a remote player
                                add_player(lastcharacter, numstat, INPUTNONE);
                            }
                        }
                        // Turn on the stat display
                        add_stat(lastcharacter);
                    }

                    // Set the starting level
                    if(chrisplayer[lastcharacter]==bfalse)
                    {
                        // Let the character gain levels
                        level = level - 1;
                        while(chrexperiencelevel[lastcharacter] < level && chrexperience[lastcharacter] < MAXXP)
                        {
                            give_experience(lastcharacter, MAXXP, XPDIRECT);
                        }
                    }
                    if(ghost)
                    {
                        // Make the character a ghost !!!BAD!!!  Can do with enchants
                        chralpha[lastcharacter] = 128;
                        chrlight[lastcharacter] = 255;
                    }
                }
            }
        }
        fclose(fileread);
    }
    else
    {
        general_error(0, 0, newloadname);
    }
    clear_messages();


    // Make sure local players are displayed first
    sort_stat();


    // Fix tilting trees problem
    tilt_characters_to_terrain();

    // Assume RTS mode always has players...  So it doesn't quit
    if(rtscontrol)  nolocalplayers = bfalse;
}

//--------------------------------------------------------------------------------------------
void set_one_player_latch(unsigned short player)
{
    // ZZ> This function converts input readings to latch settings, so players can
    //     move around
    float newx, newy;
    unsigned short turnsin, turncos, character;
    unsigned char device;
    float dist, scale;
    float inputx, inputy;


    // Check to see if we need to bother
    if(plavalid[player] && pladevice[player] != INPUTNONE)
    {
        // Make life easier
        character = plaindex[player];
        device = pladevice[player];

        // Clear the player's latch buffers
        plalatchbutton[player] = 0;
        plalatchx[player] = 0;
        plalatchy[player] = 0;

        // Mouse routines
        if((device & INPUTMOUSE) && mouseon)
        {
            // Movement
            newx = 0;
            newy = 0;
            if((autoturncamera==255 && numlocalpla == 1) ||
               !control_mouse_is_pressed(MOS_CAMERA))  // Don't allow movement in camera control mode
            {
                dist = sqrt(mousex*mousex+mousey*mousey);
                if(dist > 0)
                {
                    scale = mousesense/dist;
                    if(dist < mousesense)
                    {
                        scale = dist/mousesense;
                    }
                    scale = scale/mousesense;
                    if(chrattachedto[character]!=MAXCHR)
                    {
                        // Mounted
                        inputx = mousex*chrmaxaccel[chrattachedto[character]]*scale;
                        inputy = mousey*chrmaxaccel[chrattachedto[character]]*scale;
                    }
                    else
                    {
                        // Unmounted
                        inputx = mousex*chrmaxaccel[character]*scale;
                        inputy = mousey*chrmaxaccel[character]*scale;
                    }
                    turnsin = (camturnleftrightone*16383);
                    turnsin = turnsin&16383;
                    turncos = (turnsin+4096)&16383;
                    if(autoturncamera==255 &&
                       numlocalpla == 1 &&
                       control_mouse_is_pressed(MOS_CAMERA)==0)  inputx = 0;
                    newx = (inputx*turntosin[turncos]+inputy*turntosin[turnsin]);
                    newy = (-inputx*turntosin[turnsin]+inputy*turntosin[turncos]);
//                    plalatchx[player]+=newx;
//                    plalatchy[player]+=newy;
                }
            }
            plalatchx[player]+=newx*mousecover+mouselatcholdx*mousesustain;
            plalatchy[player]+=newy*mousecover+mouselatcholdy*mousesustain;
            mouselatcholdx = plalatchx[player];
            mouselatcholdy = plalatchy[player];
            // Sustain old movements to ease mouse play
//            plalatchx[player]+=mouselatcholdx*mousesustain;
//            plalatchy[player]+=mouselatcholdy*mousesustain;
//            mouselatcholdx = plalatchx[player];
//            mouselatcholdy = plalatchy[player];
            // Read buttons
            if(control_mouse_is_pressed(MOS_JUMP))
                plalatchbutton[player] |= LATCHBUTTONJUMP;
            if(control_mouse_is_pressed(MOS_LEFT_USE))
                plalatchbutton[player] |= LATCHBUTTONLEFT;
            if(control_mouse_is_pressed(MOS_LEFT_GET))
                plalatchbutton[player] |= LATCHBUTTONALTLEFT;
            if(control_mouse_is_pressed(MOS_LEFT_PACK))
                plalatchbutton[player] |= LATCHBUTTONPACKLEFT;
            if(control_mouse_is_pressed(MOS_RIGHT_USE))
                plalatchbutton[player] |= LATCHBUTTONRIGHT;
            if(control_mouse_is_pressed(MOS_RIGHT_GET))
                plalatchbutton[player] |= LATCHBUTTONALTRIGHT;
            if(control_mouse_is_pressed(MOS_RIGHT_PACK))
                plalatchbutton[player] |= LATCHBUTTONPACKRIGHT;
        }


        // Joystick A routines
        if((device & INPUTJOYA) && joyaon)
        {
            // Movement
            if((autoturncamera==255 && numlocalpla == 1) ||
               !control_joya_is_pressed(JOA_CAMERA))
            {
                newx = 0;
                newy = 0;
                inputx = 0;
                inputy = 0;
                dist = sqrt(joyax*joyax+joyay*joyay);
                if(dist > 0)
                {
                    scale = 1.0/dist;
                    if(chrattachedto[character]!=MAXCHR)
                    {
                        // Mounted
                        inputx = joyax*chrmaxaccel[chrattachedto[character]]*scale;
                        inputy = joyay*chrmaxaccel[chrattachedto[character]]*scale;
                    }
                    else
                    {
                        // Unmounted
                        inputx = joyax*chrmaxaccel[character]*scale;
                        inputy = joyay*chrmaxaccel[character]*scale;
                    }
                }
                turnsin = (camturnleftrightone*16383);
                turnsin = turnsin&16383;
                turncos = (turnsin+4096)&16383;
                if(autoturncamera==255 &&
                   numlocalpla == 1 &&
                   !control_joya_is_pressed(JOA_CAMERA))  inputx = 0;
                newx = (inputx*turntosin[turncos]+inputy*turntosin[turnsin]);
                newy = (-inputx*turntosin[turnsin]+inputy*turntosin[turncos]);
                plalatchx[player]+=newx;
                plalatchy[player]+=newy;
            }
            // Read buttons
            if(control_joya_is_pressed(JOA_JUMP))
                plalatchbutton[player] |= LATCHBUTTONJUMP;
            if(control_joya_is_pressed(JOA_LEFT_USE))
                plalatchbutton[player] |= LATCHBUTTONLEFT;
            if(control_joya_is_pressed(JOA_LEFT_GET))
                plalatchbutton[player] |= LATCHBUTTONALTLEFT;
            if(control_joya_is_pressed(JOA_LEFT_PACK))
                plalatchbutton[player] |= LATCHBUTTONPACKLEFT;
            if(control_joya_is_pressed(JOA_RIGHT_USE))
                plalatchbutton[player] |= LATCHBUTTONRIGHT;
            if(control_joya_is_pressed(JOA_RIGHT_GET))
                plalatchbutton[player] |= LATCHBUTTONALTRIGHT;
            if(control_joya_is_pressed(JOA_RIGHT_PACK))
                plalatchbutton[player] |= LATCHBUTTONPACKRIGHT;
        }


        // Joystick B routines
        if((device & INPUTJOYB) && joybon)
        {
            // Movement
            if((autoturncamera==255 && numlocalpla == 1) ||
               !control_joyb_is_pressed(JOB_CAMERA))
            {
                newx = 0;
                newy = 0;
                inputx = 0;
                inputy = 0;
                dist = sqrt(joybx*joybx+joyby*joyby);
                if(dist > 0)
                {
                    scale = 1.0/dist;
                    if(chrattachedto[character]!=MAXCHR)
                    {
                        // Mounted
                        inputx = joybx*chrmaxaccel[chrattachedto[character]]*scale;
                        inputy = joyby*chrmaxaccel[chrattachedto[character]]*scale;
                    }
                    else
                    {
                        // Unmounted
                        inputx = joybx*chrmaxaccel[character]*scale;
                        inputy = joyby*chrmaxaccel[character]*scale;
                    }
                }
                turnsin = (camturnleftrightone*16383);
                turnsin = turnsin&16383;
                turncos = (turnsin+4096)&16383;
                if(autoturncamera==255 &&
                   numlocalpla == 1 &&
                   !control_joyb_is_pressed(JOB_CAMERA))  inputx = 0;
                newx = (inputx*turntosin[turncos]+inputy*turntosin[turnsin]);
                newy = (-inputx*turntosin[turnsin]+inputy*turntosin[turncos]);
                plalatchx[player]+=newx;
                plalatchy[player]+=newy;
            }
            // Read buttons
            if(control_joyb_is_pressed(JOB_JUMP))
                plalatchbutton[player] |= LATCHBUTTONJUMP;
            if(control_joyb_is_pressed(JOB_LEFT_USE))
                plalatchbutton[player] |= LATCHBUTTONLEFT;
            if(control_joyb_is_pressed(JOB_LEFT_GET))
                plalatchbutton[player] |= LATCHBUTTONALTLEFT;
            if(control_joyb_is_pressed(JOB_LEFT_PACK))
                plalatchbutton[player] |= LATCHBUTTONPACKLEFT;
            if(control_joyb_is_pressed(JOB_RIGHT_USE))
                plalatchbutton[player] |= LATCHBUTTONRIGHT;
            if(control_joyb_is_pressed(JOB_RIGHT_GET))
                plalatchbutton[player] |= LATCHBUTTONALTRIGHT;
            if(control_joyb_is_pressed(JOB_RIGHT_PACK))
                plalatchbutton[player] |= LATCHBUTTONPACKRIGHT;
        }

        // Keyboard routines
        if((device & INPUTKEY) && keyon)
        {
            // Movement
            if(chrattachedto[character]!=MAXCHR)
            {
                // Mounted
                inputx = (control_key_is_pressed(KEY_RIGHT)-control_key_is_pressed(KEY_LEFT))*chrmaxaccel[chrattachedto[character]];
                inputy = (control_key_is_pressed(KEY_DOWN)-control_key_is_pressed(KEY_UP))*chrmaxaccel[chrattachedto[character]];
            }
            else
            {
                // Unmounted
                inputx = (control_key_is_pressed(KEY_RIGHT)-control_key_is_pressed(KEY_LEFT))*chrmaxaccel[character];
                inputy = (control_key_is_pressed(KEY_DOWN)-control_key_is_pressed(KEY_UP))*chrmaxaccel[character];
            }
            turnsin = (camturnleftrightone*16383);
            turnsin = turnsin&16383;
            turncos = (turnsin+4096)&16383;
            if(autoturncamera==255 && numlocalpla == 1)  inputx = 0;
            newx = (inputx*turntosin[turncos]+inputy*turntosin[turnsin]);
            newy = (-inputx*turntosin[turnsin]+inputy*turntosin[turncos]);
            plalatchx[player]+=newx;
            plalatchy[player]+=newy;
            // Read buttons
            if(control_key_is_pressed(KEY_JUMP))
                plalatchbutton[player] |= LATCHBUTTONJUMP;
            if(control_key_is_pressed(KEY_LEFT_USE))
                plalatchbutton[player] |= LATCHBUTTONLEFT;
            if(control_key_is_pressed(KEY_LEFT_GET))
                plalatchbutton[player] |= LATCHBUTTONALTLEFT;
            if(control_key_is_pressed(KEY_LEFT_PACK))
                plalatchbutton[player] |= LATCHBUTTONPACKLEFT;
            if(control_key_is_pressed(KEY_RIGHT_USE))
                plalatchbutton[player] |= LATCHBUTTONRIGHT;
            if(control_key_is_pressed(KEY_RIGHT_GET))
                plalatchbutton[player] |= LATCHBUTTONALTRIGHT;
            if(control_key_is_pressed(KEY_RIGHT_PACK))
                plalatchbutton[player] |= LATCHBUTTONPACKRIGHT;
        }
    }
}

//--------------------------------------------------------------------------------------------
void set_local_latches(void)
{
    // ZZ> This function emulates AI thinkin' by setting latches from input devices
    int cnt;

    cnt = 0;
    while(cnt < MAXPLAYER)
    {
        set_one_player_latch(cnt);
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void make_onwhichfan(void)
{
    // ZZ> This function figures out which fan characters are on and sets their level
    unsigned short character;
    int x, y, ripand, distance;
	//int volume;
    float level;




    // First figure out which fan each character is in
    character = 0;
    while(character < MAXCHR)
    {
        if(chron[character] && (!chrinpack[character]))
        {
            x = chrxpos[character];
            y = chrypos[character];
            x = x>>7;
            y = y>>7;
            chronwhichfan[character] = meshfanstart[y]+x;
        }
        character++;
    }

    // Get levels every update
    character = 0;
    while(character < MAXCHR)
    {
        if(chron[character] && (!chrinpack[character]))
        {
            level = get_level(chrxpos[character], chrypos[character], chronwhichfan[character], chrwaterwalk[character])+RAISE;
            if(chralive[character])
            {
                if(meshfx[chronwhichfan[character]]&MESHFXDAMAGE && chrzpos[character]<=chrlevel[character]+DAMAGERAISE && chrattachedto[character]==MAXCHR)
                {
                    if((chrdamagemodifier[character][damagetiletype]&DAMAGESHIFT) != 3 && !chrinvictus[character]) // 3 means they're pretty well immune
                    {
                        distance = ABS(camtrackx-chrxpos[character])+ABS(camtracky-chrypos[character]);
                        if(distance < damagetilemindistance)
                        {
                           damagetilemindistance = distance;
                        }
                        if(distance < damagetilemindistance + 256)
                        {
                           damagetilesoundtime = 0;
                        }
                        if(chrdamagetime[character] == 0)
                        {
                            damage_character(character, 32768, damagetileamount, 1, damagetiletype, DAMAGETEAM, chrbumplast[character], DAMFXBLOC|DAMFXARMO);
                            chrdamagetime[character] = DAMAGETILETIME;
                        }
                        if(damagetileparttype != -1 && (wldframe&damagetilepartand)==0)
                        {
                            spawn_one_particle(chrxpos[character], chrypos[character], chrzpos[character],
                                               0, MAXMODEL, damagetileparttype, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, 0, MAXCHR);
                        }
                    }
                    if(chrreaffirmdamagetype[character]==damagetiletype)
                    {
                        if((wldframe&TILEREAFFIRMAND)==0)
                            reaffirm_attached_particles(character);
                    }
                }
            }
            if(chrzpos[character] < watersurfacelevel && (meshfx[chronwhichfan[character]]&MESHFXWATER))
            {
                if(chrinwater[character]==bfalse)
                {
                    // Splash
                    if(chrattachedto[character]==MAXCHR)
                    {
                        spawn_one_particle(chrxpos[character], chrypos[character], watersurfacelevel+RAISE,
                                           0, MAXMODEL, SPLASH, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, 0, MAXCHR);
                    }
                    chrinwater[character] = btrue;
                    if(wateriswater)
                    {
                        chralert[character]|=ALERTIFINWATER;
                    }
                }
                else
                {
                    if(chrzpos[character]>watersurfacelevel-RIPPLETOLERANCE && capripple[chrmodel[character]])
                    {
                        // Ripples
                        ripand = ((int)chrxvel[character]!=0)|((int)chryvel[character]!=0);
                        ripand = RIPPLEAND>>ripand;
                        if((wldframe&ripand)==0&&chrzpos[character]<watersurfacelevel&&chralive[character])
                        {
                            spawn_one_particle(chrxpos[character], chrypos[character], watersurfacelevel,
                                               0, MAXMODEL, RIPPLE, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, 0, MAXCHR);
                        }
                    }
                    if(wateriswater && (wldframe&7) == 0)
                    {
                        chrjumpready[character] = btrue;
                        chrjumpnumber[character]=1; //chrjumpnumberreset[character];
                    }
                }
                chrxvel[character]=chrxvel[character]*waterfriction;
                chryvel[character]=chryvel[character]*waterfriction;
                chrzvel[character]=chrzvel[character]*waterfriction;
            }
            else
            {
                chrinwater[character] = bfalse;
            }
            chrlevel[character] = level;
        }
        character++;
    }


    // Play the damage tile sound
    if(damagetilesound >= 0)
    {
        if((wldframe & 3)==0)
        {
            // Change the volume...
/*PORT
            volume = -(damagetilemindistance + (damagetilesoundtime<<8));
            volume = volume<<VOLSHIFT;
            if(volume > VOLMIN)
            {
                lpDSBuffer[damagetilesound]->SetVolume(volume);
            }
            if(damagetilesoundtime < TILESOUNDTIME)  damagetilesoundtime++;
            else damagetilemindistance = 9999;
*/
        }
    }
}

//--------------------------------------------------------------------------------------------
void bump_characters(void)
{
    // ZZ> This function sets handles characters hitting other characters or particles
    unsigned short character, particle, entry, pip, direction;
    unsigned int chara, charb, fanblock, prtidparent, prtidtype, chridvulnerability, eveidremove;
    signed char hide;
    int cnt, tnc, dist, chrinblock, prtinblock, enchant, temp;
    float xa, ya, za, xb, yb, zb;
    float ax, ay, nx, ny, scale;  // For deflection
    unsigned short facing;


    // Clear the lists
    fanblock = 0;
    while(fanblock < numfanblock)
    {
        meshbumplistchrnum[fanblock] = 0;
        meshbumplistprtnum[fanblock] = 0;
        fanblock++;
    }



    // Fill 'em back up
    character = 0;
    while(character < MAXCHR)
    {
        if(chron[character]&&(!chrinpack[character])&&(chrattachedto[character]==MAXCHR||chrreaffirmdamagetype[character]!=DAMAGENULL))
        {
            hide = caphidestate[chrmodel[character]];
            if(hide == NOHIDE || hide != chraistate[character])
            {
                chrholdingweight[character] = 0;
                fanblock = (((int)chrxpos[character])>>9)+meshblockstart[((int)chrypos[character])>>9];
                // Insert before any other characters on the block
                entry = meshbumplistchr[fanblock];
                chrbumpnext[character] = entry;
                meshbumplistchr[fanblock] = character;
                meshbumplistchrnum[fanblock]++;
            }
        }
        character++;
    }
    particle = 0;
    while(particle < MAXPRT)
    {
        if(prton[particle]&&prtbumpsize[particle])
        {
            fanblock = (((int)prtxpos[particle])>>9)+meshblockstart[((int)prtypos[particle])>>9];
            // Insert before any other particles on the block
            entry = meshbumplistprt[fanblock];
            prtbumpnext[particle] = entry;
            meshbumplistprt[fanblock] = particle;
            meshbumplistprtnum[fanblock]++;
        }
        particle++;
    }



    // Check collisions with other characters and bump particles
    // Only check each pair once
    fanblock = 0;
    while(fanblock < numfanblock)
    {
        chara = meshbumplistchr[fanblock];
        chrinblock = meshbumplistchrnum[fanblock];
        prtinblock = meshbumplistprtnum[fanblock];
        cnt = 0;
        while(cnt < chrinblock)
        {
            xa = chrxpos[chara];
            ya = chrypos[chara];
            za = chrzpos[chara];
            chridvulnerability = capidsz[chrmodel[chara]][IDSZVULNERABILITY];
            // Don't let items bump
            if(chrbumpheight[chara])//chrisitem[chara]==bfalse)
            {
                charb = chrbumpnext[chara];  // Don't collide with self
                tnc = cnt+1;
                while(tnc < chrinblock)
                {
                    if(chrbumpheight[charb])//chrisitem[charb]==bfalse)
                    {
                        xb = chrxpos[charb];
                        yb = chrypos[charb];
                        zb = chrzpos[charb];
                        // First check absolute value diamond
                        xb = ABS(xa-xb);
                        yb = ABS(ya-yb);
                        dist = xb+yb;
                        if(dist < chrbumpsizebig[chara] || dist < chrbumpsizebig[charb])
                        {
                            // Then check bounding box square...  Square+Diamond=Octagon
                            if((xb<chrbumpsize[chara] || xb<chrbumpsize[charb])&&
                            (yb<chrbumpsize[chara] || yb<chrbumpsize[charb]))
                            {
                               // Pretend that they collided
                                chrbumplast[chara]=charb;
                                chrbumplast[charb]=chara;
                                // Now see if either is on top the other like a platform
                                if(za>zb+chrbumpheight[charb]-PLATTOLERANCE+chrzvel[chara]-chrzvel[charb]&&(capcanuseplatforms[chrmodel[chara]]||za>zb+chrbumpheight[charb]))
                                {
                                    // Is A falling on B?
                                    if(za<zb+chrbumpheight[charb]&&chrplatform[charb])//&&chrflyheight[chara]==0)
                                    {
                                        // A is inside, coming from above
                                        chrzpos[chara]=(chrzpos[chara])*PLATKEEP+(chrzpos[charb]+chrbumpheight[charb]+PLATADD)*PLATASCEND;
                                        chrxvel[chara]+=(chrxvel[charb])*platstick;
                                        chryvel[chara]+=(chryvel[charb])*platstick;
                                        if(chrzvel[chara]<chrzvel[charb])
                                            chrzvel[chara]=chrzvel[charb];
                                        chrturnleftright[chara]+=(chrturnleftright[charb]-chroldturn[charb]);
                                        chrjumpready[chara]=btrue;
                                        chrjumpnumber[chara]=chrjumpnumberreset[chara];
                                        chrholdingweight[charb]=chrweight[chara];
                                    }
                                }
                                else
                                {
                                    if(zb>za+chrbumpheight[chara]-PLATTOLERANCE+chrzvel[charb]-chrzvel[chara]&&(capcanuseplatforms[chrmodel[charb]]||zb>za+chrbumpheight[chara]))
                                    {
                                        // Is B falling on A?
                                        if(zb<za+chrbumpheight[chara]&&chrplatform[chara])//&&chrflyheight[charb]==0)
                                        {
                                            // B is inside, coming from above
                                            chrzpos[charb]=(chrzpos[charb])*PLATKEEP+(chrzpos[chara]+chrbumpheight[chara]+PLATADD)*PLATASCEND;
                                            chrxvel[charb]+=(chrxvel[chara])*platstick;
                                            chryvel[charb]+=(chryvel[chara])*platstick;
                                            if(chrzvel[charb]<chrzvel[chara])
                                                chrzvel[charb]=chrzvel[chara];
                                            chrturnleftright[charb]+=(chrturnleftright[chara]-chroldturn[chara]);
                                            chrjumpready[charb]=btrue;
                                            chrjumpnumber[charb]=chrjumpnumberreset[charb];
                                            chrholdingweight[chara]=chrweight[charb];
                                        }
                                    }
                                    else
                                    {
                                        // They are inside each other, which ain't good
                                        // Only collide if moving toward the other
                                        if(chrxvel[chara]>0)
                                        {
                                            if(chrxpos[chara]<chrxpos[charb]) { chrxvel[charb]+=chrxvel[chara]*chrbumpdampen[charb];  chrxvel[chara]=-chrxvel[chara]*chrbumpdampen[chara];  chrxpos[chara]=chroldx[chara]; }
                                        }
                                        else
                                        {
                                            if(chrxpos[chara]>chrxpos[charb]) { chrxvel[charb]+=chrxvel[chara]*chrbumpdampen[charb];  chrxvel[chara]=-chrxvel[chara]*chrbumpdampen[chara];  chrxpos[chara]=chroldx[chara]; }
                                        }
                                        if(chryvel[chara]>0)
                                        {
                                            if(chrypos[chara]<chrypos[charb]) { chryvel[charb]+=chryvel[chara]*chrbumpdampen[charb];  chryvel[chara]=-chryvel[chara]*chrbumpdampen[chara];  chrypos[chara]=chroldy[chara]; }
                                        }
                                        else
                                        {
                                            if(chrypos[chara]>chrypos[charb]) { chryvel[charb]+=chryvel[chara]*chrbumpdampen[charb];  chryvel[chara]=-chryvel[chara]*chrbumpdampen[chara];  chrypos[chara]=chroldy[chara]; }
                                        }
                                        if(chrxvel[charb]>0)
                                        {
                                            if(chrxpos[charb]<chrxpos[chara]) { chrxvel[chara]+=chrxvel[charb]*chrbumpdampen[chara];  chrxvel[charb]=-chrxvel[charb]*chrbumpdampen[charb];  chrxpos[charb]=chroldx[charb]; }
                                        }
                                        else
                                        {
                                            if(chrxpos[charb]>chrxpos[chara]) { chrxvel[chara]+=chrxvel[charb]*chrbumpdampen[chara];  chrxvel[charb]=-chrxvel[charb]*chrbumpdampen[charb];  chrxpos[charb]=chroldx[charb]; }
                                        }
                                        if(chryvel[charb]>0)
                                        {
                                            if(chrypos[charb]<chrypos[chara]) { chryvel[chara]+=chryvel[charb]*chrbumpdampen[chara];  chryvel[charb]=-chryvel[charb]*chrbumpdampen[charb];  chrypos[charb]=chroldy[charb]; }
                                        }
                                        else
                                        {
                                            if(chrypos[charb]>chrypos[chara]) { chryvel[chara]+=chryvel[charb]*chrbumpdampen[chara];  chryvel[charb]=-chryvel[charb]*chrbumpdampen[charb];  chrypos[charb]=chroldy[charb]; }
                                        }
                                        xa=chrxpos[chara];
                                        ya=chrypos[chara];
                                        chralert[chara]=chralert[chara]|ALERTIFBUMPED;
                                        chralert[charb]=chralert[charb]|ALERTIFBUMPED;
                                    }
                                }
                            }
                        }
                    }
                    charb = chrbumpnext[charb];
                    tnc++;
                }
                // Now double check the last character we bumped into, in case it's a platform
                charb = chrbumplast[chara];
                if(chron[charb]&&(!chrinpack[charb])&&charb!=chara&&chrattachedto[charb]==MAXCHR&&chrbumpheight[chara]&&chrbumpheight[charb])
                {
                    xb = chrxpos[charb];
                    yb = chrypos[charb];
                    zb = chrzpos[charb];
                    // First check absolute value diamond
                    xb = ABS(xa-xb);
                    yb = ABS(ya-yb);
                    dist = xb+yb;
                    if(dist < chrbumpsizebig[chara] || dist < chrbumpsizebig[charb])
                    {
                        // Then check bounding box square...  Square+Diamond=Octagon
                        if((xb<chrbumpsize[chara] || xb<chrbumpsize[charb])&&
                           (yb<chrbumpsize[chara] || yb<chrbumpsize[charb]))
                        {
                            // Now see if either is on top the other like a platform
                            if(za>zb+chrbumpheight[charb]-PLATTOLERANCE+chrzvel[chara]-chrzvel[charb]&&(capcanuseplatforms[chrmodel[chara]]||za>zb+chrbumpheight[charb]))
                            {
                                // Is A falling on B?
                                if(za<zb+chrbumpheight[charb]&&chrplatform[charb]&&chralive[chara])//&&chrflyheight[chara]==0)
                                {
                                    // A is inside, coming from above
                                    chrzpos[chara]=(chrzpos[chara])*PLATKEEP+(chrzpos[charb]+chrbumpheight[charb]+PLATADD)*PLATASCEND;
                                    chrxvel[chara]+=(chrxvel[charb])*platstick;
                                    chryvel[chara]+=(chryvel[charb])*platstick;
                                    if(chrzvel[chara]<chrzvel[charb])
                                        chrzvel[chara]=chrzvel[charb];
                                    chrturnleftright[chara]+=(chrturnleftright[charb]-chroldturn[charb]);
                                    chrjumpready[chara]=btrue;
                                    chrjumpnumber[chara]=chrjumpnumberreset[chara];
                                    if(madactionvalid[chrmodel[chara]][ACTIONMI]&&chralive[chara]&&chralive[charb]&&chrismount[charb]&&chrisitem[chara]==bfalse&&chrholdingwhich[charb][0]==MAXCHR&&chrattachedto[chara]==MAXCHR&&chrjumptime[chara]==0&&chrflyheight[chara]==0)
                                    {
                                        attach_character_to_mount(chara, charb, GRIPONLY);
                                        chrbumplast[chara]=chara;
                                        chrbumplast[charb]=charb;
                                    }
                                    chrholdingweight[charb]=chrweight[chara];
                                }
                            }
                            else
                            {
                                if(zb>za+chrbumpheight[chara]-PLATTOLERANCE+chrzvel[charb]-chrzvel[chara]&&(capcanuseplatforms[chrmodel[charb]]||zb>za+chrbumpheight[chara]))
                                {
                                    // Is B falling on A?
                                    if(zb<za+chrbumpheight[chara]&&chrplatform[chara]&&chralive[charb])//&&chrflyheight[charb]==0)
                                    {
                                        // B is inside, coming from above
                                        chrzpos[charb]=(chrzpos[charb])*PLATKEEP+(chrzpos[chara]+chrbumpheight[chara]+PLATADD)*PLATASCEND;
                                        chrxvel[charb]+=(chrxvel[chara])*platstick;
                                        chryvel[charb]+=(chryvel[chara])*platstick;
                                        if(chrzvel[charb]<chrzvel[chara])
                                            chrzvel[charb]=chrzvel[chara];
                                        chrturnleftright[charb]+=(chrturnleftright[chara]-chroldturn[chara]);
                                        chrjumpready[charb]=btrue;
                                        chrjumpnumber[charb]=chrjumpnumberreset[charb];
                                        if(madactionvalid[chrmodel[charb]][ACTIONMI]&&chralive[chara]&&chralive[charb]&&chrismount[chara]&&chrisitem[charb]==bfalse&&chrholdingwhich[chara][0]==MAXCHR&&chrattachedto[charb]==MAXCHR&&chrjumptime[charb]==0&&chrflyheight[charb]==0)
                                        {
                                            attach_character_to_mount(charb, chara, GRIPONLY);
                                            chrbumplast[chara]=chara;
                                            chrbumplast[charb]=charb;
                                        }
                                        chrholdingweight[chara]=chrweight[charb];
                                    }
                                }
                                else
                                {
                                    // They are inside each other, which ain't good
                                    // Only collide if moving toward the other
                                    if(chrxvel[chara]>0)
                                    {
                                        if(chrxpos[chara]<chrxpos[charb]) { chrxvel[charb]+=chrxvel[chara]*chrbumpdampen[charb];  chrxvel[chara]=-chrxvel[chara]*chrbumpdampen[chara];  chrxpos[chara]=chroldx[chara]; }
                                    }
                                    else
                                    {
                                        if(chrxpos[chara]>chrxpos[charb]) { chrxvel[charb]+=chrxvel[chara]*chrbumpdampen[charb];  chrxvel[chara]=-chrxvel[chara]*chrbumpdampen[chara];  chrxpos[chara]=chroldx[chara]; }
                                    }
                                    if(chryvel[chara]>0)
                                    {
                                        if(chrypos[chara]<chrypos[charb]) { chryvel[charb]+=chryvel[chara]*chrbumpdampen[charb];  chryvel[chara]=-chryvel[chara]*chrbumpdampen[chara];  chrypos[chara]=chroldy[chara]; }
                                    }
                                    else
                                    {
                                        if(chrypos[chara]>chrypos[charb]) { chryvel[charb]+=chryvel[chara]*chrbumpdampen[charb];  chryvel[chara]=-chryvel[chara]*chrbumpdampen[chara];  chrypos[chara]=chroldy[chara]; }
                                    }
                                    if(chrxvel[charb]>0)
                                    {
                                        if(chrxpos[charb]<chrxpos[chara]) { chrxvel[chara]+=chrxvel[charb]*chrbumpdampen[chara];  chrxvel[charb]=-chrxvel[charb]*chrbumpdampen[charb];  chrxpos[charb]=chroldx[charb]; }
                                    }
                                    else
                                    {
                                        if(chrxpos[charb]>chrxpos[chara]) { chrxvel[chara]+=chrxvel[charb]*chrbumpdampen[chara];  chrxvel[charb]=-chrxvel[charb]*chrbumpdampen[charb];  chrxpos[charb]=chroldx[charb]; }
                                    }
                                    if(chryvel[charb]>0)
                                    {
                                        if(chrypos[charb]<chrypos[chara]) { chryvel[chara]+=chryvel[charb]*chrbumpdampen[chara];  chryvel[charb]=-chryvel[charb]*chrbumpdampen[charb];  chrypos[charb]=chroldy[charb]; }
                                    }
                                    else
                                    {
                                        if(chrypos[charb]>chrypos[chara]) { chryvel[chara]+=chryvel[charb]*chrbumpdampen[chara];  chryvel[charb]=-chryvel[charb]*chrbumpdampen[charb];  chrypos[charb]=chroldy[charb]; }
                                    }
                                    xa=chrxpos[chara];
                                    ya=chrypos[chara];
                                    chralert[chara]=chralert[chara]|ALERTIFBUMPED;
                                    chralert[charb]=chralert[charb]|ALERTIFBUMPED;
                                }
                            }
                        }
                    }
                }
            }
            // Now check collisions with every bump particle in same area
            if(chralive[chara])
            {
                particle = meshbumplistprt[fanblock];
                tnc = 0;
                while(tnc < prtinblock)
                {
                    xb = prtxpos[particle];
                    yb = prtypos[particle];
                    zb = prtzpos[particle];
                    // First check absolute value diamond
                    xb = ABS(xa-xb);
                    yb = ABS(ya-yb);
                    dist = xb+yb;
                    if(dist < chrbumpsizebig[chara] || dist < prtbumpsizebig[particle])
                    {
                        // Then check bounding box square...  Square+Diamond=Octagon
                        if((xb<chrbumpsize[chara]  ||xb<prtbumpsize[particle])&&
                           (yb<chrbumpsize[chara]  ||yb<prtbumpsize[particle])&&
                           (zb>za-prtbumpheight[particle]&&zb<za+chrbumpheight[chara]+prtbumpheight[particle]))
                        {
                            pip = prtpip[particle];
                            if(zb>za+chrbumpheight[chara]+prtzvel[particle]&&prtzvel[particle]<0&&chrplatform[chara]&&prtattachedtocharacter[particle]==MAXCHR)
                            {
                                // Particle is falling on A
                                prtzpos[particle]=za+chrbumpheight[chara];
                                prtzvel[particle]=-prtzvel[particle]*pipdampen[pip];
                                prtxvel[particle]+=(chrxvel[chara])*platstick;
                                prtyvel[particle]+=(chryvel[chara])*platstick;
                            }
                            // Check reaffirmation of particles
                            if(prtattachedtocharacter[particle]!=chara)
                            {
                                if(chrreloadtime[chara]==0)
                                {
                                    if(chrreaffirmdamagetype[chara]==prtdamagetype[particle]&&chrdamagetime[chara]==0)
                                    {
                                        reaffirm_attached_particles(chara);
                                    }
                                }
                            }
                            // Check for missile treatment
                            if((chrdamagemodifier[chara][prtdamagetype[particle]]&3) < 2 ||
                               chrmissiletreatment[chara] == MISNORMAL ||
                               prtattachedtocharacter[particle] != MAXCHR ||
                               (prtchr[particle] == chara && pipfriendlyfire[pip]==bfalse) ||
                               (chrmana[chrmissilehandler[chara]] < (chrmissilecost[chara]<<4) && chrcanchannel[chrmissilehandler[chara]] == bfalse))
                            {
                                if((teamhatesteam[prtteam[particle]][chrteam[chara]]||(pipfriendlyfire[pip]&&((chara!=prtchr[particle]&&chara!=chrattachedto[prtchr[particle]])||piponlydamagefriendly[pip])))&&chrinvictus[chara]==bfalse)
                                {
                                    spawn_bump_particles(chara, particle); // Catch on fire
                                    if((prtdamagebase[particle]|prtdamagerand[particle])>1)
                                    {
                                        prtidparent = capidsz[prtmodel[particle]][IDSZPARENT];
                                        prtidtype = capidsz[prtmodel[particle]][IDSZTYPE];
                                        if(chrdamagetime[chara]==0 && prtattachedtocharacter[particle]!=chara && (pipdamfx[pip]&DAMFXARRO) == 0)
                                        {
                                            // Normal particle damage
                                            if(pipallowpush[pip])
                                            {
                                                chrxvel[chara]=prtxvel[particle]*chrbumpdampen[chara];
                                                chryvel[chara]=prtyvel[particle]*chrbumpdampen[chara];
                                                chrzvel[chara]=prtzvel[particle]*chrbumpdampen[chara];
                                            }
                                            direction = (atan2(prtyvel[particle], prtxvel[particle])+PI)*65535/(2*PI);
                                            direction = chrturnleftright[chara]-direction+32768;
                                            // Check all enchants to see if they are removed
                                            enchant = chrfirstenchant[chara];
                                            while(enchant != MAXENCHANT)
                                            {
                                                eveidremove = everemovedbyidsz[enceve[enchant]];
                                                temp = encnextenchant[enchant];
                                                if(eveidremove != IDSZNONE && (eveidremove == prtidtype || eveidremove == prtidparent))
                                                {
                                                    remove_enchant(enchant);
                                                }
                                                enchant = temp;
                                            }

											//Apply intelligence/wisdom bonus damage for particles with the [IDAM] and [WDAM] expansions (Low ability gives penality)
											if(pipintdamagebonus[pip]) prtdamagebase[particle] += ((chrintelligence[prtchr[particle]]-3584)*0.25);		//+1 bonus for every 4 points of intelligence 
											if(pipwisdamagebonus[pip]) prtdamagebase[particle] += ((chrwisdom[prtchr[particle]]-3584)*0.25);		    //and/or wisdom above 14. Below 14 gives -1 instead!

                                            // Damage the character
											if(chridvulnerability != IDSZNONE && (chridvulnerability == prtidtype || chridvulnerability == prtidparent))
                                            {
                                                damage_character(chara, direction, prtdamagebase[particle]<<1, prtdamagerand[particle]<<1, prtdamagetype[particle], prtteam[particle], prtchr[particle], pipdamfx[pip]);
                                                chralert[chara]|=ALERTIFHITVULNERABLE;
                                            }
                                            else
                                            {
                                                damage_character(chara, direction, prtdamagebase[particle], prtdamagerand[particle], prtdamagetype[particle], prtteam[particle], prtchr[particle], pipdamfx[pip]);
                                            }
                                            // Do confuse effects
                                            if((madframefx[chrframe[chara]]&MADFXINVICTUS)==bfalse || pipdamfx[pip]&DAMFXBLOC)
                                            {
                                                if(pipgrogtime[pip] != 0 && capcanbegrogged[chrmodel[chara]])
                                                {
                                                    chrgrogtime[chara]+=pipgrogtime[pip];
                                                    if(chrgrogtime[chara]<0)  chrgrogtime[chara] = 32767;
                                                    chralert[chara] = chralert[chara]|ALERTIFGROGGED;
                                                }
                                                if(pipdazetime[pip] != 0 && capcanbedazed[chrmodel[chara]])
                                                {
                                                    chrdazetime[chara]+=pipdazetime[pip];
                                                    if(chrdazetime[chara]<0)  chrdazetime[chara] = 32767;
                                                    chralert[chara] = chralert[chara]|ALERTIFDAZED;
                                                }
                                            }
                                            // Notify the attacker of a scored hit
                                            if(prtchr[particle]!=MAXCHR)
                                            {
                                                chralert[prtchr[particle]]=chralert[prtchr[particle]]|ALERTIFSCOREDAHIT;
                                                chrhitlast[prtchr[particle]]=chara;
                                            }
                                        }
                                        if((wldframe&31)==0 && prtattachedtocharacter[particle]==chara)
                                        {
                                            // Attached particle damage ( Burning )
                                            if(pipxyvelbase[pip]==0)
                                            {
                                                // Make character limp
                                                chrxvel[chara]=0;
                                                chryvel[chara]=0;
                                            }
                                            damage_character(chara, 32768, prtdamagebase[particle], prtdamagerand[particle], prtdamagetype[particle], prtteam[particle], prtchr[particle], pipdamfx[pip]);
                                        }
                                    }
                                    if(pipendbump[pip])
                                    {
                                        if(pipbumpmoney[pip])
                                        {
                                            if(chrcangrabmoney[chara]&&chralive[chara]&&chrdamagetime[chara]==0&&chrmoney[chara]!=MAXMONEY)
                                            {
                                                if(chrismount[chara])
                                                {
                                                    // Let mounts collect money for their riders
                                                    if(chrholdingwhich[chara][0] != MAXCHR)
                                                    {
                                                        chrmoney[chrholdingwhich[chara][0]]+=pipbumpmoney[pip];
                                                        if(chrmoney[chrholdingwhich[chara][0]] > MAXMONEY) chrmoney[chrholdingwhich[chara][0]] = MAXMONEY;
                                                        if(chrmoney[chrholdingwhich[chara][0]] < 0) chrmoney[chrholdingwhich[chara][0]] = 0;
                                                        prttime[particle]=1;
                                                    }
                                                }
                                                else
                                                {
                                                    // Normal money collection
                                                    chrmoney[chara]+=pipbumpmoney[pip];
                                                    if(chrmoney[chara] > MAXMONEY) chrmoney[chara] = MAXMONEY;
                                                    if(chrmoney[chara] < 0) chrmoney[chara] = 0;
                                                    prttime[particle]=1;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            prttime[particle]=1;
                                            // Only hit one character, not several
                                            prtdamagebase[particle] = 0;
                                            prtdamagerand[particle] = 1;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                if(prtchr[particle] != chara)
                                {
                                    cost_mana(chrmissilehandler[chara], (chrmissilecost[chara]<<4), prtchr[particle]);
                                    // Treat the missile
                                    if(chrmissiletreatment[chara] == MISDEFLECT)
                                    {
                                        // Use old position to find normal
                                        ax = prtxpos[particle] - prtxvel[particle];
                                        ay = prtypos[particle] - prtyvel[particle];
                                        ax = chrxpos[chara] - ax;
                                        ay = chrypos[chara] - ay;
                                        // Find size of normal
                                        scale = ax*ax + ay*ay;
                                        if(scale > 0)
                                        {
                                            // Make the normal a unit normal
                                            scale = sqrt(scale);
                                            nx = ax/scale;
                                            ny = ay/scale;
                                            // Deflect the incoming ray off the normal
                                            scale = (prtxvel[particle]*nx + prtyvel[particle]*ny)*2;
                                            ax = scale*nx;
                                            ay = scale*ny;
                                            prtxvel[particle] = prtxvel[particle] - ax;
                                            prtyvel[particle] = prtyvel[particle] - ay;
                                        }
                                    }
                                    else
                                    {
                                        // Reflect it back in the direction it came
                                        prtxvel[particle] = -prtxvel[particle];
                                        prtyvel[particle] = -prtyvel[particle];
                                    }
                                    // Change the owner of the missile
                                    if(piphoming[pip] == bfalse)
                                    {
                                        prtteam[particle] = chrteam[chara];
                                        prtchr[particle] = chara;
                                    }
                                    // Change the direction of the particle
                                    if(piprotatetoface[pip])
                                    {
                                        // Turn to face new direction
                                        facing = atan2(prtyvel[particle], prtxvel[particle])*65535/(2*PI);
                                        facing+=32768;
                                        prtfacing[particle] = facing;
                                    }
                                }
                            }
                        }
                    }
                    particle = prtbumpnext[particle];
                    tnc++;
                }
            }
            chara = chrbumpnext[chara];
            cnt++;
        }
        fanblock++;
    }
}


//--------------------------------------------------------------------------------------------
void stat_return()
{
    // ZZ> This function brings mana and life back
    int cnt, owner, target, eve;


    // Do reload time
    cnt = 0;
    while(cnt < MAXCHR)
    {
        if(chrreloadtime[cnt] > 0)
        {
            chrreloadtime[cnt]--;
        }
        cnt++;
    }



    // Do stats
    if(statclock==ONESECOND)
    {
        // Reset the clock
        statclock = 0;


        // Do all the characters
        cnt = 0;
        while(cnt < MAXCHR)
        {
            if(chron[cnt]&&(!chrinpack[cnt])&&chralive[cnt])
            {
                chrmana[cnt]+=chrmanareturn[cnt] >> MANARETURNSHIFT;
                if(chrmana[cnt] < 0)
                    chrmana[cnt] = 0;
                if(chrmana[cnt] > chrmanamax[cnt])
                    chrmana[cnt] = chrmanamax[cnt];
                chrlife[cnt]+=chrlifereturn[cnt];
                if(chrlife[cnt] < 1)
                    chrlife[cnt] = 1;
                if(chrlife[cnt] > chrlifemax[cnt])
                    chrlife[cnt] = chrlifemax[cnt];
                if(chrgrogtime[cnt] > 0)
                {
                    chrgrogtime[cnt]--;
                    if(chrgrogtime[cnt] < 0)
                        chrgrogtime[cnt] = 0;
                }
                if(chrdazetime[cnt] > 0)
                {
                    chrdazetime[cnt]--;
                    if(chrdazetime[cnt] < 0)
                        chrdazetime[cnt] = 0;
                }
            }
            cnt++;
        }


        // Run through all the enchants as well
        cnt = 0;
        while(cnt < MAXENCHANT)
        {
            if(encon[cnt])
            {
                if(enctime[cnt] != 0)
                {
                    if(enctime[cnt] > 0)
                    {
                        enctime[cnt]--;
                    }
                    owner = encowner[cnt];
                    target = enctarget[cnt];
                    eve = enceve[cnt];


                    // Do drains
                    if(chralive[owner])
                    {
                        // Change life
                        chrlife[owner]+=encownerlife[cnt];
                        if(chrlife[owner] < 1)
                        {
                            chrlife[owner] = 1;
                            kill_character(owner, target);
                        }
                        if(chrlife[owner] > chrlifemax[owner])
                        {
                            chrlife[owner] = chrlifemax[owner];
                        }
                        // Change mana
                        if(cost_mana(owner, -encownermana[cnt], target)==bfalse && eveendifcantpay[eve])
                        {
                            remove_enchant(cnt);
                        }
                    }
                    else
                    {
                        if(evestayifnoowner[eve]==bfalse)
                        {
                            remove_enchant(cnt);
                        }
                    }
                    if(encon[cnt])
                    {
                        if(chralive[target])
                        {
                            // Change life
                            chrlife[target]+=enctargetlife[cnt];
                            if(chrlife[target] < 1)
                            {
                                chrlife[target] = 1;
                                kill_character(target, owner);
                            }
                            if(chrlife[target] > chrlifemax[target])
                            {
                                chrlife[target] = chrlifemax[target];
                            }
                            // Change mana
                            if(cost_mana(target, -enctargetmana[cnt], owner)==bfalse && eveendifcantpay[eve])
                            {
                                remove_enchant(cnt);
                            }
                        }
                        else
                        {
                            remove_enchant(cnt);
                        }
                    }
                }
                else
                {
                    remove_enchant(cnt);
                }
            }
            cnt++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void pit_kill()
{
    // ZZ> This function kills any character in a deep pit...
    int cnt;

    if(pitskill)
    {
        if(pitclock > 19)
        {
            pitclock = 0;


            // Kill any particles that fell in a pit, if they die in water...
            cnt = 0;
            while(cnt < MAXPRT)
            {
                if(prton[cnt])
                {
                    if(prtzpos[cnt] < PITDEPTH && pipendwater[prtpip[cnt]])
                    {
                        prttime[cnt] = 1;
                    }
                }
                cnt++;
            }



            // Kill any characters that fell in a pit...
            cnt = 0;
            while(cnt < MAXCHR)
            {
                if(chron[cnt] && chralive[cnt] && chrinpack[cnt] == bfalse)
                {
                    if(chrinvictus[cnt] == bfalse && chrzpos[cnt] < PITDEPTH && chrattachedto[cnt]==MAXCHR)
                    {
                        // Got one!
                        kill_character(cnt, MAXCHR);
                        chrxvel[cnt] = 0;
                        chryvel[cnt] = 0;
                    }
                }
                cnt++;
            }
        }
        else
        {
            pitclock++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void reset_players()
{
    // ZZ> This function clears the player list data
    int cnt, tnc;


    // Reset the local data stuff
    localseekurse = bfalse;
    localseeinvisible = bfalse;
    alllocalpladead = bfalse;


    // Reset the initial player data and latches
    cnt = 0;
    while(cnt < MAXPLAYER)
    {
        plavalid[cnt] = bfalse;
        plaindex[cnt] = 0;
        plalatchx[cnt] = 0;
        plalatchy[cnt] = 0;
        plalatchbutton[cnt] = 0;
        tnc = 0;
        while(tnc < MAXLAG)
        {
            platimelatchx[cnt][tnc] = 0;
            platimelatchy[cnt][tnc] = 0;
            platimelatchbutton[cnt][tnc] = 0;
            tnc++;
        }
        pladevice[cnt] = INPUTNONE;
        cnt++;
    }
    numpla = 0;
    nexttimestamp = -1;
    numplatimes = STARTTALK+1;
    if(hostactive) numplatimes++;
}

//--------------------------------------------------------------------------------------------
void resize_characters()
{
    // ZZ> This function makes the characters get bigger or smaller, depending
    //     on their sizegoto and sizegototime
    int cnt, item, mount;
    unsigned char willgetcaught;
    float newsize;


    cnt = 0;
    while(cnt < MAXCHR)
    {
        if(chron[cnt] && chrsizegototime[cnt])
        {
            // Make sure it won't get caught in a wall
            willgetcaught = bfalse;
            if(chrsizegoto[cnt] > chrfat[cnt])
            {
                chrbumpsize[cnt] += 10;
                if(__chrhitawall(cnt))
                {
                    willgetcaught = btrue;
                }
                chrbumpsize[cnt] -= 10;
            }


            // If it is getting caught, simply halt growth until later
            if(willgetcaught==bfalse)
            {
                // Figure out how big it is
                chrsizegototime[cnt]--;
                newsize = chrsizegoto[cnt];
                if(chrsizegototime[cnt] != 0)
                {
                    newsize = (chrfat[cnt] * .90) + (newsize * .10);
                }


                // Make it that big...
                chrfat[cnt] = newsize;
                chrshadowsize[cnt] = chrshadowsizesave[cnt]*newsize;
                chrbumpsize[cnt] = chrbumpsizesave[cnt]*newsize;
                chrbumpsizebig[cnt] = chrbumpsizebigsave[cnt]*newsize;
                chrbumpheight[cnt] = chrbumpheightsave[cnt]*newsize;
                chrweight[cnt] = capweight[chrmodel[cnt]]*newsize;
                if(capweight[chrmodel[cnt]]==255) chrweight[cnt] = 65535;


                // Now come up with the magic number
                mount = chrattachedto[cnt];
                if(mount == MAXCHR)
                {
                    chrscale[cnt] = newsize*madscale[chrmodel[cnt]]*4;
                }
                else
                {
                    chrscale[cnt] = newsize/(chrfat[mount]*1280);
                }


                // Make in hand items stay the same size...
                newsize = newsize*1280;
                item = chrholdingwhich[cnt][0];
                if(item != MAXCHR)
                    chrscale[item] = chrfat[item]/newsize;
                item = chrholdingwhich[cnt][1];
                if(item != MAXCHR)
                    chrscale[item] = chrfat[item]/newsize;
            }
        }
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void export_one_character_name(char *szSaveName, unsigned short character)
{
    // ZZ> This function makes the naming.txt file for the character
    FILE* filewrite;
    int profile;
    char cTmp;
    int cnt, tnc;


    // Can it export?
    profile = chrmodel[character];
    filewrite = fopen(szSaveName, "w");
    if(filewrite)
    {
        cnt = 0;
        cTmp = chrname[character][0];
        cnt++;
        while(cnt < MAXCAPNAMESIZE && cTmp != 0)
        {
            fprintf(filewrite, ":");
            tnc = 0;
            while(tnc < 8 && cTmp != 0)
            {
                if(cTmp == ' ')
                {
                    fprintf(filewrite, "_");
                }
                else
                {
                    fprintf(filewrite, "%c", cTmp);
                }
                cTmp = chrname[character][cnt];
                tnc++;
                cnt++;
            }
            fprintf(filewrite, "\n");
            fprintf(filewrite, ":STOP\n\n");
        }
        fclose(filewrite);
    }
}

//--------------------------------------------------------------------------------------------
void export_one_character_profile(char *szSaveName, unsigned short character)
{
    // ZZ> This function creates a data.txt file for the given character.
    //     it is assumed that all enchantments have been done away with
    FILE* filewrite;
    int profile;
    int damagetype, skin;
    char types[10] = "SCPHEFIZ";
    char codes[4];


    // General stuff
    profile = chrmodel[character];


    // Open the file
    filewrite = fopen(szSaveName, "w");
    if(filewrite)
    {
        // Real general data
        fprintf(filewrite, "Slot number    : -1\n");  // -1 signals a flexible load thing
        funderf(filewrite, "Class name     : ", capclassname[profile]);
        ftruthf(filewrite, "Uniform light  : ", capuniformlit[profile]);
        fprintf(filewrite, "Maximum ammo   : %d\n", capammomax[profile]);
        fprintf(filewrite, "Current ammo   : %d\n", chrammo[character]);
        fgendef(filewrite, "Gender         : ", chrgender[character]);
        fprintf(filewrite, "\n");



        // Object stats
        fprintf(filewrite, "Life color     : %d\n", chrlifecolor[character]);
        fprintf(filewrite, "Mana color     : %d\n", chrmanacolor[character]);
        fprintf(filewrite, "Life           : %4.2f\n", chrlifemax[character]/256.0);
        fpairof(filewrite, "Life up        : ", caplifeperlevelbase[profile], caplifeperlevelrand[profile]);
        fprintf(filewrite, "Mana           : %4.2f\n", chrmanamax[character]/256.0);
        fpairof(filewrite, "Mana up        : ", capmanaperlevelbase[profile], capmanaperlevelrand[profile]);
        fprintf(filewrite, "Mana return    : %4.2f\n", chrmanareturn[character]/256.0);
        fpairof(filewrite, "Mana return up : ", capmanareturnperlevelbase[profile], capmanareturnperlevelrand[profile]);
        fprintf(filewrite, "Mana flow      : %4.2f\n", chrmanaflow[character]/256.0);
        fpairof(filewrite, "Mana flow up   : ", capmanaflowperlevelbase[profile], capmanaflowperlevelrand[profile]);
        fprintf(filewrite, "STR            : %4.2f\n", chrstrength[character]/256.0);
        fpairof(filewrite, "STR up         : ", capstrengthperlevelbase[profile], capstrengthperlevelrand[profile]);
        fprintf(filewrite, "WIS            : %4.2f\n", chrwisdom[character]/256.0);
        fpairof(filewrite, "WIS up         : ", capwisdomperlevelbase[profile], capwisdomperlevelrand[profile]);
        fprintf(filewrite, "INT            : %4.2f\n", chrintelligence[character]/256.0);
        fpairof(filewrite, "INT up         : ", capintelligenceperlevelbase[profile], capintelligenceperlevelrand[profile]);
        fprintf(filewrite, "DEX            : %4.2f\n", chrdexterity[character]/256.0);
        fpairof(filewrite, "DEX up         : ", capdexterityperlevelbase[profile], capdexterityperlevelrand[profile]);
        fprintf(filewrite, "\n");



        // More physical attributes
        fprintf(filewrite, "Size           : %4.2f\n", chrsizegoto[character]);
        fprintf(filewrite, "Size up        : %4.2f\n", capsizeperlevel[profile]);
        fprintf(filewrite, "Shadow size    : %d\n", capshadowsize[profile]);
        fprintf(filewrite, "Bump size      : %d\n", capbumpsize[profile]);
        fprintf(filewrite, "Bump height    : %d\n", capbumpheight[profile]);
        fprintf(filewrite, "Bump dampen    : %4.2f\n", capbumpdampen[profile]);
        fprintf(filewrite, "Weight         : %d\n", capweight[profile]);
        fprintf(filewrite, "Jump power     : %4.2f\n", capjump[profile]);
        fprintf(filewrite, "Jump number    : %d\n", capjumpnumber[profile]);
        fprintf(filewrite, "Sneak speed    : %d\n", capsneakspd[profile]);
        fprintf(filewrite, "Walk speed     : %d\n", capwalkspd[profile]);
        fprintf(filewrite, "Run speed      : %d\n", caprunspd[profile]);
        fprintf(filewrite, "Fly to height  : %d\n", capflyheight[profile]);
        fprintf(filewrite, "Flashing AND   : %d\n", capflashand[profile]);
        fprintf(filewrite, "Alpha blending : %d\n", capalpha[profile]);
        fprintf(filewrite, "Light blending : %d\n", caplight[profile]);
        ftruthf(filewrite, "Transfer blend : ", captransferblend[profile]);
        fprintf(filewrite, "Sheen          : %d\n", capsheen[profile]);
        ftruthf(filewrite, "Phong mapping  : ", capenviro[profile]);
        fprintf(filewrite, "Texture X add  : %4.2f\n", capuoffvel[profile]/65535.0);
        fprintf(filewrite, "Texture Y add  : %4.2f\n", capvoffvel[profile]/65535.0);
        ftruthf(filewrite, "Sticky butt    : ", capstickybutt[profile]);
        fprintf(filewrite, "\n");



        // Invulnerability data
        ftruthf(filewrite, "Invictus       : ", capinvictus[profile]);
        fprintf(filewrite, "NonI facing    : %d\n", capnframefacing[profile]);
        fprintf(filewrite, "NonI angle     : %d\n", capnframeangle[profile]);
        fprintf(filewrite, "I facing       : %d\n", capiframefacing[profile]);
        fprintf(filewrite, "I angle        : %d\n", capiframeangle[profile]);
        fprintf(filewrite, "\n");



        // Skin defenses
        fprintf(filewrite, "Base defense   : %3d %3d %3d %3d\n", 255-capdefense[profile][0], 255-capdefense[profile][1],
                                                                 255-capdefense[profile][2], 255-capdefense[profile][3]);
        damagetype = 0;
        while(damagetype < MAXDAMAGETYPE)
        {
            fprintf(filewrite, "%c damage shift : %3d %3d %3d %3d\n", types[damagetype],
                                                                      capdamagemodifier[profile][damagetype][0]&DAMAGESHIFT,
                                                                      capdamagemodifier[profile][damagetype][1]&DAMAGESHIFT,
                                                                      capdamagemodifier[profile][damagetype][2]&DAMAGESHIFT,
                                                                      capdamagemodifier[profile][damagetype][3]&DAMAGESHIFT);
            damagetype++;
        }
        damagetype = 0;
        while(damagetype < MAXDAMAGETYPE)
        {
            skin = 0;
            while(skin < 4)
            {
                codes[skin] = 'F';
                if(capdamagemodifier[profile][damagetype][skin]&DAMAGEINVERT)
                    codes[skin] = 'T';
                if(capdamagemodifier[profile][damagetype][skin]&DAMAGECHARGE)
                    codes[skin] = 'C';
                skin++;
            }
            fprintf(filewrite, "%c damage code  : %3c %3c %3c %3c\n", types[damagetype], codes[0], codes[1], codes[2], codes[3]);
            damagetype++;
        }
        fprintf(filewrite, "Acceleration   : %3.0f %3.0f %3.0f %3.0f\n", capmaxaccel[profile][0]*80,
                                                                         capmaxaccel[profile][1]*80, 
                                                                         capmaxaccel[profile][2]*80,
                                                                         capmaxaccel[profile][3]*80);
        fprintf(filewrite, "\n");



        // Experience and level data
        fprintf(filewrite, "EXP for 2nd    : %d\n", capexperienceforlevel[profile][1]);
        fprintf(filewrite, "EXP for 3rd    : %d\n", capexperienceforlevel[profile][2]);
        fprintf(filewrite, "EXP for 4th    : %d\n", capexperienceforlevel[profile][3]);
        fprintf(filewrite, "EXP for 5th    : %d\n", capexperienceforlevel[profile][4]);
        fprintf(filewrite, "EXP for 6th    : %d\n", capexperienceforlevel[profile][5]);
        fprintf(filewrite, "Starting EXP   : %d\n", chrexperience[character]);
        fprintf(filewrite, "EXP worth      : %d\n", capexperienceworth[profile]);
        fprintf(filewrite, "EXP exchange   : %5.3f\n", capexperienceexchange[profile]);
        fprintf(filewrite, "EXPSECRET      : %4.2f\n", capexperiencerate[profile][0]);
        fprintf(filewrite, "EXPQUEST       : %4.2f\n", capexperiencerate[profile][1]);
        fprintf(filewrite, "EXPDARE        : %4.2f\n", capexperiencerate[profile][2]);
        fprintf(filewrite, "EXPKILL        : %4.2f\n", capexperiencerate[profile][3]);
        fprintf(filewrite, "EXPMURDER      : %4.2f\n", capexperiencerate[profile][4]);
        fprintf(filewrite, "EXPREVENGE     : %4.2f\n", capexperiencerate[profile][5]);
        fprintf(filewrite, "EXPTEAMWORK    : %4.2f\n", capexperiencerate[profile][6]);
        fprintf(filewrite, "EXPROLEPLAY    : %4.2f\n", capexperiencerate[profile][7]);
        fprintf(filewrite, "\n");



        // IDSZ identification tags
        undo_idsz(capidsz[profile][0]);
        fprintf(filewrite, "IDSZ Parent    : [%s]\n", valueidsz);
        undo_idsz(capidsz[profile][1]);
        fprintf(filewrite, "IDSZ Type      : [%s]\n", valueidsz);
        undo_idsz(capidsz[profile][2]);
        fprintf(filewrite, "IDSZ Skill     : [%s]\n", valueidsz);
        undo_idsz(capidsz[profile][3]);
        fprintf(filewrite, "IDSZ Special   : [%s]\n", valueidsz);
        undo_idsz(capidsz[profile][4]);
        fprintf(filewrite, "IDSZ Hate      : [%s]\n", valueidsz);
        undo_idsz(capidsz[profile][5]);
        fprintf(filewrite, "IDSZ Vulnie    : [%s]\n", valueidsz);
        fprintf(filewrite, "\n");



        // Item and damage flags
        ftruthf(filewrite, "Is an item     : ", capisitem[profile]);
        ftruthf(filewrite, "Is a mount     : ", capismount[profile]);
        ftruthf(filewrite, "Is stackable   : ", capisstackable[profile]);
        ftruthf(filewrite, "Name known     : ", chrnameknown[character]);
        ftruthf(filewrite, "Usage known    : ", capusageknown[profile]);
        ftruthf(filewrite, "Is exportable  : ", capcancarrytonextmodule[profile]);
        ftruthf(filewrite, "Requires skill : ", capneedskillidtouse[profile]);
        ftruthf(filewrite, "Is platform    : ", capplatform[profile]);
        ftruthf(filewrite, "Collects money : ", capcangrabmoney[profile]);
        ftruthf(filewrite, "Can open stuff : ", capcanopenstuff[profile]);
        fprintf(filewrite, "\n");



        // Other item and damage stuff
        fdamagf(filewrite, "Damage type    : ", capdamagetargettype[profile]);
        factiof(filewrite, "Attack type    : ", capweaponaction[profile]);
        fprintf(filewrite, "\n");



        // Particle attachments
        fprintf(filewrite, "Attached parts : %d\n", capattachedprtamount[profile]);
        fdamagf(filewrite, "Reaffirm type  : ", capattachedprtreaffirmdamagetype[profile]);
        fprintf(filewrite, "Particle type  : %d\n", capattachedprttype[profile]);
        fprintf(filewrite, "\n");



        // Character hands
        ftruthf(filewrite, "Left valid     : ", capgripvalid[profile][0]);
        ftruthf(filewrite, "Right valid    : ", capgripvalid[profile][1]);
        fprintf(filewrite, "\n");



        // Particle spawning on attack
        ftruthf(filewrite, "Part on weapon : ", capattackattached[profile]);
        fprintf(filewrite, "Part type      : %d\n", capattackprttype[profile]);
        fprintf(filewrite, "\n");



        // Particle spawning for GoPoof
        fprintf(filewrite, "Poof amount    : %d\n", capgopoofprtamount[profile]);
        fprintf(filewrite, "Facing add     : %d\n", capgopoofprtfacingadd[profile]);
        fprintf(filewrite, "Part type      : %d\n", capgopoofprttype[profile]);
        fprintf(filewrite, "\n");



        // Particle spawning for blood
        ftruthf(filewrite, "Blood valid    : ", capbloodvalid[profile]);
        fprintf(filewrite, "Part type      : %d\n", capbloodprttype[profile]);
        fprintf(filewrite, "\n");



        // Extra stuff
        ftruthf(filewrite, "Waterwalking   : ", capwaterwalk[profile]);
        fprintf(filewrite, "Bounce dampen  : %5.3f\n", capdampen[profile]);
        fprintf(filewrite, "\n");



        // More stuff
        fprintf(filewrite, "Life healing   : %5.3f\n", caplifeheal[profile]/256.0);
        fprintf(filewrite, "Mana cost      : %5.3f\n", capmanacost[profile]/256.0);
        fprintf(filewrite, "Life return    : %d\n", caplifereturn[profile]);
        fprintf(filewrite, "Stopped by     : %d\n", capstoppedby[profile]);
        funderf(filewrite, "Skin 0 name    : ", capskinname[profile][0]);
        funderf(filewrite, "Skin 1 name    : ", capskinname[profile][1]);
        funderf(filewrite, "Skin 2 name    : ", capskinname[profile][2]);
        funderf(filewrite, "Skin 3 name    : ", capskinname[profile][3]);
        fprintf(filewrite, "Skin 0 cost    : %d\n", capskincost[profile][0]);
        fprintf(filewrite, "Skin 1 cost    : %d\n", capskincost[profile][1]);
        fprintf(filewrite, "Skin 2 cost    : %d\n", capskincost[profile][2]);
        fprintf(filewrite, "Skin 3 cost    : %d\n", capskincost[profile][3]);
        fprintf(filewrite, "STR dampen     : %5.3f\n", capstrengthdampen[profile]);
        fprintf(filewrite, "\n");



        // Another memory lapse
        ftruthf(filewrite, "No rider attak : ", btrue-capridercanattack[profile]);
        ftruthf(filewrite, "Can be dazed   : ", capcanbedazed[profile]);
        ftruthf(filewrite, "Can be grogged : ", capcanbegrogged[profile]);
        fprintf(filewrite, "NOT USED       : 0\n");
        fprintf(filewrite, "NOT USED       : 0\n");
        ftruthf(filewrite, "Can see invisi : ", capcanseeinvisible[profile]);
        fprintf(filewrite, "Kursed chance  : %d\n", chriskursed[character]*100);
        fprintf(filewrite, "Footfall sound : %d\n", capwavefootfall[profile]);
        fprintf(filewrite, "Jump sound     : %d\n", capwavejump[profile]);
        fprintf(filewrite, "\n");


        // Expansions
        fprintf(filewrite, ":[GOLD] %d\n", chrmoney[character]);
        if(capskindressy[profile]&1)
            fprintf(filewrite, ":[DRES] 0\n");
        if(capskindressy[profile]&2)
            fprintf(filewrite, ":[DRES] 1\n");
        if(capskindressy[profile]&4)
            fprintf(filewrite, ":[DRES] 2\n");
        if(capskindressy[profile]&8)
            fprintf(filewrite, ":[DRES] 3\n");
        if(capresistbumpspawn[profile])
            fprintf(filewrite, ":[STUK] 0\n");
        if(capistoobig[profile])
            fprintf(filewrite, ":[PACK] 0\n");
        if(capreflect[profile]==bfalse)
            fprintf(filewrite, ":[VAMP] 1\n");
        if(capalwaysdraw[profile])
            fprintf(filewrite, ":[DRAW] 1\n");
        if(capisranged[profile])
            fprintf(filewrite, ":[RANG] 1\n");
        if(caphidestate[profile]!=NOHIDE)
            fprintf(filewrite, ":[HIDE] %d\n", caphidestate[profile]);
        if(capisequipment[profile])
            fprintf(filewrite, ":[EQUI] 1\n");
        if(capbumpsizebig[profile]==(capbumpsize[profile]<<1))
            fprintf(filewrite, ":[SQUA] 1\n");
        if(capicon[profile] != capusageknown[profile])
            fprintf(filewrite, ":[ICON] %d\n", capicon[profile]);
        if(capforceshadow[profile])
            fprintf(filewrite, ":[SHAD] 1\n");
        if(capcanseekurse[profile])
            fprintf(filewrite, ":[CKUR] 1\n");
        if(capripple[profile] == capisitem[profile])
            fprintf(filewrite, ":[RIPP] %d\n", capripple[profile]);
        fprintf(filewrite, ":[PLAT] %d\n", capcanuseplatforms[profile]);
        fprintf(filewrite, ":[SKIN] %d\n", chrtexture[character]-madskinstart[profile]);
        fprintf(filewrite, ":[CONT] %d\n", chraicontent[character]);
        fprintf(filewrite, ":[STAT] %d\n", chraistate[character]);
        fprintf(filewrite, ":[LEVL] %d\n", chrexperiencelevel[character]);
        fclose(filewrite);
    }
}

//--------------------------------------------------------------------------------------------
void export_one_character_skin(char *szSaveName, unsigned short character)
{
    // ZZ> This function creates a skin.txt file for the given character.
    FILE* filewrite;
    int profile;


    // General stuff
    profile = chrmodel[character];


    // Open the file
    filewrite = fopen(szSaveName, "w");
    if(filewrite)
    {
        fprintf(filewrite, "This file is used only by the import menu\n");
        fprintf(filewrite, ": %d\n", (chrtexture[character]-madskinstart[profile])&3);
        fclose(filewrite);
    }
}

//--------------------------------------------------------------------------------------------
int load_one_character_profile(char *szLoadName)
{
    // ZZ> This function fills a character profile with data from data.txt, returning
    // the object slot that the profile was stuck into.  It may cause the program
    // to abort if bad things happen.
    FILE* fileread;
    int object;
    char szTmp[256];
    int iTmp;
    float fTmp;
    char cTmp;
    int damagetype, level, xptype, idsz;
    int test;

    // Open the file
    fileread = fopen(szLoadName, "r");
    //printf(" DIAG: trying to read %s\n",szLoadName);
    if(fileread!=NULL)
    {
        globalname = szLoadName;
        // Read in the object slot
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); object=iTmp;
        if(object < 0)
        {
            if(importobject < 0)
            {
                sprintf(szTmp, "-1 OBJECT SLOT (%s)", szLoadName);
                general_error(object, object, szTmp);
            }
            else
            {
                object = importobject;
            }
        }



        // Read in the real general data
        goto_colon(fileread);  get_name(fileread, capclassname[object]);


        // Make sure we don't load over an existing model
        if(madused[object])
        {
            sprintf(szTmp, "<--MODEL SLOT USED TWICE (%s)", capclassname[object]);
            general_error(object, object, szTmp);
        }
        madused[object] = btrue;


        // Light cheat
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capuniformlit[object] = bfalse;
/*PORT
            if(cTmp=='T' || cTmp=='t' || shading == D3DSHADE_FLAT)  capuniformlit[object] = btrue;
*/
        // Ammo
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capammomax[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capammo[object] = iTmp;
        // Gender
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capgender[object] = GENOTHER;
            if(cTmp=='F' || cTmp=='f')  capgender[object] = GENFEMALE;
            if(cTmp=='M' || cTmp=='m')  capgender[object] = GENMALE;
            if(cTmp=='R' || cTmp=='r')  capgender[object] = GENRANDOM;
        // Read in the object stats
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  caplifecolor[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capmanacolor[object] = iTmp;
        goto_colon(fileread);  read_pair(fileread);
            caplifebase[object] = pairbase;  capliferand[object] = pairrand;
        goto_colon(fileread);  read_pair(fileread);
            caplifeperlevelbase[object] = pairbase;  caplifeperlevelrand[object] = pairrand;
        goto_colon(fileread);  read_pair(fileread);
            capmanabase[object] = pairbase;  capmanarand[object] = pairrand;
        goto_colon(fileread);  read_pair(fileread);
            capmanaperlevelbase[object] = pairbase;  capmanaperlevelrand[object] = pairrand;
        goto_colon(fileread);  read_pair(fileread);
            capmanareturnbase[object] = pairbase;  capmanareturnrand[object] = pairrand;
        goto_colon(fileread);  read_pair(fileread);
            capmanareturnperlevelbase[object] = pairbase;  capmanareturnperlevelrand[object] = pairrand;
        goto_colon(fileread);  read_pair(fileread);
            capmanaflowbase[object] = pairbase;  capmanaflowrand[object] = pairrand;
        goto_colon(fileread);  read_pair(fileread);
            capmanaflowperlevelbase[object] = pairbase;  capmanaflowperlevelrand[object] = pairrand;
        goto_colon(fileread);  read_pair(fileread);
            capstrengthbase[object] = pairbase;  capstrengthrand[object] = pairrand;
        goto_colon(fileread);  read_pair(fileread);
            capstrengthperlevelbase[object] = pairbase;  capstrengthperlevelrand[object] = pairrand;
        goto_colon(fileread);  read_pair(fileread);
            capwisdombase[object] = pairbase;  capwisdomrand[object] = pairrand;
        goto_colon(fileread);  read_pair(fileread);
            capwisdomperlevelbase[object] = pairbase;  capwisdomperlevelrand[object] = pairrand;
        goto_colon(fileread);  read_pair(fileread);
            capintelligencebase[object] = pairbase;  capintelligencerand[object] = pairrand;
        goto_colon(fileread);  read_pair(fileread);
            capintelligenceperlevelbase[object] = pairbase;  capintelligenceperlevelrand[object] = pairrand;
        goto_colon(fileread);  read_pair(fileread);
            capdexteritybase[object] = pairbase;  capdexterityrand[object] = pairrand;
        goto_colon(fileread);  read_pair(fileread);
            capdexterityperlevelbase[object] = pairbase;  capdexterityperlevelrand[object] = pairrand;

        // More physical attributes
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  capsize[object] = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  capsizeperlevel[object] = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capshadowsize[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capbumpsize[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capbumpheight[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  capbumpdampen[object] = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capweight[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  capjump[object] = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capjumpnumber[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capsneakspd[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capwalkspd[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  caprunspd[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capflyheight[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capflashand[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capalpha[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  caplight[object] = iTmp;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            captransferblend[object] = bfalse;
            if(cTmp=='T' || cTmp=='t')  captransferblend[object] = btrue;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capsheen[object] = iTmp;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capenviro[object] = bfalse;
            if(cTmp=='T' || cTmp=='t')  capenviro[object] = btrue;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  capuoffvel[object] = fTmp*65535;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  capvoffvel[object] = fTmp*65535;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capstickybutt[object] = bfalse;
            if(cTmp=='T' || cTmp=='t')  capstickybutt[object] = btrue;


        // Invulnerability data
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capinvictus[object] = bfalse;
            if(cTmp=='T' || cTmp=='t')  capinvictus[object] = btrue;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capnframefacing[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capnframeangle[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capiframefacing[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capiframeangle[object] = iTmp;
        // Resist burning and stuck arrows with nframe angle of 1 or more
        if(capnframeangle[object] > 0)
        {
            if(capnframeangle[object] == 1)
            {
                capnframeangle[object] = 0;
            }
        }


        // Skin defenses ( 4 skins )
        goto_colon(fileread);
            fscanf(fileread, "%d", &iTmp);  capdefense[object][0] = 255-iTmp;
            fscanf(fileread, "%d", &iTmp);  capdefense[object][1] = 255-iTmp;
            fscanf(fileread, "%d", &iTmp);  capdefense[object][2] = 255-iTmp;
            fscanf(fileread, "%d", &iTmp);  capdefense[object][3] = 255-iTmp;
        damagetype = 0;
        while(damagetype < MAXDAMAGETYPE)
        {
            goto_colon(fileread);
                fscanf(fileread, "%d", &iTmp);  capdamagemodifier[object][damagetype][0] = iTmp;
                fscanf(fileread, "%d", &iTmp);  capdamagemodifier[object][damagetype][1] = iTmp;
                fscanf(fileread, "%d", &iTmp);  capdamagemodifier[object][damagetype][2] = iTmp;
                fscanf(fileread, "%d", &iTmp);  capdamagemodifier[object][damagetype][3] = iTmp;
            damagetype++;
        }
        damagetype = 0;
        while(damagetype < MAXDAMAGETYPE)
        {
            goto_colon(fileread);
                cTmp = get_first_letter(fileread);  if(cTmp=='T' || cTmp=='t')  capdamagemodifier[object][damagetype][0]|=DAMAGEINVERT;
                                                    if(cTmp=='C' || cTmp=='c')  capdamagemodifier[object][damagetype][0]|=DAMAGECHARGE;
                cTmp = get_first_letter(fileread);  if(cTmp=='T' || cTmp=='t')  capdamagemodifier[object][damagetype][1]|=DAMAGEINVERT;
                                                    if(cTmp=='C' || cTmp=='c')  capdamagemodifier[object][damagetype][1]|=DAMAGECHARGE;
                cTmp = get_first_letter(fileread);  if(cTmp=='T' || cTmp=='t')  capdamagemodifier[object][damagetype][2]|=DAMAGEINVERT;
                                                    if(cTmp=='C' || cTmp=='c')  capdamagemodifier[object][damagetype][2]|=DAMAGECHARGE;
                cTmp = get_first_letter(fileread);  if(cTmp=='T' || cTmp=='t')  capdamagemodifier[object][damagetype][3]|=DAMAGEINVERT;
                                                    if(cTmp=='C' || cTmp=='c')  capdamagemodifier[object][damagetype][3]|=DAMAGECHARGE;
            damagetype++;
        }
        goto_colon(fileread);
            fscanf(fileread, "%f", &fTmp);  capmaxaccel[object][0] = fTmp/80.0;
            fscanf(fileread, "%f", &fTmp);  capmaxaccel[object][1] = fTmp/80.0;
            fscanf(fileread, "%f", &fTmp);  capmaxaccel[object][2] = fTmp/80.0;
            fscanf(fileread, "%f", &fTmp);  capmaxaccel[object][3] = fTmp/80.0;


        // Experience and level data
        capexperienceforlevel[object][0] = 0;
        level = 1;
        while(level < MAXLEVEL)
        {
            goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capexperienceforlevel[object][level] = iTmp;
            level++;
        }
        goto_colon(fileread);  read_pair(fileread);
            pairbase = pairbase>>8;
            pairrand = pairrand>>8;
            if(pairrand < 1)  pairrand = 1;
            capexperiencebase[object] = pairbase;
            capexperiencerand[object] = pairrand;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capexperienceworth[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  capexperienceexchange[object] = fTmp;
        xptype = 0;
        while(xptype < MAXEXPERIENCETYPE)
        {
            goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  capexperiencerate[object][xptype] = fTmp+0.001;
            xptype++;
        }


        // IDSZ tags
        idsz = 0;
        while(idsz < MAXIDSZ)
        {
            goto_colon(fileread);  iTmp = get_idsz(fileread);  capidsz[object][idsz] = iTmp;
            idsz++;
        }


        // Item and damage flags
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capisitem[object] = bfalse;  capripple[object] = btrue;
            if(cTmp=='T' || cTmp=='t')  { capisitem[object] = btrue; capripple[object] = bfalse; }
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capismount[object] = bfalse;
            if(cTmp=='T' || cTmp=='t')  capismount[object] = btrue;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capisstackable[object] = bfalse;
            if(cTmp=='T' || cTmp=='t')  capisstackable[object] = btrue;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capnameknown[object] = bfalse;
            if(cTmp=='T' || cTmp=='t')  capnameknown[object] = btrue;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capusageknown[object] = bfalse;
            if(cTmp=='T' || cTmp=='t')  capusageknown[object] = btrue;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capcancarrytonextmodule[object] = bfalse;
            if(cTmp=='T' || cTmp=='t')  capcancarrytonextmodule[object] = btrue;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capneedskillidtouse[object] = bfalse;
            if(cTmp=='T' || cTmp=='t')  capneedskillidtouse[object] = btrue;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capplatform[object] = bfalse;
            if(cTmp=='T' || cTmp=='t')  capplatform[object] = btrue;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capcangrabmoney[object] = bfalse;
            if(cTmp=='T' || cTmp=='t')  capcangrabmoney[object] = btrue;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capcanopenstuff[object] = bfalse;
            if(cTmp=='T' || cTmp=='t')  capcanopenstuff[object] = btrue;



        // More item and damage stuff
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            if(cTmp=='S' || cTmp=='s')  capdamagetargettype[object] = DAMAGESLASH;
            if(cTmp=='C' || cTmp=='c')  capdamagetargettype[object] = DAMAGECRUSH;
            if(cTmp=='P' || cTmp=='p')  capdamagetargettype[object] = DAMAGEPOKE;
            if(cTmp=='H' || cTmp=='h')  capdamagetargettype[object] = DAMAGEHOLY;
            if(cTmp=='E' || cTmp=='e')  capdamagetargettype[object] = DAMAGEEVIL;
            if(cTmp=='F' || cTmp=='f')  capdamagetargettype[object] = DAMAGEFIRE;
            if(cTmp=='I' || cTmp=='i')  capdamagetargettype[object] = DAMAGEICE;
            if(cTmp=='Z' || cTmp=='z')  capdamagetargettype[object] = DAMAGEZAP;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capweaponaction[object] = what_action(cTmp);


        // Particle attachments
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capattachedprtamount[object] = iTmp;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            if(cTmp=='N' || cTmp=='n')  capattachedprtreaffirmdamagetype[object] = DAMAGENULL;
            if(cTmp=='S' || cTmp=='s')  capattachedprtreaffirmdamagetype[object] = DAMAGESLASH;
            if(cTmp=='C' || cTmp=='c')  capattachedprtreaffirmdamagetype[object] = DAMAGECRUSH;
            if(cTmp=='P' || cTmp=='p')  capattachedprtreaffirmdamagetype[object] = DAMAGEPOKE;
            if(cTmp=='H' || cTmp=='h')  capattachedprtreaffirmdamagetype[object] = DAMAGEHOLY;
            if(cTmp=='E' || cTmp=='e')  capattachedprtreaffirmdamagetype[object] = DAMAGEEVIL;
            if(cTmp=='F' || cTmp=='f')  capattachedprtreaffirmdamagetype[object] = DAMAGEFIRE;
            if(cTmp=='I' || cTmp=='i')  capattachedprtreaffirmdamagetype[object] = DAMAGEICE;
            if(cTmp=='Z' || cTmp=='z')  capattachedprtreaffirmdamagetype[object] = DAMAGEZAP;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capattachedprttype[object] = iTmp;


        // Character hands
        capgripvalid[object][0] = bfalse;
        capgripvalid[object][1] = bfalse;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            if(cTmp=='T' || cTmp=='t')  capgripvalid[object][0] = btrue;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            if(cTmp=='T' || cTmp=='t')  capgripvalid[object][1] = btrue;




        // Attack order ( weapon )
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capattackattached[object] = bfalse;
            if(cTmp=='T' || cTmp=='t')  capattackattached[object] = btrue;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capattackprttype[object] = iTmp;


        // GoPoof
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capgopoofprtamount[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capgopoofprtfacingadd[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capgopoofprttype[object] = iTmp;


        // Blood
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capbloodvalid[object] = bfalse;
            if(cTmp=='T' || cTmp=='t')  capbloodvalid[object] = btrue;
            if(cTmp=='U' || cTmp=='u')  capbloodvalid[object] = ULTRABLOODY;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capbloodprttype[object] = iTmp;


        // Stuff I forgot
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capwaterwalk[object] = bfalse;
            if(cTmp=='T' || cTmp=='t')  capwaterwalk[object] = btrue;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  capdampen[object] = fTmp;


        // More stuff I forgot
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  caplifeheal[object] = fTmp*256;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  capmanacost[object] = fTmp*256;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  caplifereturn[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capstoppedby[object] = iTmp|MESHFXIMPASS;
        goto_colon(fileread);  get_name(fileread, capskinname[object][0]);
        goto_colon(fileread);  get_name(fileread, capskinname[object][1]);
        goto_colon(fileread);  get_name(fileread, capskinname[object][2]);
        goto_colon(fileread);  get_name(fileread, capskinname[object][3]);
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capskincost[object][0] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capskincost[object][1] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capskincost[object][2] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  capskincost[object][3] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  capstrengthdampen[object] = fTmp;



        // Another memory lapse
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            capridercanattack[object] = btrue;
            if(cTmp=='T' || cTmp=='t')  capridercanattack[object] = bfalse;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);  // Can be dazed
            capcanbedazed[object] = bfalse;
            if(cTmp=='T' || cTmp=='t')  capcanbedazed[object] = btrue;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);  // Can be grogged
            capcanbegrogged[object] = bfalse;
            if(cTmp=='T' || cTmp=='t')  capcanbegrogged[object] = btrue;
        goto_colon(fileread);  // !!!BAD!!! Life add
        goto_colon(fileread);  // !!!BAD!!! Mana add
        goto_colon(fileread);  cTmp = get_first_letter(fileread);  // Can see invisible
            capcanseeinvisible[object] = bfalse;
            if(cTmp=='T' || cTmp=='t')  capcanseeinvisible[object] = btrue;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);      // Chance of kursed
            capkursechance[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);      // Footfall sound
            if(iTmp < -1) iTmp = -1;
            if(iTmp > MAXWAVE-1) iTmp = MAXWAVE-1;
            capwavefootfall[object] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);      // Jump sound
            if(iTmp < -1) iTmp = -1;
            if(iTmp > MAXWAVE-1) iTmp = MAXWAVE-1;
            capwavejump[object] = iTmp;


        // Clear expansions...
        capskindressy[object] = bfalse;
        capresistbumpspawn[object] = bfalse;
        capistoobig[object] = bfalse;
        capreflect[object] = btrue;
        capalwaysdraw[object] = bfalse;
        capisranged[object] = bfalse;
        caphidestate[object] = NOHIDE;
        capisequipment[object] = bfalse;
        capbumpsizebig[object] = capbumpsize[object]+(capbumpsize[object]>>1);
        capcanseekurse[object] = bfalse;
        capmoney[object] = 0;
        capicon[object] = capusageknown[object];
        capforceshadow[object] = bfalse;
        capskinoverride[object] = NOSKINOVERRIDE;
        capcontentoverride[object] = 0;
        capstateoverride[object] = 0;
        capleveloverride[object] = 0;
        capcanuseplatforms[object] = !capplatform[object];
        // Read expansions
        while(goto_colon_yesno(fileread))
        {
            idsz = get_idsz(fileread);
            fscanf(fileread, "%c%d", &cTmp, &iTmp);
            test = ('D'-'A'<<15)|('R'-'A'<<10)|('E'-'A'<<5)|('S'-'A');  // [DRES]
            if(idsz == test)  capskindressy[object]|= 1<<iTmp;
            test = ('G'-'A'<<15)|('O'-'A'<<10)|('L'-'A'<<5)|('D'-'A');  // [GOLD]
            if(idsz == test)  capmoney[object] = iTmp;
            test = ('S'-'A'<<15)|('T'-'A'<<10)|('U'-'A'<<5)|('K'-'A');  // [STUK]
            if(idsz == test)  capresistbumpspawn[object] = 1-iTmp;
            test = ('P'-'A'<<15)|('A'-'A'<<10)|('C'-'A'<<5)|('K'-'A');  // [PACK]
            if(idsz == test)  capistoobig[object] = 1-iTmp;
            test = ('V'-'A'<<15)|('A'-'A'<<10)|('M'-'A'<<5)|('P'-'A');  // [VAMP]
            if(idsz == test)  capreflect[object] = 1-iTmp;
            test = ('D'-'A'<<15)|('R'-'A'<<10)|('A'-'A'<<5)|('W'-'A');  // [DRAW]
            if(idsz == test)  capalwaysdraw[object] = iTmp;
            test = ('R'-'A'<<15)|('A'-'A'<<10)|('N'-'A'<<5)|('G'-'A');  // [RANG]
            if(idsz == test)  capisranged[object] = iTmp;
            test = ('H'-'A'<<15)|('I'-'A'<<10)|('D'-'A'<<5)|('E'-'A');  // [HIDE]
            if(idsz == test)  caphidestate[object] = iTmp;
            test = ('E'-'A'<<15)|('Q'-'A'<<10)|('U'-'A'<<5)|('I'-'A');  // [EQUI]
            if(idsz == test)  capisequipment[object] = iTmp;
            test = ('S'-'A'<<15)|('Q'-'A'<<10)|('U'-'A'<<5)|('A'-'A');  // [SQUA]
            if(idsz == test)  capbumpsizebig[object] = capbumpsize[object]<<1;
            test = ('I'-'A'<<15)|('C'-'A'<<10)|('O'-'A'<<5)|('N'-'A');  // [ICON]
            if(idsz == test)  capicon[object] = iTmp;
            test = ('S'-'A'<<15)|('H'-'A'<<10)|('A'-'A'<<5)|('D'-'A');  // [SHAD]
            if(idsz == test)  capforceshadow[object] = iTmp;
            test = ('C'-'A'<<15)|('K'-'A'<<10)|('U'-'A'<<5)|('R'-'A');  // [CKUR]
            if(idsz == test)  capcanseekurse[object] = iTmp;
            test = ('S'-'A'<<15)|('K'-'A'<<10)|('I'-'A'<<5)|('N'-'A');  // [SKIN]
            if(idsz == test)  capskinoverride[object] = iTmp&3;
            test = ('C'-'A'<<15)|('O'-'A'<<10)|('N'-'A'<<5)|('T'-'A');  // [CONT]
            if(idsz == test)  capcontentoverride[object] = iTmp;
            test = ('S'-'A'<<15)|('T'-'A'<<10)|('A'-'A'<<5)|('T'-'A');  // [STAT]
            if(idsz == test)  capstateoverride[object] = iTmp;
            test = ('L'-'A'<<15)|('E'-'A'<<10)|('V'-'A'<<5)|('L'-'A');  // [LEVL]
            if(idsz == test)  capleveloverride[object] = iTmp;
            test = ('P'-'A'<<15)|('L'-'A'<<10)|('A'-'A'<<5)|('T'-'A');  // [PLAT]
            if(idsz == test)  capcanuseplatforms[object] = iTmp;
            test = ('R'-'A'<<15)|('I'-'A'<<10)|('P'-'A'<<5)|('P'-'A');  // [RIPP]
            if(idsz == test)  capripple[object] = iTmp;
        }
        fclose(fileread);
    }
    else
    {
        // The data file wasn't found
        general_error(0, 0, "DATA.TXT NOT FOUND");
    }

    return object;
}

//--------------------------------------------------------------------------------------------
int get_skin(char *filename)
{
    // ZZ> This function reads the skin.txt file...
    FILE*   fileread;
    int skin;


    skin = 0;
    fileread = fopen(filename, "r");
    if(fileread)
    {
        goto_colon_yesno(fileread);
        fscanf(fileread, "%d", &skin);
        skin = skin & 3;
        fclose(fileread);
    }
    return skin;
}

//--------------------------------------------------------------------------------------------
void check_player_import(char *dirname)
{
    // ZZ> This function figures out which players may be imported, and loads basic
    //     data for each
    char searchname[128];
    char filename[128];
    int skin;
    bool_t keeplooking;
    const char *foundfile;


    // Set up...
    numloadplayer = 0;

    // Search for all objects
    sprintf(searchname, "%s/*.obj", dirname);
    foundfile = fs_findFirstFile(dirname, "obj");
    keeplooking = 1;
    if(foundfile != NULL)
    {
        while(keeplooking && numloadplayer < MAXLOADPLAYER)
        {
            //fprintf(stderr,"foundfile=%s keeplooking=%d numload=%d/%d\n",foundfile,keeplooking,numloadplayer,MAXLOADPLAYER);
            prime_names();
            sprintf(loadplayerdir[numloadplayer], "%s", foundfile);
            
			sprintf(filename, "%s/%s/skin.txt", dirname, foundfile);
            skin = get_skin(filename);
            
			snprintf(filename, 128, "%s/%s/tris.md2", dirname, foundfile);
			load_one_md2(filename, numloadplayer);

			sprintf(filename, "%s/%s/icon%d.bmp", dirname, foundfile, skin);
            load_one_icon(filename);

            sprintf(filename, "%s/%s/naming.txt", dirname, foundfile);
            read_naming(0, filename);
            naming_names(0);
			sprintf(loadplayername[numloadplayer], "%s", namingnames);

            numloadplayer++;

            foundfile=fs_findNextFile();
            if(foundfile==NULL) keeplooking=0; else keeplooking=1;
        }
    }
    fs_findClose();
    nullicon = globalnumicon;
    load_one_icon("basicdat/nullicon.bmp");
    keybicon = globalnumicon;
    load_one_icon("basicdat/keybicon.bmp");
    mousicon = globalnumicon;
    load_one_icon("basicdat/mousicon.bmp");
    joyaicon = globalnumicon;
    load_one_icon("basicdat/joyaicon.bmp");
    joybicon = globalnumicon;
    load_one_icon("basicdat/joybicon.bmp");
    keybplayer = 0;
    mousplayer = 0;
    joyaplayer = 0;
    joybplayer = 0;
}

