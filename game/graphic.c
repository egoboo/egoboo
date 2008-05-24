/* Egoboo - graphic.c
 * All sorts of stuff related to drawing the game, and all sorts of other stuff
 * (such as data loading) that really should not be in here. 
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
#include "Log.h"

#ifdef __unix__
#include <unistd.h>
#define min(a,b) ( ((a)<(b))? (a):(b) )
#endif

// Defined in egoboo.h
SDL_Surface *displaySurface = NULL;
bool_t	gTextureOn = bfalse;

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



// This needs work
void Begin3DMode()
{
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(mProjection.v);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(mView.v);
	glMultMatrixf(mWorld.v);
}

void End3DMode()
{
}

/********************> Begin2DMode() <*****/
void Begin2DMode( void )
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();									// Reset The Projection Matrix
  glOrtho(0, scrx, 0, scry, 1, -1);					// Set up an orthogonal projection

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
}	

/********************> End2DMode() <*****/
void End2DMode( void )
{
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
}

//---------------------------------------------------------------------------------------------
int get_level(unsigned char x, unsigned char y, unsigned int fan, unsigned char waterwalk)
{
    // ZZ> This function returns the height of a point within a mesh fan, precise
    //     If waterwalk is nonzero and the fan is watery, then the level returned is the
    //     level of the water.
    int z0, z1, z2, z3;         // Height of each fan corner
    int zleft, zright,zdone;    // Weighted height of each side

    x = x&127;
    y = y&127;
    z0 = meshvrtz[meshvrtstart[fan]+0];
    z1 = meshvrtz[meshvrtstart[fan]+1];
    z2 = meshvrtz[meshvrtstart[fan]+2];
    z3 = meshvrtz[meshvrtstart[fan]+3];

    zleft = (z0*(128-y)+z3*y)>>7;
    zright = (z1*(128-y)+z2*y)>>7;
    zdone = (zleft*(128-x)+zright*x)>>7;
    if(waterwalk)
    {
        if(watersurfacelevel>zdone && (meshfx[fan]&MESHFXWATER) && wateriswater)
        {
            return watersurfacelevel;
        }
    }
    return zdone;
}

//---------------------------------------------------------------------------------------------
void release_all_textures()
{
	// ZZ> This function clears out all of the textures
	int cnt;
	
	for (cnt = 0; cnt < MAXTEXTURE; cnt++)
		GLTexture_Release(&txTexture[cnt]);
}

//--------------------------------------------------------------------------------------------
void load_one_icon(char *szLoadName)
{
	// ZZ> This function is used to load an icon.  Most icons are loaded
	//     without this function though...
	
    GLTexture_Load( &TxIcon[globalnumicon], szLoadName );//lpDDSIcon[globalnumicon] = DDLoadBitmap(lpDD, szLoadName, 0, 0, bfalse);
    if ( GLTexture_GetTextureID( &TxIcon[globalnumicon] ) != 0 )//if(lpDDSIcon[globalnumicon]!=NULL)
	{
		/* PORT
		DDSetColorKey(lpDDSIcon[globalnumicon], 0); */
	}
	globalnumicon++;
	
}

//---------------------------------------------------------------------------------------------
void prime_titleimage()
{
    // ZZ> This function sets the title image pointers to NULL
    int cnt;

    for (cnt = 0; cnt < MAXMODULE; cnt++)
        TxTitleImage[cnt].textureID=0;
    
	//titlerect.x = 0;
    //titlerect.w = TITLESIZE;
    //titlerect.y = 0;
    //titlerect.h = TITLESIZE;
}

//---------------------------------------------------------------------------------------------
void prime_icons()
{
    // ZZ> This function sets the icon pointers to NULL
    int cnt;

    for (cnt = 0; cnt < MAXTEXTURE+1; cnt++)
    {
        //lpDDSIcon[cnt]=NULL;
        TxIcon[cnt].textureID=-1;
        madskintoicon[cnt]=0;
    }
    iconrect.left = 0;
    iconrect.right = 32;
    iconrect.top = 0;
    iconrect.bottom = 32;
    globalnumicon=0;
}

//---------------------------------------------------------------------------------------------
void release_all_icons()
{
	// ZZ> This function clears out all of the icons
	int cnt;
	
	for (cnt = 0; cnt < MAXTEXTURE+1; cnt++)
		GLTexture_Release( &TxIcon[cnt] );

	prime_icons();	/* Do we need this? */
}

//---------------------------------------------------------------------------------------------
void release_all_titleimages()
{
	// ZZ> This function clears out all of the title images
	int cnt;
	
	for (cnt = 0; cnt < MAXMODULE; cnt++)
		GLTexture_Release( &TxTitleImage[cnt] );
}

//---------------------------------------------------------------------------------------------
void release_all_models()
{
    // ZZ> This function clears out all of the models
	int cnt;
    for (cnt = 0; cnt < MAXMODEL; cnt++)
    {
        capclassname[cnt][0] = 0;
        madused[cnt] = bfalse;
        madname[cnt][0] = '*';
        madname[cnt][1] = 'N';
        madname[cnt][2] = 'O';
        madname[cnt][3] = 'N';
        madname[cnt][4] = 'E';
        madname[cnt][5] = '*';
        madname[cnt][6] = 0;
    }
    madloadframe=0;
}


//--------------------------------------------------------------------------------------------
void release_grfx(void)
{
    // ZZ> This function frees up graphics/input/sound resources.

	SDL_Quit ();
}

//--------------------------------------------------------------------------------------------
void release_map()
{
	// ZZ> This function releases all the map images
	
    GLTexture_Release( &TxMap );
}

//--------------------------------------------------------------------------------------------
void debug_message(char *text)
{
    // ZZ> This function sticks a message in the display queue and sets its timer
    int slot = get_free_message();
    // Copy the message
    int write = 0;
    int read = 0;
    char cTmp = text[read];  read++;
    msgtime[slot] = MESSAGETIME;
    
	while(cTmp != 0)
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
    if(numpla > 1)
    {
        sprintf(endtext, "Sadly, they were never heard from again...");
        endtextwrite = 42;  // Where to append further text
    }
    else
    {
        if(numpla == 0)
        {
            // No players???  RTS module???
            sprintf(endtext, "The game has ended...");
            endtextwrite = 21;
        }
        else
        {
            // One player
            sprintf(endtext, "Sadly, no trace was ever found...");
            endtextwrite = 33;  // Where to append further text
        }
    }
}

//--------------------------------------------------------------------------------------------
void append_end_text(int message, unsigned short character)
{
    // ZZ> This function appends a message to the end-module text
    int read, cnt;
    char *eread;
    char szTmp[256];
    char cTmp, lTmp;
    unsigned short target, owner;

    target = chraitarget[character];
    owner = chraiowner[character];
    if(message<msgtotal)
    {
        // Copy the message
        read = msgindex[message];
        cnt=0;
        cTmp = msgtext[read];  read++;
        while(cTmp != 0)
        {
            if(cTmp == '%')
            {
                // Escape sequence
                eread = szTmp;
                szTmp[0] = 0;
                cTmp = msgtext[read];  read++;
                if(cTmp == 'n')  // Name
                {
                    if(chrnameknown[character])
                        sprintf(szTmp, "%s", chrname[character]);
                    else
                    {
                        lTmp = capclassname[chrmodel[character]][0];
                        if(lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U')
                            sprintf(szTmp, "an %s", capclassname[chrmodel[character]]);
                        else
                            sprintf(szTmp, "a %s", capclassname[chrmodel[character]]);
                    }
                    if(cnt == 0 && szTmp[0] == 'a')  szTmp[0] = 'A';
                }
                if(cTmp == 'c')  // Class name
                {
                    eread = capclassname[chrmodel[character]];
                }
                if(cTmp == 't')  // Target name
                {
                    if(chrnameknown[target])
                        sprintf(szTmp, "%s", chrname[target]);
                    else
                    {
                        lTmp = capclassname[chrmodel[target]][0];
                        if(lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U')
                            sprintf(szTmp, "an %s", capclassname[chrmodel[target]]);
                        else
                            sprintf(szTmp, "a %s", capclassname[chrmodel[target]]);
                    }
                    if(cnt == 0 && szTmp[0] == 'a')  szTmp[0] = 'A';
                }
                if(cTmp == 'o')  // Owner name
                {
                    if(chrnameknown[owner])
                        sprintf(szTmp, "%s", chrname[owner]);
                    else
                    {
                        lTmp = capclassname[chrmodel[owner]][0];
                        if(lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U')
                            sprintf(szTmp, "an %s", capclassname[chrmodel[owner]]);
                        else
                            sprintf(szTmp, "a %s", capclassname[chrmodel[owner]]);
                    }
                    if(cnt == 0 && szTmp[0] == 'a')  szTmp[0] = 'A';
                }
                if(cTmp == 's')  // Target class name
                {
                    eread = capclassname[chrmodel[target]];
                }
                if(cTmp >= '0' && cTmp <= '3')  // Target's skin name
                {
                    eread = capskinname[chrmodel[target]][cTmp-'0'];
                }
                if(cTmp == 'd')  // tmpdistance value
                {
                    sprintf(szTmp, "%d", valuetmpdistance);
                }
                if(cTmp == 'x')  // tmpx value
                {
                    sprintf(szTmp, "%d", valuetmpx);
                }
                if(cTmp == 'y')  // tmpy value
                {
                    sprintf(szTmp, "%d", valuetmpy);
                }
                if(cTmp == 'D')  // tmpdistance value
                {
                    sprintf(szTmp, "%2d", valuetmpdistance);
                }
                if(cTmp == 'X')  // tmpx value
                {
                    sprintf(szTmp, "%2d", valuetmpx);
                }
                if(cTmp == 'Y')  // tmpy value
                {
                    sprintf(szTmp, "%2d", valuetmpy);
                }
                if(cTmp == 'a')  // Character's ammo
                {
                    if(chrammoknown[character])
                        sprintf(szTmp, "%d", chrammo[character]);
                    else
                        sprintf(szTmp, "?");
                }
                if(cTmp == 'k')  // Kurse state
                {
                    if(chriskursed[character])
                        sprintf(szTmp, "kursed");
                    else
                        sprintf(szTmp, "unkursed");
                }
                if(cTmp == 'p')  // Character's possessive
                {
                    if(chrgender[character] == GENFEMALE)
                    {
                        sprintf(szTmp, "her");
                    }
                    else
                    {
                        if(chrgender[character] == GENMALE)
                        {
                            sprintf(szTmp, "his");
                        }
                        else
                        {
                            sprintf(szTmp, "its");
                        }
                    }
                }
                if(cTmp == 'm')  // Character's gender
                {
                    if(chrgender[character] == GENFEMALE)
                    {
                        sprintf(szTmp, "female ");
                    }
                    else
                    {
                        if(chrgender[character] == GENMALE)
                        {
                            sprintf(szTmp, "male ");
                        }
                        else
                        {
                            sprintf(szTmp, " ");
                        }
                    }
                }
                if(cTmp == 'g')  // Target's possessive
                {
                    if(chrgender[target] == GENFEMALE)
                    {
                        sprintf(szTmp, "her");
                    }
                    else
                    {
                        if(chrgender[target] == GENMALE)
                        {
                            sprintf(szTmp, "his");
                        }
                        else
                        {
                            sprintf(szTmp, "its");
                        }
                    }
                }
                // Copy the generated text
                cTmp = *eread;  eread++;
                while(cTmp != 0 && endtextwrite < MAXENDTEXT-1)
                {
                    endtext[endtextwrite] = cTmp;
                    cTmp = *eread;  eread++;
                    endtextwrite++;
                }
            }
            else
            {
                // Copy the letter
                if(endtextwrite < MAXENDTEXT-1)
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
void make_textureoffset(void)
{
    // ZZ> This function sets up for moving textures
    int cnt;
    for (cnt = 0; cnt < 256; cnt++)
        textureoffset[cnt] = cnt*1.0/256;
}

//--------------------------------------------------------------------------------------------
void create_szfpstext(int frames)
{
    // ZZ> This function fills in the number of frames in "000 Frames per Second"
    frames = frames&511;
    szfpstext[0] = '0'+(frames/100);
    szfpstext[1] = '0'+((frames/10)%10);
    szfpstext[2] = '0'+(frames%10);
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
    cnt = 0;
    while(cnt < nummeshrenderlist)
    {
        fan = meshrenderlist[cnt];
        meshinrenderlist[fan] = bfalse;
        cnt++;
    }
    nummeshrenderlist = 0;
    nummeshrenderlistref = 0;
    nummeshrenderlistsha = 0;

    // Make sure it doesn't die ugly !!!BAD!!!

    // It works better this way...
    cornery[cornerlistlowtohighy[3]]+=256;

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
    divx = ylist[3]-ylist[0]; if(divx < 1) return;
    stepx = xlist[3]-xlist[0];
    basex = xlist[0];


    // Find the points in each edge
    leftlist[0] = 0;  leftnum = 1;
    rightlist[0] = 0;  rightnum = 1;
    if(xlist[1] < (stepx*(ylist[1]-ylist[0])/divx)+basex)
    {
        leftlist[leftnum] = 1;  leftnum++;
		cornerx[1]-=512;
    }
    else
    {
        rightlist[rightnum] = 1;  rightnum++;
		cornerx[1]+=512;
    }
    if(xlist[2] < (stepx*(ylist[2]-ylist[0])/divx)+basex)
    {
        leftlist[leftnum] = 2;  leftnum++;
		cornerx[2]-=512;
    }
    else
    {
        rightlist[rightnum] = 2;  rightnum++;
		cornerx[2]+=512;
    }
    leftlist[leftnum] = 3;  leftnum++;
    rightlist[rightnum] = 3;  rightnum++;


    // Make the left edge ( rowstart )
    fany = ylist[0]>>7;
    row = 0;
    cnt = 1;
    while(cnt < leftnum)
    {
        from = leftlist[cnt-1];  to = leftlist[cnt];
        x = xlist[from];
        divx = ylist[to]-ylist[from];
        stepx = 0;
        if(divx > 0)
        {
            stepx = ((xlist[to]-xlist[from])<<7)/divx;
        }
		x-=256;
        run = ylist[to]>>7;
        while(fany < run)
        {
            if(fany >= 0 && fany < meshsizey)
            {
                fanx = x>>7;
                if(fanx < 0)  fanx = 0;
                if(fanx >= meshsizex)  fanx = meshsizex-1;
                fanrowstart[row] = fanx;
                row++;
            }
            x+=stepx;
            fany++;
        }
        cnt++;
    }
    numrow = row;


    // Make the right edge ( rowrun )
    fany = ylist[0]>>7;
    row = 0;
    cnt = 1;
    while(cnt < rightnum)
    {
        from = rightlist[cnt-1];  to = rightlist[cnt];
        x = xlist[from];
		//x+=128;
        divx = ylist[to]-ylist[from];
        stepx = 0;
        if(divx > 0)
        {
            stepx = ((xlist[to]-xlist[from])<<7)/divx;
        }
        run = ylist[to]>>7;
        while(fany < run)
        {
            if(fany >= 0 && fany < meshsizey)
            {
                fanx = x>>7;
                if(fanx < 0)  fanx = 0;
                if(fanx >= meshsizex-1)  fanx = meshsizex-1;//-2
                fanrowrun[row] = ABS(fanx-fanrowstart[row])+1;
                row++;
            }
            x+=stepx;
            fany++;
        }
        cnt++;
    }

	if(numrow != row)
	{
		general_error(numrow, row, "ROW");
	}

    // Fill 'em up again
    fany = ylist[0]>>7;
    if(fany < 0) fany = 0;
    if(fany >= meshsizey) fany = meshsizey-1;
    row = 0;
    while(row < numrow)
    {
        cnt = meshfanstart[fany]+fanrowstart[row];
        run = fanrowrun[row];
        fanx = 0;
        while(fanx < run)
        {
            if(nummeshrenderlist < MAXMESHRENDER)
            {
                // Put each tile in basic list
                meshinrenderlist[cnt] = btrue;
                meshrenderlist[nummeshrenderlist] = cnt;
                nummeshrenderlist++;
                // Put each tile in one other list, for shadows and relections
                if(meshfx[cnt]&MESHFXSHA)
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

    camturnleftrightone = (camturnleftright)/(2*PI);
    camturnleftrightshort = camturnleftrightone*65536;

    // Request matrices needed for local machine
    make_dolist();
    order_dolist();
}

//--------------------------------------------------------------------------------------------
void animate_tiles()
{
    // This function changes the animated tile frame
    if((wldframe & animtileupdateand)==0)
    {
        animtileframeadd = (animtileframeadd + 1) & animtileframeand;
    }
}

//--------------------------------------------------------------------------------------------
void load_basic_textures( char *modname )
{
	// ZZ> This function loads the standard textures for a module
	char newloadname[256];
	
	// Particle sprites
	GLTexture_LoadA( &txTexture[0], "basicdat/globalparticles/particle.bmp", TRANSCOLOR );
	
	// Module background tiles
	make_newloadname( modname, "gamedat/tile0.bmp", newloadname );
	GLTexture_LoadA( &txTexture[1], newloadname, TRANSCOLOR );
	/* PORT		txTexture[1].SetColorKey(TRANSCOLOR); */
	make_newloadname( modname, "gamedat/tile1.bmp", newloadname );
    GLTexture_LoadA(  &txTexture[2], newloadname, TRANSCOLOR );
    /* PORT		txTexture[2].SetColorKey(TRANSCOLOR); */
	make_newloadname( modname, "gamedat/tile2.bmp", newloadname );
	GLTexture_LoadA( &txTexture[3], newloadname, TRANSCOLOR );
	/* PORT		txTexture[3].SetColorKey(TRANSCOLOR); */
	make_newloadname( modname, "gamedat/tile3.bmp", newloadname );
	GLTexture_LoadA( &txTexture[4], newloadname, TRANSCOLOR );
	/* PORT		txTexture[4].SetColorKey(TRANSCOLOR); */
	
	// Water textures
	make_newloadname( modname, "gamedat/watertop.bmp", newloadname );
	GLTexture_Load( &txTexture[5], newloadname);
	make_newloadname( modname, "gamedat/waterlow.bmp", newloadname );
	GLTexture_Load( &txTexture[6], newloadname );// This is also used as far background
	
	
	// Texture 7 is the phong map
	if ( phongon )
	{
		GLTexture_Load( &txTexture[7], "basicdat/phong.bmp" );
	}
	
}

//--------------------------------------------------------------------------------------------
unsigned short action_number()
{
    // ZZ> This function returns the number of the action in cFrameName, or
    //     it returns NOACTION if it could not find a match
    int cnt;
    char first, second;


    first = cFrameName[0];
    second = cFrameName[1];
    for (cnt = 0; cnt < MAXACTION; cnt++)
    {
        if(first == cActionName[cnt][0] && second == cActionName[cnt][1])
            return cnt;
    }

    return NOACTION;
}

//--------------------------------------------------------------------------------------------
unsigned short action_frame()
{
    // ZZ> This function returns the frame number in the third and fourth characters
    //     of cFrameName
    int number;
    sscanf(&cFrameName[2], "%d", &number);
    return number;
}

//--------------------------------------------------------------------------------------------
unsigned short test_frame_name(char letter)
{
    // ZZ> This function returns btrue if the 4th, 5th, 6th, or 7th letters
    //     of the frame name matches the input argument
    if(cFrameName[4]==letter) return btrue;
    if(cFrameName[4]==0) return bfalse;
    if(cFrameName[5]==letter) return btrue;
    if(cFrameName[5]==0) return bfalse;
    if(cFrameName[6]==letter) return btrue;
    if(cFrameName[6]==0) return bfalse;
    if(cFrameName[7]==letter) return btrue;
    return bfalse;
}

//--------------------------------------------------------------------------------------------
void action_copy_correct(int object, unsigned short actiona, unsigned short actionb)
{
    // ZZ> This function makes sure both actions are valid if either of them
    //     are valid.  It will copy start and ends to mirror the valid action.
    if(madactionvalid[object][actiona]==madactionvalid[object][actionb])
    {
        // They are either both valid or both invalid, in either case we can't help
    }
    else
    {
        // Fix the invalid one
        if(madactionvalid[object][actiona]==bfalse)
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
void get_walk_frame(int object, int lip, int action)
{
    // ZZ> This helps make walking look right
    int frame = 0;
	int framesinaction = madactionend[object][action]-madactionstart[object][action];
	
    while(frame < 16)
    {
        int framealong = 0;
        if(framesinaction > 0)
        {
            framealong = ((frame*framesinaction/16) + 2)%framesinaction;
        }
        madframeliptowalkframe[object][lip][frame]=madactionstart[object][action]+framealong;
        frame++;
    }
}

//--------------------------------------------------------------------------------------------
void get_framefx(int frame)
{
    // ZZ> This function figures out the IFrame invulnerability, and Attack, Grab, and
    //     Drop timings
    unsigned short fx = 0;

    if(test_frame_name('I'))
        fx=fx|MADFXINVICTUS;
    if(test_frame_name('L'))
    {
        if(test_frame_name('A'))
            fx=fx|MADFXACTLEFT;
        if(test_frame_name('G'))
            fx=fx|MADFXGRABLEFT;
        if(test_frame_name('D'))
            fx=fx|MADFXDROPLEFT;
        if(test_frame_name('C'))
            fx=fx|MADFXCHARLEFT;
    }
    if(test_frame_name('R'))
    {
        if(test_frame_name('A'))
            fx=fx|MADFXACTRIGHT;
        if(test_frame_name('G'))
            fx=fx|MADFXGRABRIGHT;
        if(test_frame_name('D'))
            fx=fx|MADFXDROPRIGHT;
        if(test_frame_name('C'))
            fx=fx|MADFXCHARRIGHT;
    }
    if(test_frame_name('S'))
        fx=fx|MADFXSTOP;
    if(test_frame_name('F'))
        fx=fx|MADFXFOOTFALL;
    if(test_frame_name('P'))
        fx=fx|MADFXPOOF;
    madframefx[frame] = fx;
}

//--------------------------------------------------------------------------------------------
void make_framelip(int object, int action)
{
    // ZZ> This helps make walking look right
    int frame, framesinaction;

    if(madactionvalid[object][action])
    {
        framesinaction = madactionend[object][action]-madactionstart[object][action];
        frame = madactionstart[object][action];
        while(frame < madactionend[object][action])
        {
            madframelip[frame] = (frame-madactionstart[object][action])*15/framesinaction;
            madframelip[frame] = (madframelip[frame])&15;
            frame++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void get_actions(int object)
{
    // ZZ> This function creates the frame lists for each action based on the
    //     name of each md2 frame in the model
    int frame, framesinaction;
    int action, lastaction;


    // Clear out all actions and reset to invalid
    action = 0;
    while(action < MAXACTION)
    {
        madactionvalid[object][action]=bfalse;
        action++;
    }


    // Set the primary dance action to be the first frame, just as a default
    madactionvalid[object][ACTIONDA]=btrue;
    madactionstart[object][ACTIONDA]=madframestart[object];
    madactionend[object][ACTIONDA]=madframestart[object]+1;


    // Now go huntin' to see what each frame is, look for runs of same action
    //printf("DIAG: ripping md2\n");
    rip_md2_frame_name(0);
    //printf("DIAG: done ripping md2\n");
    lastaction = action_number();  framesinaction = 0;
    frame = 0;
    while(frame < madframes[object])
    {
        rip_md2_frame_name(frame);
        action = action_number();
        if(lastaction == action)
        {
            framesinaction++;
        }
        else
        {
            // Write the old action
            if(lastaction < MAXACTION)
            {
                madactionvalid[object][lastaction]=btrue;
                madactionstart[object][lastaction]=madframestart[object]+frame-framesinaction;
                madactionend[object][lastaction]=madframestart[object]+frame;
            }
            framesinaction = 1;
            lastaction = action;
        }
        get_framefx(madframestart[object]+frame);
        frame++;
    }
    // Write the old action
    if(lastaction < MAXACTION)
    {
        madactionvalid[object][lastaction]=btrue;
        madactionstart[object][lastaction]=madframestart[object]+frame-framesinaction;
        madactionend[object][lastaction]=madframestart[object]+frame;
    }

	// Make sure actions are made valid if a similar one exists
    action_copy_correct(object, ACTIONDA, ACTIONDB);  // All dances should be safe
    action_copy_correct(object, ACTIONDB, ACTIONDC);
    action_copy_correct(object, ACTIONDC, ACTIONDD);
    action_copy_correct(object, ACTIONDB, ACTIONDC);
    action_copy_correct(object, ACTIONDA, ACTIONDB);
        action_copy_correct(object, ACTIONUA, ACTIONUB);
        action_copy_correct(object, ACTIONUB, ACTIONUC);
        action_copy_correct(object, ACTIONUC, ACTIONUD);
    action_copy_correct(object, ACTIONTA, ACTIONTB);
    action_copy_correct(object, ACTIONTC, ACTIONTD);
        action_copy_correct(object, ACTIONCA, ACTIONCB);
        action_copy_correct(object, ACTIONCC, ACTIONCD);
    action_copy_correct(object, ACTIONSA, ACTIONSB);
    action_copy_correct(object, ACTIONSC, ACTIONSD);
        action_copy_correct(object, ACTIONBA, ACTIONBB);
        action_copy_correct(object, ACTIONBC, ACTIONBD);
    action_copy_correct(object, ACTIONLA, ACTIONLB);
    action_copy_correct(object, ACTIONLC, ACTIONLD);
        action_copy_correct(object, ACTIONXA, ACTIONXB);
        action_copy_correct(object, ACTIONXC, ACTIONXD);
    action_copy_correct(object, ACTIONFA, ACTIONFB);
    action_copy_correct(object, ACTIONFC, ACTIONFD);
        action_copy_correct(object, ACTIONPA, ACTIONPB);
        action_copy_correct(object, ACTIONPC, ACTIONPD);
    action_copy_correct(object, ACTIONZA, ACTIONZB);
    action_copy_correct(object, ACTIONZC, ACTIONZD);
        action_copy_correct(object, ACTIONWA, ACTIONWB);
        action_copy_correct(object, ACTIONWB, ACTIONWC);
        action_copy_correct(object, ACTIONWC, ACTIONWD);
        action_copy_correct(object, ACTIONDA, ACTIONWD);  // All walks should be safe
        action_copy_correct(object, ACTIONWC, ACTIONWD);
        action_copy_correct(object, ACTIONWB, ACTIONWC);
        action_copy_correct(object, ACTIONWA, ACTIONWB);
    action_copy_correct(object, ACTIONJA, ACTIONJB);
    action_copy_correct(object, ACTIONJB, ACTIONJC);
    action_copy_correct(object, ACTIONDA, ACTIONJC);    // All jumps should be safe
    action_copy_correct(object, ACTIONJB, ACTIONJC);
    action_copy_correct(object, ACTIONJA, ACTIONJB);
        action_copy_correct(object, ACTIONHA, ACTIONHB);
        action_copy_correct(object, ACTIONHB, ACTIONHC);
        action_copy_correct(object, ACTIONHC, ACTIONHD);
        action_copy_correct(object, ACTIONHB, ACTIONHC);
        action_copy_correct(object, ACTIONHA, ACTIONHB);
    action_copy_correct(object, ACTIONKA, ACTIONKB);
    action_copy_correct(object, ACTIONKB, ACTIONKC);
    action_copy_correct(object, ACTIONKC, ACTIONKD);
    action_copy_correct(object, ACTIONKB, ACTIONKC);
    action_copy_correct(object, ACTIONKA, ACTIONKB);
        action_copy_correct(object, ACTIONMH, ACTIONMI);
    action_copy_correct(object, ACTIONDA, ACTIONMM);
    action_copy_correct(object, ACTIONMM, ACTIONMN);

	
	// Create table for doing transition from one type of walk to another...
    // Clear 'em all to start
    for (frame = 0; frame < madframes[object]; frame++)
        madframelip[frame+madframestart[object]] = 0;

    // Need to figure out how far into action each frame is
    make_framelip(object, ACTIONWA);
    make_framelip(object, ACTIONWB);
    make_framelip(object, ACTIONWC);
    // Now do the same, in reverse, for walking animations
    get_walk_frame(object, LIPDA, ACTIONDA);
    get_walk_frame(object, LIPWA, ACTIONWA);
    get_walk_frame(object, LIPWB, ACTIONWB);
    get_walk_frame(object, LIPWC, ACTIONWC);
}

//--------------------------------------------------------------------------------------------
void make_mad_equally_lit(int model)
{
    // ZZ> This function makes ultra low poly models look better
    int frame, cnt, vert;

    if(madused[model])
    {
        frame = madframestart[model];
        for (cnt = 0; cnt < madframes[model]; cnt++)
        {
			vert = 0;
            while(vert < MAXVERTICES)
            {
                madvrta[frame][vert]=EQUALLIGHTINDEX;
                vert++;
            }
            frame++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void check_copy(char* loadname, int object)
{
    // ZZ> This function copies a model's actions
    FILE *fileread;
    int actiona, actionb;
    char szOne[16], szTwo[16];


    madmsgstart[object] = 0;
    fileread = fopen(loadname, "r");
    if(fileread)
    {
        while(goto_colon_yesno(fileread))
        {
            fscanf(fileread, "%s%s", szOne, szTwo);
            actiona = what_action(szOne[0]);
            actionb = what_action(szTwo[0]);
            action_copy_correct(object, actiona, actionb);
            action_copy_correct(object, actiona+1, actionb+1);
            action_copy_correct(object, actiona+2, actionb+2);
            action_copy_correct(object, actiona+3, actionb+3);
        }
        fclose(fileread);
    }
}

//--------------------------------------------------------------------------------------------
int load_one_object(int skin, char* tmploadname)
{
    // ZZ> This function loads one object and returns the number of skins
    int object;
    int numskins, numicon;
    char newloadname[256];
    char wavename[256];
    int cnt;
    char ctmp;

    //printf(" DIAG: entered load_one_object\n");
    // Load the object data file and get the object number
    make_newloadname(tmploadname, "/data.txt", newloadname);
    object = load_one_character_profile(newloadname);


    //printf(" DIAG: making up model name\n");
    // Make up a name for the model...  IMPORT\TEMP0000.OBJ
    cnt = 0;
    ctmp = tmploadname[cnt];
    while(ctmp != 0 && cnt < 127)
    {
        madname[object][cnt] = ctmp;
        cnt++;
        ctmp = tmploadname[cnt];
    }
    madname[object][cnt]=0;


    //printf(" DIAG: appending slash\n");
    // Append a slash to the tmploadname
    sprintf(newloadname, "%s", tmploadname);
    sprintf(tmploadname, "%s/", newloadname);


    // Load the AI script for this object
    make_newloadname(tmploadname, "script.txt", newloadname);
    //printf(" DIAG: load ai script %s\n",newloadname);
    if(load_ai_script(newloadname))
    {
        // Create a reference to the one we just loaded
        madai[object]=iNumAis-1;
    }


    //printf(" DIAG: load object model\n");
    // Load the object model
    make_newloadname(tmploadname, "tris.md2", newloadname);

#ifdef __unix__
    // unix is case sensitive, but sometimes this file is called tris.MD2
    if (access(newloadname, R_OK))
    {
        make_newloadname(tmploadname, "tris.MD2", newloadname);
        // still no luck !
        if (access(newloadname, R_OK))
        {
            fprintf(stderr, "ERROR: cannot open: %s\n", newloadname);
            SDL_Quit ();
            exit(1);
        }
    }
#endif
        
    load_one_md2(newloadname, object);
	md2_models[object] = md2_loadFromFile(newloadname);


    //printf(" DIAG: fixing lighting\n");
    // Fix lighting if need be
    if(capuniformlit[object])
    {
        make_mad_equally_lit(object);
    }


    //printf(" DIAG: creating actions\n");
    // Create the actions table for this object
    get_actions(object);


    //printf(" DIAG: copy actions\n");
    // Copy entire actions to save frame space COPY.TXT
    make_newloadname(tmploadname, "copy.txt", newloadname);
    check_copy(newloadname, object);


    //printf(" DIAG: loading messages\n");
    // Load the messages for this object
    make_newloadname(tmploadname, "message.txt", newloadname);
    load_all_messages(newloadname, object);


    //printf(" DIAG: doing random naming\n");
    // Load the random naming table for this object
    make_newloadname(tmploadname, "naming.txt", newloadname);
    read_naming(object, newloadname);


    //printf(" DIAG: loading particles\n");
    // Load the particles for this object
    for (cnt = 0; cnt < MAXPRTPIPPEROBJECT; cnt++)
    {
        sprintf(newloadname, "%spart%d.txt", tmploadname, cnt);
        load_one_particle(newloadname, object, cnt);
    }


    //printf(" DIAG: loading waves\n");
    // Load the waves for this object
    for (cnt = 0; cnt < MAXWAVE; cnt++)
    {
        sprintf(wavename, "sound%d.wav", cnt);
        make_newloadname(tmploadname, wavename, newloadname);
        capwaveindex[object][cnt] = Mix_LoadWAV(newloadname);
    }


    //printf(" DIAG: loading enchantments\n");
    // Load the enchantment for this object
    make_newloadname(tmploadname, "enchant.txt", newloadname);
    load_one_enchant_type(newloadname, object);


    //printf(" DIAG: loading skins and icons (PORTED EXCEPT ALPHA)\n");
    // Load the skins and icons
    madskinstart[object] = skin;
    numskins = 0;
    numicon = 0;
    make_newloadname(tmploadname, "tris0.bmp", newloadname);
	
	/*PORT ALPHA STUFF*/
    GLTexture_LoadA( &txTexture[skin+numskins], newloadname, TRANSCOLOR );
    if ( GLTexture_GetTextureID( &txTexture[skin+numskins] ) != 0 )
    {
        //txTexture[skin+numskins].SetColorKey(TRANSCOLOR);  // Port to new alpha code
        numskins++;
        make_newloadname(tmploadname, "icon0.bmp", newloadname);
        GLTexture_Load( &TxIcon[globalnumicon], newloadname );
        if ( GLTexture_GetTextureID( &TxIcon[globalnumicon] ) != 0 )
        {
            //DDSetColorKey(lpDDSIcon[globalnumicon], 0);	// Port to new alpha code
            while( numicon < numskins )
            {
                madskintoicon[skin+numicon]=globalnumicon;
                if(object==SPELLBOOK)  bookicon = globalnumicon;
                numicon++;
            }
            globalnumicon++;
        }
    }
    make_newloadname(tmploadname, "tris1.bmp", newloadname);
    GLTexture_LoadA( &txTexture[skin+numskins], newloadname, TRANSCOLOR );
    if ( GLTexture_GetTextureID( &txTexture[skin+numskins] ) != 0 )
    {
        //txTexture[skin+numskins].SetColorKey(TRANSCOLOR);	// Port to new alpha code
        numskins++;
        make_newloadname(tmploadname, "icon1.bmp", newloadname);
        GLTexture_Load( &TxIcon[globalnumicon], newloadname );
        if ( GLTexture_GetTextureID( &TxIcon[globalnumicon] ) != 0 )
        {
            //DDSetColorKey(lpDDSIcon[globalnumicon], 0);	// Port to new alpha code
            while(numicon<numskins)
            {
                madskintoicon[skin+numicon]=globalnumicon;
                numicon++;
            }
            globalnumicon++;
        }
    }
    make_newloadname(tmploadname, "tris2.bmp", newloadname);
    GLTexture_LoadA( &txTexture[skin+numskins], newloadname, TRANSCOLOR );
    if ( GLTexture_GetTextureID( &txTexture[skin+numskins] ) != 0 )
    {
        //txTexture[skin+numskins].SetColorKey(TRANSCOLOR);	// port to new alpha code
        numskins++;
        make_newloadname(tmploadname, "icon2.bmp", newloadname);
        GLTexture_Load( &TxIcon[globalnumicon], newloadname );
        if ( GLTexture_GetTextureID( &TxIcon[globalnumicon] ) != 0 )
        {
            //DDSetColorKey(lpDDSIcon[globalnumicon], 0);	// port to new alpha code
            while(numicon<numskins)
            {
                madskintoicon[skin+numicon]=globalnumicon;
                numicon++;
            }
            globalnumicon++;
        }
    }
    make_newloadname(tmploadname, "tris3.bmp", newloadname);
    GLTexture_LoadA( &txTexture[skin+numskins], newloadname, TRANSCOLOR );
    if ( GLTexture_GetTextureID( &txTexture[skin+numskins] ) != 0 )
    {
        //txTexture[skin+numskins].SetColorKey(TRANSCOLOR);	// port to new alpha code
        numskins++;
        make_newloadname(tmploadname, "icon3.bmp", newloadname);
        GLTexture_Load( &TxIcon[globalnumicon], newloadname );
        if ( GLTexture_GetTextureID( &TxIcon[globalnumicon] ) != 0 )
        {
            //DDSetColorKey(lpDDSIcon[globalnumicon], 0);	// port to new alpha code
            while(numicon<numskins)
            {
                madskintoicon[skin+numicon]=globalnumicon;
                numicon++;
            }
            globalnumicon++;
        }
    }
	/* FINISH PORTING */
    
    madskins[object] = numskins;
    if(numskins == 0)
    {
        // If we didn't get a skin, set it to the water texture
        madskinstart[object] = 5;
        madskins[object] = 1;
    }


    //printf(" DIAG: leaving load_one_obj\n");
    return numskins;
}

//--------------------------------------------------------------------------------------------
void load_all_objects(char *modname)
{
    // ZZ> This function loads a module's objects
    const char *filehandle;
    bool_t keeplooking;
    FILE* fileread;
    char newloadname[256];
    char filename[256];
    int cnt;
    int skin;
    int importplayer;


    // Log all of the script errors
    //printf(" DIAG: opening ParseErr\n");
    globalparseerr = fopen("ParseErr.txt", "w");
	parseerror = bfalse;
    fprintf(globalparseerr, "This file documents typos found in the AI scripts...\n");

    // Clear the import slots...
    //printf(" DIAG: Clearing import slots\n");
    for (cnt = 0; cnt < MAXMODEL; cnt++)
        capimportslot[cnt] = 10000;
    
    // Load the import directory
    //printf(" DIAG: loading inport dir\n");
    importplayer = -1;
    skin = 8;  // Character skins start at 8...  Trust me
    if(importvalid)
    {
        for (cnt = 0; cnt < MAXIMPORT; cnt++)
        {
            sprintf(filename, "import/temp%04d.obj", cnt);
            // Make sure the object exists...
            sprintf(newloadname, "%s/data.txt", filename);
            fileread = fopen(newloadname, "r");
            if(fileread)
            {
	      //printf("Found import slot %04d\n", cnt);

                fclose(fileread);
                // Load it...
                if((cnt % 9) == 0)
                {
                    importplayer++;
                }
                importobject = ((importplayer)*9)+(cnt%9);
                capimportslot[importobject] = cnt;
                skin += load_one_object(skin, filename);
            }
        }
    }
    //printf(" DIAG: emptying directory\n");
    //empty_import_directory();  // Free up that disk space...

    // Search for .obj directories and load them
    //printf(" DIAG: Searching for .objs\n");
    importobject = -100;
	make_newloadname(modname, "objects/", newloadname);
    filehandle= fs_findFirstFile(newloadname, "obj");

    keeplooking = 1;
    if(filehandle!= NULL)
    {
        while(keeplooking)
        {
			//printf(" DIAG: keeplooking\n");
            sprintf(filename, "%s%s", newloadname, filehandle);
            skin += load_one_object(skin, filename);
			
			filehandle = fs_findNextFile();
			
            keeplooking = (filehandle != NULL);
        }
    }
	fs_findClose();
    //printf(" DIAG: Done Searching for .objs\n");
    fclose(globalparseerr);
}

//--------------------------------------------------------------------------------------------
void load_bars(char* szBitmap)
{
	// ZZ> This function loads the status bar bitmap
	int cnt;
	
	GLTexture_LoadA(&TxBars, szBitmap, 0);
	if (&TxBars == NULL)
		general_error(0, 0, "NO BARS!!!");

	
	// Make the blit rectangles
	for (cnt = 0; cnt < NUMBAR; cnt++)
    {
        tabrect[cnt].left = 0;
        tabrect[cnt].right = TABX;
        tabrect[cnt].top = cnt*BARY;
        tabrect[cnt].bottom = (cnt+1)*BARY;
        barrect[cnt].left = TABX;
        barrect[cnt].right = BARX;  // This is reset whenever a bar is drawn
        barrect[cnt].top = tabrect[cnt].top;
        barrect[cnt].bottom = tabrect[cnt].bottom;
    }

    // Set the transparent color
    //DDSetColorKey(lpDDSBars, 0); port to new alpha code
}

//--------------------------------------------------------------------------------------------
void load_map(char* szModule, int sysmem)
{
	// ZZ> This function loads the map bitmap and the blip bitmap
	char szMap[256];
	
	// Turn it all off
    mapon = bfalse;
    youarehereon = bfalse;
    numblip = 0;

	// Load the images
	sprintf(szMap, "%sgamedat/plan.bmp", szModule);
	GLTexture_Load(&TxMap, szMap);
	if (GLTexture_GetTextureID == 0) log_error("Cannot load map: %s", szMap);

    // Set up the rectangles
    maprect.left   = 0;
    maprect.right  = MAPSIZE;
    maprect.top    = 0;
    maprect.bottom = MAPSIZE;
	
}

//--------------------------------------------------------------------------------------------
void load_font(char* szBitmap, char* szSpacing, int sysmem)
{
    // ZZ> This function loads the font bitmap and sets up the coordinates
    //     of each font on that bitmap...  Bitmap must have 16x6 fonts
    int cnt, i, y, xsize, ysize, xdiv, ydiv;
    int xstt, ystt;
    int xspacing, yspacing;
    unsigned char cTmp;
    FILE *fileread;


    GLTexture_LoadA(&TxFont, szBitmap, 0);
    if(GLTexture_GetTextureID(&TxFont) == 0)
         general_error(0, 0, "NO FONTS!!!");


    // Clear out the conversion table
    for (cnt = 0; cnt < 256; cnt++)
        asciitofont[cnt] = 0;
    

    // Get the size of the bitmap
    xsize = GLTexture_GetImageWidth(&TxFont);
    ysize = GLTexture_GetImageHeight(&TxFont);
    if(xsize == 0 || ysize == 0)
         general_error(xsize, ysize, "BAD FONT SIZE!!!");
    

    // Figure out the general size of each font
    ydiv = ysize/NUMFONTY;
    xdiv = xsize/NUMFONTX;


    // Figure out where each font is and its spacing
    fileread = fopen(szSpacing, "r");
    if(fileread==NULL)
         general_error(xsize, ysize, "FONT SPACING NOT AVAILABLE!!!");
    
	globalname = szSpacing;
    cnt = 0;
    y = 0;
    
    xstt = 0;
    ystt = 0;

    // Uniform font height is at the top
    goto_colon(fileread);
    fscanf(fileread, "%d", &yspacing);
    fontoffset = scry - yspacing;

    // Mark all as unused
    for (i=0; i < 255; i++) 
		asciitofont[i] = 255;

    for (i=0; i < 96; i++) 
	{
        goto_colon(fileread);
        fscanf(fileread, "%c%d", &cTmp, &xspacing);
        if (asciitofont[cTmp] == 255) asciitofont[cTmp] = cnt;
        if (xstt+xspacing+1 > 255) 
		{
          xstt = 0;
          ystt += yspacing;
        }
        fontrect[cnt].x = xstt;
        fontrect[cnt].w = xspacing;
        fontrect[cnt].y = ystt;
        fontrect[cnt].h = yspacing-2;
        fontxspacing[cnt] = xspacing+1;
        xstt += xspacing+1;
        cnt++;
    }
    fclose(fileread);


    // Space between lines
    fontyspacing = (yspacing>>1)+FONTADD;
}

//--------------------------------------------------------------------------------------------
void make_water()
{
    // ZZ> This function sets up water movements
    int layer, frame, point, mode, cnt;
    float temp;
    unsigned char spek;


    layer = 0;
	    while(layer < numwaterlayer)
    {
        if(waterlight)  waterlayeralpha[layer] = 255;  // Some cards don't support alpha lights...
        waterlayeru[layer] = 0;
        waterlayerv[layer] = 0;
        frame = 0;
        while(frame < MAXWATERFRAME)
        {
            // Do first mode
            mode = 0;
            for (point = 0; point < WATERPOINTS; point++)
            {
                temp = sin((frame*2*PI/MAXWATERFRAME)+(2*PI*point/WATERPOINTS)+(2*PI*layer/MAXWATERLAYER));
                waterlayerzadd[layer][frame][mode][point] = temp*waterlayeramp[layer];
                waterlayercolor[layer][frame][mode][point] = (waterlightlevel[layer]*(temp+1.0))+waterlightadd[layer];
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
    for (cnt = 0; cnt < 256; cnt++)
    {
        spek = 0;
        if(cnt > waterspekstart)
        {
            temp = cnt-waterspekstart;
            temp = temp/(256-waterspekstart);
            temp = temp*temp;
            spek = temp*waterspeklevel;
        }

		// [claforte] Probably need to replace this with a 
		//            glColor4f(spek/256.0f, spek/256.0f, spek/256.0f, 1.0f) call:

        if(1) // PORT: if(shading == D3DSHADE_FLAT)
          waterspek[cnt] = 0;
        else
          waterspek[cnt] = 0xff000000|(spek<<16)|(spek<<8)|(spek);
    }
}

//--------------------------------------------------------------------------------------------
void read_wawalite(char *modname)
{
    // ZZ> This function sets up water and lighting for the module
    char newloadname[256];
    FILE* fileread;
    float lx, ly, lz, la;
    float fTmp;
    char cTmp;
    int iTmp;

    make_newloadname(modname, "gamedat/wawalite.txt", newloadname);
    fileread = fopen(newloadname, "r");
    if(fileread)
    {
        goto_colon(fileread);
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
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  numwaterlayer = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterspekstart = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterspeklevel = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterdouselevel = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  watersurfacelevel = iTmp;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            if(cTmp=='T' || cTmp=='t')  waterlight = btrue;
            else waterlight = bfalse;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            wateriswater = bfalse;
            if(cTmp=='T' || cTmp=='t')  wateriswater = btrue;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            if((cTmp=='T' || cTmp=='t') && overlayvalid)  overlayon = btrue;
            else overlayon = bfalse;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            if((cTmp=='T' || cTmp=='t') && backgroundvalid)  clearson = bfalse;
            else clearson = btrue;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayerdistx[0] = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayerdisty[0] = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayerdistx[1] = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayerdisty[1] = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  foregroundrepeat = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  backgroundrepeat = iTmp;


        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlayerz[0] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlayeralpha[0] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlayerframeadd[0] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlightlevel[0] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlightadd[0] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayeramp[0] = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayeruadd[0] = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayervadd[0] = fTmp;

        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlayerz[1] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlayeralpha[1] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlayerframeadd[1] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlightlevel[1] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlightadd[1] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayeramp[1] = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayeruadd[1] = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayervadd[1] = fTmp;

        waterlayeru[0] = 0;
        waterlayerv[0] = 0;
        waterlayeru[1] = 0;
        waterlayerv[1] = 0;
        waterlayerframe[0] = rand()&WATERFRAMEAND;
        waterlayerframe[1] = rand()&WATERFRAMEAND;
        // Read light data second
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  lx = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  ly = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  lz = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  la = fTmp;
        // Read tile data third
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  hillslide = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  slippyfriction = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  airfriction = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterfriction = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  noslipfriction = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  gravity = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  animtileupdateand = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  animtileframeand = iTmp;
                                                               biganimtileframeand = (iTmp<<1)+1;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  damagetileamount = iTmp;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            if(cTmp=='S' || cTmp=='s')  damagetiletype = DAMAGESLASH;
            if(cTmp=='C' || cTmp=='c')  damagetiletype = DAMAGECRUSH;
            if(cTmp=='P' || cTmp=='p')  damagetiletype = DAMAGEPOKE;
            if(cTmp=='H' || cTmp=='h')  damagetiletype = DAMAGEHOLY;
            if(cTmp=='E' || cTmp=='e')  damagetiletype = DAMAGEEVIL;
            if(cTmp=='F' || cTmp=='f')  damagetiletype = DAMAGEFIRE;
            if(cTmp=='I' || cTmp=='i')  damagetiletype = DAMAGEICE;
            if(cTmp=='Z' || cTmp=='z')  damagetiletype = DAMAGEZAP;
        // Read weather data fourth
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            weatheroverwater = bfalse;
            if(cTmp=='T' || cTmp=='t')  weatheroverwater = btrue;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  weathertimereset = iTmp;
        weathertime = weathertimereset;
        weatherplayer = 0;
        // Read extra data
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            meshexploremode = bfalse;
            if(cTmp=='T' || cTmp=='t')  meshexploremode = btrue;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            usefaredge = bfalse;
            if(cTmp=='T' || cTmp=='t')  usefaredge = btrue;
        camswing = 0;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  camswingrate = fTmp;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  camswingamp = fTmp;


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
        if(goto_colon_yesno(fileread))
        {
            fogon = fogallowed;
            fscanf(fileread, "%f", &fTmp);  fogtop = fTmp;
            goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  fogbottom = fTmp;
            goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  fogred = fTmp*255;
            goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  foggrn = fTmp*255;
            goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  fogblu = fTmp*255;
            goto_colon(fileread);  cTmp = get_first_letter(fileread);
              if(cTmp == 'F' || cTmp == 'f')  fogaffectswater = bfalse;
            fogdistance = (fogtop-fogbottom);
            if(fogdistance < 1.0)  fogon = bfalse;


            // Read extra stuff for damage tile particles...
            if(goto_colon_yesno(fileread))
            {
                fscanf(fileread, "%d", &iTmp);  damagetileparttype = iTmp;
                goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);
                damagetilepartand = iTmp;
                goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);
                damagetilesound = iTmp;
            }
        }

        // Allow slow machines to ignore the fancy stuff
        if(twolayerwateron == bfalse && numwaterlayer > 1)
        {
            numwaterlayer = 1;
            iTmp = waterlayeralpha[0];
            iTmp = ((waterlayeralpha[1]*iTmp)>>8) + iTmp;
            if(iTmp > 255) iTmp = 255;
            waterlayeralpha[0] = iTmp;
        }


        fclose(fileread);
        // Do it
		//printf("entering light stuff\n");
        make_lighttable(lx, ly, lz, la);
        make_lighttospek();
        make_water();
    }
    else
    {
        general_error(0, 0, "WAWALITE.TXT NOT READ");
    }
}

//--------------------------------------------------------------------------------------------
void render_background(unsigned short texture)
{
    // ZZ> This function draws the large background
    GLVERTEX vtlist[4];	
    Uint32 light;
    float size;
    unsigned short rotate;
    float sinsize, cossize;
    float x, y, z, u, v;
    int i;


    // Flat shade this?
    if(!shading) glShadeModel (GL_FLAT);

	glBindTexture ( GL_TEXTURE_2D, GLTexture_GetTextureID ( &txTexture[texture] ));    

	// Figure out the screen coordinates of its corners
	x = scrx/2.0;
	y = scry/2.0;
	z = .99999;
	u = waterlayeru[1];
	v = waterlayerv[1];
	rotate=16384+8192-camturnleftrightshort;
	rotate = rotate>>2;
	size = x + y + 1;
	sinsize = turntosin[rotate]*size;
	cossize = turntosin[(rotate+4096)&16383]*size;

	light = (0xffffffff);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	vtlist[0].x = x + cossize;
	vtlist[0].y = y - sinsize;
	vtlist[0].z = z;
	//vtlist[0].dcSpecular = 0;
	vtlist[0].s = 0+u;
	vtlist[0].t = 0+v;

	vtlist[1].x = x + sinsize;
	vtlist[1].y = y + cossize;
	vtlist[1].z = z;
	//vtlist[1].dcSpecular = 0;
	vtlist[1].s = backgroundrepeat+u;
	vtlist[1].t = 0+v;

	vtlist[2].x = x - cossize;
	vtlist[2].y = y + sinsize;
	vtlist[2].z = z;
	//vtlist[2].dcSpecular = 0;
	vtlist[2].s = backgroundrepeat+u;
	vtlist[2].t = backgroundrepeat+v;

	vtlist[3].x = x - sinsize;
	vtlist[3].y = y - cossize;
	vtlist[3].z = z;
	//vtlist[3].dcSpecular = 0;
	vtlist[3].s = 0+u;
	vtlist[3].t = backgroundrepeat+v;

	glBegin (GL_TRIANGLE_FAN);
	for (i = 0; i < 4; i++)
	{
		glTexCoord2f (vtlist[i].s, vtlist[i].t);
		glVertex3f (vtlist[i].x, vtlist[i].y, vtlist[i].z);
	}
	glEnd ();   
}

/* TODO: Implement this below*/
//--------------------------------------------------------------------------------------------
void render_foreground_overlay(unsigned short texture)
{
  GLVERTEX vtlist[4];
  float size;
  float sinsize, cossize;
  float x, y, z, u, v;
  int i;
  unsigned short rotate;
  float loc_foregroundrepeat;

  // Figure out the screen coordinates of its corners
	x = scrx<<6;
    y = scry<<6;
	z = 0;
	u = waterlayeru[1];
	v = waterlayerv[1];
	size = x + y + 1;
	rotate=16384+8192-camturnleftrightshort;
	rotate = rotate>>2;
	sinsize = turntosin[rotate]*size;
	cossize = turntosin[(rotate+4096)&16383]*size;

  loc_foregroundrepeat = foregroundrepeat * min(x/scrx, y/scrx) / 4.0;
  

  vtlist[0].x = x + cossize;
  vtlist[0].y = y - sinsize;
  vtlist[0].z = z;
  vtlist[0].s = 0+u;
  vtlist[0].t = 0+v;

  vtlist[1].x = x + sinsize;
  vtlist[1].y = y + cossize;
  vtlist[1].z = z;
  vtlist[1].s = loc_foregroundrepeat+u;
  vtlist[1].t = v;

  vtlist[2].x = x - cossize;
  vtlist[2].y = y + sinsize;
  vtlist[2].z = z;
  vtlist[2].s = loc_foregroundrepeat+u;
  vtlist[2].t = loc_foregroundrepeat+v;

  vtlist[3].x = x - sinsize;
  vtlist[3].y = y - cossize;
  vtlist[3].z = z;
  vtlist[3].s = u;
  vtlist[3].t = loc_foregroundrepeat+v;

  //-------------------------------------------------
  glPushAttrib(GL_LIGHTING_BIT|GL_DEPTH_BUFFER_BIT|GL_HINT_BIT);

    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST); // make sure that the texture is as smooth as possible

    glShadeModel(GL_FLAT); // Flat shade this

    glDepthMask (GL_FALSE); // do not write into the depth buffer
    glDepthFunc(GL_ALWAYS); // make it appear over the top of everything

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_COLOR); // make the texture a filter

    glBindTexture ( GL_TEXTURE_2D, GLTexture_GetTextureID ( &txTexture[texture] ));  

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
    for (i = 0; i < 4; i++)
    {
      glTexCoord2f(vtlist[i].s, vtlist[i].t);
      glVertex3f (vtlist[i].x, vtlist[i].y, vtlist[i].z);
    }
    glEnd();

  glPopAttrib();
  //-------------------------------------------------
}

//--------------------------------------------------------------------------------------------
void render_shadow(int character)
{
  // ZZ> This function draws a NIFTY shadow
  GLVERTEX v[4];

  float x, y;
  float level;
  float height, size_umbra, size_penumbra;
  float alpha_umbra, alpha_penumbra;
  signed char hide;
  int i;

  hide = caphidestate[chrmodel[character]];
  if(hide == NOHIDE || hide != chraistate[character])
  {
    // Original points
    level = chrlevel[character];
    level += SHADOWRAISE;
    height = chrmatrix[character]_CNV(3,2)-level;
    if(height<0) height = 0;

    size_umbra    = 1.5*(chrbumpsize[character] - height/30.0);
    size_penumbra = 1.5*(chrbumpsize[character] + height/30.0);

    if(height>0)
    {
      float factor_penumbra = (1.5)*((chrbumpsize[character])/size_penumbra);
      float factor_umbra = (1.5)*((chrbumpsize[character])/size_umbra);
      alpha_umbra = 0.3/factor_umbra/factor_umbra/1.5;
      alpha_penumbra = 0.3/factor_penumbra/factor_penumbra/1.5;
    }
    else
    {
      alpha_umbra    = 0.3;
      alpha_penumbra = 0.3;
    };

    /* PORT:if(g_foginfo.on)
    {
    alpha = (alpha>>1) + 64;
    z = (chr.level);
    if(z < g_foginfo.top)
    {
    if(z < g_foginfo.bottom)
    {
    alpha += 80;
    }
    else
    {
    z = 1.0 - ((z-g_foginfo.bottom)/g_foginfo.distance);
    alpha += (80 * z);
    }
    }
    }*/

    x = chrmatrix[character]_CNV(3,0);
    y = chrmatrix[character]_CNV(3,1);

    // Choose texture.
	glBindTexture ( GL_TEXTURE_2D, GLTexture_GetTextureID( &txTexture[particletexture] ));

   
	//GOOD SHADOW
	v[0].s = particleimageu[238][0];
	v[0].t = particleimagev[238][0];

	v[1].s = particleimageu[255][1];
	v[1].t = particleimagev[238][0];

	v[2].s = particleimageu[255][1];
	v[2].t = particleimagev[255][1];

	v[3].s = particleimageu[238][0];
	v[3].t = particleimagev[255][1];

    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
    glDepthMask(GL_FALSE);

    if(size_penumbra>0)
    {
      v[0].x = x+size_penumbra;
      v[0].y = y-size_penumbra;
      v[0].z = level;

      v[1].x = x+size_penumbra;
      v[1].y = y+size_penumbra;
      v[1].z = level;

      v[2].x = x-size_penumbra;
      v[2].y = y+size_penumbra;
      v[2].z = level;

      v[3].x = x-size_penumbra;
      v[3].y = y-size_penumbra;
      v[3].z = level;

      glBegin(GL_TRIANGLE_FAN);
      glColor4f(alpha_penumbra, alpha_penumbra, alpha_penumbra, 1.0);
      for (i = 0; i < 4; i++)
      {
        glTexCoord2f (v[i].s, v[i].t);
        glVertex3f (v[i].x, v[i].y, v[i].z);
      }
      glEnd();
    };

    if(size_umbra>0)
    {
      v[0].x = x+size_umbra;
      v[0].y = y-size_umbra;
      v[0].z = level+0.1;

      v[1].x = x+size_umbra;
      v[1].y = y+size_umbra;
      v[1].z = level+0.1;

      v[2].x = x-size_umbra;
      v[2].y = y+size_umbra;
      v[2].z = level+0.1;

      v[3].x = x-size_umbra;
      v[3].y = y-size_umbra;
      v[3].z = level+0.1;

      glBegin(GL_TRIANGLE_FAN);
      glColor4f(alpha_umbra, alpha_umbra, alpha_umbra, 1.0);
      for (i = 0; i < 4; i++)
      {
        glTexCoord2f (v[i].s, v[i].t);
        glVertex3f (v[i].x, v[i].y, v[i].z);
      }
      glEnd();
    };

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
  };
}

//--------------------------------------------------------------------------------------------
void render_bad_shadow(int character)
{
    // ZZ> This function draws a sprite shadow
    GLVERTEX v[4];
	float size, x, y;
    unsigned char ambi;
    //DWORD light;
    float level; //, z;
    int height;
    signed char hide;
    unsigned char trans;
	int i;


    hide = caphidestate[chrmodel[character]];
    if(hide == NOHIDE || hide != chraistate[character])
    {
        // Original points
        level = chrlevel[character];
        level+=SHADOWRAISE;
        height = chrmatrix[character]_CNV(3,2)-level;
        if(height > 255)  return;
        if(height < 0) height = 0;
        size = chrshadowsize[character]-((height*chrshadowsize[character])>>8);
        if(size < 1) return;
        ambi = chrlightlevel[character]>>4;  // LUL >>3;
        trans = ((255-height)>>1)+64;
/*      if(fogon)
        {
            z = (chrlevel[character]);
            if(z < fogtop)
            {
                if(z > fogbottom)
                {
                    z = ((z-fogbottom)/fogdistance);
                    trans = (trans * z);
                    if(trans < 64)  trans = 64;
                }
                else
                {
                    trans = 64;
                }
            }
        }*/
        //light = (trans<<24) | (ambi<<16) | (ambi<<8) | ambi;
        glColor4f(ambi/255.0, ambi/255.0, ambi/255.0, trans/255.0);
		
		x = chrmatrix[character]_CNV(3,0);
        y = chrmatrix[character]_CNV(3,1);
        v[0].x = (float) x+size;
        v[0].y = (float) y-size;
        v[0].z = (float) level;

        v[1].x = (float) x+size;
        v[1].y = (float) y+size;
        v[1].z = (float) level;
        
		v[2].x = (float) x-size;
        v[2].y = (float) y+size;
        v[2].z = (float) level;
        
		v[3].x = (float) x-size;
        v[3].y = (float) y-size;
        v[3].z = (float) level;
        
        // Choose texture and matrix
        glBindTexture ( GL_TEXTURE_2D, GLTexture_GetTextureID( &txTexture[particletexture] ));
		
		v[0].s = particleimageu[236][0];
		v[0].t = particleimagev[236][0];

		v[1].s = particleimageu[253][1];
		v[1].t = particleimagev[236][0];

		v[2].s = particleimageu[253][1];
		v[2].t = particleimagev[253][1];

		v[3].s = particleimageu[236][0];
		v[3].t = particleimagev[253][1];

		glBegin(GL_TRIANGLE_FAN);
		for (i = 0; i < 4; i++) 
		{
			glTexCoord2f (v[i].s, v[i].t);
			glVertex3f (v[i].x, v[i].y, v[i].z);
		}
		glEnd();   		
    }
}




//--------------------------------------------------------------------------------------------
void light_characters()
{
    // ZZ> This function figures out character lighting
    int cnt, tnc, x, y;
    unsigned short tl, tr, bl, br;
    unsigned short light;

    cnt = 0;
    while(cnt < numdolist)
    {
        tnc = dolist[cnt];
        x = chrxpos[tnc];
        y = chrypos[tnc];
        x = (x&127)>>5;  // From 0 to 3
        y = (y&127)>>5;  // From 0 to 3
        light = 0;
        tl = meshvrtl[meshvrtstart[chronwhichfan[tnc]]+0];
        tr = meshvrtl[meshvrtstart[chronwhichfan[tnc]]+1];
        br = meshvrtl[meshvrtstart[chronwhichfan[tnc]]+2];
        bl = meshvrtl[meshvrtstart[chronwhichfan[tnc]]+3];

        // Interpolate lighting level using tile corners
        switch(x)
        {
            case 0:
                light+=tl<<1;
                light+=bl<<1;
                break;
            case 1:
            case 2:
                light+=tl;
                light+=tr;
                light+=bl;
                light+=br;
                break;
            case 3:
                light+=tr<<1;
                light+=br<<1;
                break;
        }
        switch(y)
        {
            case 0:
                light+=tl<<1;
                light+=tr<<1;
                break;
            case 1:
            case 2:
                light+=tl;
                light+=tr;
                light+=bl;
                light+=br;
                break;
            case 3:
                light+=bl<<1;
                light+=br<<1;
                break;
        }
        light=light>>3;
        chrlightlevel[tnc] = light;


        if(meshexploremode==bfalse)
        {
            // Look up light direction using corners again
            tl = (tl<<8)&0xf000;
            tr = (tr<<4)&0x0f00;
            br = (br)&0x00f0;
            bl = bl>>4;
            tl = tl|tr|br|bl;
            chrlightturnleftright[tnc] = (lightdirectionlookup[tl]<<8);
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
    while(cnt < MAXPRT)
    {
        if(prton[cnt])
        {
            character = prtattachedtocharacter[cnt];
            if(character != MAXCHR)
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
void set_fan_light(int fanx, int fany, unsigned short particle)
{
    // ZZ> This function is a little helper, lighting the selected fan
    //     with the chosen particle
    float x, y;
    int fan, vertex, lastvertex;
    float level;
    float light;


    if(fanx >= 0 && fanx < meshsizex && fany >= 0 && fany < meshsizey)
    {
        fan = fanx+meshfanstart[fany];
        vertex = meshvrtstart[fan];
        lastvertex = vertex + meshcommandnumvertices[meshtype[fan]];
        while(vertex < lastvertex)
        {
            light = meshvrta[vertex];
            x = prtxpos[particle]-meshvrtx[vertex];
            y = prtypos[particle]-meshvrty[vertex];
            level = (x*x + y*y)/prtdynalightfalloff[particle];
            level = 255-level;
            level=level*prtdynalightlevel[particle];
            if(level > light)
            {
                if(level > 255) level = 255;
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
    if(meshexploremode)
    {
        // Set base light level in explore mode...  Don't need to do every frame
        if((allframe & 7) == 0)
        {
            cnt = 0;
            while(cnt < MAXPRT)
            {
                if(prton[cnt] && prtdynalighton[cnt])
                {
                    fanx = prtxpos[cnt];
                    fany = prtypos[cnt];
                    fanx = fanx>>7;
                    fany = fany>>7;
                    addy = -DYNAFANS;
                    while(addy <= DYNAFANS)
                    {
                        addx = -DYNAFANS;
                        while(addx <= DYNAFANS)
                        {
                            set_fan_light(fanx+addx, fany+addy, cnt);
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
        //if(shading != D3DSHADE_FLAT)
		//GS - if(shading == shading)
        {
            // Add to base light level in normal mode
            entry = 0;
            while(entry < nummeshrenderlist)
            {
                fan = meshrenderlist[entry];
                vertex = meshvrtstart[fan];
                lastvertex = vertex + meshcommandnumvertices[meshtype[fan]];
                while(vertex < lastvertex)
                {
                    // Do light particles
                    light = meshvrta[vertex];
                    cnt = 0;
                    while(cnt < numdynalight)
                    {
                        x = dynalightlistx[cnt]-meshvrtx[vertex];
                        y = dynalightlisty[cnt]-meshvrty[vertex];
                        level = (x*x + y*y)/dynalightfalloff[cnt];
                        level = 255-level;
                        if(level > 0)
                        {
                            light+=level*dynalightlevel[cnt];
                        }
                        cnt++;
                    }
                    if(light > 255) light = 255;
                    if(light < 0) light = 0;
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
	glLoadMatrixf(mView.v);
	glMultMatrixf(mWorld.v);


    // Bottom layer first
    if(numwaterlayer > 1 && waterlayerz[1] > -waterlayeramp[1])
    {
        cnt = 0;
        while(cnt < nummeshrenderlist)
        {
            if(meshfx[meshrenderlist[cnt]]&MESHFXWATER)
            {
                // !!!BAD!!! Water will get screwed up if meshsizex is odd
                render_water_fan(meshrenderlist[cnt], 1, ((meshrenderlist[cnt]>>watershift)&2)+(meshrenderlist[cnt]&1));
            }
            cnt++;
        }
    }
    // Top layer second
    if(numwaterlayer > 0 && waterlayerz[0] > -waterlayeramp[0])
    {
        cnt = 0;
        while(cnt < nummeshrenderlist)
        {
            if(meshfx[meshrenderlist[cnt]]&MESHFXWATER)
            {
                // !!!BAD!!! Water will get screwed up if meshsizex is odd
                render_water_fan(meshrenderlist[cnt], 0, ((meshrenderlist[cnt]>>watershift)&2)+(meshrenderlist[cnt]&1));
            }
            cnt++;
        }
    }
	
    // Foreground overlay
    if(overlayon)
    {
        render_foreground_overlay(5);  // Texture 5 is watertop.bmp
    }
}

//--------------------------------------------------------------------------------------------
void draw_scene_sadreflection()
{
    // ZZ> This function draws 3D objects
    unsigned short cnt, tnc;
    unsigned char trans;
    rect_t rect;// = {0, 0, scrx, scry};	// Don't know why this isn't working on the Mac, it should
	
	rect.left = 0;
	rect.right = 0;
	rect.top = scrx;
	rect.bottom = scry;

    // ZB> Clear the z-buffer
	glClear(GL_DEPTH_BUFFER_BIT);
    
	// Clear the image if need be
    if (clearson)
    	glClear(GL_COLOR_BUFFER_BIT);
    else
    {
		// Render the background
        render_background(6);  // 6 is the texture for waterlow.bmp
    }


    // Render the reflective floors
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
    meshlasttexture = 0;
    for (cnt = 0; cnt < nummeshrenderlistref; cnt++)
        render_fan(meshrenderlistref[cnt]);
    
	if(refon)
    {
        // Render reflections of characters
	
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);

		glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

		glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        for (cnt = 0; cnt < numdolist; cnt++)
        {
            tnc = dolist[cnt];
            if((meshfx[chronwhichfan[tnc]]&MESHFXDRAWREF))
            {
                render_refmad(tnc, chralpha[tnc]&chrlight[tnc]);
            }
        }
        
		// Render the reflected sprites
        glFrontFace(GL_CW);
        render_refprt();
        
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
    }

    // Render the shadow floors
    meshlasttexture = 0;
    for (cnt = 0; cnt < nummeshrenderlistsha; cnt++)
        render_fan(meshrenderlistsha[cnt]);
    
    // Render the shadows
    if(shaon)
    {
	    if(shasprite)
        {
            // Bad shadows
            glDepthMask(GL_FALSE);
    		glEnable(GL_BLEND);
			glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            
			for (cnt = 0; cnt < numdolist; cnt++)
            {
                tnc = dolist[cnt];
                if(chrattachedto[tnc] == MAXCHR)
                {
                    if(((chrlight[tnc]==255 && chralpha[tnc]==255) || capforceshadow[chrmodel[tnc]]) && chrshadowsize[tnc]!=0)
                    {
                        render_bad_shadow(tnc);
                    }
                }
            }
        
			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);
        }
        else
        {
            // Good shadows for me
	        glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_COLOR, GL_ZERO);
            for (cnt = 0; cnt < numdolist; cnt++)
            {
                tnc = dolist[cnt];
                if(chrattachedto[tnc] == MAXCHR)
                {
                    if(((chrlight[tnc]==255 && chralpha[tnc]==255) || capforceshadow[chrmodel[tnc]]) && chrshadowsize[tnc]!=0)
                    {
                        render_shadow(tnc);
                    }
                }
            }
		    glDisable(GL_BLEND);
		    glDepthMask(GL_TRUE);
        }
    }

	glAlphaFunc(GL_GREATER, 0);
	glEnable(GL_ALPHA_TEST);
	glDisable(GL_CULL_FACE);

    // Render the normal characters
    for (cnt = 0; cnt < numdolist; cnt++)
    {
        tnc = dolist[cnt];
        if(chralpha[tnc]==255 && chrlight[tnc]==255)
            render_mad(tnc, 255);
    }

	// Render the sprites
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);

    // Now render the transparent characters
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (cnt = 0; cnt < numdolist; cnt++)
    {
        tnc = dolist[cnt];
        if(chralpha[tnc]!=255 && chrlight[tnc]==255)
        {
            trans = chralpha[tnc];
            if(trans < SEEINVISIBLE && (localseeinvisible || chrislocalplayer[tnc]))  trans = SEEINVISIBLE;
            render_mad(tnc, trans);
        }
    }
    
	// Alpha water
    if(!waterlight)  render_water();
    
	// Then do the light characters
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    for (cnt = 0; cnt < numdolist; cnt++)
    {
        tnc = dolist[cnt];
        if(chrlight[tnc]!=255)
        {
            trans = chrlight[tnc];
            if(trans < SEEINVISIBLE && (localseeinvisible || chrislocalplayer[tnc]))  trans = SEEINVISIBLE;
            render_mad(tnc, trans);
        }
    
		// Do phong highlights
        if(phongon && chralpha[tnc]==255 && chrlight[tnc]==255 && chrenviro[tnc]==bfalse && chrsheen[tnc] > 0)
        {
            unsigned short texturesave;
            chrenviro[tnc] = btrue;
            texturesave = chrtexture[tnc];
            chrtexture[tnc] = 7;  // The phong map texture...
            render_mad(tnc, chrsheen[tnc]<<4);
            chrtexture[tnc] = texturesave;
            chrenviro[tnc] = bfalse;
        }
    }

    // Light water
    if(waterlight)  render_water();

    // Turn Z buffer back on, alphablend off
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    render_prt();
    glDisable(GL_ALPHA_TEST);
    
	glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // Done rendering
}

//--------------------------------------------------------------------------------------------
void draw_scene_zreflection()
{
    // ZZ> This function draws 3D objects
    unsigned short cnt, tnc;
    unsigned char trans;

    // Clear the image if need be
    // PORT: I don't think this is needed if(clearson) { clear_surface(lpDDSBack); }
    // Zbuffer is cleared later

    // Render the reflective floors
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // Render the background
    if (clearson == bfalse)
        render_background(6);  // 6 is the texture for waterlow.bmp

    meshlasttexture = 0;

    for (cnt = 0; cnt < nummeshrenderlistref; cnt++)
        render_fan(meshrenderlistref[cnt]);

    // BAD: DRAW SHADOW STUFF TOO
	for (cnt = 0; cnt < nummeshrenderlistsha; cnt++)
        render_fan(meshrenderlistsha[cnt]);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    if(refon)
    {
        // Render reflections of characters
        glFrontFace(GL_CCW);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthFunc(GL_LEQUAL);

        for (cnt = 0; cnt < numdolist; cnt++)
		{
            tnc = dolist[cnt];
            if((meshfx[chronwhichfan[tnc]]&MESHFXDRAWREF))
                render_refmad(tnc, chralpha[tnc]&chrlight[tnc]);
        }
		
		// [claforte] I think this is wrong... I think we should choose some other depth func.
		glDepthFunc(GL_ALWAYS);

        // Render the reflected sprites
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
        glFrontFace(GL_CW);
        render_refprt();
        
		glDisable(GL_BLEND);
		glDepthFunc(GL_LEQUAL);
        glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
    }
    
	// Clear the Zbuffer at a bad time...  But hey, reflections work with Voodoo
    //lpD3DVViewport->Clear(1, &rect, D3DCLEAR_ZBUFFER);
    // Not sure if this is cool or not - DDOI
    // glClear ( GL_DEPTH_BUFFER_BIT );

    // Render the shadow floors
	meshlasttexture = 0;

    for (cnt = 0; cnt < nummeshrenderlistsha; cnt++)
        render_fan(meshrenderlistsha[cnt]);

    // Render the shadows
    if (shaon)
    {
        if (shasprite)
        {
            // Bad shadows
    		glDepthMask(GL_FALSE);
    		glEnable(GL_BLEND);
    		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            for (cnt = 0; cnt < numdolist; cnt++)
            {
                tnc = dolist[cnt];
                if(chrattachedto[tnc] == MAXCHR)
                {
                    if(((chrlight[tnc]==255 && chralpha[tnc]==255) || capforceshadow[chrmodel[tnc]]) && chrshadowsize[tnc]!=0)
                        render_bad_shadow(tnc);
                }
            }
		    glDisable(GL_BLEND);
		    glDepthMask(GL_TRUE);
        }
        else
        {
            // Good shadows for me
			glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_COLOR, GL_ZERO);

            for (cnt = 0; cnt < numdolist; cnt++)
			{
                tnc = dolist[cnt];
                if(chrattachedto[tnc] == MAXCHR)
				{
                    if(((chrlight[tnc]==255 && chralpha[tnc]==255) || capforceshadow[chrmodel[tnc]]) && chrshadowsize[tnc]!=0)
                        render_shadow(tnc);
				}
            }

		    glDisable(GL_BLEND);
		    glDepthMask ( GL_TRUE );
        }
    }

	glAlphaFunc(GL_GREATER, 0);
	glEnable(GL_ALPHA_TEST);

    // Render the normal characters
	for (cnt = 0; cnt < numdolist; cnt++)
    {
        tnc = dolist[cnt];
        if(chralpha[tnc]==255 && chrlight[tnc]==255)
            render_mad(tnc, 255);
    }

    // Render the sprites
	glDepthMask ( GL_FALSE );
    glEnable(GL_BLEND);

    // Now render the transparent characters
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (cnt = 0; cnt < numdolist; cnt++)
	{
        tnc = dolist[cnt];
        if(chralpha[tnc]!=255 && chrlight[tnc]==255)
        {
            trans = chralpha[tnc];
            if(trans < SEEINVISIBLE && (localseeinvisible || chrislocalplayer[tnc]))  trans = SEEINVISIBLE;
            render_mad(tnc, trans);
        }
    }

    // And alpha water floors
    if(!waterlight)  
		render_water();
    
	// Then do the light characters
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    
	for (cnt = 0; cnt < numdolist; cnt++)
    {
        tnc = dolist[cnt];
        if(chrlight[tnc]!=255)
        {
            trans = chrlight[tnc];
            if(trans < SEEINVISIBLE && (localseeinvisible || chrislocalplayer[tnc]))  trans = SEEINVISIBLE;
            render_mad(tnc, trans);
        }

        // Do phong highlights
        if(phongon && chralpha[tnc]==255 && chrlight[tnc]==255 && chrenviro[tnc]==bfalse && chrsheen[tnc] > 0)
        {
            unsigned short texturesave;
            chrenviro[tnc] = btrue;
            texturesave = chrtexture[tnc];
            chrtexture[tnc] = 7;  // The phong map texture...
            render_mad(tnc, chrsheen[tnc]<<4);
            chrtexture[tnc] = texturesave;
            chrenviro[tnc] = bfalse;
        }
    }

    // Do light water
    if(waterlight)  
		render_water();

    // Turn Z buffer back on, alphablend off
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    render_prt();
    glDisable(GL_ALPHA_TEST);

    glDepthMask(GL_TRUE);
    
	glDisable(GL_BLEND);

    // Done rendering
}

//--------------------------------------------------------------------------------------------
bool_t get_mesh_memory()
{
    // ZZ> This function gets a load of memory for the terrain mesh
    floatmemory = (float *) malloc(maxtotalmeshvertices*BYTESFOREACHVERTEX);
    if(floatmemory == NULL)
        return bfalse;

    meshvrtx = &floatmemory[0];
    meshvrty = &floatmemory[1*maxtotalmeshvertices];
    meshvrtz = &floatmemory[2*maxtotalmeshvertices];
    meshvrta = (unsigned char *) &floatmemory[3*maxtotalmeshvertices];
    meshvrtl = &meshvrta[maxtotalmeshvertices];
    return btrue;
}

//--------------------------------------------------------------------------------------------
void draw_blip(unsigned char color, int x, int y)
{
	float xl,xr,yt,yb;
	int width, height;

	// ZZ> This function draws a blip
	if(x > 0 && y > 0)
	{
		EnableTexturing();
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glNormal3f(0.0f, 0.0f, 1.0f);

		glBindTexture(GL_TEXTURE_2D, GLTexture_GetTextureID( &TxBlip ));
		xl = ((float)bliprect[color].left)/32;
		xr = ((float)bliprect[color].right)/32;
		yt = ((float)bliprect[color].top)/4;
		yb = ((float)bliprect[color].bottom)/4;
		width = bliprect[color].right-bliprect[color].left; height=bliprect[color].bottom-bliprect[color].top;

		glBegin(GL_QUADS);
			glTexCoord2f( xl, yb );   glVertex2i( x-1,       scry-y-1-height );
			glTexCoord2f( xr, yb );   glVertex2i( x-1+width, scry-y-1-height );
			glTexCoord2f( xr, yt );   glVertex2i( x-1+width, scry-y-1 );
			glTexCoord2f( xl, yt );   glVertex2i( x-1,       scry-y-1 );
		glEnd();

	}
}

//--------------------------------------------------------------------------------------------
void draw_one_icon(int icontype, int x, int y, unsigned char sparkle)
{
    // ZZ> This function draws an icon
    int position, blipx, blipy;
    float xl,xr,yt,yb;
    int width, height;

    if(TxIcon[icontype].textureID>=0) //if(lpDDSIcon[icontype])
    {
		EnableTexturing();		// Enable texture mapping
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		//lpDDSBack->BltFast(x, y, lpDDSIcon[icontype], &iconrect, DDBLTFAST_NOCOLORKEY);
		glBindTexture( GL_TEXTURE_2D, GLTexture_GetTextureID( &TxIcon[icontype] ) );
		xl=((float)iconrect.left)/32;
		xr=((float)iconrect.right)/32;
		yt=((float)iconrect.top)/32;
		yb=((float)iconrect.bottom)/32;
		width=iconrect.right-iconrect.left; height=iconrect.bottom-iconrect.top;
		glBegin(GL_QUADS);
			glTexCoord2f( xl, yb );   glVertex2i( x,       scry-y-height );
			glTexCoord2f( xr, yb );   glVertex2i( x+width, scry-y-height );
			glTexCoord2f( xr, yt );   glVertex2i( x+width, scry-y );
			glTexCoord2f( xl, yt );   glVertex2i( x,       scry-y );
		glEnd();
    }

    if(sparkle != NOSPARKLE)
    {
        position = wldframe&31;
        position = (SPARKLESIZE * position >> 5);

        blipx = x + SPARKLEADD + position;
        blipy = y + SPARKLEADD;
        draw_blip(sparkle, blipx, blipy);

        blipx = x + SPARKLEADD + SPARKLESIZE;
        blipy = y + SPARKLEADD + position;
        draw_blip(sparkle, blipx, blipy);

        blipx = blipx - position;
        blipy = y + SPARKLEADD + SPARKLESIZE;
        draw_blip(sparkle, blipx, blipy);

        blipx = x + SPARKLEADD;
        blipy = blipy - position;
        draw_blip(sparkle, blipx, blipy);
    }
}

//--------------------------------------------------------------------------------------------
void draw_one_font(int fonttype, int x, int y)
{
	// ZZ> This function draws a letter or number
	// GAC> Very nasty version for starters.  Lots of room for improvement.
	GLfloat dx,dy,fx1,fx2,fy1,fy2;
	GLuint x2,y2;

	y = fontoffset - y;
	x2 = x + fontrect[fonttype].w;
	y2 = y + fontrect[fonttype].h;
	dx = 2.0/512;
	dy = 1.0/256;
	fx1 = fontrect[fonttype].x*dx + 0.001;
	fx2 = (fontrect[fonttype].x+fontrect[fonttype].w)*dx - 0.001;
	fy1 = fontrect[fonttype].y*dy + 0.001;
	fy2 = (fontrect[fonttype].y+fontrect[fonttype].h)*dy;

	glBegin(GL_QUADS);
		glTexCoord2f(fx1,fy2);   glVertex2i( x, y);
		glTexCoord2f(fx2,fy2);   glVertex2i(x2, y);
		glTexCoord2f(fx2,fy1);   glVertex2i(x2,y2);
		glTexCoord2f(fx1,fy1);   glVertex2i( x,y2);
	glEnd();
}

//--------------------------------------------------------------------------------------------
void draw_map(int x, int y)
{
    // ZZ> This function draws the map

    //printf("draw map getting called\n");
	
    EnableTexturing();
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    //glNormal3f( 0, 0, 1 );

    glBindTexture( GL_TEXTURE_2D, GLTexture_GetTextureID( &TxMap ) );
    glBegin(GL_QUADS);
    	glTexCoord2f (0.0, 1.0); glVertex2i (x, 	scry-y-MAPSIZE);
		glTexCoord2f (1.0, 1.0); glVertex2i (x+MAPSIZE, scry-y-MAPSIZE);
		glTexCoord2f (1.0, 0.0); glVertex2i (x+MAPSIZE, scry-y);
		glTexCoord2f (0.0, 0.0); glVertex2i (x, 	scry-y);
    glEnd();	
}

//--------------------------------------------------------------------------------------------
int draw_one_bar(int bartype, int x, int y, int ticks, int maxticks)
{
    // ZZ> This function draws a bar and returns the y position for the next one
    int noticks;
    float xl, xr, yt, yb;
    int width, height;

    EnableTexturing();               // Enable texture mapping
    glColor4f( 1, 1, 1, 1 );

    if(maxticks>0 && ticks >= 0)
    {
        // Draw the tab
        glBindTexture( GL_TEXTURE_2D, GLTexture_GetTextureID( &TxBars ) );
        xl=((float)tabrect[bartype].left)/128;
        xr=((float)tabrect[bartype].right)/128;
        yt=((float)tabrect[bartype].top)/128;
        yb=((float)tabrect[bartype].bottom)/128;
        width=tabrect[bartype].right-tabrect[bartype].left; height=tabrect[bartype].bottom-tabrect[bartype].top;
        glBegin( GL_QUADS );
                glTexCoord2f( xl, yb );   glVertex2i( x,       scry-y-height );
                glTexCoord2f( xr, yb );   glVertex2i( x+width, scry-y-height );
                glTexCoord2f( xr, yt );   glVertex2i( x+width, scry-y );
                glTexCoord2f( xl, yt );   glVertex2i( x,       scry-y );
        glEnd();

        // Error check
        if(maxticks>MAXTICK) maxticks = MAXTICK;
        if(ticks>maxticks) ticks = maxticks;

        // Draw the full rows of ticks
        x+=TABX;
        while(ticks >= NUMTICK)
        {
            barrect[bartype].right = BARX;
            glBindTexture( GL_TEXTURE_2D, GLTexture_GetTextureID( &TxBars ) );
            xl=((float)barrect[bartype].left)/128;
            xr=((float)barrect[bartype].right)/128;
            yt=((float)barrect[bartype].top)/128;
            yb=((float)barrect[bartype].bottom)/128;
            width=barrect[bartype].right-barrect[bartype].left; height=barrect[bartype].bottom-barrect[bartype].top;
            glBegin( GL_QUADS );
                glTexCoord2f( xl, yb );   glVertex2i( x,       scry-y-height );
                glTexCoord2f( xr, yb );   glVertex2i( x+width, scry-y-height );
                glTexCoord2f( xr, yt );   glVertex2i( x+width, scry-y );
                glTexCoord2f( xl, yt );   glVertex2i( x,       scry-y );
            glEnd();
            y+=BARY;
            ticks-=NUMTICK;
            maxticks-=NUMTICK;
        }


        // Draw any partial rows of ticks
        if(maxticks > 0)
        {
            // Draw the filled ones
            barrect[bartype].right = (ticks<<3)+TABX;
            glBindTexture( GL_TEXTURE_2D, GLTexture_GetTextureID( &TxBars ) );
            xl=((float)barrect[bartype].left)/128;
            xr=((float)barrect[bartype].right)/128;
            yt=((float)barrect[bartype].top)/128;
            yb=((float)barrect[bartype].bottom)/128;
            width=barrect[bartype].right-barrect[bartype].left; height=barrect[bartype].bottom-barrect[bartype].top;
            glBegin( GL_QUADS );
                glTexCoord2f( xl, yb );   glVertex2i( x,       scry-y-height );
                glTexCoord2f( xr, yb );   glVertex2i( x+width, scry-y-height );
                glTexCoord2f( xr, yt );   glVertex2i( x+width, scry-y );
                glTexCoord2f( xl, yt );   glVertex2i( x,       scry-y );
            glEnd();

            // Draw the empty ones
            noticks = maxticks-ticks;
            if(noticks > (NUMTICK-ticks)) noticks = (NUMTICK-ticks);
            barrect[0].right = (noticks<<3)+TABX;
            glBindTexture( GL_TEXTURE_2D, GLTexture_GetTextureID( &TxBars ) );
            xl=((float)barrect[0].left)/128;
            xr=((float)barrect[0].right)/128;
            yt=((float)barrect[0].top)/128;
            yb=((float)barrect[0].bottom)/128;
            width=barrect[0].right-barrect[0].left; height=barrect[0].bottom-barrect[0].top;
            glBegin( GL_QUADS );
                glTexCoord2f( xl, yb );   glVertex2i( (ticks<<3)+x,       scry-y-height );
                glTexCoord2f( xr, yb );   glVertex2i( (ticks<<3)+x+width, scry-y-height );
                glTexCoord2f( xr, yt );   glVertex2i( (ticks<<3)+x+width, scry-y );
                glTexCoord2f( xl, yt );   glVertex2i( (ticks<<3)+x,       scry-y );
            glEnd();
            maxticks-=NUMTICK;
            y+=BARY;
        }


        // Draw full rows of empty ticks
        while(maxticks >= NUMTICK)
        {
            barrect[0].right = BARX;
            glBindTexture( GL_TEXTURE_2D, GLTexture_GetTextureID( &TxBars ) );
            xl=((float)barrect[0].left)/128;
            xr=((float)barrect[0].right)/128;
            yt=((float)barrect[0].top)/128;
            yb=((float)barrect[0].bottom)/128;
            width=barrect[0].right-barrect[0].left; height=barrect[0].bottom-barrect[0].top;
            glBegin( GL_QUADS );
                glTexCoord2f( xl, yb );   glVertex2i( x,       scry-y-height );
                glTexCoord2f( xr, yb );   glVertex2i( x+width, scry-y-height );
                glTexCoord2f( xr, yt );   glVertex2i( x+width, scry-y );
                glTexCoord2f( xl, yt );   glVertex2i( x,       scry-y );
            glEnd();
            y+=BARY;
            maxticks-=NUMTICK;
        }


        // Draw the last of the empty ones
        if(maxticks > 0)
        {
            barrect[0].right = (maxticks<<3)+TABX;
            glBindTexture( GL_TEXTURE_2D, GLTexture_GetTextureID( &TxBars ) );
            xl=((float)barrect[0].left)/128;
            xr=((float)barrect[0].right)/128;
            yt=((float)barrect[0].top)/128;
            yb=((float)barrect[0].bottom)/128;
            width=barrect[0].right-barrect[0].left; height=barrect[0].bottom-barrect[0].top;
            glBegin( GL_QUADS );
                glTexCoord2f( xl, yb );   glVertex2i( x,       scry-y-height );
                glTexCoord2f( xr, yb );   glVertex2i( x+width, scry-y-height );
                glTexCoord2f( xr, yt );   glVertex2i( x+width, scry-y );
                glTexCoord2f( xl, yt );   glVertex2i( x,       scry-y );
            glEnd();
            y+=BARY;
        }
    }

    return y;
}

//--------------------------------------------------------------------------------------------
void BeginText(void)
{
    EnableTexturing();		// Enable texture mapping
    glBindTexture( GL_TEXTURE_2D, GLTexture_GetTextureID( &TxFont ) );
    glAlphaFunc(GL_GREATER,0);
    glEnable(GL_ALPHA_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
}

//--------------------------------------------------------------------------------------------
void EndText()
{
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
}

//--------------------------------------------------------------------------------------------
void draw_string(char *szText, int x, int y)
{
    // ZZ> This function spits a line of null terminated text onto the backbuffer
    unsigned char cTmp = szText[0];
    int cnt = 1;

    BeginText();

    while(cTmp != 0)
    {
        // Convert ASCII to our own little font
        if(cTmp == '~')
        {
            // Use squiggle for tab
            x = x & (~TABAND);
            x+=TABAND+1;
        }
		else if(cTmp == '\n')
		{
			break;
		}
		else
        {
            // Normal letter
            cTmp = asciitofont[cTmp];
            draw_one_font(cTmp, x, y);
            x+=fontxspacing[cTmp];
        }
        cTmp=szText[cnt];
        cnt++;
    }
    EndText();
}

//--------------------------------------------------------------------------------------------
int length_of_word(char *szText)
{
    // ZZ> This function returns the number of pixels the
    //     next word will take on screen in the x direction
    
    // Count all preceeding spaces
    int x = 0;
    int cnt = 0;
    unsigned char cTmp = szText[cnt];

    while(cTmp == ' ' || cTmp == '~' || cTmp == '\n')
    {
        if(cTmp == ' ')
        {
            x+=fontxspacing[asciitofont[cTmp]];
        }
        else if (cTmp == '~')
        {
            x+=TABAND+1;
        }
        cnt++;
        cTmp = szText[cnt];
    }


    while(cTmp != ' ' && cTmp != '~' && cTmp != '\n' && cTmp != 0)
    {
        x += fontxspacing[asciitofont[cTmp]];
        cnt++;
        cTmp = szText[cnt];
    }
    return x;
}

//--------------------------------------------------------------------------------------------
int draw_wrap_string(char *szText, int x, int y, int maxx)
{
    // ZZ> This function spits a line of null terminated text onto the backbuffer,
    //     wrapping over the right side and returning the new y value
	int sttx = x;
	unsigned char cTmp = szText[0];
	int newy = y+fontyspacing;
	unsigned char newword = btrue;
	int cnt = 1;

	BeginText();

	maxx = maxx+sttx;

	while(cTmp != 0)
    {
        // Check each new word for wrapping
        if(newword)
        {
			int endx = x + length_of_word(szText + cnt - 1);
            
            newword = bfalse;
            if(endx > maxx)
            {
                // Wrap the end and cut off spaces and tabs
                x = sttx+fontyspacing;
                y+=fontyspacing;
                newy+=fontyspacing;
                while(cTmp == ' ' || cTmp == '~')
                {
                    cTmp=szText[cnt];
                    cnt++;
                }
            }
        }
        else
        {
            if(cTmp == '~')
            {
                // Use squiggle for tab
                x = x & (~TABAND);
                x+=TABAND+1;
            }
			else if(cTmp == '\n')
			{
				x = sttx;
				y+=fontyspacing;
				newy+=fontyspacing;
			}
            else
            {
                // Normal letter
                cTmp = asciitofont[cTmp];
                draw_one_font(cTmp, x, y);
                x+=fontxspacing[cTmp];
            }
            cTmp=szText[cnt];
            if(cTmp == '~' || cTmp == ' ')
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
int draw_status(unsigned short character, int x, int y)
{
    // ZZ> This function shows a character's icon, status and inventory
    //     The x,y coordinates are the top left point of the image to draw
    unsigned short item;
    char cTmp;
    char *readtext;

    int life = chrlife[character]>>8;
    int lifemax = chrlifemax[character]>>8;
    int mana = chrmana[character]>>8;
    int manamax = chrmanamax[character]>>8;
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
    if(chrnameknown[character])
      readtext = chrname[character];
    else
      readtext = capclassname[chrmodel[character]];

    for (cnt = 0; cnt < 6; cnt++)
    {
        cTmp = readtext[cnt];
        if(cTmp == ' ' || cTmp == 0)
        {
            generictext[cnt] = 0;
            break;
        }
        else
            generictext[cnt] = cTmp;
    }
    generictext[6] = 0;
    draw_string(generictext, x+8, y); y+=fontyspacing;


    // Write the character's money
    sprintf(generictext, "$%4d", chrmoney[character]);
    draw_string(generictext, x+8, y); y+=fontyspacing+8;


    // Draw the icons
    draw_one_icon(madskintoicon[chrtexture[character]], x+40, y, chrsparkle[character]);
    item = chrholdingwhich[character][0];
    if(item!=MAXCHR)
    {
        if(chricon[item])
        {
            draw_one_icon(madskintoicon[chrtexture[item]], x+8, y, chrsparkle[item]);
            if(chrammomax[item]!=0 && chrammoknown[item])
            {
                if(capisstackable[chrmodel[item]]==bfalse || chrammo[item]>1)
                {
                    // Show amount of ammo left
                    sprintf(generictext, "%2d", chrammo[item]);
                    draw_string(generictext, x+8, y-8);
                }
            }
        }
        else
            draw_one_icon(bookicon+(chrmoney[item]&3), x+8, y, chrsparkle[item]);
    }
    else
        draw_one_icon(nullicon, x+8, y, NOSPARKLE);

    item = chrholdingwhich[character][1];
    if(item!=MAXCHR)
    {
        if(chricon[item])
        {
            draw_one_icon(madskintoicon[chrtexture[item]], x+72, y, chrsparkle[item]);
            if(chrammomax[item]!=0 && chrammoknown[item])
            {
                if(capisstackable[chrmodel[item]]==bfalse || chrammo[item]>1)
                {
                    // Show amount of ammo left
                    sprintf(generictext, "%2d", chrammo[item]);
                    draw_string(generictext, x+72, y-8);
                }
            }
        }
        else
            draw_one_icon(bookicon+(chrmoney[item]&3), x+72, y, chrsparkle[item]);
    }
    else
        draw_one_icon(nullicon, x+72, y, NOSPARKLE);

    y+=32;

    // Draw the bars
    if(chralive[character])
        y = draw_one_bar(chrlifecolor[character], x, y, life, lifemax);
    else
        y = draw_one_bar(0, x, y, 0, lifemax);  // Draw a black bar

    y = draw_one_bar(chrmanacolor[character], x, y, mana, manamax);
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
    if(staton)
    {
        for (cnt = 0; cnt < numstat && y < scry; cnt++)
            y = draw_status(statlist[cnt], scrx-BARX, y);
    }

    // Map display
    if(mapon)
    {
        draw_map(0, scry-MAPSIZE);
        
		for (cnt = 0; cnt < numblip; cnt++)
            draw_blip(blipc[cnt], blipx[cnt], blipy[cnt]+scry-MAPSIZE);
        
		if(youarehereon && (wldframe&8))
        {
            for (cnt = 0; cnt < MAXPLAYER; cnt++)
            {
                if(plavalid[cnt] && pladevice[cnt] != INPUTNONE)
                {
                    tnc = plaindex[cnt];
                    if(chralive[tnc])
                        draw_blip(0, chrxpos[tnc]*MAPSIZE/meshedgex, (chrypos[tnc]*MAPSIZE/meshedgey)+scry-MAPSIZE);
                }
            }
        }
    }


    // FPS text
    y = 0;
    if(outofsync)
    {
        sprintf(text, "OUT OF SYNC, TRY RTS...");
        draw_string(text, 0, y);  y+=fontyspacing;
    }

    if(parseerror)
    {
        sprintf(text, "SCRIPT ERROR ( SEE PARSEERR.TXT )");
        //draw_string(text, 0, y);       We want a stable release, outcomment this so  
		y+=fontyspacing;
    }
    
	if(fpson)
	{
        draw_string(szfpstext, 0, y);  
		y+=fontyspacing;
    }


    if(SDLKEYDOWN(SDLK_F1))
    {
        // In-Game help
        sprintf(text, "!!!MOUSE HELP!!!");
        draw_string(text, 0, y);  y+=fontyspacing;
        if(rtscontrol)
        {
            if(allselect)
            {
                sprintf(text, "  Left Click to order units");
                draw_string(text, 0, y);  y+=fontyspacing;
            }
            else
            {
                sprintf(text, "  Left Drag to select units");
                draw_string(text, 0, y);  y+=fontyspacing;
                sprintf(text, "  Left Click to order them");
                draw_string(text, 0, y);  y+=fontyspacing;
            }
        }
        else
        {
            sprintf(text, "  Edit CONTROLS.TXT to change");
            draw_string(text, 0, y);  y+=fontyspacing;
            sprintf(text, "  Left Click to use an item");
            draw_string(text, 0, y);  y+=fontyspacing;
            sprintf(text, "  Left and Right Click to grab");
            draw_string(text, 0, y);  y+=fontyspacing;
            sprintf(text, "  Middle Click to jump");
            draw_string(text, 0, y);  y+=fontyspacing;
            sprintf(text, "  A and S keys do stuff");
            draw_string(text, 0, y);  y+=fontyspacing;
        }
        sprintf(text, "  Right Drag to move camera");
        draw_string(text, 0, y);  y+=fontyspacing;
    }
    if(SDLKEYDOWN(SDLK_F2))
    {
        // In-Game help
        sprintf(text, "!!!JOYSTICK HELP!!!");
        draw_string(text, 0, y);  y+=fontyspacing;
        if(rtscontrol)
        {
            sprintf(text, "  Joystick not available");
            draw_string(text, 0, y);  y+=fontyspacing;
        }
        else
        {
            sprintf(text, "  Edit CONTROLS.TXT to change");
            draw_string(text, 0, y);  y+=fontyspacing;
            sprintf(text, "  Hit the buttons");
            draw_string(text, 0, y);  y+=fontyspacing;
            sprintf(text, "  You'll figure it out");
            draw_string(text, 0, y);  y+=fontyspacing;
        }
    }
    if(SDLKEYDOWN(SDLK_F3))
    {
        // In-Game help
        sprintf(text, "!!!KEYBOARD HELP!!!");
        draw_string(text, 0, y);  y+=fontyspacing;
        if(rtscontrol)
        {
            sprintf(text, "  Keyboard not available");
            draw_string(text, 0, y);  y+=fontyspacing;
        }
        else
        {
            sprintf(text, "  Edit CONTROLS.TXT to change");
            draw_string(text, 0, y);  y+=fontyspacing;
            sprintf(text, "  TGB control one hand");
            draw_string(text, 0, y);  y+=fontyspacing;
            sprintf(text, "  YHN control the other");
            draw_string(text, 0, y);  y+=fontyspacing;
            sprintf(text, "  Keypad to move and jump");
            draw_string(text, 0, y);  y+=fontyspacing;
            sprintf(text, "  Number keys for stats");
            draw_string(text, 0, y);  y+=fontyspacing;
        }
    }
    if(SDLKEYDOWN(SDLK_F5))
    {
        // Debug information
        sprintf(text, "!!!DEBUG MODE-5!!!");
        draw_string(text, 0, y);  y+=fontyspacing;
        sprintf(text, "  CAM %f %f", camx, camy);
        draw_string(text, 0, y);  y+=fontyspacing;
        tnc = plaindex[0];
        sprintf(text, "  PLA0DEF %d %d %d %d %d %d %d %d", 
                            chrdamagemodifier[tnc][0]&3,
                            chrdamagemodifier[tnc][1]&3,
                            chrdamagemodifier[tnc][2]&3,
                            chrdamagemodifier[tnc][3]&3,
                            chrdamagemodifier[tnc][4]&3,
                            chrdamagemodifier[tnc][5]&3,
                            chrdamagemodifier[tnc][6]&3,
                            chrdamagemodifier[tnc][7]&3);
        draw_string(text, 0, y);  y+=fontyspacing;
        tnc = plaindex[0];
        sprintf(text, "  PLA0 %5.1f %5.1f", chrxpos[tnc]/128.0, chrypos[tnc]/128.0);
        draw_string(text, 0, y);  y+=fontyspacing;
        tnc = plaindex[1];
        sprintf(text, "  PLA1 %5.1f %5.1f", chrxpos[tnc]/128.0, chrypos[tnc]/128.0);
        draw_string(text, 0, y);  y+=fontyspacing;
    }
    if(SDLKEYDOWN(SDLK_F6))
    {
        // More debug information
        sprintf(text, "!!!DEBUG MODE-6!!!");
        draw_string(text, 0, y);  y+=fontyspacing;
        sprintf(text, "  FREEPRT %d", numfreeprt);
        draw_string(text, 0, y);  y+=fontyspacing;
        sprintf(text, "  FREECHR %d",  numfreechr);
        draw_string(text, 0, y);  y+=fontyspacing;
        sprintf(text, "  MACHINE %d", localmachine);
        draw_string(text, 0, y);  y+=fontyspacing;
        sprintf(text, "  EXPORT %d", exportvalid);
        draw_string(text, 0, y);  y+=fontyspacing;
        sprintf(text, "  FOGAFF %d", fogaffectswater);
        draw_string(text, 0, y);  y+=fontyspacing;
        sprintf(text, "  PASS %d/%d", numshoppassage, numpassage);
        draw_string(text, 0, y);  y+=fontyspacing;
        sprintf(text, "  NETPLAYERS %d", numplayer);
        draw_string(text, 0, y);  y+=fontyspacing;
        sprintf(text, "  DAMAGEPART %d", damagetileparttype);
        draw_string(text, 0, y);  y+=fontyspacing;
    }
    if(SDLKEYDOWN(SDLK_F7))
    {
        // White debug mode
        sprintf(text, "!!!DEBUG MODE-7!!!");
        draw_string(text, 0, y);  y+=fontyspacing;
        sprintf(text, "CAM %f %f %f %f", (mView)_CNV(0,0), (mView)_CNV(1,0), (mView)_CNV(2,0), (mView)_CNV(3,0));
        draw_string(text, 0, y);  y+=fontyspacing;
        sprintf(text, "CAM %f %f %f %f", (mView)_CNV(0,1), (mView)_CNV(1,1), (mView)_CNV(2,1), (mView)_CNV(3,1));
        draw_string(text, 0, y);  y+=fontyspacing;
        sprintf(text, "CAM %f %f %f %f", (mView)_CNV(0,2), (mView)_CNV(1,2), (mView)_CNV(2,2), (mView)_CNV(3,2));
        draw_string(text, 0, y);  y+=fontyspacing;
        sprintf(text, "CAM %f %f %f %f", (mView)_CNV(0,3), (mView)_CNV(1,3), (mView)_CNV(2,3), (mView)_CNV(3,3));
        draw_string(text, 0, y);  y+=fontyspacing;
        sprintf(text, "x %f", camcenterx);
        draw_string(text, 0, y);  y+=fontyspacing;
        sprintf(text, "y %f", camcentery);
        draw_string(text, 0, y);  y+=fontyspacing;
        sprintf(text, "turn %d %d", autoturncamera, doturntime);
        draw_string(text, 0, y);  y+=fontyspacing;
    }

	//Draw paused text
	if(gamepaused && !SDLKEYDOWN(SDLK_F11))
	{
		sprintf(text, "GAME PAUSED");
		draw_string(text, -90 + scrx/2, 0 + scry/2);
	}

	//Pressed panic button
	if(SDLKEYDOWN(SDLK_q) && SDLKEYDOWN(SDLK_LCTRL))
	{
		gameactive = bfalse;
		moduleactive = bfalse;
	}

    if(timeron)
    {
        fifties = (timervalue%50)<<1;
        seconds = ((timervalue/50)%60);
        minutes = (timervalue/3000);
        sprintf(text, "=%d:%02d:%02d=", minutes, seconds, fifties);
        draw_string(text, 0, y);  
		y+=fontyspacing;
    }
    if(waitingforplayers)
    {
        sprintf(text, "Waiting for players...");
        draw_string(text, 0, y);  
		y+=fontyspacing;
    }
    if(!rtscontrol)
    {
        if(alllocalpladead || respawnanytime)
        {
            if(respawnvalid)
            {
                sprintf(text, "PRESS SPACE TO RESPAWN");
            }
            else
            {
                sprintf(text, "PRESS ESCAPE TO QUIT");
            }
            draw_string(text, 0, y);  
			y+=fontyspacing;
        }
        else
        {
            if(beatmodule)
            {
                sprintf(text, "VICTORY!  PRESS ESCAPE");
                draw_string(text, 0, y);  
				y+=fontyspacing;
            }
        }
    }


    // Network message input
    if(netmessagemode)
    {
        y = draw_wrap_string(netmessage, 0, y, scrx-wraptolerance);
    }


    // Messages
    if(messageon)
    {
        // Display the messages
        tnc = msgstart;
        
        for (cnt = 0; cnt < maxmessage; cnt++)
        {
            if(msgtime[tnc]>0)
            {
                y = draw_wrap_string(msgtextdisplay[tnc], 0, y, scrx-wraptolerance);
                msgtime[tnc] -= msgtimechange;
            }
            tnc++;
            tnc = tnc % maxmessage;
        }
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
    
	if(zreflect) //DO REFLECTIONS
        draw_scene_zreflection();
    else
        draw_scene_sadreflection();
	
  //Foreground overlay
  if(overlayon)
  {
		render_foreground_overlay(5);  // Texture 5 is watertop.bmp
  }

	End3DMode();
}

//--------------------------------------------------------------------------------------------
void build_select(float tlx, float tly, float brx, float bry, unsigned char team)
{
    // ZZ> This function checks which characters are in the selection rectangle
/*PORT
	D3DLVERTEX v[MAXPRT];
    D3DTLVERTEX vt[MAXPRT];
    int numbertocheck, character, cnt, first, sound;


    // Unselect old ones
    clear_select();


    // Figure out who to check
    numbertocheck = 0;
    cnt = 0;
    while(cnt < MAXCHR)
    {
        if(chrteam[cnt] == team && chron[cnt] && !chrinpack[cnt])
        {
            v[numbertocheck].x = (D3DVALUE) chrxpos[cnt];
            v[numbertocheck].y = (D3DVALUE) chrypos[cnt];
            v[numbertocheck].z = (D3DVALUE) chrzpos[cnt];
            v[numbertocheck].color = cnt;  // Store an index in the color slot...
            v[numbertocheck].dwReserved = 0;
            numbertocheck++;
        }
        cnt++;
    }


    // Figure out where the points go onscreen
    lpD3DDDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &mWorld);
    transform_vertices(numbertocheck, v, vt);


    first = btrue;
    cnt = 0;
    while(cnt < numbertocheck)
    {
        // Only check if in front of camera
        if(vt[cnt].dvRHW > 0)
        {
            // Check the rectangle
            if(vt[cnt].dvSX > tlx && vt[cnt].dvSX < brx)
            {
                if(vt[cnt].dvSY > tly && vt[cnt].dvSY < bry)
                {
                    // Select the character
                    character = v[cnt].color;
                    add_select(character);
                    if(first)
                    {
                        // Play the select speech for the first one picked
                        sound = chrwavespeech[character][SPEECHSELECT];
                        if(sound >= 0 && sound < MAXWAVE)
                        play_sound_pvf(capwaveindex[chrmodel[character]][sound], PANMID, VOLMAX, 11025);
                        first = bfalse;
                    }
                }
            }
        }
        cnt++;
    }
*/
}

//--------------------------------------------------------------------------------------------
unsigned short build_select_target(float tlx, float tly, float brx, float bry, unsigned char team)
{
    // ZZ> This function checks which characters are in the selection rectangle,
    //     and returns the first one found
/*PORT
	D3DLVERTEX v[MAXPRT];
    D3DTLVERTEX vt[MAXPRT];
    int numbertocheck, character, cnt;


    // Figure out who to check
    numbertocheck = 0;
    // Add enemies first
    cnt = 0;
    while(cnt < MAXCHR)
    {
        if(teamhatesteam[team][chrteam[cnt]] && chron[cnt] && !chrinpack[cnt])
        {
            v[numbertocheck].x = (D3DVALUE) chrxpos[cnt];
            v[numbertocheck].y = (D3DVALUE) chrypos[cnt];
            v[numbertocheck].z = (D3DVALUE) chrzpos[cnt];
            v[numbertocheck].color = cnt;  // Store an index in the color slot...
            v[numbertocheck].dwReserved = 0;
            numbertocheck++;
        }
        cnt++;
    }
    // Add allies next
    cnt = 0;
    while(cnt < MAXCHR)
    {
        if(teamhatesteam[team][chrteam[cnt]] == bfalse && chron[cnt] && !chrinpack[cnt])
        {
            v[numbertocheck].x = (D3DVALUE) chrxpos[cnt];
            v[numbertocheck].y = (D3DVALUE) chrypos[cnt];
            v[numbertocheck].z = (D3DVALUE) chrzpos[cnt];
            v[numbertocheck].color = cnt;  // Store an index in the color slot...
            v[numbertocheck].dwReserved = 0;
            numbertocheck++;
        }
        cnt++;
    }


    // Figure out where the points go onscreen
    lpD3DDDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &mWorld);
    transform_vertices(numbertocheck, v, vt);


    cnt = 0;
    while(cnt < numbertocheck)
    {
        // Only check if in front of camera
        if(vt[cnt].dvRHW > 0)
        {
            // Check the rectangle
            if(vt[cnt].dvSX > tlx && vt[cnt].dvSX < brx)
            {
                if(vt[cnt].dvSY > tly && vt[cnt].dvSY < bry)
                {
                    // Select the character
                    character = v[cnt].color;
                    return character;
                }
            }
        }
        cnt++;
    }
    return MAXCHR;
*/
return 0;
}

//--------------------------------------------------------------------------------------------
void move_rtsxy()
{
    // ZZ> This function iteratively transforms the cursor back to world coordinates
/*PORT
	D3DLVERTEX v[1];
    D3DTLVERTEX vt[1];
    int numbertocheck, x, y, fan;
    float sin, cos, trailrate, level;



    // Figure out where the rtsxy is at on the screen
    x = rtsx;
    y = rtsy;
    fan = meshfanstart[y>>7]+(x>>7);
    level = get_level(rtsx, rtsy, fan, bfalse);
    v[0].x = (D3DVALUE) rtsx;
    v[0].y = (D3DVALUE) rtsy;
    v[0].z = level;
    v[0].color = 0;
    v[0].dwReserved = 0;
    numbertocheck = 1;


    // Figure out where the points go onscreen
    lpD3DDDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &mWorld);
    transform_vertices(numbertocheck, v, vt);


    if(vt[0].dvRHW < 0)
    {
        // Move it to camtrackxy if behind the camera
        rtsx = camtrackx;
        rtsy = camtracky;
    }
    else
    {
        // Move it to closer to the onscreen cursor
        trailrate = ABS(cursorx-vt[0].dvSX) + ABS(cursory-vt[0].dvSY);
        trailrate *= rtstrailrate;
        sin = turntosin[camturnleftrightshort>>2]*trailrate;
        cos = turntosin[((camturnleftrightshort>>2)+4096)&16383]*trailrate;
        if(vt[0].dvSX < cursorx)
        {
            rtsx += cos;
            rtsy -= sin;
        }
        else
        {
            rtsx -= cos;
            rtsy += sin;
        }



        if(vt[0].dvSY < cursory)
        {
            rtsx += sin;
            rtsy += cos;
        }
        else
        {
            rtsx -= sin;
            rtsy -= cos;
        }
    }
*/
}

//--------------------------------------------------------------------------------------------
void do_cursor_rts()
{
/*    // This function implements the RTS mouse cursor
    int sttx, stty, endx, endy, target, leader;
    signed short sound;


    if(mousebutton[1] == 0)
    {
        cursorx+=mousex;
        cursory+=mousey;
    }
    if(cursorx < 6)  cursorx = 6;  if (cursorx > scrx-16)  cursorx = scrx-16;
    if(cursory < 8)  cursory = 8;  if (cursory > scry-24)  cursory = scry-24;
    move_rtsxy();
    if(mousebutton[0])
    {
        // Moving the end select point
        pressed = btrue;
        rtsendx = cursorx+5;
        rtsendy = cursory+7;

        // Draw the selection rectangle
        if(allselect == bfalse)
        {
            sttx = rtssttx;  endx = rtsendx;  if(sttx > endx)  {  sttx = rtsendx;  endx = rtssttx; }
            stty = rtsstty;  endy = rtsendy;  if(stty > endy)  {  stty = rtsendy;  endy = rtsstty; }
            draw_trim_box(sttx, stty, endx, endy);
        }
    }
    else
    {
        if(pressed)
        {
            // See if we selected anyone
            if((ABS(rtssttx - rtsendx) + ABS(rtsstty - rtsendy)) > 10 && allselect == bfalse)
            {
                // We drew a box alright
                sttx = rtssttx;  endx = rtsendx;  if(sttx > endx)  {  sttx = rtsendx;  endx = rtssttx; }
                stty = rtsstty;  endy = rtsendy;  if(stty > endy)  {  stty = rtsendy;  endy = rtsstty; }
                build_select(sttx, stty, endx, endy, rtslocalteam);
            }
            else
            {
                // We want to issue an order
                if(numrtsselect > 0)
                {
                    leader = rtsselect[0];
                    sttx = rtssttx-20;  endx = rtssttx+20;
                    stty = rtsstty-20;  endy = rtsstty+20;
                    target = build_select_target(sttx, stty, endx, endy, rtslocalteam);
                    if(target == MAXCHR)
                    {
                        // No target...
                        if(SDLKEYDOWN(SDLK_LSHIFT) || SDLKEYDOWN(SDLK_RSHIFT))
                        {
                            send_rts_order(rtsx, rtsy, RTSTERRAIN, target);
                            sound = chrwavespeech[leader][SPEECHTERRAIN];
                        }
                        else
                        {
                            send_rts_order(rtsx, rtsy, RTSMOVE, target);
                            sound = wldframe&1;  // Move or MoveAlt
                            sound = chrwavespeech[leader][sound];
                        }
                    }
                    else
                    {
                        if(teamhatesteam[rtslocalteam][chrteam[target]])
                        {
                            // Target is an enemy, so issue an attack order
                            send_rts_order(rtsx, rtsy, RTSATTACK, target);
                            sound = chrwavespeech[leader][SPEECHATTACK];
                        }
                        else
                        {
                            // Target is a friend, so issue an assist order
                            send_rts_order(rtsx, rtsy, RTSASSIST, target);
                            sound = chrwavespeech[leader][SPEECHASSIST];
                        }
                    }
                    // Do unit speech at 11025 KHz
                    if(sound >= 0 && sound < MAXWAVE)
                    {
                        play_sound_pvf(capwaveindex[chrmodel[leader]][sound], PANMID, VOLMAX, 11025);
                    }
                }
            }
            pressed = bfalse;
        }


        // Moving the select point
        rtssttx = cursorx+5;
        rtsstty = cursory+7;
        rtsendx = cursorx+5;
        rtsendy = cursory+7;
    }

    // GAC - Don't forget to BeginText() and EndText();
    BeginText();
    draw_one_font(11, cursorx-5, cursory-7);
    EndText ();*/
}

//--------------------------------------------------------------------------------------------
void draw_main()
{
    // ZZ> This function does all the drawing stuff
    //printf("DIAG: Drawing scene nummeshrenderlistref=%d\n",nummeshrenderlistref);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_scene();
    draw_text();
    if(rtscontrol)  
		do_cursor_rts();
    
	flip_pages();
    allframe++;
    fpsframe++;
}

//--------------------------------------------------------------------------------------------
int load_one_title_image(int titleimage, char *szLoadName)
{
	// ZZ> This function loads a title in the specified image slot, forcing it into
	//     system memory.  Returns btrue if it worked
	GLTexture_Load( &TxTitleImage[titleimage], szLoadName );
	if ( GLTexture_GetTextureID != 0 )
	{
		/* PORT
		// Set the transparent color
		DDSetColorKey(lpDDSTitleImage[titleimage], 0); */
		return btrue;
	}
	return bfalse;
	
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
    strcpy(searchname, "modules/*.mod");

    // Log a directory list
    filesave = fopen("modules.txt", "w");
	if ( filesave != NULL )
	{
		fprintf(filesave, "This file logs all of the modules found\n");
		fprintf(filesave, "** Denotes an invalid module (Or unlocked)\n\n");
	}

    // Search for .mod directories
    FileName = fs_findFirstFile("modules", "mod");
    globalnummodule = 0;
    while(FileName && globalnummodule < MAXMODULE)
    {
        sprintf(modloadname[globalnummodule], "%s", FileName);
        sprintf(loadname, "modules/%s/gamedat/menu.txt", FileName);
        if(get_module_data(globalnummodule, loadname))
        {
            sprintf(loadname, "modules/%s/gamedat/title.bmp", FileName);
            if(load_one_title_image(globalnummodule, loadname))
            {
                fprintf(filesave, "%02d.  %s\n", globalnummodule, modlongname[globalnummodule]);
                globalnummodule++;
            }
            else
            {
                fprintf(filesave, "**.  %s\n", FileName);
            }
        }
        else
        {
            fprintf(filesave, "**.  %s\n", FileName);
        }
        FileName = fs_findNextFile();
    }
    fs_findClose();
    if ( filesave != NULL ) fclose(filesave);
}

//--------------------------------------------------------------------------------------------
void load_blip_bitmap()
{
    //This function loads the blip bitmaps
	int cnt;
	
	GLTexture_Load( &TxBlip, "basicdat/blip.bmp" );

	// Set up the rectangles
	for (cnt = 0; cnt < NUMBAR; cnt++)
	{
		bliprect[cnt].left   = cnt * BLIPSIZE;
		bliprect[cnt].right  = (cnt * BLIPSIZE) + BLIPSIZE;
		bliprect[cnt].top    = 0;
		bliprect[cnt].bottom = BLIPSIZE;
	}
}

//--------------------------------------------------------------------------------------------
void draw_trimx(int x, int y, int length)
{
	// ZZ> This function draws a horizontal trim bar
	GLfloat	txWidth, txHeight, txLength;
	
	if ( GLTexture_GetTextureID( &TxTrim ) != 0 )//if( lpDDSTrimX )
	{
		/*while( length > 0 )
        {
			trimrect.right = length;
			if(length > TRIMX)  trimrect.right = TRIMX;
			trimrect.bottom = 4;
			lpDDSBack->BltFast(x, y, lpDDSTrimX, &trimrect, DDBLTFAST_NOCOLORKEY);
			length-=TRIMX;
			x+=TRIMX;
		}*/
		
		/* Calculate the texture width, height, and length */
		txWidth = ( GLfloat )( GLTexture_GetImageWidth( &TxTrim )/GLTexture_GetDimensions( &TxTrim ) );
		txHeight = ( GLfloat )( GLTexture_GetImageHeight( &TxTrim )/GLTexture_GetDimensions( &TxTrim ) );
		txLength = ( GLfloat )( length/GLTexture_GetImageWidth( &TxTrim ) );
		
		
		/* Bind our texture */
		glBindTexture( GL_TEXTURE_2D, GLTexture_GetTextureID( &TxTrim ) );
		
		/* Draw the trim */
		glColor4f( 1, 1, 1, 1 );
		glBegin( GL_QUADS );
			glTexCoord2f( 0, 1 );	glVertex2f( x, scry - y );
			glTexCoord2f( 0, 1 - txHeight );	glVertex2f( x, scry - y - GLTexture_GetImageHeight( &TxTrim ) );
			glTexCoord2f( txWidth*txLength, 1 - txHeight );	glVertex2f( x + length, scry - y - GLTexture_GetImageHeight( &TxTrim ) );
			glTexCoord2f( txWidth*txLength, 1 );	glVertex2f( x + length, scry - y );
		glEnd();
	}
}

//--------------------------------------------------------------------------------------------
void draw_trimy(int x, int y, int length)
{
	// ZZ> This function draws a vertical trim bar
	GLfloat	txWidth, txHeight, txLength;
	
	if ( GLTexture_GetTextureID( &TxTrim ) != 0 )//if(lpDDSTrimY)
	{
		/*while(length > 0)
		{
			trimrect.bottom = length;
			if(length > TRIMY)  trimrect.bottom = TRIMY;
			trimrect.right = 4;
			lpDDSBack->BltFast(x, y, lpDDSTrimY, &trimrect, DDBLTFAST_NOCOLORKEY);
			length-=TRIMY;
			y+=TRIMY;
		}*/
		
		/* Calculate the texture width, height, and length */
		txWidth = ( GLfloat )( GLTexture_GetImageWidth( &TxTrim )/GLTexture_GetDimensions( &TxTrim ) );
		txHeight = ( GLfloat )( GLTexture_GetImageHeight( &TxTrim )/GLTexture_GetDimensions( &TxTrim ) );
		txLength = ( GLfloat )( length/GLTexture_GetImageHeight( &TxTrim ) );
		
		/* Bind our texture */
		glBindTexture( GL_TEXTURE_2D, GLTexture_GetTextureID( &TxTrim ) );
		
		/* Draw the trim */
		glColor4f( 1, 1, 1, 1 );
		glBegin( GL_QUADS );
			glTexCoord2f( 0, 1 );	glVertex2f( x, scry - y );
			glTexCoord2f( 0, 1 - txHeight*txLength );	glVertex2f( x, scry - y - length );
			glTexCoord2f( txWidth, 1 - txHeight*txLength );	glVertex2f( x + GLTexture_GetImageWidth( &TxTrim ), scry - y - length );
			glTexCoord2f( txWidth, 1 );	glVertex2f( x + GLTexture_GetImageWidth( &TxTrim ), scry - y );			
		glEnd();
	}
}

//--------------------------------------------------------------------------------------------
void draw_trim_box(int left, int top, int right, int bottom)
{
    // ZZ> This function draws a trim rectangle
    float l,t,r,b;
    l=((float)left)/scrx;
    r=((float)right)/scrx;
    t=((float)top)/scry;
    b=((float)bottom)/scry;

	Begin2DMode();
    
	draw_trimx(left, top, right-left);
	draw_trimx(left, bottom-4, right-left);
	draw_trimy(left, top, bottom-top);
	draw_trimy(right-4, top, bottom-top);

	End2DMode();
}

//--------------------------------------------------------------------------------------------
void draw_trim_box_opening(int left, int top, int right, int bottom, float amount)
{
    // ZZ> This function draws a trim rectangle, scaled around its center
    int x = (left + right)>>1;
    int y = (top + bottom)>>1;
    left   = (x * (1.0-amount)) + (left * amount);
    right  = (x * (1.0-amount)) + (right * amount);
    top    = (y * (1.0-amount)) + (top * amount);
    bottom = (y * (1.0-amount)) + (bottom * amount);
    draw_trim_box(left, top, right, bottom);
}

//--------------------------------------------------------------------------------------------
void load_menu()
{
    // ZZ> This function loads all of the menu data...  Images are loaded into system
    // memory

    load_font( "basicdat/font.bmp", "basicdat/font.txt", btrue );
    load_all_menu_images();
}

//--------------------------------------------------------------------------------------------
void draw_titleimage(int image, int x, int y)
{
	// ZZ> This function draws a title image on the backbuffer
	GLfloat	txWidth, txHeight;
	
	if ( GLTexture_GetTextureID( &TxTitleImage[image] ) != 0 )//if(lpDDSTitleImage[image])
	{
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		Begin2DMode();
		glNormal3f( 0, 0, 1 );	// glNormal3f( 0, 1, 0 );
		
		/* Calculate the texture width & height */
		txWidth = ( GLfloat )( GLTexture_GetImageWidth( &TxTitleImage[image] )/GLTexture_GetDimensions( &TxTitleImage[image] ) );
		txHeight = ( GLfloat )( GLTexture_GetImageHeight( &TxTitleImage[image] )/GLTexture_GetDimensions( &TxTitleImage[image] ) );

		/* Bind the texture */
		glBindTexture( GL_TEXTURE_2D, GLTexture_GetTextureID( &TxTitleImage[image] ) );
		
		/* Draw the quad */
		glBegin( GL_QUADS );
			glTexCoord2f( 0, 1 );	glVertex2f( x, scry - y - GLTexture_GetImageHeight( &TxTitleImage[image] ) );
			glTexCoord2f( txWidth, 1 );	glVertex2f( x + GLTexture_GetImageWidth( &TxTitleImage[image] ), scry - y - GLTexture_GetImageHeight( &TxTitleImage[image] ) );
			glTexCoord2f( txWidth, 1-txHeight );	glVertex2f( x + GLTexture_GetImageWidth( &TxTitleImage[image] ), scry - y );
			glTexCoord2f( 0, 1-txHeight );	glVertex2f( x, scry - y );
		glEnd();
		
		End2DMode();
	}
}

//--------------------------------------------------------------------------------------------
void do_cursor()
{
    // This function implements a mouse cursor
    read_input(NULL);
    cursorx=mousex;  if(cursorx < 6)  cursorx = 6;  if (cursorx > scrx-16)  cursorx = scrx-16;
    cursory=mousey;  if(cursory < 8)  cursory = 8;  if (cursory > scry-24)  cursory = scry-24;
    clicked = bfalse;
    if(mousebutton[0] && pressed == bfalse)
    {
        clicked = btrue;
    }
    pressed = mousebutton[0];
    BeginText();  // Needed to setup text mode
    //draw_one_font(11, cursorx-5, cursory-7);
    draw_one_font(95, cursorx-5, cursory-7);
    EndText();    // Needed when done with text mode
}

/********************> Reshape3D() <*****/
void Reshape3D( int w, int h )
{
	glViewport( 0, 0, w, h );
}

int glinit(int argc, char **argv)
{
	GLfloat intensity[] = {1.0,1.0,1.0,1.0};

	//DEBUG!
	//GLfloat fogColor[4]= {0.5f, 0.5f, 0.5f, 1.0f};		//Fog color
	//DEBUG END

	/* Depth testing stuff */
	glClearDepth( 1.0 );
	glDepthFunc( GL_LESS );
	glEnable( GL_DEPTH_TEST );

	//Load the current graphical settings
	load_graphics();

	//DEBUG FOG
/*	glClearColor(0.5f,0.5f,0.5f,1.0f);						// We'll Clear To The Color Of The Fog ( Modified )
	glFogi(GL_FOG_MODE, GL_EXP);							// Fog Mode
	glFogfv(GL_FOG_COLOR, fogColor);						// Set Fog Color
	glFogf(GL_FOG_DENSITY, 0.35f);							// How Dense Will The Fog Be
	glHint(GL_FOG_HINT, GL_DONT_CARE);							// Fog Hint Value
	glFogf(GL_FOG_START, 1.0f);								// Fog Start Depth
	glFogf(GL_FOG_END, 5.0f);								// Fog End Depth
//	glFogf(GL_FOG_START, 258.0f);								// Fog Start Depth
//	glFogf(GL_FOG_END, 262.0f);								// Fog End Depth
	glEnable(GL_FOG);										// Enables GL_FOG
	//DEBUG FOG*/

	//fill mode
    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK,  GL_FILL);
	
	/* Enable a single OpenGL light. */
	//glLightfv(GL_LIGHT0, GL_SPECULAR, light_diffuse);
	//glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	//glEnable(GL_LIGHT0);
	glDisable(GL_LIGHTING);
	//glLightModelfv(GL_LIGHT_MODEL_AMBIENT,intensity);
	
	/* Backface culling */
	glEnable (GL_CULL_FACE);   // This seems implied - DDOI 
	glCullFace( GL_BACK );
	
	glEnable(GL_COLOR_MATERIAL);	// Need this for color + lighting
	EnableTexturing();		// Enable texture mapping
	

	return 1;
}

void sdlinit(int argc, char **argv)
{
    int 		colordepth;
	SDL_Surface *theSurface;
	
	if ( SDL_Init( SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_JOYSTICK) < 0 )
	{
		fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}
	
	atexit(SDL_Quit);

#ifdef __unix__
	/* GLX doesn't differentiate between 24 and 32 bpp, asking for 32 bpp
	   will cause SDL_SetVideoMode to fail with:
	   Unable to set video mode: Couldn't find matching GLX visual */
	if (scrd == 32)
		scrd = 24;
#endif

	colordepth=scrd/3;

	/* Setup the cute windows manager icon */
	theSurface =SDL_LoadBMP("basicdat/icon.bmp");
	if ( theSurface == NULL )
	{
		fprintf(stderr, "Unable to load icon\n");
		exit(1);
	}
 	SDL_WM_SetIcon(theSurface, NULL);
	
	/* Set the OpenGL Attributes */
#ifndef __unix__
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, colordepth );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, colordepth  );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE,  colordepth );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, scrd );
#endif
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	
	displaySurface = SDL_SetVideoMode( scrx, scry, scrd, SDL_DOUBLEBUF | SDL_OPENGL | (fullscreen ? SDL_FULLSCREEN : 0));
	if ( displaySurface == NULL )
	{
		fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
		exit(1);
	}
	
	// Set the window name
	SDL_WM_SetCaption( "Egoboo", "Egoboo" );

	if ( gGrabMouse )
	{
		SDL_WM_GrabInput ( SDL_GRAB_ON );
	}

	if ( gHideMouse )
	{
		SDL_ShowCursor(0);	// Hide the mouse cursor
	}
	
	if(SDL_NumJoysticks() > 0) {
		sdljoya = SDL_JoystickOpen(0);
        joyaon = btrue;
	}
}

void load_graphics()
{
	//This function loads all the graphics based on the game settings
	GLenum quality;

	//Check if the computer graphic driver supports anisotropic filtering
	if(texturefilter == 4)
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
		if (!strstr((char*)glGetString(GL_EXTENSIONS), "GL_EXT_texture_filter_anisotropic"))
		{
			log_warning("Your graphics driver does not support anisotropic filtering.\n");
			texturefilter = 3; //Set filtering to trillienar instead
		}
	}

    //Enable prespective correction?
	if(perspective) quality = GL_NICEST;
	else quality = GL_FASTEST;
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, quality);

	//Enable dithering? (This actually reduces quality but increases preformance)
	if(dither) glEnable(GL_DITHER);
	else glDisable(GL_DITHER);

	//Enable gourad shading? (Important!)
	if(shading) quality = GL_SMOOTH;
	else quality = GL_FLAT;
	glShadeModel(quality);

	//Enable antialiasing?
	if(antialiasing)
	{
		glEnable(GL_POINT_SMOOTH);
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT,    GL_NICEST);
		glHint(GL_POINT_SMOOTH_HINT,   GL_NICEST);
	//	glEnable(GL_POLYGON_SMOOTH);					//Caused some glitches
	//	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	}
	else
	{
		glDisable(GL_POINT_SMOOTH);
		glDisable(GL_LINE_SMOOTH);
//		glDisable(GL_POLYGON_SMOOTH);
	}    
}
