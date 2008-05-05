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

#ifndef egoboo_Md2_h
#define egoboo_Md2_h


typedef struct Md2Vertex
{
	float x, y, z;
	unsigned normal;	// index to id-normal array
}Md2Vertex;

typedef struct Md2TexCoord
{
	float s, t;
}Md2TexCoord;

typedef struct Md2Triangle
{
	short vertexIndices[3];
	short texCoordIndices[3];
}Md2Triangle;

typedef struct Md2Frame
{
	char name[16];
	float min[3], max[3];		// axis-aligned bounding box limits
	Md2Vertex *vertices;
}Md2Frame;

typedef struct Md2SkinName
{
	char name[64];
}Md2SkinName;

typedef struct Md2Model
{
	int numVertices;
	int numTexCoords;
	int numTriangles;
	int numSkins;
	int numFrames;

	Md2SkinName *skins;
	Md2TexCoord *texCoords;
	Md2Triangle *triangles;
	Md2Frame    *frames;
}Md2Model;

extern Md2Model *md2_loadFromFile(const char *fileName);
extern void      md2_freeModel(Md2Model *model);

#endif // include guard
