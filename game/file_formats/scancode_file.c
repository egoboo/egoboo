#include "scancode_file.h"

#include "log.h"
#include "input.h"

#include "egoboo_vfs.h"
#include "egoboo_fileutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int       scantag_count = 0;
scantag_t scantag[MAXTAG];

static void   scantag_reset();
static bool_t scantag_read_one( vfs_FILE *fileread );

//--------------------------------------------------------------------------------------------
// Tag Reading
//--------------------------------------------------------------------------------------------
void scantag_reset()
{
    // ZZ> This function resets the tags
    scantag_count = 0;
}

//--------------------------------------------------------------------------------------------
bool_t scantag_read_one( vfs_FILE *fileread )
{
    // ZZ> This function finds the next tag, returning btrue if it found one

    bool_t retval;

    retval = goto_colon( NULL, fileread, btrue ) && (scantag_count < MAXTAG);
    if ( retval )
    {
        fget_string( fileread, scantag[scantag_count].name, SDL_arraysize(scantag[scantag_count].name) );
        scantag[scantag_count].value = fget_int( fileread );
        scantag_count++;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void scantag_read_all( const char *szFilename )
{
    // ZZ> This function reads the scancode.txt file
    vfs_FILE* fileread;

    scantag_reset();

    fileread = vfs_openRead( szFilename );
    if ( NULL == fileread )
    {
        log_error( "Cannot read %s.", szFilename );
    }

    while ( scantag_read_one( fileread ) );

    vfs_close( fileread );
}

//--------------------------------------------------------------------------------------------
int scantag_get_value( const char *string )
{
    // ZZ> This function matches the string with its tag, and returns the value...
    //    It will return 255 if there are no matches.
    int cnt;

    cnt = 0;

    while ( cnt < scantag_count )
    {
        if ( 0 == strcmp( string, scantag[cnt].name ) )
        {
            // They match
            return scantag[cnt].value;
        }

        cnt++;
    }

    // No matches
    return 255;
}

//--------------------------------------------------------------------------------------------
char* scantag_get_string( Sint32 device, Sint32 tag, bool_t is_key )
{
    // ZF> This translates a input tag value to a string
    int cnt;

    if ( device >= INPUT_DEVICE_JOY ) device = INPUT_DEVICE_JOY;
    if ( device == INPUT_DEVICE_KEYBOARD ) is_key = btrue;

    for ( cnt = 0; cnt < scantag_count; cnt++ )
    {
        // do not search invalid keys
        if ( is_key )
        {
            if ( 'K' != scantag[cnt].name[0] ) continue;
        }
        else
        {
            switch ( device )
            {
                case INPUT_DEVICE_MOUSE:
                    if ( 'M' != scantag[cnt].name[0] ) continue;
                    break;

                case INPUT_DEVICE_JOY:
                    if ( 'J' != scantag[cnt].name[0] ) continue;
                    break;
            }
        };
        if ( tag == scantag[cnt].value)
        {
            return scantag[cnt].name;
        }
    }

    // No matches
    return "N/A";
}