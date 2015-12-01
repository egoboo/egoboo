#pragma once

#include "egolib/FileFormats/map_tile_dictionary.h"
#include "egolib/FileFormats/treasure_table_file.h"
#include "egolib/FileFormats/wawalite_file.h"

extern tile_dictionary_t tile_dict;

/// @todo ZF> This should probably be moved into a game.c data structure or something like that, we should also implement
///    so that any local module can override the default randomtreasure.txt found in basicdat folder
extern treasure_table_t treasureTableList[MAX_TABLES];

extern wawalite_data_t wawalite_data;

/**
 * @brief Search treasure tables for random treasure. Follows references, detects circles.
 * @param treasureTableName the name of the treasure table to start searching
 * @param [out] treasureName reference to a std::string which is assigned the name of the treasure
 * @return @a true if a treasure was found, @a false otherwise
 * @pre @a treasureTableName must be a valid treasure table name.
 * @remarks A valid treasure table name starts with <tt>%</tt>.
 * @post If @a true was returned, @a treasureName was assigned a treasure name.
 * If @a false was returned, @a treasure name was assigned the empty string.
 * @remarks Follows references, detects circles.
 */
bool get_random_treasure(const std::string& treasureTableName, std::string& treasureName);
/// This loads all the treasure tables from <tt>randomtreasure.txt</tt>.
void init_random_treasure_tables_vfs(const std::string& filepath);
