/*
DirMan - A small crossplatform class to manage directories

Copyright (c) 2017 Vitaliy Novichkov <admin@wohlnet.ru>

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifndef _WIN32
#include <limits.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <memory.h>

#include "dirman.h"
#include "dirman_private.h"

void DirMan::DirMan_private::setPath(const std::string &dirPath)
{
    char resolved_path[PATH_MAX];
    memset(resolved_path, 0, PATH_MAX);
    realpath(dirPath.c_str(), resolved_path);
    m_dirPath = resolved_path;
    delEnd(m_dirPath, '/');
}

bool DirMan::DirMan_private::getListOfFiles(std::vector<std::string> &list, const std::vector<std::string> &suffix_filters)
{
    list.clear();

    dirent *dent = NULL;
    DIR *srcdir = opendir(m_dirPath.c_str());
    if(srcdir == NULL)
        return false;

    while((dent = readdir(srcdir)) != NULL)
    {
        struct stat st;
        if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
            continue;

        if(fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0)
            continue;
        if(S_ISREG(st.st_mode))
        {
            if(matchSuffixFilters(dent->d_name, suffix_filters))
                list.push_back(dent->d_name);
        }
    }
    closedir(srcdir);
    return true;
}


bool DirMan::DirMan_private::fetchListFromWalker(std::string &curPath, std::vector<std::string> &list)
{
    if(m_walkerState.digStack.empty())
        return false;

    list.clear();

    std::string path = m_walkerState.digStack.top();
    m_walkerState.digStack.pop();

    dirent *dent = NULL;
    DIR *srcdir = opendir(path.c_str());
    if(srcdir == NULL) //Can't read this directory. Continue
        return true;

    while((dent = readdir(srcdir)) != NULL)
    {
        struct stat st;
        if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
            continue;

        if(fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0)
            continue;

        if(S_ISDIR(st.st_mode))
            m_walkerState.digStack.push(path + "/" + dent->d_name);
        else if(S_ISREG(st.st_mode))
        {
            if(matchSuffixFilters(dent->d_name, m_walkerState.suffix_filters))
                list.push_back(dent->d_name);
        }
    }
    closedir(srcdir);
    curPath = path;

    return true;
}

bool DirMan::exists(const std::string &dirPath)
{
    DIR *dir = opendir(dirPath.c_str());
    if(dir)
    {
        closedir(dir);
        return true;
    }
    else
        return false;
}

bool DirMan::mkAbsDir(const std::string &dirPath)
{
    return ::mkdir(dirPath.c_str(), S_IRWXU | S_IRWXG) == 0;
}

bool DirMan::mkAbsPath(const std::string &dirPath)
{
    return false;
}

#endif