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

#ifdef _WIN32
#include <windows.h>
#include <algorithm>

#include "dirman.h"
#include "dirman_private.h"

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

void DirMan::DirMan_private::setPath(const std::string &dirPath)
{
    m_dirPathW = Str2WStr(dirPath);
    wchar_t fullPath[MAX_PATH];
    GetFullPathNameW(m_dirPathW.c_str(), MAX_PATH, fullPath, NULL);
    m_dirPathW = fullPath;
    //Force UNIX paths
    std::replace(m_dirPathW.begin(), m_dirPathW.end(), L'\\', L'/');
    m_dirPath = WStr2Str(m_dirPathW);
    delEnd(m_dirPathW, L'/');
    delEnd(m_dirPath, '/');
}

bool DirMan::DirMan_private::getListOfFiles(std::vector<std::string> &list, const std::vector<std::string> &suffix_filters)
{
    list.clear();
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

    return true;
}

bool DirMan::DirMan_private::fetchListFromWalker(std::string &curPath, std::vector<std::string> &list)
{
    if(m_walkerState.digStack.empty())
        return false;

    list.clear();

    std::wstring path = m_walkerState.digStack.top();
    m_walkerState.digStack.pop();

    HANDLE hFind;
    WIN32_FIND_DATAW data;

    hFind = FindFirstFileW((path + L"/*").c_str(), &data);
    if(hFind == INVALID_HANDLE_VALUE)
        return true; //Can't read this directory. Continue
    do
    {
        if((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            if((wcscmp(data.cFileName, L"..") == 0) || (wcscmp(data.cFileName, L".") == 0))
                continue;

            m_walkerState.digStack.push(path + L"/" + data.cFileName);
        }
        else
        {
            std::string fileNameU = WStr2Str(data.cFileName);
            if(matchSuffixFilters(fileNameU, m_walkerState.suffix_filters))
                list.push_back(fileNameU);
        }
    }
    while(FindNextFileW(hFind, &data));

    FindClose(hFind);
    curPath = WStr2Str(path);

    return true;
}

bool DirMan::exists(const std::string &dirPath)
{
    DWORD ftyp = GetFileAttributesW(Str2WStr(dirPath).c_str());
    if(ftyp == INVALID_FILE_ATTRIBUTES)
        return false;   //something is wrong with your path!
    if(ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true;    // this is a directory!
    return false;       // this is not a directory!
}

bool DirMan::mkAbsDir(const std::string &dirPath)
{
    return (CreateDirectory(Str2WStr(dirPath).c_str(), NULL) != FALSE);
}

bool DirMan::mkAbsPath(const std::string &dirPath)
{
#warning("DirMan::mkAbsPath is not implemented!")
    return false;
}

#endif
