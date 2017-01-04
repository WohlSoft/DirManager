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

#ifndef DIRMAN_H
#define DIRMAN_H

#include <string>
#include <stack>
#include <vector>

class DirMan
{
    std::string m_dirPath;
    #ifdef _WIN32
    std::wstring m_dirPathW;
    #endif
    struct Iterator
    {
        std::stack<std::string>     digStack;
        std::vector<std::string>    filesList;
        std::vector<std::string>    suffix_filters;
    } m_iterator;

    bool matchSuffixFilters(const std::string &name, const std::vector<std::string> &suffixFilters);

public:
    explicit DirMan(const std::string &dirPath = "./");
    virtual ~DirMan();

    /**
     * @brief Change root path
     * @param dirPath absolute or relative to current application path
     */
    void     setPath(const std::string &dirPath);

    /**
     * @brief Get list of files in this directory
     * @param list target list to output
     * @return true if success, false if any error has occouped
     */
    bool     getListOfFiles(std::vector<std::string> &list, const std::vector<std::string> &suffix_filters = std::vector<std::string>());

    /**
     * @brief Absolude directory path
     * @return string
     */
    std::string absolutePath();

    /**
     * @brief Is this directory exists
     * @return true is current directory is exists
     */
    bool        exists();

    /**
     * @brief Check if any directory exists
     * @param dirPath path of directory
     * @return true if directory is exists
     */
    static bool exists(const std::string &dirPath);

    bool        beginIteration(const std::vector<std::string> &suffix_filters = std::vector<std::string>());
    bool        getListOfFilesFromIterator(std::string &curPath, std::vector<std::string> &list);
};

#endif // DIRMAN_H
