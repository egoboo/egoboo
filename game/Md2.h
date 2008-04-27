/* Egoboo - Md2.h
 * This code is not currently in use.
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

/* Adapted from "Tactics - MD2_Model.h" by Jonathan Fischer
 * A class for loading/using Quake 2 and Egoboo md2 models.
 *
 * Creating/destroying objects of this class is done in the same fashion as
 * Textures, so see Texture.h for details.
 */

#ifndef egoboo_Md2_h
#define egoboo_Md2_h


#include "id_md2.h"
#include <SDL_opengl.h>
#include <memory.h>

#pragma pack(push,1)
typedef struct ego_md2_vertex_t
{
  float x, y, z;
  unsigned normal;  // index to id-normal array
} MD2_Vertex;

typedef struct ego_md2_texcoord_t
{
  float s, t;
} MD2_TexCoord;

typedef struct ego_md2_frame_t
{
  char name[16];
  float bbmin[3], bbmax[3];    // axis-aligned bounding box limits
  MD2_Vertex *vertices;
} MD2_Frame;
#pragma pack(pop)

typedef md2_triangle MD2_Triangle;
typedef md2_skinname MD2_SkinName;

typedef struct ego_md2_glcommand_t MD2_GLCommand;

struct ego_md2_glcommand_t
{
  MD2_GLCommand * next;
  GLenum          gl_mode;
  signed int      command_count;
  md2_gldata    * data;
};

void MD2_GLCommand_construct(MD2_GLCommand * m);
void MD2_GLCommand_destruct(MD2_GLCommand * m);

MD2_GLCommand * MD2_GLCommand_new();
MD2_GLCommand * MD2_GLCommand_new_vector(int n);
void MD2_GLCommand_delete(MD2_GLCommand * m);
void MD2_GLCommand_delete_vector(MD2_GLCommand * v, int n);


typedef struct ego_md2_model_t
{
  int m_numVertices;
  int m_numTexCoords;
  int m_numTriangles;
  int m_numSkins;
  int m_numFrames;
  int m_numCommands;

  MD2_SkinName  *m_skins;
  MD2_TexCoord  *m_texCoords;
  MD2_Triangle  *m_triangles;
  MD2_Frame     *m_frames;
  MD2_GLCommand *m_commands;
} MD2_Model;

void md2_construct(MD2_Model * m);
void md2_destruct(MD2_Model * m);

MD2_Model * md2_new();
MD2_Model * md2_new_vector(int n);
void md2_delete(MD2_Model * m);
void md2_delete_vector(MD2_Model * v, int n);


MD2_Model * md2_load(const char *fileName, MD2_Model* m);

void md2_deallocate(MD2_Model * m);

#define INLINE __inline

INLINE int md2_get_numVertices(MD2_Model * m)  { return m->m_numVertices; }
INLINE int md2_get_numTexCoords(MD2_Model * m) { return m->m_numTexCoords; }
INLINE int md2_get_numTriangles(MD2_Model * m) { return m->m_numTriangles; }
INLINE int md2_get_numSkins(MD2_Model * m)     { return m->m_numSkins; }
INLINE int md2_get_numFrames(MD2_Model * m)    { return m->m_numFrames; }

const MD2_SkinName  *md2_get_Skin(MD2_Model * m, int index);
const MD2_Frame     *md2_get_Frame(MD2_Model * m, int index);
INLINE const MD2_TexCoord  *md2_get_TexCoords(MD2_Model * m) { return m->m_texCoords; }
INLINE const MD2_Triangle  *md2_get_Triangles(MD2_Model * m) { return m->m_triangles; }
INLINE const MD2_GLCommand *md2_get_Commands(MD2_Model * m)  { return m->m_commands;  }

#endif // include guard



