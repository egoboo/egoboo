// egolib/Platform/file_mac.m

// Egoboo, Copyright (C) 2000 Aaron Bishop

#import <Foundation/NSPathUtilities.h>
#import <Foundation/NSFileManager.h>
#import <Foundation/NSBundle.h>
#import "egolib/Platform/NSFileManager+DirectoryLocations.h"

#include "egolib/file_common.h"

struct s_mac_find_context : Id::NonCopyable
{
    NSDirectoryEnumerator *dirEnum;
    NSString *dirEnumExtension;
    NSString *dirEnumPath;
    std::string currentFile;
};

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
    @autoreleasepool {
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

        dataPath = [rootBundle resourcePath];
        userPath = [[NSFileManager defaultManager] documentsDirectory];
        //configPath = [[NSFileManager defaultManager] applicationSupportDirectory];

        [dataPath retain];
        [userPath retain];

        NSLog(@"sys_fs_init: Data directory is %@", dataPath);
        NSLog(@"sys_fs_init: User directory is %@", userPath);
        NSLog(@"sys_fs_init: Config directory is %@", dataPath);
        return 0;
    }
}

std::string fs_getDataDirectory()
{
    return [dataPath UTF8String];
}

std::string fs_getUserDirectory()
{
    return [userPath UTF8String];
}

std::string fs_getConfigDirectory()
{
    return [dataPath UTF8String];
}

bool fs_copyFile(const std::string& source, const std::string& target)
{
    @autoreleasepool {
        BOOL didCopy;
        NSString *sourcePath, *targetPath;

        sourcePath = [[NSString alloc] initWithUTF8String:source.c_str()];
        targetPath = [[NSString alloc] initWithUTF8String:target.c_str()];

        didCopy = [[NSFileManager defaultManager] copyItemAtPath:sourcePath toPath:targetPath error:nil];

        [sourcePath release];
        [targetPath release];
        return didCopy;
    }
}

//---------------------------------------------------------------------------------------------
//Directory Functions--------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

// Return the next file in the directory that matches the criteria specified in fs_findFirstFile
const char *fs_findNextFile(fs_find_context_t *fs_search)
{
    @autoreleasepool {
        NSString *fileName;
        NSString *pathName;

        if (fs_search == nullptr || fs_search->ptr.m == nullptr || fs_search->type != mac_find)
            return nullptr;

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
                    context->currentFile = [fileName UTF8String];
                    return context->currentFile.c_str();
                }
            }
            else
            {
                context->currentFile = [fileName UTF8String];
                return context->currentFile.c_str();
            }
        }

        return nullptr;
    }
}

// Stop the current find operation
void fs_findClose(fs_find_context_t *fs_search)
{
    @autoreleasepool {
        if (fs_search == nullptr || fs_search->ptr.m == nullptr || fs_search->type != mac_find)
            return;

        s_mac_find_context *context = fs_search->ptr.m;
        
        [context->dirEnum release];
        [context->dirEnumPath release];
        [context->dirEnumExtension release];
    
        delete context;
        
        fs_search->type = unknown_find;
        fs_search->ptr.v = nullptr;
    }
}

// Begin enumerating files in a directory.  The enumeration is not recursive; subdirectories
// won't be searched.  If 'extension' is not nullptr, only files with the given extension will
// be returned.
const char *fs_findFirstFile(const char *path, const char *extension, fs_find_context_t *fs_search)
{
    @autoreleasepool {
        NSString *searchPath;

        if (fs_search == nullptr)
            return nullptr;

        fs_search->type = mac_find;
        fs_search->ptr.m = new s_mac_find_context();
        if (fs_search->ptr.m == nullptr)
            return nullptr;

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
            delete context;
            fs_search->type = unknown_find;
            return nullptr;
        }
        
        [context->dirEnum retain];

        if (extension != nullptr)
        {
            context->dirEnumExtension = [[NSString alloc] initWithUTF8String:extension];
        }

        context->dirEnumPath = searchPath;

        return fs_findNextFile(fs_search);
    }
}
