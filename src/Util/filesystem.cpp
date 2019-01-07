/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifdef WIN32 //TODO: use GetFullPathNameW in system_complete if getcwd not supported

#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <cstdio> //remove()
#include <cerrno>
#include <cstring>
#include <iostream>
#include <fstream>

#include <Util/include/debug.h>
#include <Util/include/filesystem.h>

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

extern int errno;

namespace filesystem
{
    /* Removes a file or folder from the specified path. */
    bool remove(const std::string &path)
    {
        if(exists(path) == false)
            return false;

        if(remove(path.c_str()) == 0)
            return true;

        return false;
    }

    /* Determines if the file or folder from the specified path exists. */
    bool exists(const std::string &path)
    {
        if(access(path.c_str(), F_OK) != -1)
            return true;

        return false;
    }

    /* Copy a file. */
    bool copy_file(const std::string &pathSource, const std::string &pathDest)
    {
        try
        {
            /* Make sure destination is a file, not a directory. */
            if (exists(pathDest) && is_directory(pathDest))
                return false;

            /* If destination file exists, remove it (ie, we overwrite the file) */
            if (exists(pathDest))
                remove(pathDest);

            /* Get the input stream of source file. */
            std::ifstream sourceFile(pathSource, std::ios::binary);
            sourceFile.exceptions(std::ios::badbit);

            /* Get the output stream of destination file. */
            std::ofstream destFile(pathDest, std::ios::binary);
            destFile.exceptions(std::ios::badbit);

            /* Copy the bytes. */
            destFile << sourceFile.rdbuf();

            /* Close the file handles and flush buffers. */
            sourceFile.close();
            destFile.close();

#ifndef WIN32
            /* Set destination file permissions (read/write by owner only for data files) */
            mode_t m = S_IRUSR | S_IWUSR;
            chmod(pathDest.c_str(), m);
#endif

        }
        catch(const std::ios_base::failure &e)
        {
            return debug::error(FUNCTION " failed to write %s", __PRETTY_FUNCTION__, e.what());
        }

        return true;
    }

    /* Determines if the specified path is a folder. */
    bool is_directory(const std::string &path)
    {
        struct stat statbuf;

        if(stat(path.c_str(), &statbuf) != 0)
            return false;

        if(S_ISDIR(statbuf.st_mode))
            return true;

        return false;
    }

    /*  Recursively create directories along the path if they don't exist. */
    bool create_directories(const std::string &path)
    {
        for(auto it = path.begin(); it != path.end(); ++it)
        {
            if(*it == '/' && it != path.begin())
            {
                if(!create_directory(std::string(path.begin(), it)))
                    return false;
            }
        }
        return true;
    }

    /* Create a single directory at the path if it doesn't already exist. */
    bool create_directory(const std::string &path)
    {
        if(exists(path)) //if the directory exists, don't attempt to create it
            return true;

        /* Set directory with read/write/search permissions for owner/group/other */
        mode_t m = S_IRWXU | S_IRWXG | S_IRWXO;
        int status = mkdir(path.c_str(), m);

        /* Handle failures. */
        if(status < 0)
        {
            return debug::error(FUNCTION "Failed to create directory: %s\nReason: %s", __PRETTY_FUNCTION__,
                path.c_str(), strerror(errno));
        }

        return true;
    }

    /* Obtain the system complete path from a given relative path. */
    std::string system_complete(const std::string &path)
    {
        char buffer[MAX_PATH] = {0};

        std::string rel_path = path;
        std::string abs_path;

        //get the path of the current directory and append path name to that
        abs_path = getcwd(buffer, MAX_PATH);
    #ifdef WIN32
        rel_path += '\\';
        abs_path += '\\';
    #else
        rel_path += '/';
        abs_path += '/';
    #endif

        return abs_path + rel_path;
    }

}
