#include "egolib/FileFormats/Globals.hpp"
#include "egolib/strutil.h"

tile_dictionary_t tile_dict;
treasure_table_t treasureTableList[MAX_TABLES];
wawalite_data_t wawalite_data;

/// Find the first treasure table of the specified name.
treasure_table_t *find_treasure_table(const std::string& name) {
    for (size_t i = 0; i < MAX_TABLES; i++) {
        if (std::string(treasureTableList[i].table_name) == name) {
            return &(treasureTableList[i]);
        }
    }
    return nullptr;
}

bool get_random_treasure(const std::string& treasureTableName, std::string& treasureName)
{
	// The empty string is not a valid treasure table name and treasure table names must start with '%'.
    if (treasureTableName.empty() || '%' != treasureTableName[0]) {
        return false;
    }
    // To detect circular references, we keep the names of visited tables in this set.
    std::unordered_set<std::string> visited;
    // Make a local copy of the table name.
    std::string n = treasureTableName;

    bool found = false;
	// Keep searching until we have found something or abort searching for other reasons.
    while (!found) {
        // Check if this table name was already visited.
        auto it = visited.find(n);
        // If this table name was already visted ...
        if (it != visited.cend())
        {
            // ... stop searching.
            break;
        }
        // Mark this table name as visited.
        visited.insert(n);
        // Find the table for the name.
        treasure_table_t *t = find_treasure_table(n);
        // If the table does not exist or is empty ...
        if (nullptr == t && 0 == t->size) {
            // ... stop searching.
            break;
        } else {
            // Pick a random index number between 0 and size - 1 of the table and get the element at this index.
            int i = generate_irand_pair(IPair(0, t->size-1));
            n = t->object_list[i];
            // If this is not a reference to yet another treasure table ...
            if ('%' != n[0]) {
                // ... an entry was found.
                found = true;
            }
            // Otherwise we keep searching.
        }
    }
    if (found) {
        treasureName = n;
        return true;
    } else {
        treasureName = std::string();
        return false;
    }
}

egolib_rv init_random_treasure_tables_vfs(const std::string& filepath)
{
	//ZF> This loads all the treasure tables from randomtreasure.txt
	int num_table;

	// Try to open a context.
	ReadContext ctxt(filepath);
	if (!ctxt.ensureOpen())
	{
		Log::get().warn("unable to load random treasure tables file `%s`\n", filepath.c_str());
		return rv_error;
	}

	//Load each treasure table
	num_table = 0;
	while (ctxt.skipToColon(true))
	{
		//Load the name of this table
		STRING temporary;
		vfs_read_name(ctxt, temporary, SDL_arraysize(temporary));

		//Stop here if we are already full
		if (num_table >= MAX_TABLES)
		{
			Log::get().warn("Cannot load random treasure table: %s (We only support up to %i tables, consider increasing MAX_TABLES) \n", temporary, MAX_TABLES);
			break;
		}

		snprintf(treasureTableList[num_table].table_name, SDL_arraysize(treasureTableList[num_table].table_name), "%%%s", temporary);

		//Load all objects in this treasure table
		treasureTableList[num_table].size = 0;
		load_one_treasure_table_vfs(ctxt, treasureTableList[num_table]);
		num_table++;
	}
	return rv_success;
}
