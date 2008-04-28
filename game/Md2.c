/* Egoboo - Md2.c
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

#include "MD2.h"
#include "id_md2.h"
#include "egoboo_math.h"
#include <SDL_endian.h>    // TODO: Roll my own endian stuff so that I don't have to include
                           // SDL outside of the stuff that touches video/audio/input/etc.
                           // Not a high priority

MD2_Model* md2_load(const char *fileName, MD2_Model* mdl)
{
  md2_header header;
  FILE *f;
  int i, v;
  MD2_Model *model;
  int bfound;

  if(fileName == NULL) return NULL;

  // Open up the file, and make sure it's a MD2 model
  f = fopen(fileName, "rb");
  if (f == NULL) return NULL;

  fread(&header, sizeof(header), 1, f);

  // Convert the byte GOrder.ing in the header, if we need to
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
  header.magic            = SDL_Swap32BE(header.magic);
  header.version          = SDL_Swap32BE(header.version);
  header.skinWidth        = SDL_Swap32BE(header.skinWidth);
  header.skinHeight       = SDL_Swap32BE(header.skinHeight);
  header.frameSize        = SDL_Swap32BE(header.frameSize);
  header.numSkins         = SDL_Swap32BE(header.numSkins);
  header.numVertices      = SDL_Swap32BE(header.numVertices);
  header.numTexCoords     = SDL_Swap32BE(header.numTexCoords);
  header.numTriangles     = SDL_Swap32BE(header.numTriangles);
  header.numGlCommands    = SDL_Swap32BE(header.numGlCommands);
  header.numFrames        = SDL_Swap32BE(header.numFrames);
  header.offsetSkins      = SDL_Swap32BE(header.offsetSkins);
  header.offsetTexCoords  = SDL_Swap32BE(header.offsetTexCoords);
  header.offsetTriangles  = SDL_Swap32BE(header.offsetTriangles);
  header.offsetFrames     = SDL_Swap32BE(header.offsetFrames);
  header.offsetGlCommands = SDL_Swap32BE(header.offsetGlCommands);
  header.offsetEnd        = SDL_Swap32BE(header.offsetEnd);
#endif

  if(header.magic != MD2_MAGIC_NUMBER || header.version != MD2_VERSION)
  {
    fclose(f);
    return NULL;
  }

  // Allocate a MD2_Model to hold all this stuff
  model = (NULL==mdl) ? md2_new() : mdl;
  model->m_numVertices  = header.numVertices;
  model->m_numTexCoords = header.numTexCoords;
  model->m_numTriangles = header.numTriangles;
  model->m_numSkins     = header.numSkins;
  model->m_numFrames    = header.numFrames;

  model->m_texCoords = calloc( sizeof(MD2_TexCoord), header.numTexCoords);
  model->m_triangles = calloc( sizeof(MD2_Triangle), header.numTriangles);
  model->m_skins     = calloc( sizeof(MD2_SkinName), header.numSkins);
  model->m_frames    = calloc( sizeof(MD2_Frame),    header.numFrames);

  for (i = 0;i < header.numFrames; i++)
  {
    model->m_frames[i].vertices = calloc( sizeof(MD2_Vertex), header.numVertices);
  }

  // Load the texture coordinates from the file, normalizing them as we go
  fseek(f, header.offsetTexCoords, SEEK_SET);
  for (i = 0;i < header.numTexCoords; i++)
  {
    md2_texcoord tc;
    fread(&tc, sizeof(tc), 1, f);

    // Convert the byte order of the texture coordinates, if we need to
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
    tc.s = SDL_Swap16(tc.s);
    tc.t = SDL_Swap16(tc.t);
#endif
    model->m_texCoords[i].s = tc.s / (float)header.skinWidth;
    model->m_texCoords[i].t = tc.t / (float)header.skinHeight;
  }

  // Load triangles from the file.  I use the same memory layout as the file
  // on a little endian machine, so they can just be read directly
  fseek(f, header.offsetTriangles, SEEK_SET);
  fread(model->m_triangles, sizeof(md2_triangle), header.numTriangles, f);

  // Convert the byte GOrder.ing on the triangles, if necessary
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
  for(int i = 0;i < header.numTriangles;i++)
  {
    for(v = 0;v < 3;v++)
    {
      model->m_triangles[i].vertexIndices[v]   = SDL_Swap16(model->triangles[i].vertexIndices[v]);
      model->m_triangles[i].texCoordIndices[v] = SDL_Swap16(model->triangles[i].texCoordIndices[v]);
    }
  }
#endif

  // Load the skin names.  Again, I can load them directly
  fseek(f, header.offsetSkins, SEEK_SET);
  fread(model->m_skins, sizeof(md2_skinname), header.numSkins, f);

  // Load the frames of animation
  fseek(f, header.offsetFrames, SEEK_SET);
  for(i = 0;i < header.numFrames;i++)
  {
    char frame_buffer[MD2_MAX_FRAMESIZE];
    md2_frame *frame;

    // read the current frame
    fread(frame_buffer, header.frameSize, 1, f);
    frame = (md2_frame*)frame_buffer;

    // Convert the byte GOrder.ing on the scale & translate vectors, if necessary
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
    frame->scale[0] = swapFloat(frame->scale[0]);
    frame->scale[1] = swapFloat(frame->scale[1]);
    frame->scale[2] = swapFloat(frame->scale[2]);

    frame->translate[0] = swapFloat(frame->translate[0]);
    frame->translate[1] = swapFloat(frame->translate[1]);
    frame->translate[2] = swapFloat(frame->translate[2]);
#endif

    // unpack the md2 vertices from this frame
    bfound = (1==0);
    for(v=0; v<header.numVertices; v++)
    {
      model->m_frames[i].vertices[v].x = frame->vertices[v].vertex[0] * frame->scale[0] + frame->translate[0];
      model->m_frames[i].vertices[v].y = frame->vertices[v].vertex[1] * frame->scale[1] + frame->translate[1];
      model->m_frames[i].vertices[v].z = frame->vertices[v].vertex[2] * frame->scale[2] + frame->translate[2];

      model->m_frames[i].vertices[v].normal = frame->vertices[v].lightNormalIndex;

      // Calculate the bounding box for this frame
      if(!bfound)
      {
        model->m_frames[i].bbmin[0] = model->m_frames[i].vertices[v].x;
        model->m_frames[i].bbmin[1] = model->m_frames[i].vertices[v].y;
        model->m_frames[i].bbmin[2] = model->m_frames[i].vertices[v].z;

        model->m_frames[i].bbmax[0] = model->m_frames[i].vertices[v].x;
        model->m_frames[i].bbmax[1] = model->m_frames[i].vertices[v].y;
        model->m_frames[i].bbmax[2] = model->m_frames[i].vertices[v].z;

        bfound = (1==1);
      }
      else
      {
        model->m_frames[i].bbmin[0] = MIN( model->m_frames[i].bbmin[0], model->m_frames[i].vertices[v].x - 0.001f);
        model->m_frames[i].bbmin[1] = MIN( model->m_frames[i].bbmin[1], model->m_frames[i].vertices[v].y - 0.001f);
        model->m_frames[i].bbmin[2] = MIN( model->m_frames[i].bbmin[2], model->m_frames[i].vertices[v].z - 0.001f);

        model->m_frames[i].bbmax[0] = MAX( model->m_frames[i].bbmax[0], model->m_frames[i].vertices[v].x + 0.001f);
        model->m_frames[i].bbmax[1] = MAX( model->m_frames[i].bbmax[1], model->m_frames[i].vertices[v].y + 0.001f);
        model->m_frames[i].bbmax[2] = MAX( model->m_frames[i].bbmax[2], model->m_frames[i].vertices[v].z + 0.001f);
      }
    }

    //make sure to copy the frame name!
    strncpy(model->m_frames[i].name, frame->name, 16);
  }

  //Load up the pre-computed OpenGL optimizations
  if(header.numGlCommands > 0)
  {
    Uint32          cmd_cnt = 0;
    MD2_GLCommand * cmd     = NULL;
    fseek(f, header.offsetGlCommands, SEEK_SET);

    //count the commands
    while( cmd_cnt < header.numGlCommands )
    {
      //read the number of commands in the strip
      Sint32 commands;
      fread(&commands, sizeof(Sint32), 1, f);
      commands = SDL_SwapLE32(commands);
      if(commands==0)
        break;

      cmd = MD2_GLCommand_new();
      cmd->command_count = commands;

      //set the GL drawing mode
      if(cmd->command_count > 0)
      {
        cmd->gl_mode = GL_TRIANGLE_STRIP;
      }
      else
      {
        cmd->command_count = -cmd->command_count;
        cmd->gl_mode = GL_TRIANGLE_FAN;
      }

      //allocate the data
      cmd->data = calloc( sizeof(md2_gldata), cmd->command_count);

      //read in the data
      fread(cmd->data, sizeof(md2_gldata), cmd->command_count, f);

      //translate the data, if necessary
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
      for(int i=0; i<cmd->command_count; i++)
      {
        cmd->data[i].index = SDL_swap32(cmd->data[i].s);
        cmd->data[i].s     = swapFloat(cmd->data[i].s);
        cmd->data[i].t     = swapFloat(cmd->data[i].t);
      };
#endif

      // attach it to the command list
      cmd->next         = model->m_commands;
      model->m_commands = cmd;

      cmd_cnt += cmd->command_count;
    };
  }


  // Close the file, we're done with it
  fclose(f);

  return model;
}

const MD2_SkinName *md2_get_Skin(MD2_Model * m, int index)
{
  if(index >= 0 && index < m->m_numSkins)
  {
    return &m->m_skins[index];
  }
  return NULL;
}

const MD2_Frame *md2_get_Frame(MD2_Model * m, int index)
{
  if(index >= 0 && index < m->m_numFrames)
  {
    return &m->m_frames[index];
  }
  return NULL;
}


//MD2_Model* MD2_Manager::loadFromFile(const char *fileName, MD2_Model* mdl)
//{
//  // ignore garbage input
//  if (!fileName || !fileName[0]) return NULL;
//
//  // See if it's already been loaded
//  ModelMap::iterator i = modelCache.find(string(fileName));
//  if (i != modelCache.end())
//  {
//    return i->second->retain();
//  }
//
//  // No?  Try loading it
//  MD2_Model *model = md2_load(MD2_Model * m, fileName, mdl);
//  if (model == NULL)
//  {
//    // no luck
//    return NULL;
//  }
//
//  ModList[string(fileName)].elCache = model;
//  return model;
//}
//


void MD2_GLCommand_construct(MD2_GLCommand * m)
{
  m->next = NULL;
  m->data = NULL;
};

void MD2_GLCommand_destruct(MD2_GLCommand * m)
{
  if(NULL==m) return;

  if(NULL!=m->next)
  {
    MD2_GLCommand_delete(m->next);
    m->next = NULL;
  };

  if(NULL!=m->data)
  {
    free(m->data);
    m->data = NULL;
  }
};

MD2_GLCommand * MD2_GLCommand_new()
{
  MD2_GLCommand * m;
  m = calloc(sizeof(MD2_GLCommand),1);
  MD2_GLCommand_construct(m);
  return m;
};

MD2_GLCommand * MD2_GLCommand_new_vector(int n)
{
  int i;
  MD2_GLCommand * v = calloc(sizeof(MD2_GLCommand),n);
  for(i=0; i<n; i++) MD2_GLCommand_construct(&v[i]);
  return v;
}

void MD2_GLCommand_delete(MD2_GLCommand * m)
{
  if(NULL==m) return;
  MD2_GLCommand_destruct(m);
  free(m);
};

void MD2_GLCommand_delete_vector(MD2_GLCommand * v, int n)
{
  int i;
  if(NULL==v || 0 == n) return;
  for(i=0; i<n; i++) MD2_GLCommand_destruct(&v[i]);
  free(v);
};


void md2_construct(MD2_Model * m)
{
  m->m_numVertices  = 0;
  m->m_numTexCoords = 0;
  m->m_numTriangles = 0;
  m->m_numSkins     = 0;
  m->m_numFrames    = 0;

  m->m_skins     = NULL;
  m->m_texCoords = NULL;
  m->m_triangles = NULL;
  m->m_frames    = NULL;
  m->m_commands  = NULL;
}

void md2_deallocate(MD2_Model * m)
{
  if(m->m_skins != NULL)
  {
    free( m->m_skins );
    m->m_skins = NULL;
    m->m_numSkins = 0;
  }

  if(m->m_texCoords != NULL)
  {
    free( m->m_texCoords );
    m->m_texCoords = NULL;
    m->m_numTexCoords = 0;
  }

  if(m->m_triangles != NULL)
  {
    free( m->m_triangles );
    m->m_triangles = NULL;
    m->m_numTriangles = 0;
  }

  if(m->m_frames != NULL)
  {
    int i;
    for(i = 0;i < m->m_numFrames; i++)
    {
      if(m->m_frames[i].vertices != NULL)
      {
        free( m->m_frames[i].vertices );
      }
    }
    free( m->m_frames );
    m->m_frames = NULL;
    m->m_numFrames = 0;
  }

  if(m->m_commands!=NULL)
  {
    free( m->m_commands );
    m->m_commands = NULL;
  }
};

void md2_destruct(MD2_Model * m)
{
  if(NULL==m) return;
  md2_deallocate(m);
}

MD2_Model * md2_new()
{
  MD2_Model * m;
  m = calloc(sizeof(MD2_Model),1);
  md2_construct(m);
  return m;
};

MD2_Model * md2_new_vector(int n)
{
  int i;
  MD2_Model * v = calloc(sizeof(MD2_Model),n);
  for(i=0; i<n; i++) md2_construct(&v[i]);
  return v;
}

void md2_delete(MD2_Model * m)
{
  if(NULL==m) return;
  md2_destruct(m);
  free(m);
};

void md2_delete_vector(MD2_Model * v, int n)
{
  int i;
  if(NULL==v || 0 == n) return;
  for(i=0; i<n; i++) md2_destruct(&v[i]);
  free(v);
};



////---------------------------------------------------------------------------------------------
//int rip_md2_header( void )
//{
//  // ZZ> This function makes sure an md2 is really an md2
//  int iTmp;
//  int* ipIntPointer;
//
//  // Check the file type
//  ipIntPointer = ( int* ) cLoadBuffer;
//  iTmp = ipIntPointer[0];
//
//#if SDL_BYTEORDER != SDL_LIL_ENDIAN
//  iTmp = SDL_Swap32( iTmp );
//#endif
//
//  if ( iTmp != MD2START ) return bfalse;
//
//  return btrue;
//}
//
////---------------------------------------------------------------------------------------------
//void fix_md2_normals( Uint16 modelindex )
//{
//  // ZZ> This function helps light not flicker so much
//  int cnt, tnc;
//  Uint8 indexofcurrent, indexofnext, indexofnextnext, indexofnextnextnext;
//  Uint8 indexofnextnextnextnext;
//  Uint32 frame;
//
//  frame = MadList[modelindex].framestart;
//  cnt = 0;
//  while ( cnt < MadList[modelindex].vertices )
//  {
//    tnc = 0;
//    while ( tnc < MadList[modelindex].frames )
//    {
//      indexofcurrent = MadList[frame].vrta[cnt];
//      indexofnext = MadList[frame+1].vrta[cnt];
//      indexofnextnext = MadList[frame+2].vrta[cnt];
//      indexofnextnextnext = MadList[frame+3].vrta[cnt];
//      indexofnextnextnextnext = MadList[frame+4].vrta[cnt];
//      if ( indexofcurrent == indexofnextnext && indexofnext != indexofcurrent )
//      {
//        MadList[frame+1].vrta[cnt] = indexofcurrent;
//      }
//      if ( indexofcurrent == indexofnextnextnext )
//      {
//        if ( indexofnext != indexofcurrent )
//        {
//          MadList[frame+1].vrta[cnt] = indexofcurrent;
//        }
//        if ( indexofnextnext != indexofcurrent )
//        {
//          MadList[frame+2].vrta[cnt] = indexofcurrent;
//        }
//      }
//      if ( indexofcurrent == indexofnextnextnextnext )
//      {
//        if ( indexofnext != indexofcurrent )
//        {
//          MadList[frame+1].vrta[cnt] = indexofcurrent;
//        }
//        if ( indexofnextnext != indexofcurrent )
//        {
//          MadList[frame+2].vrta[cnt] = indexofcurrent;
//        }
//        if ( indexofnextnextnext != indexofcurrent )
//        {
//          MadList[frame+3].vrta[cnt] = indexofcurrent;
//        }
//      }
//      tnc++;
//    }
//    cnt++;
//  }
//}
//
//---------------------------------------------------------------------------------------------
//void rip_md2_commands( Uint16 modelindex )
//{
//  // ZZ> This function converts an md2's GL commands into our little command list thing
//  int iTmp;
//  float fTmpu, fTmpv;
//  int iNumVertices;
//  int tnc;
//
//  char* cpCharPointer = ( char* ) cLoadBuffer;
//  int* ipIntPointer = ( int* ) cLoadBuffer;
//  float* fpFloatPointer = ( float* ) cLoadBuffer;
//
//  // Number of GL commands in the MD2
//  int iNumCommands = ipIntPointer[9];
//
//#if SDL_BYTEORDER != SDL_LIL_ENDIAN
//  iNumCommands = SDL_Swap32( iNumCommands );
//#endif
//
//  // Offset (in DWORDS) from the start of the file to the gl command list.
//  int iCommandOffset = ipIntPointer[15] >> 2;
//
//#if SDL_BYTEORDER != SDL_LIL_ENDIAN
//  iCommandOffset = SDL_Swap32( iCommandOffset );
//#endif
//
//  // Read in each command
//  // iNumCommands isn't the number of commands, rather the number of dwords in
//  // the command list...  Use iCommandCount to figure out how many we use
//  int iCommandCount = 0;
//  int entry = 0;
//
//  int cnt = 0;
//  while ( cnt < iNumCommands )
//  {
//    iNumVertices = ipIntPointer[iCommandOffset];
//
//#if SDL_BYTEORDER != SDL_LIL_ENDIAN
//    iNumVertices = SDL_Swap32( iNumVertices );
//#endif
//
//    iCommandOffset++;
//    cnt++;
//
//    if ( iNumVertices != 0 )
//    {
//      if ( iNumVertices < 0 )
//      {
//        // Fans start with a negative
//        iNumVertices = -iNumVertices;
//        // PORT: MadList[modelindex].commandtype[iCommandCount] = (Uint8) D3DPT_TRIANGLEFAN;
//        MadList[modelindex].commandtype[iCommandCount] = GL_TRIANGLE_FAN;
//        MadList[modelindex].commandsize[iCommandCount] = ( Uint8 ) iNumVertices;
//      }
//      else
//      {
//        // Strips start with a positive
//        MadList[modelindex].commandtype[iCommandCount] = GL_TRIANGLE_STRIP;
//        MadList[modelindex].commandsize[iCommandCount] = ( Uint8 ) iNumVertices;
//      }
//
//      // Read in vertices for each command
//      tnc = 0;
//      while ( tnc < iNumVertices )
//      {
//        fTmpu = fpFloatPointer[iCommandOffset];  iCommandOffset++;  cnt++;
//        fTmpv = fpFloatPointer[iCommandOffset];  iCommandOffset++;  cnt++;
//        iTmp = ipIntPointer[iCommandOffset];  iCommandOffset++;  cnt++;
//
//#if SDL_BYTEORDER != SDL_LIL_ENDIAN
//        fTmpu = LoadFloatByteswapped( &fTmpu );
//        fTmpv = LoadFloatByteswapped( &fTmpv );
//        iTmp = SDL_Swap32( iTmp );
//#endif
//        MadList[modelindex].commandu[entry] = fTmpu - ( .5 / 64 ); // GL doesn't align correctly
//        MadList[modelindex].commandv[entry] = fTmpv - ( .5 / 64 ); // with D3D
//        MadList[modelindex].commandvrt[entry] = ( Uint16 ) iTmp;
//        entry++;
//        tnc++;
//      }
//      iCommandCount++;
//    }
//  }
//  MadList[modelindex].commands = iCommandCount;
//}

//---------------------------------------------------------------------------------------------
//char * rip_md2_frame_name( MD2_Model * m, int frame )
//{
//  // ZZ> This function gets frame names from the load buffer, it returns
//  //     btrue if the name in cFrameName[] is valid
//  int iFrameOffset;
//  int iNumVertices;
//  int iNumFrames;
//  int cnt;
//  MD2_Frame * pFrame;
//  char      * pFrameName;
//  bool_t foundname;
//
//
//  if(NULL == m) return bfalse;
//
//  // Jump to the Frames section of the md2 data
//
//
//  ipNamePointer = ( int* ) pFrame->name;
//
//
//  iNumVertices = ipIntPointer[6];
//  iNumFrames = ipIntPointer[10];
//  iFrameOffset = ipIntPointer[14] >> 2;
//
//#if SDL_BYTEORDER != SDL_LIL_ENDIAN
//  iNumVertices = SDL_Swap32( iNumVertices );
//  iNumFrames = SDL_Swap32( iNumFrames );
//  iFrameOffset = SDL_Swap32( iFrameOffset );
//#endif
//
//
//  // Chug through each frame
//  foundname = bfalse;
//
//  for ( cnt = 0; cnt < iNumFrames && !foundname; cnt++ )
//  {
//    pFrame     = md2_get_Frame(m , frame);
//    pFrameName = pFrame->name;
//
//    iFrameOffset += 6;
//    if ( cnt == frame )
//    {
//      ipNamePointer[0] = ipIntPointer[iFrameOffset]; iFrameOffset++;
//      ipNamePointer[1] = ipIntPointer[iFrameOffset]; iFrameOffset++;
//      ipNamePointer[2] = ipIntPointer[iFrameOffset]; iFrameOffset++;
//      ipNamePointer[3] = ipIntPointer[iFrameOffset]; iFrameOffset++;
//      foundname = btrue;
//    }
//    else
//    {
//      iFrameOffset += 4;
//    }
//    iFrameOffset += iNumVertices;
//    cnt++;
//  }
//  cFrameName[15] = 0;  // Make sure it's null terminated
//  return foundname;
//}

//---------------------------------------------------------------------------------------------
//void rip_md2_frames( MD2_Model * m )
//{
//  // ZZ> This function gets frames from the load buffer and adds them to
//  //     the indexed model
//  Uint8 cTmpx, cTmpy, cTmpz;
//  Uint8 cTmpNormalIndex;
//  float fRealx, fRealy, fRealz;
//  float fScalex, fScaley, fScalez;
//  float fTranslatex, fTranslatey, fTranslatez;
//  int iFrameOffset;
//  int iNumVertices;
//  int iNumFrames;
//  int cnt, tnc;
//  char* cpCharPointer;
//  int* ipIntPointer;
//  float* fpFloatPointer;
//
//  if(NULL == m) return;
//
//
//  // Jump to the Frames section of the md2 data
//  cpCharPointer = ( char* ) cLoadBuffer;
//  ipIntPointer = ( int* ) cLoadBuffer;
//  fpFloatPointer = ( float* ) cLoadBuffer;
//
//
//  iNumVertices = md2_get_numVertices(m);
//  iNumFrames   = md2_get_numFrames(m);
//
//
//  for( cnt = 0; cnt < iNumFrames; cnt++ )
//  {
//    MD2_Frame * = MD2_Frame(m, cnt);
//
//    fScalex = fpFloatPointer[iFrameOffset]; iFrameOffset++;
//    fScaley = fpFloatPointer[iFrameOffset]; iFrameOffset++;
//    fScalez = fpFloatPointer[iFrameOffset]; iFrameOffset++;
//    fTranslatex = fpFloatPointer[iFrameOffset]; iFrameOffset++;
//    fTranslatey = fpFloatPointer[iFrameOffset]; iFrameOffset++;
//    fTranslatez = fpFloatPointer[iFrameOffset]; iFrameOffset++;
//
//#if SDL_BYTEORDER != SDL_LIL_ENDIAN
//    fScalex = LoadFloatByteswapped( &fScalex );
//    fScaley = LoadFloatByteswapped( &fScaley );
//    fScalez = LoadFloatByteswapped( &fScalez );
//
//    fTranslatex = LoadFloatByteswapped( &fTranslatex );
//    fTranslatey = LoadFloatByteswapped( &fTranslatey );
//    fTranslatez = LoadFloatByteswapped( &fTranslatez );
//#endif
//
//    iFrameOffset += 4;
//    tnc = 0;
//    while ( tnc < iNumVertices )
//    {
//      // This should work because it's reading a single character
//      cTmpx = cpCharPointer[( iFrameOffset<<2 )];
//      cTmpy = cpCharPointer[( iFrameOffset<<2 ) +1];
//      cTmpz = cpCharPointer[( iFrameOffset<<2 ) +2];
//      cTmpNormalIndex = cpCharPointer[( iFrameOffset<<2 ) +3];
//      fRealx = ( cTmpx * fScalex ) + fTranslatex;
//      fRealy = ( cTmpy * fScaley ) + fTranslatey;
//      fRealz = ( cTmpz * fScalez ) + fTranslatez;
//      MadList[madloadframe].vrtx[tnc] = -fRealx * 3.5;
//      MadList[madloadframe].vrty[tnc] = fRealy * 3.5;
//      MadList[madloadframe].vrtz[tnc] = fRealz * 3.5;
//      MadList[madloadframe].vrta[tnc] = cTmpNormalIndex;
//      iFrameOffset++;
//      tnc++;
//    }
//    madloadframe++;
//    cnt++;
//  }
//}