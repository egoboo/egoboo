#pragma once

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

#include <stdio.h>

#include "egoboo_typedef.h"

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
struct s_win32_find_context;
struct s_linux_find_context;
struct s_mac_find_context;

enum e_fs_find_type
{
    unknown_find = 0,
    win32_find,
    linux_find,
    mac_find
};
typedef enum e_fs_find_type fs_find_type_t;

//---------------------------------------------------------------------------------------------
union u_fs_find_ptr
{
    void                        * v;
    struct s_win32_find_context * w;
    struct s_linux_find_context * l;
    struct s_mac_find_context   * m;
};
typedef union u_fs_find_ptr fs_find_ptr_t;

//---------------------------------------------------------------------------------------------
struct s_fs_find_context
{
    fs_find_type_t type;
    fs_find_ptr_t  ptr;
};

typedef struct s_fs_find_context fs_find_context_t;

//---------------------------------------------------------------------------------------------
/// Filesystem functions

int DirGetAttrib( const char *fromdir );

void fs_init();

const char *fs_getBinaryDirectory();
const char *fs_getDataDirectory();
const char *fs_getUserDirectory();
const char *fs_getConfigDirectory();

int  fs_fileExists( const char *filename );
int  fs_fileIsDirectory( const char *filename );
int  fs_createDirectory( const char *dirname );
int  fs_removeDirectory( const char *dirname );
void fs_deleteFile( const char *filename );
bool_t fs_copyFile( const char *source, const char *dest );
void fs_removeDirectoryAndContents( const char *dirname, int recursive );
void fs_copyDirectory( const char *sourceDir, const char *destDir );

/// Enumerate directory contents
const char *fs_findFirstFile( const char *path, const char *extension, fs_find_context_t * fs_search );
const char *fs_findNextFile( fs_find_context_t * fs_search );
void        fs_findClose( fs_find_context_t * fs_search );

//--------------------------------------------------------
// wrapper functions so we can intercept file-system calls
void _ego_clearerr(FILE *);
int _ego_fclose(FILE *);
int _ego_feof(FILE *);
int _ego_ferror(FILE *);
int _ego_fflush(FILE *);
int _ego_fgetc(FILE *);
int _ego_fgetpos(FILE *, fpos_t *);
char * _ego_fgets(char*, int, FILE *);
FILE * _ego_fopen(const char *, const char *);
//_ego_fprintf
int _ego_fputc(int, FILE *);
int _ego_fputs(const char *, FILE *);
size_t _ego_fread(void *, size_t, size_t, FILE *);
FILE * _ego_freopen( const char *, const char *, FILE *);
//_ego_fscanf(
int _ego_fseek( FILE*, long, int );
int _ego_fsetpos( FILE *, const fpos_t * );
long _ego_ftell(FILE *);
size_t _ego_fwrite( const void *, size_t, size_t, FILE *);
int _ego_getc(FILE *);
int _ego_getchar();
char * _ego_gets(char *);
//_ego_printf
int ego_putc( int, FILE *);
int ego_putchar(int);
int _ego_puts(const char *);
void _ego_rewind(FILE *);
//_ego_scanf
void _ego_setbuf( FILE *, char * );
int _ego_setvbuf( FILE *, char *, int , size_t );
//_ego_sprintf
//_ego_sscanf
FILE * _ego_tmpfile();
char * _ego_tmpnam(char *);
int _ego_ungetc(int, FILE *);
int _ego_vfprintf(FILE *, const char *, va_list );
int _ego_vprintf(const char *, va_list );
int _ego_vsprintf(char *, const char *, va_list);
int _ego_vsnprintf( char *, size_t, const char *, va_list );


//--------------------------------------------------------
// macros to dispatch file-system calls
#ifdef DEBUG_STDIO

#    define EGO_clearerr _ego_clearerr
#    define EGO_fclose _ego_fclose
#    define EGO_feof _ego_feof
#    define EGO_ferror _ego_ferror
#    define EGO_fflush _ego_fflush
#    define EGO_fgetc _ego_fgetc
#    define EGO_fgetpos _ego_fgetpos
#    define EGO_fgets _ego_fgets
#    define EGO_fopen _ego_fopen
#    define EGO_fprint _ego_fprint
#    define EGO_fputc _ego_fputc
#    define EGO_fputs _ego_fputs
#    define EGO_fread _ego_fread
#    define EGO_freopen _ego_freopen
#    define EGO_fscanf _ego_fscanf
#    define EGO_fseek _ego_fseek
#    define EGO_fsetpos _ego_fsetpos
#    define EGO_ftell _ego_ftell
#    define EGO_fwrite _ego_fwrite
#    define EGO_getc _ego_getc
#    define EGO_getchar _ego_getchar
#    define EGO_gets _ego_gets
#    define EGO_print _ego_print
#    define EGO_putc _ego_putc
#    define EGO_putchar _ego_putchar
#    define EGO_puts _ego_puts
#    define EGO_rewind _ego_rewind
//_ego_scanf
#    define EGO_setbuf _ego_setbuf
#    define EGO_setvbuf _ego_setvbuf
#    define EGO_sprint _ego_sprint
#    define EGO_sscan _ego_sscan
#    define EGO_tmpfile _ego_tmpfile
#    define EGO_tmpnam _ego_tmpnam
#    define EGO_ungetc _ego_ungetc
#    define EGO_vfprintf _ego_vfprintf
#    define EGO_vprintf _ego_vprintf
#    define EGO_vsprintf _ego_vsprintf
#    define EGO_vsnprintf _ego_vsnprintf

#else

#    define EGO_clearerr clearerr
#    define EGO_fclose fclose
#    define EGO_feof feof
#    define EGO_ferror ferror
#    define EGO_fflush fflush
#    define EGO_fgetc fgetc
#    define EGO_fgetpos fgetpos
#    define EGO_fgets fgets
#    define EGO_fopen fopen
#    define EGO_fprint fprint
#    define EGO_fputc fputc
#    define EGO_fputs fputs
#    define EGO_fread fread
#    define EGO_freopen freopen
#    define EGO_fscanf fscanf
#    define EGO_fseek fseek
#    define EGO_fsetpos fsetpos
#    define EGO_ftell ftell
#    define EGO_fwrite fwrite
#    define EGO_getc getc
#    define EGO_getchar getchar
#    define EGO_gets gets
#    define EGO_print print
#    define EGO_putc putc
#    define EGO_putchar putchar
#    define EGO_puts puts
#    define EGO_rewind rewind
//scanf
#    define EGO_setbuf setbuf
#    define EGO_setvbuf setvbuf
#    define EGO_sprint sprint
#    define EGO_sscan sscan
#    define EGO_tmpfile tmpfile
#    define EGO_tmpnam tmpnam
#    define EGO_ungetc ungetc
#    define EGO_vfprintf vfprintf
#    define EGO_vprintf vprintf
#    define EGO_vsprintf vsprintf
#    define EGO_vsnprintf vsnprintf

#endif
