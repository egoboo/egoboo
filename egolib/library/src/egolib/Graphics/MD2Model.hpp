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
/// @file egolib/Graphics/MD2Model.hpp
/// @author Johan Jansen
/// @brief Logic for loading and parsing MD2 object model files
#pragma once

#include "egolib/FileFormats/id_md2.h"
#include "egolib/bbox.h"
#include <forward_list>

typedef id_md2_skin_t MD2_SkinName;
typedef id_md2_triangle_t MD2_Triangle;

class MD2_Vertex
{
public:
	MD2_Vertex() :
		pos(0.0f, 0.0f, 0.0f),
		nrm(0.0f, 0.0f, 0.0f),
		normal(0)
	{
		//ctor
	}

	Ego::Vector3f pos;
	Ego::Vector3f nrm;
    size_t   normal;  ///< index to id-normal array
};

class MD2_TexCoord
{
public:
    MD2_TexCoord() :
    	tex(0.0f, 0.0f)
    {
    	//ctor
    }

	Ego::Vector2f tex;
};

class MD2_GLCommand
{
public:
	MD2_GLCommand() :
	    glMode(0),
	    commandCount(0),
	    data()
	{
		//ctor
	}

    GLenum         					glMode;
    int32_t        					commandCount;
    std::vector<id_glcmd_packed_t> 	data;
};

class MD2_Frame
{
public:
	MD2_Frame() :
#if 0
		name(),
#endif
		vertexList(),
		bb(),
		framelip(0),
		framefx(EMPTY_BIT_FIELD)
	{
		name[0] = '\0';
	}

    char name[16];

    std::vector<MD2_Vertex> vertexList;

    oct_bb_t bb;        ///< axis-aligned octagonal bounding box limits
    int framelip;       ///< the position in the current animation
    BIT_FIELD framefx;  ///< the special effects associated with this frame
};

class MD2Model
{
public:
    static constexpr size_t normalCount = MD2_MAX_NORMALS + 1;
	MD2Model();

	inline const std::vector<MD2_SkinName>&  	   getSkins() const {return _skins;}
	inline std::vector<MD2_Frame>&     	  	 	   getFrames() {return _frames;}
	inline const std::vector<MD2_Triangle>&  	   getTriangles() const {return _triangles;}
	inline const std::forward_list<MD2_GLCommand>& getGLCommands() const {return _commands;}
	inline size_t 								   getVertexCount() const {return _vertices;}

	/**
    * @author BB
    * @details scale every vertex in the md2 by the given amount
    **/
	void scaleModel(const float scaleX, const float scaleY, const float scaleZ);

	/**
	*
	**/
	void makeEquallyLit();

	/**
	* @return number of optimized OpenGL commands stored for rendering this model
	**/
	//size_t getCommandCount() const {return _numCommands;}

	static std::shared_ptr<MD2Model> loadFromFile(const std::string &fileName);

	static float getMD2Normal(size_t normal, size_t index);

private:
	size_t 					   	     _vertices;
    std::vector<MD2_SkinName>  	     _skins;
    std::vector<MD2_TexCoord>  	     _texCoords;
    std::vector<MD2_Triangle>  	     _triangles;
    std::vector<MD2_Frame>     	     _frames;
    std::forward_list<MD2_GLCommand> _commands;
    //size_t							 _numCommands;
};
