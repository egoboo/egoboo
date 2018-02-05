// egolib/Platform/file_mac.m

// Egoboo, Copyright (C) 2000 Aaron Bishop

#import <Foundation/NSPathUtilities.h>
#import <Foundation/NSFileManager.h>
#import <Foundation/NSBundle.h>
#import "egolib/Platform/NSFileManager+DirectoryLocations.h"

#include "egolib/file_common.h"

static NSString *dataPath = nil;
static NSString *userPath = nil;
//static NSString *configPath = nil;

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
