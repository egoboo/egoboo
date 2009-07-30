// ********************************************************************************************
// *
// *    This file is part of Egoboo.
// *
// *    Egoboo is free software: you can redistribute it and/or modify it
// *    under the terms of the GNU General Public License as published by
// *    the Free Software Foundation, either version 3 of the License, or
// *    (at your option) any later version.
// *
// *    Egoboo is distributed in the hope that it will be useful, but
// *    WITHOUT ANY WARRANTY; without even the implied warranty of
// *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// *    General Public License for more details.
// *
// *    You should have received a copy of the GNU General Public License
// *    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
// *
// ********************************************************************************************

#include "Md2.h"
#include "log.h"                // For error logging
#include "mad.h"

#include "egoboo.h"
#include "egoboo_endian.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static int  md2_rip_header();
static void md2_rip_commands( md2_ogl_commandlist_t * pclist );

Uint16      md2_loadframe = 0;
md2_frame_t Md2FrameList[MAXFRAME];

static Uint8 cLoadBuffer[MD2MAXLOADSIZE];// Where to put an MD2

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float kMd2Normals[][3] =
{
#include "id_normals.inl"
    ,
    {0, 0, 0}  // Spiky Mace
};

// TEMPORARY: Global list of Md2Models.  It's declared in egoboo.h, which
// is why I have to include it here at the moment.
struct Md2Model *md2_models[MAX_PROFILE];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void md2_freeModel( Md2Model *model )
{
    if ( model != NULL )
    {
        if ( model->texCoords != NULL ) free( model->texCoords );
        if ( model->triangles != NULL ) free( model->triangles );
        if ( model->skins != NULL ) free( model->skins );
        if ( model->frames != NULL )
        {
            int i;

            for ( i = 0; i < model->numFrames; i++ )
            {
                if ( model->frames[i].vertices != NULL ) free( model->frames[i].vertices );
            }

            free( model->frames );
        }

        free( model );
    }
}

//--------------------------------------------------------------------------------------------
Md2Model* md2_loadFromFile( const char *filename )
{
    return NULL;
}

//---------------------------------------------------------------------------------------------
int md2_rip_header()
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
void md2_rip_commands( md2_ogl_commandlist_t * pclist )
{
    // ZZ> This function converts an md2's GL commands into our little command list thing
    int iTmp;
    float fTmpu, fTmpv;
    int iNumVertices;
    int tnc;
    bool_t command_error = bfalse, entry_error = bfalse;
    int    vertex_max = 0;

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
    int iCommandCount;
    int entry;
    int cnt;

    iCommandCount = 0;
    entry = 0;
    cnt = 0;
    while ( cnt < iCommandWords )
    {
        Uint32 command_type;

        iNumVertices = ENDIAN_INT32( ipIntPointer[iCommandOffset] );  iCommandOffset++;  cnt++;
        if ( 0 == iNumVertices ) break;
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
            pclist->type[iCommandCount] = command_type;
            pclist->size[iCommandCount] = MIN(iNumVertices, MAXCOMMANDENTRIES);
        }

        // Read in vertices for each command
        entry_error = bfalse;
        for ( tnc = 0; tnc < iNumVertices; tnc++ )
        {
            fTmpu = ENDIAN_FLOAT( fpFloatPointer[iCommandOffset] );  iCommandOffset++;  cnt++;
            fTmpv = ENDIAN_FLOAT( fpFloatPointer[iCommandOffset] );  iCommandOffset++;  cnt++;
            iTmp  = ENDIAN_INT32( ipIntPointer[iCommandOffset]   );  iCommandOffset++;  cnt++;

            entry_error = entry >= MAXCOMMANDENTRIES;
            if ( iTmp > vertex_max )
            {
                vertex_max = iTmp;
            }
            if ( iTmp > MAXVERTICES ) iTmp = MAXVERTICES - 1;
            if ( !command_error && !entry_error )
            {
                pclist->u[entry]   = fTmpu - ( 0.5f / 64 ); // GL doesn't align correctly
                pclist->v[entry]   = fTmpv - ( 0.5f / 64 ); // with D3D
                pclist->vrt[entry] = iTmp;
            }

            entry++;
        }

        // count only fully valid commands
        if ( !entry_error )
        {
            iCommandCount++;
        }
    }

    if ( vertex_max >= MAXVERTICES )
    {
        log_warning("md2_rip_commands(\"%s\") - \n\tOpenGL command references vertices above preset maximum: %d of %d\n", globalparsename, vertex_max, MAXVERTICES );
    }
    if ( command_error )
    {
        log_warning("md2_rip_commands(\"%s\") - \n\tNumber of OpenGL commands exceeds preset maximum: %d of %d\n", globalparsename, iCommandCount, MAXCOMMAND );
    }
    if ( entry_error )
    {
        log_warning("md2_rip_commands(\"%s\") - \n\tNumber of OpenGL command entries exceeds preset maximum: %d of %d\n", globalparsename, entry, MAXCOMMAND );
    }

    pclist->entries = MIN(MAXCOMMANDENTRIES, entry);
    pclist->count   = MIN(MAXCOMMAND, iCommandCount);
}

//---------------------------------------------------------------------------------------------
int md2_rip_frame_name( int frame )
{
    // ZZ> This function gets frame names from the load buffer, it returns
    //    btrue if the name in cFrameName[] is valid
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
void md2_rip_frames( ego_md2_t * pmd2 )
{
    // ZZ> This function gets frames from the load buffer and adds them to
    //    the indexed model
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
    pmd2->framestart = md2_loadframe;
    pmd2->frames     = iNumFrames;
    pmd2->vertices   = iNumVertices;
    cnt = 0;

    while ( cnt < iNumFrames && md2_loadframe < MAXFRAME )
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

            Md2FrameList[md2_loadframe].vrtx[tnc] = -fRealx * 3.5f;
            Md2FrameList[md2_loadframe].vrty[tnc] =  fRealy * 3.5f;
            Md2FrameList[md2_loadframe].vrtz[tnc] =  fRealz * 3.5f;
            Md2FrameList[md2_loadframe].vrta[tnc] =  cTmpa;

            iFrameOffset++;
            tnc++;
        }

        md2_loadframe++;
        cnt++;
    }
}

//---------------------------------------------------------------------------------------------
int md2_load_one( const char* szLoadname, ego_md2_t * pmd2 )
{
    // ZZ> This function loads an id md2 file, storing the converted data in the indexed model
    //   int iFileHandleRead;
    size_t iBytesRead = 0;
    int iReturnValue;

    // Read the input file
    FILE *file = fopen( szLoadname, "rb" );
    if ( !file )
    {
        log_warning( "Cannot load file! (\"%s\")\n", szLoadname );
        return bfalse;
    }

    // Read up to MD2MAXLOADSIZE bytes from the file into the cLoadBuffer array.
    iBytesRead = fread( cLoadBuffer, 1, MD2MAXLOADSIZE, file );
    if ( iBytesRead == 0 )
        return bfalse;

    // save the filename for debugging
    globalparsename = szLoadname;

    // Check the header
    // TODO: Verify that the header's filesize correspond to iBytesRead.
    iReturnValue = md2_rip_header();
    if ( !iReturnValue )
        return bfalse;

    // Get the frame vertices
    md2_rip_frames( pmd2 );

    // Get the commands
    md2_rip_commands( &(pmd2->cmd) );

    fclose( file );

    return btrue;
}
