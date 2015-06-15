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
/// @file egolib/Graphics/MD2Model.cpp
/// @author Johan Jansen
/// @brief Logic for loading and parsing MD2 object model files

#include "egolib/Graphics/MD2Model.hpp"
#include "egolib/_math.h"
#include "egolib/bbox.h"

static const float MD2_NORMALS[EGO_NORMAL_COUNT][3] =
{
#include "egolib/FileFormats/id_normals.inl"
    , {0, 0, 0}                     ///< the "equal light" normal
};

MD2Model::MD2Model() :
	_vertices(0),
	_skins(),
	_texCoords(),
	_triangles(),
	_frames(),
	_commands()
{
	//ctor
}

float MD2Model::getMD2Normal(size_t normal, size_t index)
{
	return MD2_NORMALS[normal][index];
}

void MD2Model::scaleModel(const float scaleX, const float scaleY, const float scaleZ)
{
    for(MD2_Frame &frame : _frames)
    {
        bool boundingBoxFound = false;

        for(MD2_Vertex& vertex : frame.vertexList)
        {
            oct_vec_v2_t opos;

            vertex.pos[kX] *= scaleX;
            vertex.pos[kY] *= scaleY;
            vertex.pos[kZ] *= scaleZ;

            std::copysign(vertex.nrm[kX], scaleX);
            std::copysign(vertex.nrm[kY], scaleY);
            std::copysign(vertex.nrm[kZ], scaleZ);

			vertex.nrm.normalize();

            opos.ctor(vertex.pos);

            // Re-calculate the bounding box for this frame
            if (!boundingBoxFound)
            {
                oct_bb_set_ovec(&frame.bb, opos);
                boundingBoxFound = true;
            }
            else
            {
                frame.bb.join(opos);
            }
        }

        // we don't really want objects that have extent in more than one
        // dimension to be called empty
        if (frame.bb.empty)
        {
            if ( std::abs(frame.bb.maxs[OCT_X] - frame.bb.mins[OCT_X] ) +
                 std::abs(frame.bb.maxs[OCT_Y] - frame.bb.mins[OCT_Y] ) +
                 std::abs(frame.bb.maxs[OCT_Z] - frame.bb.mins[OCT_Z] ) > 0.0f )
            {
                oct_vec_v2_t ovec;

                ovec[OCT_X] = ovec[OCT_Y] = ovec[OCT_Z] = 1e-6;
                ovec[OCT_XY] = ovec[OCT_YX] = Ego::Math::sqrtTwo<float>() * ovec[OCT_X];
                oct_bb_self_grow(frame.bb, ovec);
            }
        }
    }
}

void MD2Model::makeEquallyLit()
{
	for(MD2_Frame &frame : _frames)
	{
	    for(MD2_Vertex &vertex : frame.vertexList)
	    {
	        vertex.normal = EGO_NORMAL_COUNT-1;
	    }
	}
}

std::shared_ptr<MD2Model> MD2Model::loadFromFile(const std::string &fileName)
{
    id_md2_header_t md2Header;

    // Open up the file, and make sure it's a MD2 model
    vfs_FILE *f = vfs_openRead(fileName);
    if(!f)
    {
        log_warning("MD2Model::loadFromFile() - could not open model (%s)\n", fileName.c_str());
        return nullptr;
    }

    vfs_read(&md2Header, sizeof(md2Header), 1, f);

    // Convert the byte ordering in the md2Header, if we need to
    md2Header.ident            = ENDIAN_TO_SYS_INT32( md2Header.ident );
    md2Header.version          = ENDIAN_TO_SYS_INT32( md2Header.version );
    md2Header.skinwidth        = ENDIAN_TO_SYS_INT32( md2Header.skinwidth );
    md2Header.skinheight       = ENDIAN_TO_SYS_INT32( md2Header.skinheight );
    md2Header.framesize        = ENDIAN_TO_SYS_INT32( md2Header.framesize );
    md2Header.num_skins        = ENDIAN_TO_SYS_INT32( md2Header.num_skins );
    md2Header.num_vertices     = ENDIAN_TO_SYS_INT32( md2Header.num_vertices );
    md2Header.num_st           = ENDIAN_TO_SYS_INT32( md2Header.num_st );
    md2Header.num_tris         = ENDIAN_TO_SYS_INT32( md2Header.num_tris );
    md2Header.size_glcmds      = ENDIAN_TO_SYS_INT32( md2Header.size_glcmds );
    md2Header.num_frames       = ENDIAN_TO_SYS_INT32( md2Header.num_frames );
    md2Header.offset_skins     = ENDIAN_TO_SYS_INT32( md2Header.offset_skins );
    md2Header.offset_st        = ENDIAN_TO_SYS_INT32( md2Header.offset_st );
    md2Header.offset_tris      = ENDIAN_TO_SYS_INT32( md2Header.offset_tris );
    md2Header.offset_frames    = ENDIAN_TO_SYS_INT32( md2Header.offset_frames );
    md2Header.offset_glcmds    = ENDIAN_TO_SYS_INT32( md2Header.offset_glcmds );
    md2Header.offset_end       = ENDIAN_TO_SYS_INT32( md2Header.offset_end );

    if (md2Header.ident != MD2_MAGIC_NUMBER || md2Header.version != MD2_VERSION)
    {
        vfs_close( f );
        log_warning( "MD2Model::loadFromFile() - model does not have valid header or identifier (%s)\n", fileName.c_str() );
        return NULL;
    }

    // Allocate a MD2_Model_t to hold all this stuff
    std::shared_ptr<MD2Model> model = std::make_shared<MD2Model>();
    if(!model)
    {
        log_error( "MD2Model::loadFromFile() - could create MD2Model\n" );
        return NULL;
    }

    //Allocate memory for the data
    model->_vertices = md2Header.num_vertices;
    model->_texCoords.resize(md2Header.num_st);
    model->_triangles.resize(md2Header.num_tris);
    model->_skins.resize(md2Header.num_skins);
    model->_frames.resize(md2Header.num_frames);

    for(MD2_Frame &frame : model->_frames)
    {
    	frame.vertexList.resize(md2Header.num_vertices);
    }

    // Load the texture coordinates from the file, normalizing them as we go
    vfs_seek(f, md2Header.offset_st);
    for(MD2_TexCoord& texCoord : model->_texCoords)
    {
        id_md2_texcoord_t tc;
        vfs_read(&tc, sizeof(tc), 1, f);

        // auto-convert the byte ordering of the texture coordinates
        tc.s = ENDIAN_TO_SYS_INT16( tc.s );
        tc.t = ENDIAN_TO_SYS_INT16( tc.t );

        texCoord.tex[SS] = tc.s / static_cast<float>(md2Header.skinwidth);
        texCoord.tex[TT] = tc.t / static_cast<float>(md2Header.skinheight);
    }

    // Load triangles from the file.  I use the same memory layout as the file
    // on a little endian machine, so they can just be read directly
    vfs_seek(f, md2Header.offset_tris);
    vfs_read(model->_triangles.data(), sizeof(id_md2_triangle_t), md2Header.num_tris, f);

    // auto-convert the byte ordering on the triangles
    for(MD2_Triangle &tris : model->_triangles)
    {
        for (size_t v = 0; v < 3; ++v)
        {
            tris.vertex[v] = ENDIAN_TO_SYS_INT16(tris.vertex[v]);
            tris.st[v]     = ENDIAN_TO_SYS_INT16(tris.st[v]);
        }
    }

    // Load the skin names.  Again, I can load them directly
    vfs_seek(f, md2Header.offset_skins);
    vfs_read(model->_skins.data(), sizeof(id_md2_skin_t), md2Header.num_skins, f);

    // Load the frames of animation
    vfs_seek(f, md2Header.offset_frames);
    for(MD2_Frame &frame : model->_frames)
    {
        id_md2_frame_header_t frame_header;

        // read the current frame
        vfs_read(&frame_header, sizeof(frame_header), 1, f);

        // Convert the byte ordering on the scale & translate vectors, if necessary
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
        frame_header.scale[0]     = ENDIAN_TO_SYS_IEEE32( frame_header.scale[0] );
        frame_header.scale[1]     = ENDIAN_TO_SYS_IEEE32( frame_header.scale[1] );
        frame_header.scale[2]     = ENDIAN_TO_SYS_IEEE32( frame_header.scale[2] );

        frame_header.translate[0] = ENDIAN_TO_SYS_IEEE32( frame_header.translate[0] );
        frame_header.translate[1] = ENDIAN_TO_SYS_IEEE32( frame_header.translate[1] );
        frame_header.translate[2] = ENDIAN_TO_SYS_IEEE32( frame_header.translate[2] );
#endif

        // unpack the md2 vertex_lst from this frame
        bool boundingBoxFound = false;
        for(MD2_Vertex &vertex : frame.vertexList)
        {
            oct_vec_v2_t ovec;
            id_md2_vertex_t frame_vert;

            // read vertex_lst one-by-one. I hope this is not endian dependent, but I have no way to check it.
            vfs_read(&frame_vert, sizeof( id_md2_vertex_t ), 1, f);

            // grab the vertex position
            vertex.pos[kX] = frame_vert.v[0] * frame_header.scale[0] + frame_header.translate[0];
            vertex.pos[kY] = frame_vert.v[1] * frame_header.scale[1] + frame_header.translate[1];
            vertex.pos[kZ] = frame_vert.v[2] * frame_header.scale[2] + frame_header.translate[2];

            // grab the normal index
            vertex.normal = frame_vert.normalIndex;
            if (vertex.normal > MD2_MAX_NORMALS) {
            	vertex.normal = MD2_MAX_NORMALS;
            }

            // expand the normal index into an actual normal
            vertex.nrm[kX] = MD2_NORMALS[frame_vert.normalIndex][0];
            vertex.nrm[kY] = MD2_NORMALS[frame_vert.normalIndex][1];
            vertex.nrm[kZ] = MD2_NORMALS[frame_vert.normalIndex][2];

            // Calculate the bounding box for this frame
            ovec.ctor(vertex.pos);
            if (!boundingBoxFound)
            {
                oct_bb_set_ovec(&frame.bb, ovec);
                boundingBoxFound = true;
            }
            else
            {
                frame.bb.join(ovec);
            }
        }

        //make sure to copy the frame name!
        strncpy(frame.name, frame_header.name, 16);
    }

    //Load up the pre-computed OpenGL optimizations
    if (md2Header.size_glcmds > 0)
    {
        //uint32_t cmd_cnt  = 0;
        int32_t  cmd_size = 0;

        // seek to the ogl command offset
        vfs_seek(f, md2Header.offset_glcmds);

        //count the commands
        cmd_size = 0;
        while (cmd_size < md2Header.size_glcmds)
        {
            int32_t commands;

            vfs_read( &commands, sizeof(int32_t), 1, f );
            cmd_size += sizeof(int32_t) / sizeof(int32_t);

            // auto-convert the byte ordering
            commands = ENDIAN_TO_SYS_INT32( commands );

            if ( 0 == commands || cmd_size == md2Header.size_glcmds ) break;

            MD2_GLCommand cmd;
            cmd.commandCount = commands;

            //set the GL drawing mode
            if (cmd.commandCount > 0)
            {
                cmd.glMode = GL_TRIANGLE_STRIP;
            }
            else
            {
                cmd.commandCount = -cmd.commandCount;
                cmd.glMode = GL_TRIANGLE_FAN;
            }

            //allocate the data
            cmd.data.resize(cmd.commandCount);

            //read in the data
            vfs_read(cmd.data.data(), sizeof(id_glcmd_packed_t), cmd.commandCount, f);
            cmd_size += (sizeof(id_glcmd_packed_t) * cmd.commandCount) / sizeof(uint32_t);

            //translate the data, if necessary
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
            for(id_glcmd_packed_t &cmdData : cmd.data)
            {
                cmdData.index = SDL_Swap32( cmdData.s );
                cmdData.s     = ENDIAN_TO_SYS_IEEE32( cmdData.s );
                cmdData.t     = ENDIAN_TO_SYS_IEEE32( cmdData.t );
            }
#endif

            // attach it to the command list
            model->_commands.push_front(cmd);

            //cmd_cnt += cmd.commandCount;
        }

        //model->_numCommands = cmd_cnt;
    }

    // Close the file, we're done with it
    vfs_close(f);

    return model;
}
