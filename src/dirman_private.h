#ifndef DIRMAN_PRIVATE_H
#define DIRMAN_PRIVATE_H

#include <string>
#include <stack>
#include <vector>
#include <stdlib.h>

template<class CHAR>
static inline void delEnd(std::basic_string<CHAR> &dirPath, CHAR ch)
{
    if(!dirPath.empty())
    {
        char last = dirPath[dirPath.size() - 1];
        if(last == ch)
            dirPath.resize(dirPath.size() - 1);
    }
}

extern bool matchSuffixFilters(const std::string &name, const std::vector<std::string> &suffixFilters);

class DirMan::DirMan_private
{
    friend class DirMan;
    std::string m_dirPath;
    #ifdef _WIN32
    std::wstring m_dirPathW;
    #endif
    struct DirWalkerState
    {
        #ifdef _WIN32
        std::stack<std::wstring>    digStack;
        #else
        std::stack<std::string>     digStack;
        #endif
        std::vector<std::string>    filesList;
        std::vector<std::string>    suffix_filters;
    }
    m_walkerState;

    void setPath(const std::string &dirPath);
    bool getListOfFiles(std::vector<std::string> &list, const std::vector<std::string> &suffix_filters);
    bool fetchListFromWalker(std::string &curPath, std::vector<std::string> &list);

public:
    DirMan_private() = default;
    DirMan_private(const DirMan_private&) = default;
};

#endif // DIRMAN_PRIVATE_H