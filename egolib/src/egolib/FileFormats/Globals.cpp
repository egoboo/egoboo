#include "egolib/FileFormats/Globals.hpp"
#include "egolib/strutil.h"

tile_dictionary_t tile_dict;
treasure_table_t treasureTableList[MAX_TABLES];
wawalite_data_t wawalite_data;

egolib_rv get_random_treasure(char * buffer, size_t buffer_length)
{
	//ZF> Gets the name for a treasure randomly selected from the specified treasure table
	//    This function effectively "converts" a table name into a random element from that table

	IPair loc_rand;
	size_t i;
	int treasure_index;

	bool found = false;
	STRING tmp_buffer;

	// Trap invalid strings
	if (0 == buffer_length || INVALID_CSTR(buffer)) return rv_error;

	// make a local copy of the string
	strncpy(tmp_buffer, buffer, SDL_arraysize(tmp_buffer));

	// Iterate through every treasure table until we find the one we want
	found = false;
	for (i = 0; i < MAX_TABLES; i++)
	{
		//Continue looking until we find the correct table
		if (0 != strcmp(treasureTableList[i].table_name, tmp_buffer)) continue;

		//Pick a random number between 0 and the length of the table to get a random element out of the array
		loc_rand.base = 0;
		loc_rand.rand = treasureTableList[i].size;
		treasure_index = generate_irand_pair(loc_rand);
		strncpy(tmp_buffer, treasureTableList[i].object_list[treasure_index], buffer_length);

		//See if it is an actual random object or a reference to a different random table
		if ('%' != tmp_buffer[0])
		{
			found = true;
		}
		else
		{
			if (rv_success == get_random_treasure(tmp_buffer, buffer_length))
			{
				found = true;
			}
		}
	}

	//Could not find anything
	if (found)
	{
		// copy the local string to the output
		strncpy(buffer, tmp_buffer, buffer_length);
	}
	else
	{
		// give a warning
		tmp_buffer[0] = CSTR_END;
		Log::get().warn("Could not find treasure table: %s!\n", buffer);
	}

	return found ? rv_success : rv_fail;
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
		load_one_treasure_table_vfs(ctxt, &treasureTableList[num_table]);
		num_table++;
	}
	return rv_success;
}
