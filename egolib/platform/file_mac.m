// mac-file.m

// Egoboo, Copyright (C) 2000 Aaron Bishop

#import <Foundation/NSPathUtilities.h>
#import <Foundation/NSFileManager.h>
#import <Foundation/NSBundle.h>
#import "NSFileManager+DirectoryLocations.h"
#include <string.h>
#include <SDL.h>
#include <SDL_endian.h>

// Proto.h is not included in this file because Egoboo's BOOL conflicts
// with Objective-C's built-in BOOL

NSDirectoryEnumerator* fs_dirEnum = nil;
NSString *fs_dirEnumExtension = nil;
NSString *fs_dirEnumPath = nil;

#define MAX_PATH 260
static NSString *binaryPath = nil;
static NSString *dataPath = nil;
static NSString *userPath = nil;
//static NSString *configPath = nil;

//---------------------------------------------------------------------------------------------
//File Routines-------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
void sys_fs_init()
{
	// JF> This function determines the temporary, import,
	// game data and save paths
	
	binaryPath = [[NSBundle mainBundle] bundlePath];
	dataPath = [binaryPath stringByAppendingString:@"/Contents/Resources"];
	userPath = [[NSFileManager defaultManager] documentsDirectory];
//	configPath = [[NSFileManager defaultManager] applicationSupportDirectory];

	NSLog(@"sys_fs_init: Game directory is %@", binaryPath);
	NSLog(@"sys_fs_init: Data directory is %@", dataPath);
	NSLog(@"sys_fs_init: User directory is %@", userPath);
	NSLog(@"sys_fs_init: Config directory is %@", dataPath);
}

const char *fs_getBinaryDirectory()
{
	return [binaryPath cString];
}

const char *fs_getDataDirectory()
{
	return [dataPath cString];
}

const char *fs_getUserDirectory()
{
	return [userPath cString];
}

const char *fs_getConfigDirectory()
{
	return [dataPath cString];
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

	NSLog(@"Copying file\n%@\nto\n%@\n", srcPath, destPath);
	if ([[NSFileManager defaultManager] copyPath:srcPath toPath:destPath handler:nil] != YES)
	{
		NSLog(@"Failed to copy!");
	}

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

	while (( fileName = [fs_dirEnum nextObject] ))
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
		searchPath = [NSString stringWithFormat: @"%@/%s", dataPath, path];
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
