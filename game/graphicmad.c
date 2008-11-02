/* Egoboo - graphicmad.c
 * Character model drawing code.
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
#include "Md2.h"
#include "id_md2.h"
#include <SDL_opengl.h>

/* Storage for blended vertices */
static GLfloat md2_blendedVertices[MD2_MAX_VERTICES][3];
static GLfloat md2_blendedNormals[MD2_MAX_VERTICES][3];

/* blend_md2_vertices
 * Blends the vertices and normals between 2 frames of a md2 model for animation.
 *
 * NOTE: Only meant to be called from draw_textured_md2, which does the necessary
 * checks to make sure that the inputs are valid.  So this function itself assumes
 * that they are valid.  User beware!
 */
static void blend_md2_vertices(const Md2Model *model, int from_, int to_, float lerp)
{
	struct Md2Frame *from, *to;
	int numVertices, i;

	from = &model->frames[from_];
	to = &model->frames[to_];
	numVertices = model->numVertices;

	if(lerp <= 0)
	{
		// copy the vertices in frame 'from' over
		for(i = 0;i < numVertices;i++)
		{
			md2_blendedVertices[i][0] = from->vertices[i].x;
			md2_blendedVertices[i][1] = from->vertices[i].y;
			md2_blendedVertices[i][2] = from->vertices[i].z;

			md2_blendedNormals[i][0] = kMd2Normals[from->vertices[i].normal][0];
			md2_blendedNormals[i][1] = kMd2Normals[from->vertices[i].normal][1];
			md2_blendedNormals[i][2] = kMd2Normals[from->vertices[i].normal][2];
		}
	} else if(lerp >= 1.0f)
	{
		// copy the vertices in frame 'to'
		for(i = 0;i < numVertices;i++)
		{
			md2_blendedVertices[i][0] = to->vertices[i].x;
			md2_blendedVertices[i][1] = to->vertices[i].y;
			md2_blendedVertices[i][2] = to->vertices[i].z;

			md2_blendedNormals[i][0] = kMd2Normals[to->vertices[i].normal][0];
			md2_blendedNormals[i][1] = kMd2Normals[to->vertices[i].normal][1];
			md2_blendedNormals[i][2] = kMd2Normals[to->vertices[i].normal][2];
		}
	} else
	{
		// mix the vertices
		for(i = 0;i < numVertices;i++)
		{
			md2_blendedVertices[i][0] = from->vertices[i].x +
				(to->vertices[i].x - from->vertices[i].x) * lerp;
			md2_blendedVertices[i][1] = from->vertices[i].y +
				(to->vertices[i].y - from->vertices[i].y) * lerp;
			md2_blendedVertices[i][2] = from->vertices[i].z +
				(to->vertices[i].z - from->vertices[i].z) * lerp;

			md2_blendedNormals[i][0] = kMd2Normals[from->vertices[i].normal][0] +
				(kMd2Normals[to->vertices[i].normal][0] - kMd2Normals[from->vertices[i].normal][0]) * lerp;
			md2_blendedNormals[i][0] = kMd2Normals[from->vertices[i].normal][1] +
				(kMd2Normals[to->vertices[i].normal][1] - kMd2Normals[from->vertices[i].normal][1]) * lerp;
			md2_blendedNormals[i][0] = kMd2Normals[from->vertices[i].normal][2] +
				(kMd2Normals[to->vertices[i].normal][2] - kMd2Normals[from->vertices[i].normal][2]) * lerp;
		}
	}
}

/* draw_textured_md2
 * Draws a Md2Model in the new format
 */
void draw_textured_md2(const Md2Model *model, int from_, int to_, float lerp)
{
	int i, numTriangles;
	const struct Md2TexCoord *tc;
	const struct Md2Triangle *triangles;
	const struct Md2Triangle *tri;

	if (model == NULL) return;
	if (from_ < 0 || from_ >= model->numFrames) return;
	if (to_ < 0 || to_ >= model->numFrames) return;

	blend_md2_vertices(model, from_, to_, lerp);

	numTriangles = model->numTriangles;
	tc = model->texCoords;
	triangles = model->triangles;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glVertexPointer(3, GL_FLOAT, 0, md2_blendedVertices);
	glNormalPointer(GL_FLOAT, 0, md2_blendedNormals);

	glBegin(GL_TRIANGLES);
	{
		for(i = 0;i < numTriangles;i++)
		{
			tri = &triangles[i];

			glTexCoord2fv((const GLfloat*)&(tc[tri->texCoordIndices[0]]));
			glArrayElement(tri->vertexIndices[0]);
			
			glTexCoord2fv((const GLfloat*)&(tc[tri->texCoordIndices[1]]));
			glArrayElement(tri->vertexIndices[1]);

			glTexCoord2fv((const GLfloat*)&(tc[tri->texCoordIndices[2]]));
			glArrayElement(tri->vertexIndices[2]);
		}
	}
	glEnd();

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}

//--------------------------------------------------------------------------------------------
void render_enviromad(unsigned short character, unsigned char trans)
{
    // ZZ> This function draws an environment mapped model
	//D3DLVERTEX v[MAXVERTICES];
    //D3DTLVERTEX vt[MAXVERTICES];
    //D3DTLVERTEX vtlist[MAXCOMMANDSIZE];
	GLVERTEX v[MAXVERTICES];
    unsigned short cnt, tnc, entry;
    unsigned short vertex;
    signed int temp;
    //float z;
    //unsigned char red, grn, blu;
    unsigned char ambi;
    //DWORD fogspec;
    GLMATRIX tempWorld = mWorld;

    unsigned short model = chrmodel[character];
    unsigned short texture = chrtexture[character];
    unsigned short frame = chrframe[character];
    unsigned short lastframe = chrlastframe[character];
    unsigned short framestt = madframestart[chrmodel[character]];
    unsigned char lip = chrlip[character]>>6;
    unsigned char lightrotation = (chrturnleftright[character]+chrlightturnleftright[character])>>8;
    Uint32 alpha = trans;
    unsigned char lightlevel = chrlightlevel[character]>>4;
    float uoffset = textureoffset[chruoffset[character]>>8]-camturnleftrightone;
    float voffset = textureoffset[chrvoffset[character]>>8];
    unsigned char rs = chrredshift[character];
    unsigned char gs = chrgrnshift[character];
    unsigned char bs = chrblushift[character];


    // Original points with linear interpolation ( lip )
    switch(lip)
    {
        case 0:  // 25% this frame
            for (cnt = 0; cnt < madtransvertices[model]; cnt++)
            {
                temp = madvrtx[lastframe][cnt];
                temp = temp+temp+temp;
                v[cnt].x = (float) (madvrtx[frame][cnt] + temp>>2);
                temp = madvrty[lastframe][cnt];
                temp = temp+temp+temp;
                v[cnt].y = (float) (madvrty[frame][cnt] + temp>>2);
                temp = madvrtz[lastframe][cnt];
                temp = temp+temp+temp;
                v[cnt].z = (float) (madvrtz[frame][cnt] + temp>>2);

                ambi = chrvrta[character][cnt];
                ambi = (ambi+ambi+ambi<<1)+ambi+lighttable[lightlevel][lightrotation][madvrta[frame][cnt]]>>3;
                chrvrta[character][cnt] = ambi;
                //v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
				v[cnt].a = (float)alpha / 255.0f;
				v[cnt].r = (float)(ambi>>rs) / 255.0f;
				v[cnt].g = (float)(ambi>>gs) / 255.0f;
				v[cnt].b = (float)(ambi>>bs) / 255.0f;
                //v[cnt].dwReserved = 0;
            }
            break;

        case 1:  // 50% this frame
            for (cnt = 0; cnt < madtransvertices[model]; cnt++)
            {
                v[cnt].x = (float) (madvrtx[frame][cnt] +
                                       madvrtx[lastframe][cnt]>>1);
                v[cnt].y = (float) (madvrty[frame][cnt] +
                                       madvrty[lastframe][cnt]>>1);
                v[cnt].z = (float) (madvrtz[frame][cnt] +
                                       madvrtz[lastframe][cnt]>>1);

                ambi = chrvrta[character][cnt];
                ambi = (ambi+ambi+ambi<<1)+ambi+lighttable[lightlevel][lightrotation][madvrta[frame][cnt]]>>3;
                chrvrta[character][cnt] = ambi;
                //v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
				v[cnt].a = (float)alpha / 255.0f;
				v[cnt].r = (float)(ambi>>rs) / 255.0f;
				v[cnt].g = (float)(ambi>>gs) / 255.0f;
				v[cnt].b = (float)(ambi>>bs) / 255.0f;
                //v[cnt].dwReserved = 0;
            }
            break;

        case 2:  // 75% this frame
            for (cnt = 0; cnt < madtransvertices[model]; cnt++)
            {
                temp = madvrtx[frame][cnt];
                temp = temp+temp+temp;
                v[cnt].x = (float) (madvrtx[lastframe][cnt] + temp>>2);
                temp = madvrty[frame][cnt];
                temp = temp+temp+temp;
                v[cnt].y = (float) (madvrty[lastframe][cnt] + temp>>2);
                temp = madvrtz[frame][cnt];
                temp = temp+temp+temp;
                v[cnt].z = (float) (madvrtz[lastframe][cnt] + temp>>2);

                ambi = chrvrta[character][cnt];
                ambi = (ambi+ambi+ambi<<1)+ambi+lighttable[lightlevel][lightrotation][madvrta[frame][cnt]]>>3;
                chrvrta[character][cnt] = ambi;
                //v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
				v[cnt].a = (float)alpha / 255.0f;
				v[cnt].r = (float)(ambi>>rs) / 255.0f;
				v[cnt].g = (float)(ambi>>gs) / 255.0f;
				v[cnt].b = (float)(ambi>>bs) / 255.0f;
                //v[cnt].dwReserved = 0;
            }
            break;

        case 3:  // 100% this frame...  This is the legible one
            for (cnt = 0; cnt < madtransvertices[model]; cnt++)
            {
                v[cnt].x = (float) madvrtx[frame][cnt];
                v[cnt].y = (float) madvrty[frame][cnt];
                v[cnt].z = (float) madvrtz[frame][cnt];

                ambi = chrvrta[character][cnt];
                ambi = (ambi+ambi+ambi<<1)+ambi+lighttable[lightlevel][lightrotation][madvrta[frame][cnt]]>>3;
                chrvrta[character][cnt] = ambi;
                //v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
				v[cnt].a = (float)alpha / 255.0f;
				v[cnt].r = (float)(ambi>>rs) / 255.0f;
				v[cnt].g = (float)(ambi>>gs) / 255.0f;
				v[cnt].b = (float)(ambi>>bs) / 255.0f;
                //v[cnt].dwReserved = 0;
            }
            break;
    }

    // Do fog...
    /*
    if(fogon && chrlight[character]==255)
    {
        // The full fog value
        alpha = 0xff000000 | (fogred<<16) | (foggrn<<8) | (fogblu);

        for (cnt = 0; cnt < madtransvertices[model]; cnt++)
        {
            // Figure out the z position of the vertex...  Not totally accurate
            z = (v[cnt].z * chrscale[character]) + chrmatrix[character](3,2);

            // Figure out the fog coloring
            if(z < fogtop)
            {
                if(z < fogbottom)
                {
                    v[cnt].specular = alpha;
                }
                else
                {
                    z = 1.0 - ((z - fogbottom)/fogdistance);  // 0.0 to 1.0...  Amount of fog to keep
                    red = fogred * z;
                    grn = foggrn * z;
                    blu = fogblu * z;
                    fogspec = 0xff000000 | (red<<16) | (grn<<8) | (blu);
                    v[cnt].specular = fogspec;
                }
            }
            else
            {
                v[cnt].specular = 0;
            }
        }
    }
    else
    {
        for (cnt = 0; cnt < madtransvertices[model]; cnt++)
            v[cnt].specular = 0;
    }
    */

    mWorld = chrmatrix[character];
    // GS - Begin3DMode ();


	glLoadMatrixf(mView.v);
	glMultMatrixf(mWorld.v);

    // Choose texture and matrix
    //lpD3DDDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, txTexture[texture].GetHandle());
    //lpD3DDDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &chrmatrix[character]);
    glBindTexture ( GL_TEXTURE_2D, GLTexture_GetTextureID (&txTexture[texture]));


    // Make new ones so we can index them and not transform 'em each time
    //if(transform_vertices(madtransvertices[model], v, vt))
        //return;


    // Render each command
    entry = 0;
    for (cnt = 0; cnt < madcommands[model]; cnt++)
    {
	glBegin (madcommandtype[model][cnt]);
    
        for (tnc = 0; tnc < madcommandsize[model][cnt]; tnc++)
        {
            vertex = madcommandvrt[model][entry];
		glColor4fv( &v[vertex].r );
	    glTexCoord2f (indextoenvirox[madvrta[framestt][vertex]]+uoffset,
			  lighttoenviroy[chrvrta[character][vertex]]+voffset);
	    glVertex3fv (&v[vertex].x);
	    /*
            vtlist[tnc].dvSX = vt[vertex].dvSX;
            vtlist[tnc].dvSY = vt[vertex].dvSY;
            vtlist[tnc].dvSZ = (vt[vertex].dvSZ);
            vtlist[tnc].dvRHW = vt[vertex].dvRHW;
            vtlist[tnc].dcColor = vt[vertex].dcColor;
            vtlist[tnc].dcSpecular = vt[vertex].dcSpecular;
            vtlist[tnc].dvTU = indextoenvirox[madvrta[framestt][vertex]]+uoffset;
            vtlist[tnc].dvTV = lighttoenviroy[chrvrta[character][vertex]]+voffset;
	    */
            entry++;
        }
	glEnd ();
        //lpD3DDDevice->DrawPrimitive((D3DPRIMITIVETYPE) madcommandtype[model][cnt],
                                    //D3DVT_TLVERTEX, (LPVOID)vtlist, tnc, NULL);
    }

    mWorld = tempWorld;
	glLoadMatrixf(mView.v);
	glMultMatrixf(mWorld.v);
    // GS - End3DMode ();
}

#if 0
//--------------------------------------------------------------------------------------------
void render_texmad(unsigned short character, unsigned char trans)
{
	Md2Model *model;
	unsigned short texture;
	int frame0, frame1;
	float lerp;
	GLMATRIX mTempWorld;

	// Grab some basic information about the model
	model = md2_models[chrmodel[character]];
	texture = chrtexture[character];
	frame0 = chrframe[character];
	frame1 = chrframe[character];
	lerp = (float)chrlip[character] / 256.0f;

	mTempWorld = mWorld;
/*
	// Lighting information
	unsigned char lightrotation = 
		(chrturnleftright[character]+chrlightturnleftright[character])>>8;
	unsigned char lightlevel = chrlightlevel[character]>>4;
	Uint32 alpha = trans<<24;
    unsigned char spek = chrsheen[character];
    
    float uoffset = textureoffset[chruoffset[character]>>8];
	float voffset = textureoffset[chrvoffset[character]>>8];
    unsigned char rs = chrredshift[character];
	unsigned char gs = chrgrnshift[character];
	unsigned char bs = chrblushift[character];


    if(phongon && trans == 255)
        spek = 0;
*/
#if 0	
    // Original points with linear interpolation ( lip )
    switch(lip)
    {
        case 0:  // 25% this frame
            for (cnt = 0; cnt < madtransvertices[model]; cnt++)
            {
                temp = madvrtx[lastframe][cnt];
                temp = temp+temp+temp;
                //v[cnt].x = (D3DVALUE) (madvrtx[frame][cnt] + temp>>2);
                v[cnt].x = (float) (madvrtx[frame][cnt] + temp>>2);
                temp = madvrty[lastframe][cnt];
                temp = temp+temp+temp;
                //v[cnt].y = (D3DVALUE) (madvrty[frame][cnt] + temp>>2);
                v[cnt].y = (float) (madvrty[frame][cnt] + temp>>2);
                temp = madvrtz[lastframe][cnt];
                temp = temp+temp+temp;
				//v[cnt].z = (D3DVALUE) (madvrtz[frame][cnt] + temp>>2);
                v[cnt].z = (float) (madvrtz[frame][cnt] + temp>>2);

                ambi = chrvrta[character][cnt];
                ambi = (ambi+ambi+ambi<<1)+ambi+lighttable[lightlevel][lightrotation][madvrta[frame][cnt]]>>3;
                chrvrta[character][cnt] = ambi;
                //v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
				v[cnt].r = (float) (ambi>>rs) / 255.0f;
				v[cnt].g = (float) (ambi>>gs) / 255.0f;
				v[cnt].b = (float) (ambi>>bs) / 255.0f;
				v[cnt].a = (float) alpha / 255.0f;

                //v[cnt].specular = lighttospek[spek][ambi];

                //v[cnt].dwReserved = 0;
            }
            break;
        case 1:  // 50% this frame
            for (cnt = 0; cnt < madtransvertices[model]; cnt++)
            {
		/*
                v[cnt].x = (D3DVALUE) (madvrtx[frame][cnt] +
                                       madvrtx[lastframe][cnt]>>1);
                v[cnt].y = (D3DVALUE) (madvrty[frame][cnt] +
                                       madvrty[lastframe][cnt]>>1);
                v[cnt].z = (D3DVALUE) (madvrtz[frame][cnt] +
                                       madvrtz[lastframe][cnt]>>1);
		*/
                v[cnt].x = (float) (madvrtx[frame][cnt] +
                                       madvrtx[lastframe][cnt]>>1);
                v[cnt].y = (float) (madvrty[frame][cnt] +
                                       madvrty[lastframe][cnt]>>1);
                v[cnt].z = (float) (madvrtz[frame][cnt] +
                                       madvrtz[lastframe][cnt]>>1);

                ambi = chrvrta[character][cnt];
                ambi = (ambi+ambi+ambi<<1)+ambi+lighttable[lightlevel][lightrotation][madvrta[frame][cnt]]>>3;
                chrvrta[character][cnt] = ambi;
                //v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
				v[cnt].r = (float) (ambi>>rs) / 255.0f;
				v[cnt].g = (float) (ambi>>gs) / 255.0f;
				v[cnt].b = (float) (ambi>>bs) / 255.0f;
				v[cnt].a = (float) alpha  / 255.0f;

                //v[cnt].specular = lighttospek[spek][ambi];

                //v[cnt].dwReserved = 0;
            }
            break;
        case 2:  // 75% this frame
            for (cnt = 0; cnt < madtransvertices[model]; cnt++)
            {
                temp = madvrtx[frame][cnt];
                temp = temp+temp+temp;
                //v[cnt].x = (D3DVALUE) (madvrtx[lastframe][cnt] + temp>>2);
                v[cnt].x = (float) (madvrtx[lastframe][cnt] + temp>>2);
                temp = madvrty[frame][cnt];
                temp = temp+temp+temp;
                //v[cnt].y = (D3DVALUE) (madvrty[lastframe][cnt] + temp>>2);
                v[cnt].y = (float) (madvrty[lastframe][cnt] + temp>>2);
                temp = madvrtz[frame][cnt];
                temp = temp+temp+temp;
                //v[cnt].z = (D3DVALUE) (madvrtz[lastframe][cnt] + temp>>2);
                v[cnt].z = (float) (madvrtz[lastframe][cnt] + temp>>2);

                ambi = chrvrta[character][cnt];
                ambi = (ambi+ambi+ambi<<1)+ambi+lighttable[lightlevel][lightrotation][madvrta[frame][cnt]]>>3;
                chrvrta[character][cnt] = ambi;
                //v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
				v[cnt].r = (float) (ambi>>rs) / 255.0f;
				v[cnt].g = (float) (ambi>>gs) / 255.0f;
				v[cnt].b = (float) (ambi>>bs) / 255.0f;
				v[cnt].a = (float) alpha / 255.0f;

                //v[cnt].specular = lighttospek[spek][ambi];

                //v[cnt].dwReserved = 0;
            }
            break;
        case 3:  // 100% this frame...  This is the legible one
            for (cnt = 0; cnt < madtransvertices[model]; cnt++)
            {

                v[cnt].x = (float) madvrtx[frame][cnt];
                v[cnt].y = (float) madvrty[frame][cnt];
                v[cnt].z = (float) madvrtz[frame][cnt];

                ambi = chrvrta[character][cnt];
                ambi = (ambi+ambi+ambi<<1)+ambi+lighttable[lightlevel][lightrotation][madvrta[frame][cnt]]>>3;
                chrvrta[character][cnt] = ambi;
                //v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
		v[cnt].r = (float) (ambi>>rs) / 255.0f;
		v[cnt].g = (float) (ambi>>gs) / 255.0f;
		v[cnt].b = (float) (ambi>>bs) / 255.0f;
		v[cnt].a = (float) alpha / 255.0f;

                //v[cnt].specular = lighttospek[spek][ambi];

                //v[cnt].dwReserved = 0;
            }
            break;
    }
#endif

    // Choose texture and matrix
    if(KEYDOWN(SDLK_F7))
    {
		glBindTexture( GL_TEXTURE_2D, GLTexture_GetTextureID( &txTexture[texture] ) );    
    }
    else
    {
		glBindTexture( GL_TEXTURE_2D, GLTexture_GetTextureID( &txTexture[texture] ) );    
    }
    
	mWorld = chrmatrix[character];

    //Begin3DMode();	
	glLoadMatrixf(mView.v);
	glMultMatrixf(mWorld.v);

    // Make new ones so we can index them and not transform 'em each time
//	if(transform_vertices(madtransvertices[model], v, vt))
  //      return;

	draw_textured_md2(model, frame0, frame1, lerp);


	mWorld = mTempWorld;
	glLoadMatrixf(mView.v);
	glMultMatrixf(mWorld.v);
}
#endif

//--------------------------------------------------------------------------------------------
void render_texmad(unsigned short character, unsigned char trans)
{
    // ZZ> This function draws a model
//    D3DLVERTEX v[MAXVERTICES];
//    D3DTLVERTEX vt[MAXVERTICES];
//    D3DTLVERTEX vtlist[MAXCOMMANDSIZE];
	GLVERTEX v[MAXVERTICES];	
	unsigned short cnt, tnc, entry;
	unsigned short vertex;
    signed int temp;
//	float z, fogtokeep;
//	unsigned char red, grn, blu;
    unsigned char ambi;
//	DWORD fogspec;

	// To make life easier
	unsigned short model = chrmodel[character];
	unsigned short texture = chrtexture[character];
	unsigned short frame = chrframe[character];
	unsigned short lastframe = chrlastframe[character];
	unsigned char lip = chrlip[character]>>6;
	unsigned char lightrotation = 
		(chrturnleftright[character]+chrlightturnleftright[character])>>8;
	unsigned char lightlevel = chrlightlevel[character]>>4;
	Uint32 alpha = trans;
    unsigned char spek = chrsheen[character];
    
    float uoffset = textureoffset[chruoffset[character]>>8];
	float voffset = textureoffset[chrvoffset[character]>>8];
    unsigned char rs = chrredshift[character];
	unsigned char gs = chrgrnshift[character];
	unsigned char bs = chrblushift[character];
	GLMATRIX mTempWorld = mWorld;

    if(phongon && trans == 255)
        spek = 0;

    // Original points with linear interpolation ( lip )
    switch(lip)
    {
        case 0:  // 25% this frame
            for (cnt = 0; cnt < madtransvertices[model]; cnt++)
            {
                temp = madvrtx[lastframe][cnt];
                temp = temp+temp+temp;
                //v[cnt].x = (D3DVALUE) (madvrtx[frame][cnt] + temp>>2);
                v[cnt].x = (float) (madvrtx[frame][cnt] + temp>>2);
                temp = madvrty[lastframe][cnt];
                temp = temp+temp+temp;
                //v[cnt].y = (D3DVALUE) (madvrty[frame][cnt] + temp>>2);
                v[cnt].y = (float) (madvrty[frame][cnt] + temp>>2);
                temp = madvrtz[lastframe][cnt];
                temp = temp+temp+temp;
				//v[cnt].z = (D3DVALUE) (madvrtz[frame][cnt] + temp>>2);
                v[cnt].z = (float) (madvrtz[frame][cnt] + temp>>2);

                ambi = chrvrta[character][cnt];
                ambi = (ambi+ambi+ambi<<1)+ambi+lighttable[lightlevel][lightrotation][madvrta[frame][cnt]]>>3;
                chrvrta[character][cnt] = ambi;
                //v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
				v[cnt].r = (float) (ambi>>rs) / 255.0f;
				v[cnt].g = (float) (ambi>>gs) / 255.0f;
				v[cnt].b = (float) (ambi>>bs) / 255.0f;
				v[cnt].a = (float) alpha / 255.0f;

                //v[cnt].specular = lighttospek[spek][ambi];

                //v[cnt].dwReserved = 0;
            }
            break;
        case 1:  // 50% this frame
            for (cnt = 0; cnt < madtransvertices[model]; cnt++)
            {
		/*
                v[cnt].x = (D3DVALUE) (madvrtx[frame][cnt] +
                                       madvrtx[lastframe][cnt]>>1);
                v[cnt].y = (D3DVALUE) (madvrty[frame][cnt] +
                                       madvrty[lastframe][cnt]>>1);
                v[cnt].z = (D3DVALUE) (madvrtz[frame][cnt] +
                                       madvrtz[lastframe][cnt]>>1);
		*/
                v[cnt].x = (float) (madvrtx[frame][cnt] +
                                       madvrtx[lastframe][cnt]>>1);
                v[cnt].y = (float) (madvrty[frame][cnt] +
                                       madvrty[lastframe][cnt]>>1);
                v[cnt].z = (float) (madvrtz[frame][cnt] +
                                       madvrtz[lastframe][cnt]>>1);

                ambi = chrvrta[character][cnt];
                ambi = (ambi+ambi+ambi<<1)+ambi+lighttable[lightlevel][lightrotation][madvrta[frame][cnt]]>>3;
                chrvrta[character][cnt] = ambi;
                //v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
				v[cnt].r = (float) (ambi>>rs) / 255.0f;
				v[cnt].g = (float) (ambi>>gs) / 255.0f;
				v[cnt].b = (float) (ambi>>bs) / 255.0f;
				v[cnt].a = (float) alpha  / 255.0f;

                //v[cnt].specular = lighttospek[spek][ambi];

                //v[cnt].dwReserved = 0;
            }
            break;
        case 2:  // 75% this frame
            for (cnt = 0; cnt < madtransvertices[model]; cnt++)
            {
                temp = madvrtx[frame][cnt];
                temp = temp+temp+temp;
                //v[cnt].x = (D3DVALUE) (madvrtx[lastframe][cnt] + temp>>2);
                v[cnt].x = (float) (madvrtx[lastframe][cnt] + temp>>2);
                temp = madvrty[frame][cnt];
                temp = temp+temp+temp;
                //v[cnt].y = (D3DVALUE) (madvrty[lastframe][cnt] + temp>>2);
                v[cnt].y = (float) (madvrty[lastframe][cnt] + temp>>2);
                temp = madvrtz[frame][cnt];
                temp = temp+temp+temp;
                //v[cnt].z = (D3DVALUE) (madvrtz[lastframe][cnt] + temp>>2);
                v[cnt].z = (float) (madvrtz[lastframe][cnt] + temp>>2);

                ambi = chrvrta[character][cnt];
                ambi = (ambi+ambi+ambi<<1)+ambi+lighttable[lightlevel][lightrotation][madvrta[frame][cnt]]>>3;
                chrvrta[character][cnt] = ambi;
                //v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
				v[cnt].r = (float) (ambi>>rs) / 255.0f;
				v[cnt].g = (float) (ambi>>gs) / 255.0f;
				v[cnt].b = (float) (ambi>>bs) / 255.0f;
				v[cnt].a = (float) alpha / 255.0f;

                //v[cnt].specular = lighttospek[spek][ambi];

                //v[cnt].dwReserved = 0;
            }
            break;
        case 3:  // 100% this frame...  This is the legible one
            for (cnt = 0; cnt < madtransvertices[model]; cnt++)
            {
		/*
                v[cnt].x = (D3DVALUE) madvrtx[frame][cnt];
                v[cnt].y = (D3DVALUE) madvrty[frame][cnt];
                v[cnt].z = (D3DVALUE) madvrtz[frame][cnt];
		*/
                v[cnt].x = (float) madvrtx[frame][cnt];
                v[cnt].y = (float) madvrty[frame][cnt];
                v[cnt].z = (float) madvrtz[frame][cnt];

                ambi = chrvrta[character][cnt];
                ambi = (ambi+ambi+ambi<<1)+ambi+lighttable[lightlevel][lightrotation][madvrta[frame][cnt]]>>3;
                chrvrta[character][cnt] = ambi;
                //v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
		v[cnt].r = (float) (ambi>>rs) / 255.0f;
		v[cnt].g = (float) (ambi>>gs) / 255.0f;
		v[cnt].b = (float) (ambi>>bs) / 255.0f;
		v[cnt].a = (float) alpha / 255.0f;

                //v[cnt].specular = lighttospek[spek][ambi];

                //v[cnt].dwReserved = 0;
            }
            break;
    }

/*
    // Do fog...
    if(fogon && chrlight[character]==255)
    {
        // The full fog value
        alpha = 0xff000000 | (fogred<<16) | (foggrn<<8) | (fogblu);

        for (cnt = 0; cnt < madtransvertices[model]; cnt++)
        {
            // Figure out the z position of the vertex...  Not totally accurate
            z = (v[cnt].z * chrscale[character]) + chrmatrix[character](3,2);

            // Figure out the fog coloring
            if(z < fogtop)
            {
                if(z < fogbottom)
                {
                    v[cnt].specular = alpha;
                }
                else
                {
                    spek = v[cnt].specular & 255;
                    z = (z - fogbottom)/fogdistance;  // 0.0 to 1.0...  Amount of old to keep
                    fogtokeep = 1.0-z;  // 0.0 to 1.0...  Amount of fog to keep
                    spek = spek * z;
                    red = (fogred * fogtokeep) + spek;
                    grn = (foggrn * fogtokeep) + spek;
                    blu = (fogblu * fogtokeep) + spek;
                    fogspec = 0xff000000 | (red<<16) | (grn<<8) | (blu);
                    v[cnt].specular = fogspec;
                }
            }
        }
    }
*/
    // Choose texture and matrix
    if(KEYDOWN(SDLK_F7))
    {
        //lpD3DDDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, NULL);
		glBindTexture( GL_TEXTURE_2D, GLTexture_GetTextureID( &txTexture[texture] ) );    
    }
    else
    {
        //lpD3DDDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, txTexture[texture].GetHandle());
		glBindTexture( GL_TEXTURE_2D, GLTexture_GetTextureID( &txTexture[texture] ) );    
    }
    
	//lpD3DDDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &chrmatrix[character]);
	mWorld = chrmatrix[character];

    //Begin3DMode();	
	glLoadMatrixf(mView.v);
	glMultMatrixf(mWorld.v);

    // Make new ones so we can index them and not transform 'em each time
//	if(transform_vertices(madtransvertices[model], v, vt))
  //      return;

    // Render each command
    entry = 0;
    
    for (cnt = 0; cnt < madcommands[model]; cnt++)
    {
	glBegin (madcommandtype[model][cnt]);
        for (tnc = 0; tnc < madcommandsize[model][cnt]; tnc++)
        {
            vertex = madcommandvrt[model][entry];
			glColor4fv( &v[vertex].r );
			glTexCoord2f ( madcommandu[model][entry]+uoffset, madcommandv[model][entry]+voffset);
			glVertex3fv (&v[vertex].x);
	    /*
            vtlist[tnc].dvSX = vt[vertex].dvSX;
            vtlist[tnc].dvSY = vt[vertex].dvSY;
            vtlist[tnc].dvSZ = (vt[vertex].dvSZ);
            vtlist[tnc].dvRHW = vt[vertex].dvRHW;
            vtlist[tnc].dcColor = vt[vertex].dcColor;
            vtlist[tnc].dcSpecular = vt[vertex].dcSpecular;
            vtlist[tnc].dvTU = madcommandu[model][entry]+uoffset;
            vtlist[tnc].dvTV = madcommandv[model][entry]+voffset;
	    */
            entry++;
        }
	glEnd ();
        //lpD3DDDevice->DrawPrimitive((D3DPRIMITIVETYPE) madcommandtype[model][cnt],
        //                            D3DVT_TLVERTEX, (LPVOID)vtlist, tnc, NULL);
    }
    //End3DMode ();

	mWorld = mTempWorld;
	glLoadMatrixf(mView.v);
	glMultMatrixf(mWorld.v);
}

//--------------------------------------------------------------------------------------------
void render_mad(unsigned short character, unsigned char trans)
{
    // ZZ> This function picks the actual function to use
    signed char hide = caphidestate[chrmodel[character]];

    if(hide == NOHIDE || hide != chraistate[character])
    {
        if(chrenviro[character])
            render_enviromad(character, trans);
        else
            render_texmad(character, trans);
    }
}

//--------------------------------------------------------------------------------------------
void render_refmad(int tnc, unsigned char trans)
{
    // ZZ> This function draws characters reflected in the floor
    if(capreflect[chrmodel[tnc]])
    {
        int level = chrlevel[tnc];
        int alphatemp = trans;
        int zpos = (chrmatrix[tnc])_CNV(3,2)-level;
        alphatemp -= zpos>>1;
        
		if(alphatemp < 0 ) alphatemp = 0;

        alphatemp = alphatemp|reffadeor;  // Fix for Riva owners

        if(alphatemp > 255 ) alphatemp = 255;

        if(alphatemp > 0)
        {
            unsigned char sheensave = chrsheen[tnc];
			chrredshift[tnc]+=1;
            chrgrnshift[tnc]+=1;
            chrblushift[tnc]+=1;
            chrsheen[tnc] = chrsheen[tnc]>>1;
            (chrmatrix[tnc])_CNV(0,2) = -(chrmatrix[tnc])_CNV(0,2);
            (chrmatrix[tnc])_CNV(1,2) = -(chrmatrix[tnc])_CNV(1,2);
            (chrmatrix[tnc])_CNV(2,2) = -(chrmatrix[tnc])_CNV(2,2);
            (chrmatrix[tnc])_CNV(3,2) = -(chrmatrix[tnc])_CNV(3,2)+level+level;
            zpos = fogon;
            fogon = bfalse;
            render_mad(tnc, alphatemp);
            fogon = zpos;
            (chrmatrix[tnc])_CNV(0,2) = -(chrmatrix[tnc])_CNV(0,2);
            (chrmatrix[tnc])_CNV(1,2) = -(chrmatrix[tnc])_CNV(1,2);
            (chrmatrix[tnc])_CNV(2,2) = -(chrmatrix[tnc])_CNV(2,2);
            (chrmatrix[tnc])_CNV(3,2) = -(chrmatrix[tnc])_CNV(3,2)+level+level;
            chrsheen[tnc] = sheensave;
            chrredshift[tnc]-=1;
            chrgrnshift[tnc]-=1;
            chrblushift[tnc]-=1;
        }
    }
}
