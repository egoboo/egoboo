
#include "passage_file.h"

#include "mpd_file.h"

#include "egoboo_fileutil.h"

//--------------------------------------------------------------------------------------------
passage_t * passage_init( passage_t * ppass )
{
    if( NULL == ppass ) return ppass;

    memset( ppass, 0, sizeof(passage_t) );

    ppass->music = NO_MUSIC;     // Set no song as default

    return ppass;
}

//--------------------------------------------------------------------------------------------
bool_t scan_passage_file( vfs_FILE * fileread, passage_t * ppass  )
{
    // ZZ> This function reads the passage file

    bool_t found;

    if( NULL == fileread || NULL == ppass ) return bfalse;

    passage_init( ppass );

    found = bfalse;
    if ( goto_colon( NULL, fileread, btrue ) )
    {
        ppass->area.left   = fget_int( fileread );
        ppass->area.top    = fget_int( fileread );
        ppass->area.right  = fget_int( fileread );
        ppass->area.bottom = fget_int( fileread );

        ppass->open = fget_bool( fileread );

        ppass->mask = MPDFX_IMPASS | MPDFX_WALL;
        if ( fget_bool( fileread ) ) ppass->mask = MPDFX_IMPASS;
        if ( fget_bool( fileread ) ) ppass->mask = MPDFX_SLIPPY;

        found = btrue;
    }

    return found;
}