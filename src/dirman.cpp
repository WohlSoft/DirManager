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

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#include <algorithm>
//TODO
#else
#include <limits.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <string.h>
#include <stdlib.h>

#include "dirman.h"

#ifdef _WIN32
static std::wstring Str2WStr(const std::string &str)
{
    std::wstring dest;
    dest.resize(str.size());
    int newlen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), &dest[0], str.length());
    dest.resize(newlen);
    return dest;
}

static std::string WStr2Str(const std::wstring &wstr)
{
    std::string dest;
    dest.resize((wstr.size() * 2));
    int newlen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.length(), &dest[0], dest.size(), NULL, NULL);
    dest.resize(newlen);
    return dest;
}
#endif

bool DirMan::matchSuffixFilters(const std::string &name, const std::vector<std::string> &suffixFilters)
{
    bool found = false;

    if(suffixFilters.empty())
        return true;//If no filter, grand everything

    for(const std::string &suffix : suffixFilters)
    {
        if(suffix.size() > name.size())
            continue;

        std::string f;
        f.reserve(name.size());
        for(const char &c : name)
            f.push_back(std::tolower(c));

        found |= (f.compare(f.size() - suffix.size(), suffix.size(), suffix) == 0);

        if(found) return true;
    }

    return found;
}

DirMan::DirMan(const std::string &dirPath):
    m_dirPath(dirPath)
{
    setPath(dirPath);
}

DirMan::~DirMan()
{}

void DirMan::setPath(const std::string &dirPath)
{
    #ifdef _WIN32
    std::wstring dirPathW = Str2WStr(dirPath);
    wchar_t fullPath[MAX_PATH];
    GetFullPathNameW(dirPathW.c_str(), MAX_PATH, fullPath, NULL);
    m_dirPathW = fullPath;
    //Force UNIX paths
    std::replace(m_dirPathW.begin(), m_dirPathW.end(), L'\\', L'/');
    m_dirPath = WStr2Str(m_dirPathW);
    #else
    char resolved_path[PATH_MAX];
    memset(resolved_path, 0, PATH_MAX);
    realpath(dirPath.c_str(), resolved_path);
    m_dirPath = resolved_path;
    #endif

    if(!m_dirPath.empty())
    {
        char last = m_dirPath[m_dirPath.size() - 1];
        if((last == '/') || (last == '\\'))
            m_dirPath.resize(m_dirPath.size() - 1);
    }
}

bool DirMan::getListOfFiles(std::vector<std::string> &list, const std::vector<std::string> &suffix_filters)
{
    list.clear();
    #ifdef _WIN32
    HANDLE hFind;
    WIN32_FIND_DATAW data;

    hFind = FindFirstFileW((m_dirPathW + L"/*").c_str(), &data);
    if(hFind == INVALID_HANDLE_VALUE)
        return false;
    do
    {
        if((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            if((wcscmp(data.cFileName, L"..") == 0) || (wcscmp(data.cFileName, L".") == 0))
                continue;
        }
        else
        {
            std::string fileName = WStr2Str(data.cFileName);
            if(matchSuffixFilters(fileName, suffix_filters))
                list.push_back(fileName);
        }
    }
    while(FindNextFileW(hFind, &data));
    FindClose(hFind);
    #else
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
    #endif
    return true;
}

std::string DirMan::absolutePath()
{
    return m_dirPath;
}

bool DirMan::exists()
{
    return exists(m_dirPath);
}

bool DirMan::exists(const std::string &dirPath)
{
    #ifdef _WIN32
    DWORD ftyp = GetFileAttributesW(Str2WStr(dirPath).c_str());
    if(ftyp == INVALID_FILE_ATTRIBUTES)
        return false;   //something is wrong with your path!
    if(ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true;    // this is a directory!
    return false;       // this is not a directory!
    #else
    DIR *dir = opendir(dirPath.c_str());
    if(dir)
    {
        closedir(dir);
        return false;
    }
    else
        return false;
    #endif
}

bool DirMan::beginIteration(const std::vector<std::string> &suffix_filters)
{
    // Clear previous state
    while(!m_iterator.digStack.empty())
        m_iterator.digStack.pop();

    // Initialize suffix filters
    m_iterator.suffix_filters.clear();
    m_iterator.suffix_filters.reserve(suffix_filters.size());
    for(const std::string &filter : suffix_filters)
    {
        std::string f;
        f.reserve(filter.size());
        for(const char &c : filter)
            f.push_back(std::tolower(c));
        m_iterator.suffix_filters.push_back(f);
    }
    // Push initial path
    m_iterator.digStack.push(m_dirPath);
    return true;
}

bool DirMan::getListOfFilesFromIterator(std::string &curPath, std::vector<std::string> &list)
{
    if(m_iterator.digStack.empty())
        return false;

    list.clear();

    std::string path = m_iterator.digStack.top();
    m_iterator.digStack.pop();

    #ifdef _WIN32
    HANDLE hFind;
    WIN32_FIND_DATAW data;

    hFind = FindFirstFileW(Str2WStr(path + "/*").c_str(), &data);
    if(hFind == INVALID_HANDLE_VALUE)
        return true; //Can't read this directory. Continue
    do
    {
        if((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            if((wcscmp(data.cFileName, L"..") == 0) || (wcscmp(data.cFileName, L".") == 0))
                continue;

            m_iterator.digStack.push(path + "/" + WStr2Str(data.cFileName));
        }
        else
        {
            std::string fileNameU = WStr2Str(data.cFileName);
            if(matchSuffixFilters(fileNameU, m_iterator.suffix_filters))
                list.push_back(fileNameU);
        }
    }
    while(FindNextFileW(hFind, &data));
    FindClose(hFind);
    #else
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
            m_iterator.digStack.push(path + "/" + dent->d_name);
        else if(S_ISREG(st.st_mode))
        {
            if(matchSuffixFilters(dent->d_name, m_iterator.suffix_filters))
                list.push_back(dent->d_name);
        }
    }
    closedir(srcdir);
    #endif

    curPath = path;

    return true;
}
