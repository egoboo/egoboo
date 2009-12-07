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

/// @file common-file.c
/// @brief Base implementation of the egoboo filesystem
/// @details File operations that are shared between various operating systems.
/// OS-specific code goes in *-file.c (such as win-file.c)

#include "file_common.h"

#include "egoboo_strutil.h"
#include "egoboo_vfs.h"
#include "egoboo_config.h"

#include "file_common.h"

#ifndef MAX_PATH
#define MAX_PATH 260  // Same value that Windows uses...
#endif

// FIXME: Doesn't handle deleting directories recursively yet.
void fs_removeDirectoryAndContents( const char *dirname, int recursive )
{
    /// @details ZZ@> This function deletes all files in a directory,
    ///    and the directory itself

    char filePath[MAX_PATH] = EMPTY_CSTR;
    const char *fileName;
    fs_find_context_t fs_search;

    // List all the files in the directory
    fileName = fs_findFirstFile( dirname, NULL, &fs_search );
    while ( fileName != NULL )
    {
        // Ignore files that start with a ., like .svn for example.
        if ( '.' != fileName[0] )
        {
            snprintf( filePath, MAX_PATH, "%s" SLASH_STR "%s", dirname, fileName );
            if ( fs_fileIsDirectory( filePath ) )
            {
                if ( recursive )
                {
                    fs_removeDirectoryAndContents( filePath, recursive );
                }
                else
                {
                    fs_removeDirectory( filePath );
                }
            }
            else
            {
                fs_deleteFile( filePath );
            }
        }
        fileName = fs_findNextFile( &fs_search );
    }
    fs_findClose( &fs_search );

    fs_removeDirectory( dirname );
}

//--------------------------------------------------------------------------------------------
void fs_copyDirectory( const char *sourceDir, const char *destDir )
{
    /// @details ZZ@> This function copies all files in a directory
    char srcPath[MAX_PATH] = EMPTY_CSTR, destPath[MAX_PATH] = EMPTY_CSTR;

    const char *fileName;
    fs_find_context_t fs_search;

    // List all the files in the directory
    fileName = fs_findFirstFile( sourceDir, NULL, &fs_search );
    if ( fileName != NULL )
    {
        // Make sure the destination directory exists
        fs_createDirectory( destDir );

        while ( fileName != NULL )
        {
            // Ignore files that begin with a .
            if ( '.' != fileName[0] )
            {
                snprintf( srcPath, MAX_PATH, "%s" SLASH_STR "%s", sourceDir, fileName );
                snprintf( destPath, MAX_PATH, "%s" SLASH_STR "%s", destDir, fileName );
                fs_copyFile( srcPath, destPath );
            }

            fileName = fs_findNextFile( &fs_search );
        }
    }

    fs_findClose( &fs_search );
}

//--------------------------------------------------------------------------------------------
int fs_fileExists( const char *filename )
{
    FILE * ptmp;

    int retval = 0;

    ptmp = EGO_fopen( filename, "rb" );
    if ( NULL != ptmp )
    {
        EGO_fclose( ptmp );
        retval = 1;
    }

    return retval;
}

void _ego_clearerr( FILE * f )
{
    printf( "----clearerr()\n" );
    clearerr( f );
}

int _ego_fclose( FILE * f )
{
    printf( "----fclose()\n" );
    return fclose( f );
}

int _ego_feof( FILE * f )
{
    printf( "----feof()\n" );
    return feof( f );
}

int _ego_ferror( FILE * f )
{
    printf( "----ferror()\n" );
    return ferror( f );
}

int _ego_fflush( FILE * f )
{
    printf( "----fflush()\n" );
    return fflush( f );
}

int _ego_fgetc( FILE * f )
{
    printf( "----fgetc()\n" );
    return fgetc( f );
}

int _ego_fgetpos( FILE * f, fpos_t * pfpos )
{
    printf( "----fgetpos()\n" );
    return fgetpos( f, pfpos );
}

char * _ego_fgets( char* str, int len, FILE * f )
{
    printf( "----fgets()\n" );
    return fgets( str, len, f );
}

FILE * _ego_fopen( const char * fn, const char * blah )
{
    printf( "----fopen()\n" );
    return fopen( fn, blah );
}

//_ego_fprintf

int _ego_fputc( int c, FILE * f )
{
    printf( "----fputc()\n" );
    return fputc( c, f );
}

int _ego_fputs( const char * str, FILE * f )
{
    printf( "----fputs()\n" );
    return fputs( str, f );
}

size_t _ego_fread( void * buf, size_t size, size_t count, FILE * f )
{
    printf( "----fread()\n" );
    return fread( buf, size, count, f );
}

FILE * _ego_freopen( const char * fn, const char * blah, FILE * f )
{
    printf( "----freopen()\n" );
    return freopen( fn, blah, f );
}

//_ego_fscanf

int _ego_fseek( FILE* f, long pos, int blah )
{
    printf( "----fseek()\n" );
    return fseek( f, pos, blah );
}

int _ego_fsetpos( FILE * f, const fpos_t * ppos )
{
    printf( "----fsetpos()\n" );
    return fsetpos( f, ppos );
}

long _ego_ftell( FILE * f )
{
    printf( "----ftell()\n" );
    return ftell( f );
}

size_t _ego_fwrite( const void * buf, size_t size, size_t count, FILE * f )
{
    printf( "----fwrite()\n" );
    return fwrite( buf, size, count, f );
}

int _ego_getc( FILE * f )
{
    printf( "----getc()\n" );
    return getc( f );
}

int _ego_getchar()
{
    printf( "----getchar()\n" );
    return getchar();
}

char * _ego_gets( char * str )
{
    printf( "----gets()\n" );
    return gets( str );
}

//_ego_printf

int _ego_putc( int c , FILE * f )
{
    printf( "----putc()\n" );
    return putc( c, f );
}

int _ego_putchar( int c )
{
    printf( "----putchar()\n" );
    return putchar( c );
}

int _ego_puts( const char * str )
{
    printf( "----puts()\n" );
    return puts( str );
}

void _ego_rewind( FILE * f )
{
    printf( "----rewind()\n" );
    rewind( f );
}

//_ego_scanf

void _ego_setbuf( FILE * f, char * buf )
{
    printf( "----setbuf()\n" );
    setbuf( f, buf );
}

int _ego_setvbuf( FILE * f, char * buf, int blah, size_t size )
{
    printf( "----setvbuf()\n" );
    return setvbuf( f, buf, blah, size );
}

//_ego_sprintf

//_ego_sscanf

FILE * _ego_tmpfile()
{
    printf( "----tmpfile()\n" );
    return tmpfile();
}

char * _ego_tmpnam( char * name )
{
    printf( "----tmpnam()\n" );
    return tmpnam( name );
}

int _ego_ungetc( int c, FILE * f )
{
    printf( "----ungetc()\n" );
    return ungetc( c, f );
}

int _ego_vfprintf( FILE * f, const char * str, va_list lst )
{
    printf( "----vfprintf()\n" );
    return vfprintf( f, str, lst );
}

int _ego_vprintf( const char * str, va_list lst )
{
    printf( "----vprintf()\n" );
    return vprintf( str,  lst );
}

int _ego_vsprintf( char * buf, const char * str, va_list lst )
{
    printf( "----vsprintf()\n" );
    return vsprintf( buf, str, lst );
}

int _ego_vsnprintf( char * buf, size_t size, const char * str, va_list lst )
{
    printf( "----vsnprintf()\n" );
    return vsnprintf( buf, size, str, lst );
}
