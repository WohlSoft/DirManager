#include <iostream>

#include "../src/dirman.h"

int main(int , char *[])
{
    DirMan myDir("../");

    std::cout << "=============Running test 1 (absolue path from relative)=============" << std::endl;
    std::cout << myDir.absolutePath() << std::endl;

    std::cout << "=============Running test 2 (list of files in folder)=============" << std::endl;
    std::vector<std::string> files;
    myDir.getListOfFiles(files);

    for(std::string &file : files)
        std::cout << file << std::endl;

    std::cout << "=============Running test 3 (walk in the subdirectories)=============" << std::endl;
    std::vector<std::string> filters;
    filters.push_back(".mid");

    myDir.beginIteration(filters);

    std::string itPath;
    while(myDir.getListOfFilesFromIterator(itPath, files))
    {
        for(std::string &file : files)
            std::cout << itPath + "/" + file << std::endl;
    }

    return 0;
}
