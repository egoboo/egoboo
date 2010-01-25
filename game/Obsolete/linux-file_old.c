// lin-file.c

// Egoboo, Copyright (C) 2000 Aaron Bishop

#include "egoboo.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/dir.h>

char command[500];
FILE *DirFiles;
char DirRead[100];

//File Routines-----------------------------------------------------------
void make_directory( char *dirname )
{
    // ZZ> This function makes a new directory
    sprintf( command, "mkdir -p %s\n", dirname );
    system( command );
}

void remove_directory( char *dirname )
{
    // ZZ> This function removes a directory
    sprintf( command, "rm -rf %s\n", dirname );
    system( command );
}

void delete_file( char *filename )
{
    // ZZ> This function deletes a file
    sprintf( command, "rm -f %s\n", filename );
    system( command );
}

void copy_file( char *source, char *dest )
{
    // ZZ> This function copies a file on the local machine
    sprintf( command, "cp -f %s %s\n", source, dest );
    system( command );
}

void delete_directory( char *dirname )
{
    // ZZ> This function deletes all files in a directory,
    //     and the directory itself
    sprintf( command, "rm -rf %s\n", dirname );
    system( command );
}

void copy_directory( char *dirname, char *todirname )
{
    // ZZ> This function copies all files in a directory
    sprintf( command, "cp -fr %s %s\n", dirname, todirname );
    system( command );
}

void empty_import_directory( void )
{
    // ZZ> This function deletes all the TEMP????.OBJ subdirectories in the IMPORT directory
    sprintf( command, "rm -rf import/temp*.obj\n" );
    system( command );
}

// Read the first directory entry
char *DirGetFirst( char *search )
{
    sprintf( command, "find %s -maxdepth 0 -printf \"%%f\\n\"  ", search );
    DirFiles = popen( command, "r" );
    if ( !feof( DirFiles ) )
    {
        fscanf( DirFiles, "%s\n", DirRead );
        return( DirRead );
    }
    else
    {
        return( NULL );
    }
}

// Read the next directory entry (NULL if done)
char *DirGetNext( void )
{
    if ( !feof( DirFiles ) )
    {
        fscanf( DirFiles, "%s\n", DirRead );
        return( DirRead );
    }
    else
    {
        return( NULL );
    }
}

// Close anything left open
void DirClose()
{
    fclose( DirFiles );
}

int ClockGetTick()
{
    return( clock() );
}

int DirGetAttrib( char *fromdir )
{
    int tmp;
#define FILE_ATTRIBUTE_DIRECTORY 0x10
#define FILE_ATTRIBUTE_NORMAL 0x0
#define FILE_ATTRIBUTE_ERROR 0xffffffff

    return( 0 );
}
