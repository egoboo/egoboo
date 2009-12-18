#pragma once

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

///
/// @file
/// @brief
/// @details functions that will be declared inside the base class

#include "Md2.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INLINE EGO_CONST MD2_SkinName_t  *md2_get_Skin    (MD2_Model_t * m, int index);
INLINE EGO_CONST MD2_Frame_t     *md2_get_Frame   (MD2_Model_t * m, int index);
INLINE EGO_CONST MD2_Triangle_t  *md2_get_Triangle(MD2_Model_t * m, int index);

INLINE EGO_CONST int md2_get_numVertices(MD2_Model_t * m);
INLINE EGO_CONST int md2_get_numTexCoords(MD2_Model_t * m);
INLINE EGO_CONST int md2_get_numTriangles(MD2_Model_t * m);
INLINE EGO_CONST int md2_get_numSkins(MD2_Model_t * m);
INLINE EGO_CONST int md2_get_numFrames(MD2_Model_t * m);
INLINE EGO_CONST int md2_get_numCommands(MD2_Model_t * m);

INLINE EGO_CONST MD2_SkinName_t  *md2_get_SkinNames(MD2_Model_t * m);
INLINE EGO_CONST MD2_TexCoord_t  *md2_get_TexCoords(MD2_Model_t * m);
INLINE EGO_CONST MD2_Triangle_t  *md2_get_Triangles(MD2_Model_t * m);
INLINE EGO_CONST MD2_Frame_t     *md2_get_Frames   (MD2_Model_t * m);
INLINE EGO_CONST MD2_GLCommand_t *md2_get_Commands (MD2_Model_t * m);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE EGO_CONST int md2_get_numVertices(MD2_Model_t * m)  { return m->m_numVertices;  }
INLINE EGO_CONST int md2_get_numTexCoords(MD2_Model_t * m) { return m->m_numTexCoords; }
INLINE EGO_CONST int md2_get_numTriangles(MD2_Model_t * m) { return m->m_numTriangles; }
INLINE EGO_CONST int md2_get_numSkins(MD2_Model_t * m)     { return m->m_numSkins;     }
INLINE EGO_CONST int md2_get_numFrames(MD2_Model_t * m)    { return m->m_numFrames;    }
INLINE EGO_CONST int md2_get_numCommands(MD2_Model_t * m)  { return m->m_numCommands;  }

INLINE EGO_CONST MD2_SkinName_t  *md2_get_SkinNames(MD2_Model_t * m) { return m->m_skins;     }
INLINE EGO_CONST MD2_TexCoord_t  *md2_get_TexCoords(MD2_Model_t * m) { return m->m_texCoords; }
INLINE EGO_CONST MD2_Triangle_t  *md2_get_Triangles(MD2_Model_t * m) { return m->m_triangles; }
INLINE EGO_CONST MD2_Frame_t     *md2_get_Frames   (MD2_Model_t * m) { return m->m_frames;    }
INLINE EGO_CONST MD2_GLCommand_t *md2_get_Commands (MD2_Model_t * m) { return m->m_commands;  }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE EGO_CONST MD2_SkinName_t *md2_get_Skin(MD2_Model_t * m, int index)
{
  if(index >= 0 && index < m->m_numSkins)
  {
    return m->m_skins + index;
  }
  return NULL;
}

//--------------------------------------------------------------------------------------------
INLINE EGO_CONST MD2_Frame_t *md2_get_Frame(MD2_Model_t * m, int index)
{
  if(index >= 0 && index < m->m_numFrames)
  {
    return m->m_frames + index;
  }
  return NULL;
}

//--------------------------------------------------------------------------------------------
INLINE EGO_CONST MD2_Triangle_t  *md2_get_Triangle(MD2_Model_t * m, int index)
{
  if(index >= 0 && index < m->m_numTriangles)
  {
    return m->m_triangles + index;
  }
  return NULL;
}
