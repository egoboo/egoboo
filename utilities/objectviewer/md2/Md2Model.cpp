//
//  Md2Model.cpp
//  EOV
//
//  Created by Jonathan Fischer on 1/18/10.
//  Copyright 2010, 2015 Jonathan Fisher, Egoboo Team. All rights reserved.
//

#include "Md2Model.h"
#include "id_md2.h"
#include <QtEndian>
#include <QIODevice>
#include <QOpenGLFunctions_1_1>

static GLfloat blendVertices[MD2_MAX_VERTICES][3];
static GLfloat blendNormals[MD2_MAX_VERTICES][3];

inline float LittleFloatToHost(float q)
{
    union
    {
        float f;
        quint32 u;
    } a;
    a.f = q;
    a.u = qFromLittleEndian(a.u);
    return a.f;
}


Md2Model::~Md2Model()
{
    if (texCoords) delete[] texCoords;
    if (triangles) delete[] triangles;
	if (frames) {
		int i;
		for (i = 0;i < numFrames; i++) {
            if (frames[i].vertices) delete[] frames[i].vertices;
		}
		
        delete[] frames;
    }
}

const Md2TexCoord* Md2Model::texCoordAtIndex(int i)
{
    if (i < 0 || i >= numTexCoords)
        return NULL;
    else
        return texCoords + i;
}

const Md2Triangle* Md2Model::triangleAtIndex(int i)
{
    if (i < 0 || i >= numTriangles)
        return NULL;
    else
        return triangles + i;
}

const Md2Frame* Md2Model::frameAtIndex(int i)
{
    if (i < 0 || i >= numFrames)
        return NULL;
    else
        return frames + i;
}

Md2Model *Md2Model::loadFromFile(QIODevice *file)
{
	struct md2_header header;
    quint64 pos;
	char frame_buffer[MD2_MAX_FRAMESIZE];
	Md2Model *model;
	int i;
	
	// Copy out the file's header, and make sure that it's a valid MD2 file
    file->read((char *)&header, sizeof(struct md2_header));

	// Make sure the byte ordering of the header fields is correct
    header.magic = qFromLittleEndian(header.magic);
    header.version = qFromLittleEndian(header.version);
	
	// Make sure it's an MD2 file
	if (header.magic != MD2_MAGIC_NUMBER || header.version != MD2_VERSION)
	{
        return NULL;
	}
	
	// Go ahead and make sure the rest of the header values are correct
    header.skinWidth = qFromLittleEndian(header.skinWidth);
    header.skinHeight = qFromLittleEndian(header.skinHeight);
    header.frameSize = qFromLittleEndian(header.frameSize);
    header.numSkins = qFromLittleEndian(header.numSkins);
    header.numVertices = qFromLittleEndian(header.numVertices);
    header.numTexCoords = qFromLittleEndian(header.numTexCoords);
    header.numTriangles = qFromLittleEndian(header.numTriangles);
    header.numGlCommands = qFromLittleEndian(header.numGlCommands);
    header.numFrames = qFromLittleEndian(header.numFrames);
    header.offsetSkins = qFromLittleEndian(header.offsetSkins);
    header.offsetTexCoords = qFromLittleEndian(header.offsetTexCoords);
    header.offsetTriangles = qFromLittleEndian(header.offsetTriangles);
    header.offsetFrames = qFromLittleEndian(header.offsetFrames);
    header.offsetGlCommands = qFromLittleEndian(header.offsetGlCommands);
    header.offsetEnd = qFromLittleEndian(header.offsetEnd);
	
	// Allocate memory to hold the model
    model = new Md2Model();
	model->numVertices = header.numVertices;
	model->numTexCoords = header.numTexCoords;
    model->numTriangles = header.numTriangles;
	model->numFrames = header.numFrames;
	
    model->texCoords = new Md2TexCoord[header.numTexCoords]();
    model->triangles = new Md2Triangle[header.numTriangles]();
    model->frames    = new Md2Frame[header.numFrames]();
	
	// Allocate memory for each frame's vertices as well
	for (i = 0;i < header.numFrames; i++)
	{
        model->frames[i].vertices = new Md2Vertex[header.numVertices]();
	}
	
    // Start loading data from the file.
    // Texture coordinates first
    pos = header.offsetTexCoords;
	for (i = 0;i < header.numTexCoords; i++)
	{
		// Convert the texture coordinates to normalized floats while
		// loading them
        struct md2_texcoord tc;
        file->seek(pos);
        file->read((char *)&tc, sizeof(md2_texcoord));
		
        tc.s = qFromLittleEndian(tc.s);
        tc.t = qFromLittleEndian(tc.t);
		
		model->texCoords[i].s = tc.s / (float)header.skinWidth;
		model->texCoords[i].t = tc.t / (float)header.skinHeight;
		
        pos += sizeof(struct md2_texcoord);
	}
	
	// Triangles can be loaded directly; their format on disk is the
    // same as the format in memory
    file->seek(header.offsetTriangles);
    file->read((char *)model->triangles, sizeof(Md2Triangle) * header.numTriangles);
	
	// Byte-swap the triangles too
	for (int i = 0;i < header.numTriangles; i++)
	{
		for (int v = 0;v < 3; v++)
		{
            model->triangles[i].vertexIndices[v] = qFromLittleEndian(model->triangles[i].vertexIndices[v]);
            model->triangles[i].texCoordIndices[v] = qFromLittleEndian(model->triangles[i].texCoordIndices[v]);
		}
	}
	
	// Last, load the frames of animation
    pos = header.offsetFrames;
	for (int i = 0;i < header.numFrames; i++)
	{
		struct md2_frame *frame;
		
        // Read the current frame
        file->seek(pos);
        file->read(frame_buffer, header.frameSize);
		frame = (struct md2_frame*)frame_buffer;
		
		// Byte-swap the scale and translate vectors
		frame->scale[0] = LittleFloatToHost(frame->scale[0]);
		frame->scale[1] = LittleFloatToHost(frame->scale[1]);
		frame->scale[2] = LittleFloatToHost(frame->scale[2]);
		
		frame->translate[0] = LittleFloatToHost(frame->translate[0]);
		frame->translate[1] = LittleFloatToHost(frame->translate[1]);
		frame->translate[2] = LittleFloatToHost(frame->translate[2]);
		
		memcpy(model->frames[i].name, frame->name, 16);
		
		// Unpack the vertices for this frame
		for (int v = 0; v < header.numVertices; v++)
		{
			model->frames[i].vertices[v].x =
			frame->vertices[v].vertex[0] * frame->scale[0] + frame->translate[0];
			model->frames[i].vertices[v].y =
			frame->vertices[v].vertex[1] * frame->scale[1] + frame->translate[1];
			model->frames[i].vertices[v].z =
			frame->vertices[v].vertex[2] * frame->scale[2] + frame->translate[2];
			
			model->frames[i].vertices[v].normal = frame->vertices[v].lightNormalIndex;
		}
		
		// Calculate the bounding box for this frame
		model->frames[i].min[0] = frame->translate[0];
		model->frames[i].min[1] = frame->translate[1];
		model->frames[i].min[2] = frame->translate[2];
		model->frames[i].max[0] = frame->translate[0] + (frame->scale[0] * 255.0f);
		model->frames[i].max[1] = frame->translate[1] + (frame->scale[1] * 255.0f);
		model->frames[i].max[2] = frame->translate[2] + (frame->scale[2] * 255.0f);
		
        pos += header.frameSize;
	}

	// Done
	return model;
}

void Md2Model::blendFrames(int frame1, int frame2, float lerp)
{
	int i;
	const Md2Frame *from, *to;
	
    from = frameAtIndex(frame1);
    to = frameAtIndex(frame2);
	
	if(lerp <= 0)
	{
		// copy the vertices in frame 'from' over
		for(i = 0;i < numVertices;i++)
		{
			blendVertices[i][0] = from->vertices[i].x;
			blendVertices[i][1] = from->vertices[i].y;
			blendVertices[i][2] = from->vertices[i].z;
			
			blendNormals[i][0] = kMd2Normals[from->vertices[i].normal][0];
			blendNormals[i][1] = kMd2Normals[from->vertices[i].normal][1];
			blendNormals[i][2] = kMd2Normals[from->vertices[i].normal][2];
		}
	} else if(lerp >= 1.0f)
	{
		// copy the vertices in frame 'to'
		for(i = 0;i < numVertices;i++)
		{
			blendVertices[i][0] = to->vertices[i].x;
			blendVertices[i][1] = to->vertices[i].y;
			blendVertices[i][2] = to->vertices[i].z;
			
			blendNormals[i][0] = kMd2Normals[to->vertices[i].normal][0];
			blendNormals[i][1] = kMd2Normals[to->vertices[i].normal][1];
			blendNormals[i][2] = kMd2Normals[to->vertices[i].normal][2];
		}
	} else
	{
		// mix the vertices
		for(i = 0;i < numVertices;i++)
		{
			blendVertices[i][0] = from->vertices[i].x +
			(to->vertices[i].x - from->vertices[i].x) * lerp;
			blendVertices[i][1] = from->vertices[i].y +
			(to->vertices[i].y - from->vertices[i].y) * lerp;
			blendVertices[i][2] = from->vertices[i].z +
			(to->vertices[i].z - from->vertices[i].z) * lerp;
			
			blendNormals[i][0] = kMd2Normals[from->vertices[i].normal][0] +
			(kMd2Normals[to->vertices[i].normal][0] - kMd2Normals[from->vertices[i].normal][0]) * lerp;
			blendNormals[i][0] = kMd2Normals[from->vertices[i].normal][1] +
			(kMd2Normals[to->vertices[i].normal][1] - kMd2Normals[from->vertices[i].normal][1]) * lerp;
			blendNormals[i][0] = kMd2Normals[from->vertices[i].normal][2] +
			(kMd2Normals[to->vertices[i].normal][2] - kMd2Normals[from->vertices[i].normal][2]) * lerp;
		}
	}
}

void Md2Model::drawBlendedFrames(int frame1, int frame2, float lerp)
{
	if (frame1 < 0 || frame1 >= numFrames) return;
	if (frame2 < 0 || frame2 >= numFrames) return;
	
    blendFrames(frame1, frame2, lerp);
	
	int i;
	Md2Triangle *tri;
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	
	glVertexPointer(3, GL_FLOAT, 0, blendVertices);
	glNormalPointer(GL_FLOAT, 0, blendNormals);
	
	glBegin(GL_TRIANGLES);
	{
		for(i = 0;i < numTriangles;i++)
		{
			tri = &triangles[i];
			
			glTexCoord2fv((const GLfloat*)&(texCoords[tri->texCoordIndices[0]]));
			glArrayElement(tri->vertexIndices[0]);
			
			glTexCoord2fv((const GLfloat*)&(texCoords[tri->texCoordIndices[1]]));
			glArrayElement(tri->vertexIndices[1]);
			
			glTexCoord2fv((const GLfloat*)&(texCoords[tri->texCoordIndices[2]]));
			glArrayElement(tri->vertexIndices[2]);
		}
	}
	glEnd();
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}

void Md2Model::drawFrame(int frame)
{
    drawBlendedFrames(frame, frame, 0);
}
