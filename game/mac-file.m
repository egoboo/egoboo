// mac-file.c

// Egoboo, Copyright (C) 2000 Aaron Bishop

#import <Foundation/NSPathUtilities.h>
#import <Foundation/NSFileManager.h>
#import <Foundation/NSBundle.h>
#include <string.h>
#include <SDL.h>
#include <SDL_endian.h>

// Proto.h is not included in this file because Egoboo's BOOL conflicts
// with Objective-C's built-in BOOL

NSDirectoryEnumerator* fs_dirEnum = nil;
NSString *fs_dirEnumExtension = nil;
NSString *fs_dirEnumPath = nil;

#define MAX_PATH 260
char fs_tempPath[MAX_PATH];
char fs_importPath[MAX_PATH];
char fs_savePath[MAX_PATH];
char fs_gamePath[MAX_PATH];
NSString *fs_workingDir = nil;

//---------------------------------------------------------------------------------------------
//File Routines-------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
void fs_init()
{
	// JF> This function determines the temporary, import,
	// game data and save paths
	NSString *tempDir, *homeDir, *gameDir, *workingDir;
	const char *str;
	
	tempDir = NSTemporaryDirectory();
	homeDir = NSHomeDirectory();
	gameDir = [[NSBundle mainBundle] bundlePath];
	workingDir = [[NSFileManager defaultManager] currentDirectoryPath];
	fs_workingDir = workingDir;
	
	NSLog(@"fs_init: Temporary directory is %@", tempDir);
	NSLog(@"fs_init: Home directory is %@", homeDir);
	NSLog(@"fs_init: Game directory is %@", gameDir);
	
	str = [tempDir cString];
	strcpy(fs_tempPath, str);
	strcpy(fs_importPath, str);
	strcat(fs_importPath, "import/");
	
	str = [homeDir cString];
	strcpy(fs_savePath, str);
	strcat(fs_savePath, "Documents/Egoboo/");
	
	str = [gameDir cString];
	strcpy(fs_gamePath, str);
	
	[tempDir release];
	[homeDir release];
	[gameDir release];
	// Don't release the working directory; it's kept as a global.
}

const char *fs_getTempDirectory()
{
	return fs_tempPath;
}

const char *fs_getImportDirectory()
{
	return fs_importPath;
}

const char *fs_getSaveDirectory()
{
	return fs_savePath;
}

const char *fs_getGameDirectory()
{
	return fs_gamePath;
}

int fs_createDirectory(const char *dirName)
{
	BOOL ok;
	
	NSString *path = [[NSString alloc] initWithCString: dirName];
	ok = [[NSFileManager defaultManager] createDirectoryAtPath:path attributes:nil];
	[path release];
	
	if (ok == YES) return 1;
	return 0;
}

int fs_removeDirectory(const char *dirName)
{
	BOOL ok;
	NSString *path = [[NSString alloc] initWithCString:dirName];
	ok = [[NSFileManager defaultManager] removeFileAtPath:path handler:nil];
	[path release];
	
	if (ok == YES) return 1;
	return 0;
}

void fs_deleteFile(const char *fileName)
{
	NSString *path = [[NSString alloc] initWithCString:fileName];
	[[NSFileManager defaultManager] removeFileAtPath:path handler:nil];
	[path release];
}

void fs_copyFile(const char *source, const char *dest)
{
	NSString *srcPath, *destPath;
	
	srcPath = [[NSString alloc] initWithCString:source];
	destPath = [[NSString alloc] initWithCString:dest];

	[[NSFileManager defaultManager] copyPath:srcPath toPath:destPath handler:nil];

	[srcPath release];
	[destPath release];
}

int fs_fileIsDirectory(const char *filename)
{
	// Returns 1 if this file is a directory
	BOOL isDir = NO;
	NSString *path;
	NSFileManager *manager;
	
	path = [[NSString alloc] initWithCString: filename];
	manager = [NSFileManager defaultManager];
	
	if ([manager fileExistsAtPath: path isDirectory: &isDir] && isDir)
	{
		return 1;
	}
	
	return 0;
	
}

//---------------------------------------------------------------------------------------------
//Directory Functions--------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

// Return the next file in the directory that matches the criteria specified in fs_findFirstFile
const char *fs_findNextFile()
{
	NSString *fileName;
	NSString *pathName;
	
	if(fs_dirEnum == nil)
	{
		return NULL;
	}
	
	while (fileName = [fs_dirEnum nextObject] )
	{
		// Also, don't go down directories recursively.
		pathName = [NSString stringWithFormat: @"%@/%@", fs_dirEnumPath, fileName];
		if(fs_fileIsDirectory([pathName cString]))
		{
			[fs_dirEnum skipDescendents];
		}
		[pathName release];
		
		if(fs_dirEnumExtension != nil)
		{
			if ([[fileName pathExtension] isEqualToString: fs_dirEnumExtension])
			{
				return [fileName cString];
			}
		} else
		{
			return [fileName cString];
		}
	}
	
	return NULL;
}

// Stop the current find operation
void fs_findClose()
{
	if (fs_dirEnum != nil)
	{
		[fs_dirEnum release];
		fs_dirEnum = nil;
	}
	
	if (fs_dirEnumPath != nil)
	{
		[fs_dirEnumPath release];
		fs_dirEnumPath = nil;
	}
	
	if (fs_dirEnumExtension != nil)
	{
		[fs_dirEnumExtension release];
		fs_dirEnumExtension = nil;
	}
}

// Begin enumerating files in a directory.  The enumeration is not recursive; subdirectories
// won't be searched.  If 'extension' is not NULL, only files with the given extension will
// be returned.
const char *fs_findFirstFile(const char *path, const char *extension)
{
	NSString *searchPath;
	
	if(fs_dirEnum != nil)
	{
		fs_findClose();
	}
	
	// If the path given is a relative one, we need to derive the full path
	// for it by appending the current working directory
	if(path[0] != '/')
	{
		searchPath = [NSString stringWithFormat: @"%@/%s", fs_workingDir, path];
	} else
	{
		searchPath = [NSString stringWithCString: path];
	}		
	
	fs_dirEnum = [[NSFileManager defaultManager] enumeratorAtPath: searchPath];
	
	
	if(extension != NULL)
	{
		fs_dirEnumExtension = [NSString stringWithCString: extension];
	}
	
	fs_dirEnumPath = searchPath;
	
	if(fs_dirEnum == nil)
	{
		return NULL;
	}
	
	return fs_findNextFile();
}

void empty_import_directory()
{
	fs_removeDirectory("./import");
	fs_createDirectory("./import");
}

// Pulled from egobootypedef.h
float LoadFloatByteswapped( float *ptr )
{
	int i;
	float *fptr;
	int *iptr = (int*)ptr;
	
	i = *iptr;
	i = SDL_Swap32(i);
	
	fptr = (float*)(&i);
	return *fptr;
}

// TEMPORARY!!!
double sys_getTime()
{
	return SDL_GetTicks() / 1000.0;
}

void sys_initialize()
{

}

void sys_shutdown()
{

}

void sys_frameStep()
{
}

int max(int a, int b)
{
	return (a >  b ? a : b);
}
