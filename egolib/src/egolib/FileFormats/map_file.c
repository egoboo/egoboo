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

#include "egolib/Log/_Include.hpp"

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
    vfs_read_Uint32(file, &_vertexCount);

    // Read the tile count in the x direction.
    vfs_read_Uint32(file, &_tileCountX);

    // Read the tile count in the y direction.
    vfs_read_Uint32(file, &_tileCountY);
}

void map_info_t::save(vfs_FILE& file) const
{
    // Write the vertex count.
    vfs_write<Uint32>(file, _vertexCount);

    // Write the tile count in the x direction.
    vfs_write<Uint32>(file, _tileCountX);

    // Write the tile count in the y direction.
    vfs_write<Uint32>(file, _tileCountY);
}

bool map_info_t::validate() const
{
    if (_vertexCount > MAP_VERTICES_MAX)
    {
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "too many vertices - received ",
                                         _vertexCount, ", allowed ", MAP_VERTICES_MAX);
        return false;
    }

    if (_tileCountX > MAP_TILE_MAX_X)
    {
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "too many files in the x direction",
                                         " - received ", _tileCountX, ", allowed ", MAP_TILE_MAX_X);
        return false;
    }

    if (_tileCountY > MAP_TILE_MAX_Y)
    {
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "too many tiles in the y direction ",
                                         " - received ", _tileCountY, ", allowed ", MAP_TILE_MAX_Y);
        return false;
    }

    uint32_t tileCount = _tileCountX * _tileCountY;
    if (tileCount >= MAP_TILE_MAX)
    {
		Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "mesh is too large - received ", tileCount, " vertices, expected not more than ", MAP_TILE_MAX, " vertices", Log::EndOfEntry);
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

const tile_info_t& map_mem_t::operator()(size_t i) const {
	if (i >= tiles.size()) {
		throw std::runtime_error("index out of bounds");
	}
	return tiles[i];
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool map_t::load(vfs_FILE& file)
{
    // Read the file version.
    Uint32 version;
    vfs_read_Uint32(file, &version);
    version = SDL_Swap32(version); // This number is backwards for our purpose.
    int mapVersion = GET_MAP_VERSION_NUMBER(version);

    bool validate = false;
    try
    {
        if (mapVersion <= 0)
        {
            Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unknown map type!", Log::EndOfEntry);
            goto Fail;
        }
        else if (mapVersion > CURRENT_MAP_VERSION_NUMBER)
        {
			Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "file version ", mapVersion, " is "
                                             "greater than highest known file version ", CURRENT_MAP_VERSION_NUMBER,
                                             Log::EndOfEntry);
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
			Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to initialize map", Log::EndOfEntry);
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
            try
            {
                map_generate_tile_twist_data(*this);
            }
            catch (...)
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
            try
            {
                map_generate_fan_type_data(*this);
            }
            catch (...)
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
		Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to find map file", "`", name, "`", Log::EndOfEntry);
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
    vfs_write<Uint32>(file, SDL_Swap32(CURRENT_MAP_ID));

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

size_t map_t::getTileIndex(Index2D index2d) const {
	if (index2d.x() < 0 || index2d.x() >= _info.getTileCountX() || index2d.y() < 0 || index2d.y() >= _info.getTileCountY()) {
		throw std::runtime_error("index out of bounds");
	} else {
		return index2d.x() + _info.getTileCountX() * index2d.y();
	}
}

const tile_info_t& map_t::operator()(size_t i) const {
	if (i >= _info.getTileCount()) {
		throw std::runtime_error("index out of bounds");
	} else {
		return _mem(i);
	}
}

const tile_info_t& map_t::operator()(size_t x, size_t y) const {
	if (x >= _info.getTileCountX() || y >= _info.getTileCountY()) {
		throw std::runtime_error("index out of bounds");
	} else {
		return _mem(x + _info.getTileCountX() * y);
	}
}