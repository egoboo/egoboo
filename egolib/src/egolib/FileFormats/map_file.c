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

/// @file egolib/FileFormats/map_file.c
/// @brief Functions for raw read and write access to the .mpd file type
/// @details

#include "egolib/FileFormats/map_file.h"
#include "egolib/FileFormats/map_file-v1.h"
#include "egolib/FileFormats/map_file-v2.h"
#include "egolib/FileFormats/map_file-v3.h"
#include "egolib/FileFormats/map_file-v4.h"
#include "egolib/vfs.h"

#include "egolib/FileFormats/map_tile_dictionary.h"

#include "egolib/map_functions.h"

#include "egolib/log.h"

#include "egolib/endian.h"
#include "egolib/fileutil.h"
#include "egolib/strutil.h"

#include "egolib/_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define CURRENT_MAP_VERSION_NUMBER (( CURRENT_MAP_VERSION_LETTER - 'A' ) + 1 )
#define CURRENT_MAP_ID             ( MAP_ID_BASE + (CURRENT_MAP_VERSION_LETTER - 'A') )
#define GET_MAP_VERSION_NUMBER(VAL) LAMBDA( (static_cast<uint32_t>(VAL)) >= (static_cast<uint32_t>(MAP_ID_BASE)), (static_cast<uint32_t>(VAL)) - (static_cast<uint32_t>(MAP_ID_BASE)) + 1, -1 )

//--------------------------------------------------------------------------------------------

bool map_t::setInfo(const map_info_t& info)
{
    // Validate the map info.
    if (!info.validate())
    {
        setInfo();
        return false;
    }

    // Allocate the map's tile and vertex memory.
    _mem.setInfo(info);

    // Store the map info.
    _info = info;

    return true;
}

//--------------------------------------------------------------------------------------------

void map_info_t::reset()
{
    _vertexCount = 0;
    _tileCountX = 0;
    _tileCountY = 0;
}

void map_info_t::load(vfs_FILE& file)
{
    // Read the vertex count.
    vfs_read_Uint32(&file, &_vertexCount);

    // Read the tile count in the x direction.
    vfs_read_Uint32(&file, &_tileCountX);

    // Read the tile count in the y direction.
    vfs_read_Uint32(&file, &_tileCountY);
}

void map_info_t::save(vfs_FILE& file) const
{
    // Write the vertex count.
    vfs_write_Uint32(&file, _vertexCount);

    // Write the tile count in the x direction.
    vfs_write_Uint32(&file, _tileCountX);

    // Write the tile count in the y direction.
    vfs_write_Uint32(&file, _tileCountY);
}

bool map_info_t::validate() const
{
    if (_vertexCount > MAP_VERTICES_MAX)
    {
        log_warning("%s:%d: too many vertices (%u/%u)!!\n", __FILE__, __LINE__, _vertexCount, MAP_VERTICES_MAX);
        return false;
    }

    if (_tileCountX > MAP_TILE_MAX_X)
    {
        log_warning("%s:%d: too many tiles in the x direction (%u/%u)!!\n", __FILE__, __LINE__, _tileCountX, MAP_TILE_MAX_X);
        return false;
    }

    if (_tileCountY > MAP_TILE_MAX_Y)
    {
        log_warning("%s:%d: too many tiles in the y direction (%u/%u)!!\n", __FILE__, __LINE__, _tileCountY, MAP_TILE_MAX_Y);
        return false;
    }

    uint32_t tileCount = _tileCountX * _tileCountY;
    if (tileCount >= MAP_TILE_MAX)
    {
        log_warning("%s:%d: - unknown version and mesh is too large (%u/%u)!!\n", __FILE__, __LINE__, tileCount, MAP_TILE_MAX);
        return false;
    }

    return true;
}

map_info_t::map_info_t(uint32_t vertexCount, uint32_t tileCountX, uint32_t tileCountY)
	: _vertexCount(vertexCount), _tileCountX(tileCountX), _tileCountY(tileCountY)
{}

map_info_t::map_info_t()
	: map_info_t(0, 0, 0)
{ }

map_info_t::map_info_t(const map_info_t& other)
    : map_info_t(other._vertexCount, other._tileCountX, other._tileCountY)
{ }

map_info_t& map_info_t::operator=(const map_info_t& other)
{
    _vertexCount = other._vertexCount;
    _tileCountX = other._tileCountX;
    _tileCountY = other._tileCountY;
    return *this;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

map_mem_t::map_mem_t() :
    tiles(), vertices()
{}

map_mem_t::map_mem_t(const map_info_t& info)
	: tiles(info.getTileCountX() * info.getTileCountY()), vertices(info.getVertexCount())
{}

map_mem_t::~map_mem_t()
{}

void map_mem_t::setInfo(const map_info_t& info)
{
	tiles.resize(info.getTileCountX() * info.getTileCountY());
	vertices.resize(info.getVertexCount());
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool map_t::load(vfs_FILE& file)
{
    // Read the file version.
    Uint32 version;
    vfs_read_Uint32(&file, &version);
    version = SDL_Swap32(version); // This number is backwards for our purpose.
    int mapVersion = GET_MAP_VERSION_NUMBER(version);

    bool validate = false;
    try
    {
        if (mapVersion <= 0)
        {
            log_warning("%s - unknown map type!!\n", __FUNCTION__);
            goto Fail;
        }
        else if (mapVersion > CURRENT_MAP_VERSION_NUMBER)
        {
            log_warning("%s - file version is too recent or invalid. Not all features will be supported %d/%d.\n", __FUNCTION__, mapVersion, CURRENT_MAP_VERSION_NUMBER);
            validate = true;
        }

        // Read the header.
        map_info_t loc_info;
        loc_info.load(file);

        // Validate the header if rerquired.
        if (validate && !loc_info.validate())
        {
            goto Fail;
        }

        // Allocate the mesh memory.
        if (!setInfo(loc_info))
        {
            log_warning("%s - could not initialize the map!!\n", __FUNCTION__);
            goto Fail;
        }

        // version 1 data is required
        if (mapVersion > 0)
        {
            if (!map_read_v1(file, *this))
            {
                goto Fail;
            }
        }

        // version 2 data is optional-ish
        if (mapVersion > 1)
        {
            if (!map_read_v2(file, *this))
            {
                goto Fail;
            }
        }
        else
        {
            if (!map_generate_tile_twist_data(this))
            {
                goto Fail;
            }
        }

        // version 3 data is optional-ish
        if (mapVersion > 2)
        {
            if (!map_read_v3(file, *this))
            {
                goto Fail;
            }
        }
        else
        {
            if (!map_generate_fan_type_data(this))
            {
                goto Fail;
            }
            if (!map_generate_vertex_data(this))
            {
                goto Fail;
            }
        }

        // version 4 data is completely optional
        if (mapVersion > 3)
        {
            if (!map_read_v4(file, *this))
            {
                goto Fail;
            }
        }
    }
    catch (std::exception&)
    {
        goto Fail;
    }
    return true;
Fail:
    setInfo();
    return false;
}

bool map_t::load(const std::string& name)
{
    vfs_FILE *file = vfs_openRead(name.c_str());
    if (!file)
    {
        log_warning("%s:%d: cannot find \"%s\"!!\n", __FILE__, __LINE__, name.c_str());
        goto Fail;
    }

    if (!load(*file))
    {
        goto Fail;
    }

    vfs_close(file);

    return true;

Fail:

    setInfo();

    if (file)
    {
        vfs_close(file);
        file = nullptr;
    }

    return false;
}

bool map_t::save(vfs_FILE& file) const
{
    int mapVersion = CURRENT_MAP_VERSION_NUMBER;
    // write the file identifier
    vfs_write_Uint32(&file, SDL_Swap32(CURRENT_MAP_ID));

    // write the map info
    _info.save(file);

    if (mapVersion > 0)
    {
        if (!map_write_v1(file, *this))
        {
            return false;
        }
    }

    if (mapVersion > 1)
    {
        if (!map_write_v2(file, *this))
        {
            return false;
        }
    }

    if (mapVersion > 2)
    {
        if (!map_write_v3(file, *this))
        {
            return false;
        }
    }

    if (mapVersion > 3)
    {
        if (!map_write_v4(file, *this))
        {
            return false;
        }
    }
    return true;
}

bool map_t::save(const std::string& name) const
{
    vfs_FILE *file = vfs_openWrite(name.c_str());
    if (!file)
    {
        return false;
    }

    if (!save(*file))
    {
        vfs_close(file);
        return false;
    }

    vfs_close(file);

    return true;
}

map_t::map_t(const map_info_t& info) :
    _info(info), _mem(info)
{}

map_t::~map_t()
{}