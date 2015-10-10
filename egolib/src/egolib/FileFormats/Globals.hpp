#pragma once

#include "egolib/FileFormats/map_tile_dictionary.h"
#include "egolib/FileFormats/treasure_table_file.h"
#include "egolib/FileFormats/wawalite_file.h"

extern tile_dictionary_t tile_dict;

/// @todo ZF> This should probably be moved into a game.c data structure or something like that, we should also implement
///    so that any local module can override the default randomtreasure.txt found in basicdat folder
extern treasure_table_t treasureTableList[MAX_TABLES];

extern wawalite_data_t wawalite_data;

egolib_rv get_random_treasure(char * buffer, size_t buffer_length);
egolib_rv init_random_treasure_tables_vfs(const std::string& filepath);