#include "FileSystem.hpp"

#if defined(_WIN32)
#include <Windows.h>
#define REGEX_DIRSEP "[/\\\\]"
#else
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#define REGEX_DIRSEP "/"
#endif

#include <iostream>
#include <memory>

namespace Editor {

#if defined(_WIN32)
FileSystem::PathStat FileSystem::stat(const std::string &pathname)
{
    DWORD attributes = GetFileAttributes(pathname.c_str());
    if (0xFFFFFFFF == attributes)
    {
        std::cerr << pathname << ": " << "GetFileAttributes failed" << std::endl;
        return FileSystem::PathStat::Failure;
    }
    DWORD rejectedMask = (FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_VIRTUAL);
    if (attributes & rejectedMask)
    {
        return PathStat::Other;
    }
    else if (attributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        return PathStat::Directory;
    }
    // It is not rejected and it is not a directory.
    if (attributes == FILE_ATTRIBUTE_NORMAL || attributes & FILE_ATTRIBUTE_ARCHIVE)
    {
        return PathStat::File;
    }
    else
    {
        return PathStat::Other;
    }
}
#else
FileSystem::PathStat FileSystem::stat(const std::string &pathname)
{
    struct stat out;
    int success = lstat(pathname.c_str(), &out);
    if (success == -1)
    {
        std::cerr << pathname << ": " << strerror(errno) << std::endl;
        return FileSystem::PathStat::Failure;
    }
    if (S_ISREG(out.st_mode)) return FileSystem::PathStat::File;
    if (S_ISDIR(out.st_mode)) return FileSystem::PathStat::Directory;
    return FileSystem::PathStat::Other;
}
#endif

std::string FileSystem::getDirectorySeparator()
{
    return id::file_system::get_directory_separator();
}

std::string FileSystem::getWorkingDirectory()
{
    return id::file_system::get_working_directory();
}

std::string FileSystem::sanitize(const std::string& pathName)
{
#ifdef _WIN32
    char buffer[MAX_PATH + 1];
    auto result = GetFullPathName(pathName.c_str(),
                                  MAX_PATH + 1,
                                  buffer,
                                  NULL);
    if (!result)
    {
        throw std::runtime_error("unable to sanitize path name");
    }
    return std::string(buffer);
#else
    // not really needed
    return pathName;
#endif
}

#if defined(_WIN32)
void FileSystem::recurDir(const std::string &pathName, std::deque<std::string> &queue)
{
    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFile((pathName + "\\*").c_str(), &ffd);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        std::cerr << pathName << ": " << "FindFirstFile failed" << std::endl;
        return;
    }
    do
    {
        if (ffd.cFileName[0] == '.')
        {
            if ((ffd.cFileName[1] == '.' && ffd.cFileName[2] == '\0') || ffd.cFileName[1] == '\0')
            {
                continue;
            }
        }
        std::string path = pathName + "\\" + ffd.cFileName;
        queue.push_back(path);
    } while (FindNextFile(hFind, &ffd) != 0);
    FindClose(hFind);
}
#else
void FileSystem::recurDir(const std::string &pathName, std::deque<std::string> &queue)
{
    DIR *dir = opendir(pathName.c_str());
    if (!dir)
    {
        std::cerr << pathName << ": " << strerror(errno) << std::endl;
        return;
    }
    while (dirent *aFile = readdir(dir))
    {
        if (aFile->d_name[0] == '.') continue;
        std::string path = pathName + "/" + aFile->d_name;
        queue.push_back(path);
    }
    closedir(dir);
}
#endif

} // namespace Editor
