#include <vector>
#include "game/module/PassageHandler.hpp"

#include "egolib/vfs.h"
#include "egolib/fileutil.h"

#include "game/game.h"
#include "game/player.h"
#include "game/char.h"

#include "game/ChrList.h"

namespace Passages
{
	//Private global
	static std::vector<std::shared_ptr<Passage>> _passages;

	void clearPassages()
	{
		_passages.clear();
	}

	void loadAllPassages()
	{
	    // Reset all of the old passages
	    _passages.clear();

	    // Load the file
	    vfs_FILE *fileread = vfs_openRead( "mp_data/passage.txt" );
	    if ( NULL == fileread ) return;

	    //Load all passages in file
	    while ( goto_colon_vfs( NULL, fileread, true ) )
	    {
	    	//read passage area
	    	irect_t area;
	        area.left   = vfs_get_int( fileread );
	        area.top    = vfs_get_int( fileread );
	        area.right  = vfs_get_int( fileread );
	        area.bottom = vfs_get_int( fileread );

	        //constrain passage area within the level
	        area.left    = CLIP( area.left,   0, PMesh->info.tiles_x - 1 );
    		area.top     = CLIP( area.top,    0, PMesh->info.tiles_y - 1 );
    		area.right   = CLIP( area.right,  0, PMesh->info.tiles_x - 1 );
    		area.bottom  = CLIP( area.bottom, 0, PMesh->info.tiles_y - 1 );

	        //Read if open by default
	        bool open = vfs_get_bool( fileread );

	        //Read mask (optional)
	        uint8_t mask = MAPFX_IMPASS | MAPFX_WALL;
	        if ( vfs_get_bool( fileread ) ) mask = MAPFX_IMPASS;
	        if ( vfs_get_bool( fileread ) ) mask = MAPFX_SLIPPY;

	        std::shared_ptr<Passage> passage = std::make_shared<Passage>(area, mask);

	        //check if we need to close the passage
	        if(!open) {
	        	passage->close();
	        }

	        //finished loading this one!
	   		_passages.push_back(passage);
	    }

	    //all done!
	    vfs_close( fileread );
	}

	void checkPassageMusic()
	{
        // Look at each player
        for ( PLA_REF ipla = 0; ipla < MAX_PLAYER; ipla++ )
        {
            CHR_REF character = PlaStack.lst[ipla].index;
            if ( !INGAME_CHR( character ) ) continue;

            //dont do items in hands or inventory
            if ( IS_ATTACHED_CHR( character ) ) continue;

            chr_t * pchr = ChrList_get_ptr( character );
            if ( !pchr->alive || !VALID_PLA( pchr->is_which_player ) ) continue;

		    //Loop through every passage
		    for(const std::shared_ptr<Passage> &passage : _passages)
		    {
	    		if(passage->checkPassageMusic(pchr)) {
	    			return;
	    		}
		    }	
        }
	}

	CHR_REF getShopOwner(const float x, const float y)
	{
		//Loop through every passage
		for(const std::shared_ptr<Passage> &passage : _passages)
	    {
	    	//Only check actual shops
	    	if(!passage->isShop()) {
	    		continue;
	    	}

	    	//Is item inside this shop?
	    	if(passage->isPointInside(x, y)) {
	    		return passage->getShopOwner();
	    	}
	    }

	    return Passage::SHOP_NOOWNER;		
	}

	void removeShopOwner(CHR_REF owner)
	{
		//Loop through every passage
		for(const std::shared_ptr<Passage> &passage : _passages)
	    {
	    	//Only check actual shops
	    	if(!passage->isShop()) {
	    		continue;
	    	}

	    	if(passage->getShopOwner() == owner) {
	    		passage->removeShop();
	    	}

	    	//TODO: mark all items in shop as normal items again
	    }
	}

	int getPassageCount()
	{
		return _passages.size();
	}

	std::shared_ptr<Passage> getPassageByID(int id)
	{
		if(id < 0 || id >= _passages.size()) {
			return nullptr;
		}

		return _passages[id];
	}
}
