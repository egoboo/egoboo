/* Tactics - id_md2.h
 * Quake 2 model file loading structures & constants
 */

#ifndef Tactics_id_md2
#define Tactics_id_md2

struct md2_header
{
    int magic; // should be 844121161( or "IDP2")
    int version; // should be 8
    int skinWidth;
    int skinHeight;
    int frameSize;
    int numSkins;
    int numVertices;
    int numTexCoords;
    int numTriangles;
    int numGlCommands;
    int numFrames;
    int offsetSkins;
    int offsetTexCoords;
    int offsetTriangles;
    int offsetFrames;
    int offsetGlCommands;
    int offsetEnd;
};

struct md2_skinname
{
    char name[64];
};

struct md2_vertex
{
    unsigned char vertex[3];
    unsigned char  lightNormalIndex;
};

struct md2_texcoord
{
    short s, t;
};

struct md2_triangle
{
    short vertexIndices[3];
    short texCoordIndices[3];
};

struct md2_frame
{
    float scale[3];
    float translate[3];
    char name[16];
    struct md2_vertex vertices[1];
};

enum Md2Constant
{
	MD2_MAX_TRIANGLES	= 4096,
	MD2_MAX_VERTICES	= 2048,
	MD2_MAX_TEXCOORDS	= 2048,
	MD2_MAX_FRAMES		= 512,
	MD2_MAX_SKINS		= 32,
	MD2_MAX_FRAMESIZE	= (MD2_MAX_VERTICES * 4 + 128),
	MD2_MAGIC_NUMBER	= 844121161,
	MD2_VERSION			= 8
};

extern float kMd2Normals[][3];

#endif
