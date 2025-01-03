/*
 * DirMan - A small crossplatform class to manage directories
 *
 * Copyright (c) 2017-2025 Vitaliy Novichkov <admin@wohlnet.ru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _WIN32
#include <stdio.h>
#include <limits.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <memory.h>

#include "../include/DirManager/dirman.h"
#include "dirman_private.h"

#if defined(__APPLE__) && MAC_OS_X_VERSION_MAX_ALLOWED < 1010 && defined(DIRMAN_HAS_FSSTATAT)
#   undef DIRMAN_HAS_FSSTATAT  /*This call isn't available at macOS older than 10.10 */
#endif

#ifdef PGE_USE_ARCHIVES
#   include "Archives/archives.h"
#endif

#ifdef __WIIU__
// Workaround to avoid the EIO error on virtual directories

static const char * s_sys_paths[] =
{
    "fs:/vol/external01/",
    "fs:/vol/content/",
    "fs:/vol/save/common/",
    "usb:/",
    nullptr
};

static int get_sys_path_offset(const std::string &dirPath)
{
    const char** p = s_sys_paths;
    int offset = 1;

    while(*p)
    {
        std::string t = std::string(*p);

        if(dirPath.size() < t.size())
        {
            p++;
            continue;
        }

        if(dirPath.substr(0, t.size()).compare(t) == 0)
        {
            offset = t.size();
            break;
        }
        p++;
    }

    return offset;
}
#endif

void DirMan::DirMan_private::setPath(const std::string &dirPath)
{
#ifdef PGE_USE_ARCHIVES
    if(Archives::has_prefix(dirPath))
    {
        m_dirPath = dirPath;

        if(m_dirPath.size() > 2 && m_dirPath[m_dirPath.size() - 2] == ':')
            return;

        delEnd(m_dirPath, '/');
        return;
    }
#endif // PGE_USE_ARCHIVES

#ifdef DIRMAN_HAS_REALPATH
    char rp[PATH_MAX];
    memset(rp, 0, PATH_MAX);
    char* realPath = realpath(dirPath.c_str(), rp);
    (void)realPath;

    if(strlen(rp) > 0)
        m_dirPath = rp;
    else // If failed
        m_dirPath = dirPath;
#else
    m_dirPath = dirPath;

    if(dirPath.size() > 2 && dirPath[dirPath.size() - 2] == ':')
        return;
#endif

    delEnd(m_dirPath, '/');
}

bool DirMan::DirMan_private::getListOfFiles(std::vector<std::string> &list, const std::vector<std::string> &suffix_filters)
{
    list.clear();

#ifdef PGE_USE_ARCHIVES
    if(Archives::has_prefix(m_dirPath))
    {
        for(auto& ent : Archives::list_dir(m_dirPath.c_str()))
        {
            if(ent.type != Archives::PATH_FILE)
                continue;

            if(!matchSuffixFilters(ent.name, suffix_filters))
                continue;

            list.push_back(std::move(ent.name));
        }
        return true;
    }
#endif // PGE_USE_ARCHIVES

    dirent *dent = nullptr;
    DIR *srcdir = opendir(m_dirPath.c_str());

    if(srcdir == nullptr)
        return false;

    while((dent = readdir(srcdir)) != nullptr)
    {
#ifdef DIRMAN_HAS_FSSTATAT
        struct stat st = {};
#endif

        if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
            continue;

#ifdef DIRMAN_HAS_FSSTATAT
        if(fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0)
            continue;

        if(S_ISREG(st.st_mode))
        {
            if(matchSuffixFilters(dent->d_name, suffix_filters))
                list.emplace_back(dent->d_name);
        }
#else
        if(dent->d_type == DT_REG)
        {
            if(matchSuffixFilters(dent->d_name, suffix_filters))
                list.emplace_back(dent->d_name);
        }
#endif
    }

    closedir(srcdir);
    return true;
}

bool DirMan::DirMan_private::getListOfFolders(std::vector<std::string>& list, const std::vector<std::string>& suffix_filters)
{
    list.clear();

#ifdef PGE_USE_ARCHIVES
    if(Archives::has_prefix(m_dirPath))
    {
        for(auto& ent : Archives::list_dir(m_dirPath.c_str()))
        {
            if(ent.type != Archives::PATH_DIR)
                continue;

            if(!matchSuffixFilters(ent.name, suffix_filters))
                continue;

            list.push_back(std::move(ent.name));
        }
        return true;
    }
#endif // PGE_USE_ARCHIVES

    dirent *dent = nullptr;
    DIR *srcdir = opendir(m_dirPath.c_str());

    if(srcdir == nullptr)
        return false;

    while((dent = readdir(srcdir)) != nullptr)
    {
#ifdef DIRMAN_HAS_FSSTATAT
        struct stat st = {};
#endif
        if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
            continue;

#ifdef DIRMAN_HAS_FSSTATAT
        if(fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0)
            continue;

        if(S_ISDIR(st.st_mode))
        {
            if(matchSuffixFilters(dent->d_name, suffix_filters))
                list.emplace_back(dent->d_name);
        }
#else
        if(dent->d_type == DT_DIR)
        {
            if(matchSuffixFilters(dent->d_name, suffix_filters))
                list.emplace_back(dent->d_name);
        }
#endif
    }

    closedir(srcdir);

    return true;
}

bool DirMan::DirMan_private::fetchListFromWalker(std::string &curPath, std::vector<std::string> &list)
{
    PUT_THREAD_GUARD();

#ifdef PGE_USE_ARCHIVES
    // unsupported for now
    if(Archives::has_prefix(m_dirPath))
        return false;
#endif // PGE_USE_ARCHIVES

    if(m_walkerState.digStack.empty())
        return false;

    list.clear();

    std::string path = m_walkerState.digStack.top();
    m_walkerState.digStack.pop();

    dirent *dent = nullptr;
    DIR *srcdir = opendir(path.c_str());
    if(srcdir == nullptr) //Can't read this directory. Continue
        return true;

    while((dent = readdir(srcdir)) != nullptr)
    {
#ifdef DIRMAN_HAS_FSSTATAT
        struct stat st;
#endif
        if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
            continue;

#ifdef DIRMAN_HAS_FSSTATAT
        if(fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0)
            continue;

        if(S_ISDIR(st.st_mode))
            m_walkerState.digStack.push(path + "/" + dent->d_name);
        else if(S_ISREG(st.st_mode))
        {
            if(matchSuffixFilters(dent->d_name, m_walkerState.suffix_filters))
                list.emplace_back(dent->d_name);
        }
#else
        if(dent->d_type == DT_DIR)
            m_walkerState.digStack.push(path + "/" + dent->d_name);
        else if(dent->d_type == DT_REG)
        {
            if(matchSuffixFilters(dent->d_name, m_walkerState.suffix_filters))
                list.emplace_back(dent->d_name);
        }
#endif
    }

    closedir(srcdir);
    curPath = path;

    return true;
}

bool DirMan::exists(const std::string &dirPath)
{
    PUT_THREAD_GUARD();

#ifdef PGE_USE_ARCHIVES
    if(Archives::has_prefix(dirPath))
        return Archives::exists(dirPath.c_str()) == Archives::PATH_DIR;
#endif // PGE_USE_ARCHIVES

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
    PUT_THREAD_GUARD();

#ifdef PGE_USE_ARCHIVES
    if(Archives::has_prefix(dirPath))
        return false;
#endif // PGE_USE_ARCHIVES

    return ::mkdir(dirPath.c_str(), S_IRWXU | S_IRWXG) == 0;
}

bool DirMan::rmAbsDir(const std::string &dirPath)
{
    PUT_THREAD_GUARD();

#ifdef PGE_USE_ARCHIVES
    if(Archives::has_prefix(dirPath))
        return false;
#endif // PGE_USE_ARCHIVES

    return ::rmdir(dirPath.c_str()) == 0;
}

bool DirMan::mkAbsPath(const std::string &dirPath)
{
    PUT_THREAD_GUARD();

#ifdef PGE_USE_ARCHIVES
    if(Archives::has_prefix(dirPath))
        return false;
#endif // PGE_USE_ARCHIVES

    char tmp[PATH_MAX + 1];
    char *p = nullptr;
    size_t len;

    memset(tmp, 0, sizeof(tmp));
    snprintf(tmp, PATH_MAX, "%s", dirPath.c_str());
    len = strlen(tmp);

    if(len > 0 && tmp[len - 1] == '/')
        tmp[len - 1] = 0;

#ifdef __WIIU__
    p = tmp + get_sys_path_offset(dirPath);
#else
    p = tmp + 1;
#endif

    for(; *p; p++)
    {
        if(*p == '/')
        {
            *p = 0;

            int err = ::mkdir(tmp, S_IRWXU | S_IRWXG);
            if((err != 0) && (errno != EEXIST))
            {
                *p = '/';
                return false;
            }
            *p = '/';
        }
    }
    return ::mkdir(tmp, S_IRWXU | S_IRWXG) == 0;
}

bool DirMan::rmAbsPath(const std::string &dirPath)
{
    PUT_THREAD_GUARD();

#ifdef PGE_USE_ARCHIVES
    if(Archives::has_prefix(dirPath))
        return false;
#endif // PGE_USE_ARCHIVES

    int ret = 0;
    struct DirStackEntry
    {
        std::string path;
        DIR         *d;
        struct dirent *p;
    };

    std::stack<DirStackEntry> dirStack;
    dirStack.push({dirPath, nullptr, nullptr});

    while(!dirStack.empty())
    {
        DirStackEntry *e = &dirStack.top();
        e->d = opendir(e->path.c_str());

        bool walkUp = false;
        if(e->d)
        {
            while((e->p = readdir(e->d)) != nullptr)
            {
#ifdef DIRMAN_HAS_FSSTATAT
                struct stat st = {};
#endif
                if(strcmp(e->p->d_name, ".") == 0 || strcmp(e->p->d_name, "..") == 0)
                    continue;

#ifdef DIRMAN_HAS_FSSTATAT
                if(fstatat(dirfd(e->d), e->p->d_name, &st, 0) < 0)
                    continue;
#else
                if(e->p->d_type == DT_UNKNOWN)
                    continue;
#endif

                std::string path = e->path + "/" + e->p->d_name;

#ifdef DIRMAN_HAS_FSSTATAT
                if(S_ISDIR(st.st_mode))
#else
                if(e->p->d_type == DT_DIR)
#endif
                {
                    closedir(e->d);
                    e->d = nullptr;
                    dirStack.push({path, nullptr, nullptr});
                    walkUp = true;
                    break;
                }
                else
                {
                    if(::unlink(path.c_str()) != 0)
                        ret = -1;
                }
            }
        }

        if(!walkUp)
        {
            if(e->d)
                closedir(e->d);
            e->d = nullptr;
            if(::rmdir(e->path.c_str()) != 0)
                ret = -1;
            e = nullptr;
            dirStack.pop();
        }
    }

    return (ret == 0);
}

#endif
