//
//  Md2Model.cpp
//  EOV
//
//  Created by Jonathan Fischer on 1/18/10.
//  Copyright 2010, 2015 Jonathan Fisher, Egoboo Team. All rights reserved.
//

struct Md2Vertex
{
	float x, y, z;
	unsigned normal;	// index to id-normal array
};

struct Md2TexCoord
{
	float s, t;
};

struct Md2Triangle
{
	short vertexIndices[3];
	short texCoordIndices[3];
};

struct Md2Frame
{
	char name[16];
	float min[3], max[3];		// axis-aligned bounding box limits
	Md2Vertex *vertices;
};

class QIODevice;

class Md2Model
{
public:
    static Md2Model *loadFromFile(QIODevice *arr);

    const Md2TexCoord *texCoordAtIndex(int i);
    const Md2Triangle *triangleAtIndex(int i);
    const Md2Frame *frameAtIndex(int i);

    void drawFrame(int frame);
    void drawBlendedFrames(int from, int to, float lerp);
    void drawFrames(int from, int to, float lerp);

    int getNumVertices() { return numVertices; }
    int getNumTexCoords() { return numTexCoords; }
    int getNumTriangles() { return numTriangles; }
    int getNumFrames() { return numFrames; }

    ~Md2Model();

private:

    void blendFrames(int from, int to, float lerp);

    int numVertices;
    int numTexCoords;
    int numTriangles;
    int numFrames;

    Md2TexCoord *texCoords;
    Md2Triangle *triangles;
    Md2Frame *frames;

    Md2Model() {}
};
