// egolib/Platform/file_mac.m

// Egoboo, Copyright (C) 2000 Aaron Bishop

#import <Foundation/NSPathUtilities.h>
#import <Foundation/NSFileManager.h>
#import <Foundation/NSBundle.h>
#import "egolib/Platform/NSFileManager+DirectoryLocations.h"

#include "egolib/file_common.h"

struct s_mac_find_context
{
    NSDirectoryEnumerator *dirEnum;
    NSString *dirEnumExtension;
    NSString *dirEnumPath;
    NSString *currentFile;
};

static NSString *binaryPath = nil;
static NSString *dataPath = nil;
static NSString *userPath = nil;
//static NSString *configPath = nil;

//---------------------------------------------------------------------------------------------
//File Routines-------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
int sys_fs_init(const char *root_path)
{
    // JF> This function determines the temporary, import,
    // game data and save paths
    
    NSBundle *rootBundle = nil;
    if (root_path != nullptr)
    {
        rootBundle = [NSBundle bundleWithPath:[NSString stringWithUTF8String:root_path]];
        if (rootBundle != nil && [rootBundle bundleIdentifier] == nil) rootBundle = nil;
        if (rootBundle == nil) NSLog(@"sys_fs_init warning: root_path given but it's not a bundle! ('%s')", root_path);
    }
    if (rootBundle == nil) rootBundle = [NSBundle mainBundle];
    if (rootBundle == nil)
    {
        NSLog(@"neither root_path nor [NSBundle mainBundle] gave a valid NSBundle!");
        return 1;
    }
    
    NSLog(@"Loading from bundle identified as %@", [rootBundle bundleIdentifier]);

    binaryPath = [rootBundle bundlePath];
    dataPath = [rootBundle resourcePath];
    userPath = [[NSFileManager defaultManager] documentsDirectory];
    //configPath = [[NSFileManager defaultManager] applicationSupportDirectory];

    [binaryPath retain];
    [dataPath retain];
    [userPath retain];

    NSLog(@"sys_fs_init: Game directory is %@", binaryPath);
    NSLog(@"sys_fs_init: Data directory is %@", dataPath);
    NSLog(@"sys_fs_init: User directory is %@", userPath);
    NSLog(@"sys_fs_init: Config directory is %@", dataPath);
    return 0;
}

const char *fs_getBinaryDirectory()
{
    return [binaryPath UTF8String];
}

const char *fs_getDataDirectory()
{
    return [dataPath UTF8String];
}

const char *fs_getUserDirectory()
{
    return [userPath UTF8String];
}

const char *fs_getConfigDirectory()
{
    return [dataPath UTF8String];
}

int fs_createDirectory(const char *dirName)
{
    BOOL ok;

    NSString *path = [[NSString alloc] initWithUTF8String: dirName];
    ok = [[NSFileManager defaultManager] createDirectoryAtPath:path
                                   withIntermediateDirectories:NO
                                                    attributes:nil
                                                         error:nil];
    [path release];

    if (ok == YES) return 1;
    return 0;
}

int fs_removeDirectory(const char *dirName)
{
    BOOL ok;
    NSString *path = [[NSString alloc] initWithUTF8String:dirName];
    ok = [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
    [path release];

    if (ok == YES) return 1;
    return 0;
}

void fs_deleteFile(const char *fileName)
{
    NSString *path = [[NSString alloc] initWithUTF8String:fileName];
    [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
    [path release];
}

bool fs_copyFile(const char *source, const char *dest)
{
    BOOL didCopy;
    NSString *srcPath, *destPath;

    srcPath = [[NSString alloc] initWithUTF8String:source];
    destPath = [[NSString alloc] initWithUTF8String:dest];

    didCopy = [[NSFileManager defaultManager] copyItemAtPath:srcPath toPath:destPath error:nil];

    [srcPath release];
    [destPath release];
    return didCopy == YES;
}

int fs_fileIsDirectory(const char *filename)
{
    // Returns 1 if this file is a directory
    BOOL fileExists;
    BOOL isDir = NO;
    NSString *path;
    NSFileManager *manager;

    path = [[NSString alloc] initWithUTF8String: filename];
    manager = [NSFileManager defaultManager];

    fileExists = [manager fileExistsAtPath:path isDirectory:&isDir];
    [path release];

    if (fileExists && isDir)
    {
        return 1;
    }

    return 0;
}

//---------------------------------------------------------------------------------------------
//Directory Functions--------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

// Return the next file in the directory that matches the criteria specified in fs_findFirstFile
const char *fs_findNextFile(fs_find_context_t * fs_search)
{
    NSString *fileName;
    NSString *pathName;

    if (fs_search == NULL || fs_search->ptr.m == NULL || fs_search->type != mac_find)
        return NULL;

    s_mac_find_context *context = fs_search->ptr.m;

    while (fileName = [context->dirEnum nextObject])
    {
        // Also, don't go down directories recursively.
        pathName = [NSString stringWithFormat:@"%@/%@", context->dirEnumPath, fileName];
        if (fs_fileIsDirectory([pathName UTF8String]))
        {
            [context->dirEnum skipDescendents];
        }

        if (context->dirEnumExtension != nil)
        {
            if ([[fileName pathExtension] isEqualToString: context->dirEnumExtension])
            {
                if (context->currentFile != nil)
                    [context->currentFile release];
                context->currentFile = fileName;
                [context->currentFile retain];
                return [fileName UTF8String];
            }
        }
        else
        {
            if (context->currentFile != nil)
                [context->currentFile release];
            context->currentFile = fileName;
            [context->currentFile retain];
            return [fileName UTF8String];
        }
    }

    return NULL;
}

// Stop the current find operation
void fs_findClose(fs_find_context_t * fs_search)
{
    if (fs_search == NULL || fs_search->ptr.m == NULL || fs_search->type != mac_find)
        return;

    s_mac_find_context *context = fs_search->ptr.m;

    if (context->dirEnum != nil)
        [context->dirEnum release];

    if (context->dirEnumPath != nil)
        [context->dirEnumPath release];

    if (context->dirEnumExtension != nil)
        [context->dirEnumExtension release];

    if (context->currentFile != nil)
        [context->currentFile release];

    EGOBOO_DELETE(context);
}

// Begin enumerating files in a directory.  The enumeration is not recursive; subdirectories
// won't be searched.  If 'extension' is not NULL, only files with the given extension will
// be returned.
const char *fs_findFirstFile(const char *path, const char *extension, fs_find_context_t * fs_search)
{
    NSString *searchPath;

    if (fs_search == NULL)
        return NULL;

    fs_search->type = mac_find;
    fs_search->ptr.m = EGOBOO_NEW(s_mac_find_context);
    if (fs_search->ptr.m == NULL)
        return NULL;

    s_mac_find_context *context = fs_search->ptr.m;

    // If the path given is a relative one, we need to derive the full path
    // for it by appending the current working directory
    if (path[0] != '/')
    {
        searchPath = [[NSString alloc] initWithFormat:@"%@/%s", dataPath, path];
    }
    else
    {
        searchPath = [[NSString alloc] initWithUTF8String:path];
    }

    context->dirEnum = [[NSFileManager defaultManager] enumeratorAtPath:searchPath];
    if (context->dirEnum == nil)
    {
        [searchPath release];
        EGOBOO_DELETE(context);
        fs_search->type = unknown_find;
        return NULL;
    }

    if (extension != NULL)
    {
        context->dirEnumExtension = [[NSString alloc] initWithUTF8String:extension];
    }

    context->dirEnumPath = searchPath;

    return fs_findNextFile(fs_search);
}

void empty_import_directory()
{
    fs_removeDirectory("./import");
    fs_createDirectory("./import");
}
