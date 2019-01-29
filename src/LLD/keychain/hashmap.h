/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_HASHMAP_H
#define NEXUS_LLD_TEMPLATES_HASHMAP_H

#include <LLD/templates/key.h>
#include <LLD/cache/template_lru.h>

#include <iostream>

#include <algorithm>

#include <condition_variable>

#include <atomic>

#include <openssl/md5.h>


//TODO: Abstract base class for all keychains
namespace LLD
{


    /** BinaryHashMap
     *
     *  This class is responsible for managing the keys to the sector database.
     *
     *  It contains a Binary Hash Map with a minimum complexity of O(1).
     *  It uses a linked file list based on index to iterate trhough files and binary Positions
     *  when there is a collision that is found.
     *
     **/
    class BinaryHashMap
    {
    protected:

        /** Mutex for Thread Synchronization. **/
        mutable std::mutex KEY_MUTEX;


        /* The condition for thread sleeping. */
        std::condition_variable CONDITION;


        /** The string to hold the database location. **/
        std::string strBaseLocation;


        /** Keychain stream object. **/
        mutable TemplateLRU<uint32_t, std::fstream *> *fileCache;


        /** Total elements in hashmap for quick inserts. **/
        mutable std::vector<uint32_t> hashmap;


        /* The cache writer thread. */
        std::thread CacheThread;


        /** The Maximum buckets allowed in the hashmap. */
        uint32_t HASHMAP_TOTAL_BUCKETS;


        /** The Maximum cache size for allocated keys. **/
        uint32_t HASHMAP_MAX_CACHE_SZIE;


        /** The Maximum key size for static key sectors. **/
        uint16_t HASHMAP_MAX_KEY_SIZE;


        /** The total space that a key consumes. */
        uint16_t HASHMAP_KEY_ALLOCATION;


        /** The keychain flags. **/
        uint8_t nFlags;


        /** Initialized flag (used for cache thread) **/
        std::atomic<bool> fCacheActive;


        /** Destructor flag. **/
        std::atomic<bool> fDestruct;


    public:

        /** Default Constructor **/
        BinaryHashMap()
        : KEY_MUTEX()
        , CONDITION()
        , strBaseLocation()
        , fileCache(new TemplateLRU<uint32_t, std::fstream*>(8))
        , hashmap(256 * 256 * 24)
        , CacheThread()
        , HASHMAP_TOTAL_BUCKETS(256 * 256 * 24)
        , HASHMAP_MAX_CACHE_SZIE(10 * 1024)
        , HASHMAP_MAX_KEY_SIZE(32)
        , HASHMAP_KEY_ALLOCATION(HASHMAP_MAX_KEY_SIZE + 13)
        , nFlags(FLAGS::APPEND)
        , fCacheActive(false)
        , fDestruct(false)
        {
            CacheThread = std::thread(std::bind(&BinaryHashMap::CacheWriter, this));
        }


        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        BinaryHashMap(std::string strBaseLocationIn, uint8_t nFlagsIn = FLAGS::APPEND)
        : KEY_MUTEX()
        , CONDITION()
        , strBaseLocation(strBaseLocationIn)
        , fileCache(new TemplateLRU<uint32_t, std::fstream*>(8))
        , hashmap(256 * 256 * 24)
        , CacheThread()
        , HASHMAP_TOTAL_BUCKETS(256 * 256 * 24)
        , HASHMAP_MAX_CACHE_SZIE(10 * 1024)
        , HASHMAP_MAX_KEY_SIZE(32)
        , HASHMAP_KEY_ALLOCATION(HASHMAP_MAX_KEY_SIZE + 13)
        , nFlags(nFlagsIn)
        , fCacheActive(false)
        , fDestruct(false)
        {
            Initialize();
            CacheThread = std::thread(std::bind(&BinaryHashMap::CacheWriter, this));
        }


        /** Default Constructor **/
        BinaryHashMap(std::string strBaseLocationIn, uint32_t nTotalBuckets, uint32_t nMaxCacheSize, uint8_t nFlagsIn = FLAGS::APPEND)
        : KEY_MUTEX()
        , CONDITION()
        , strBaseLocation(strBaseLocationIn)
        , fileCache(new TemplateLRU<uint32_t, std::fstream*>(8))
        , hashmap(nTotalBuckets)
        , CacheThread()
        , HASHMAP_TOTAL_BUCKETS(nTotalBuckets)
        , HASHMAP_MAX_CACHE_SZIE(nMaxCacheSize)
        , HASHMAP_MAX_KEY_SIZE(32)
        , HASHMAP_KEY_ALLOCATION(HASHMAP_MAX_KEY_SIZE + 13)
        , nFlags(nFlagsIn)
        , fCacheActive(false)
        , fDestruct(false)
        {
            Initialize();
            CacheThread = std::thread(std::bind(&BinaryHashMap::CacheWriter, this));
        }


        /** Copy Assignment Operator **/
        BinaryHashMap& operator=(BinaryHashMap map)
        {
            strBaseLocation       = map.strBaseLocation;
            fileCache             = map.fileCache;

            return *this;
        }


        /** Copy Constructor **/
        BinaryHashMap(const BinaryHashMap& map)
        {
            strBaseLocation    = map.strBaseLocation;
            fileCache          = map.fileCache;
        }


        /** Default Destructor **/
        ~BinaryHashMap()
        {
            fDestruct = true;
            CONDITION.notify_all();

            CacheThread.join();
            delete fileCache;
        }


        /** CompressKey
         *
         *  Compresses a given key until it matches size criteria.
         *  This function is one way and efficient for reducing key sizes.
         *
         *  @param[out] vData The binary data of key to compress.
         *  @param[in] nSize The desired size of key after compression.
         *
         **/
        void CompressKey(std::vector<uint8_t>& vData, uint16_t nSize = 32)
        {
            /* Loop until key is of desired size. */
            while(vData.size() > nSize)
            {
                /* Loop half of the key to XOR elements. */
                for(uint64_t i = 0; i < vData.size() / 2; ++i)
                    if(i * 2 < vData.size())
                        vData[i] ^= vData[i * 2];

                /* Resize the container to half its size. */
                vData.resize(vData.size() / 2);
            }
        }


        /** GetKeys
         *
         *  Placeholder.
         *
         **/
         std::vector< std::vector<uint8_t> > GetKeys()
         {
             std::vector< std::vector<uint8_t> > vKeys;

             return vKeys;
         }


        /** GetBucket
         *
         *  Calculates a bucket to be used for the hashmap allocation.
         *
         *  @param[in] vKey The key object to calculate with.
         *
         *  @return The bucket assigned to the key.
         *
         **/
        uint32_t GetBucket(const std::vector<uint8_t>& vKey)
        {
            /* Get an MD5 digest. */
            uint8_t digest[MD5_DIGEST_LENGTH];
            MD5((uint8_t *)&vKey[0], vKey.size(), (uint8_t *)&digest);

            /* Copy bytes into the bucket. */
            uint64_t nBucket;
            std::copy((uint8_t *)&digest[0], (uint8_t *)&digest[0] + 8, (uint8_t *)&nBucket);

            return nBucket % HASHMAP_TOTAL_BUCKETS;
        }


        /** Initialize
         *
         *  Initialize the binary hash map keychain.
         *
         **/
        void Initialize()
        {
            /* Create directories if they don't exist yet. */
            if(!filesystem::exists(strBaseLocation) && filesystem::create_directories(strBaseLocation))
                debug::log(0, FUNCTION, "Generated Path ", strBaseLocation);

            /* Build the hashmap indexes. */
            std::string index = debug::strprintf("%s_hashmap.index", strBaseLocation.c_str());
            if(!filesystem::exists(index))
            {
                /* Generate empty space for new file. */
                std::vector<uint8_t> vSpace(HASHMAP_TOTAL_BUCKETS * 4, 0);

                /* Write the new disk index .*/
                std::fstream stream(index, std::ios::out | std::ios::binary | std::ios::trunc);
                stream.write((char*)&vSpace[0], vSpace.size());
                stream.close();

                /* Debug output showing generation of disk index. */
                debug::log(0, FUNCTION, "Generated Disk Index of ", vSpace.size(), " bytes");
            }

            /* Read the hashmap indexes. */
            else
            {
                /* Build a vector to read the disk index. */
                std::vector<uint8_t> vIndex(HASHMAP_TOTAL_BUCKETS * 4, 0);

                /* Read the disk index bytes. */
                std::fstream stream(index, std::ios::in | std::ios::binary);
                stream.read((char*)&vIndex[0], vIndex.size());
                stream.close();

                /* Deserialize the values into memory index. */
                uint32_t nTotalKeys = 0;
                for(uint32_t nBucket = 0; nBucket < HASHMAP_TOTAL_BUCKETS; ++nBucket)
                {
                    std::copy((uint8_t *)&vIndex[nBucket * 4], (uint8_t *)&vIndex[nBucket * 4] + 4, (uint8_t *)&hashmap[nBucket]);

                    nTotalKeys += hashmap[nBucket];
                }

                /* Debug output showing loading of disk index. */
                debug::log(0, FUNCTION, "Loaded Disk Index of ", vIndex.size(), " bytes and ", nTotalKeys, " keys");
            }

            /* Build the first hashmap index file if it doesn't exist. */
            std::string file = debug::strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), 0u).c_str();
            if(!filesystem::exists(file))
            {
                /* Build a vector with empty bytes to flush to disk. */
                std::vector<uint8_t> vSpace(HASHMAP_TOTAL_BUCKETS * HASHMAP_KEY_ALLOCATION, 0);

                /* Flush the empty keychain file to disk. */
                std::fstream stream(file, std::ios::out | std::ios::binary | std::ios::trunc);
                stream.write((char*)&vSpace[0], vSpace.size());
                stream.close();

                /* Debug output showing generating of the hashmap file. */
                debug::log(0, FUNCTION, "Generated Disk Hash Map 0 of ", vSpace.size(), " bytes");
            }

            /* Load the stream object into the stream LRU cache. */
            fileCache->Put(0, new std::fstream(file, std::ios::in | std::ios::out | std::ios::binary));

            /* Set the initialization flag to complete. */
            fCacheActive = true;

            /* Notify threads it is initialized. */
            CONDITION.notify_all();
        }


        /** Get
         *
         *  Read a key index from the disk hashmaps.
         *
         *  @param[in] vKey The binary data of key.
         *  @param[out] cKey The key object to return.
         *
         *  @return True if the key was found, false otherwise.
         *
         **/
        bool Get(const std::vector<uint8_t>& vKey, SectorKey &cKey)
        {
            /* Get the assigned bucket for the hashmap. */
            uint32_t nBucket = GetBucket(vKey);

            /* Get the file binary position. */
            uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

            /* Set the cKey return value non compressed. */
            cKey.vKey = vKey;

            /* Compress any keys larger than max size. */
            std::vector<uint8_t> vKeyCompressed = vKey;
            CompressKey(vKeyCompressed, HASHMAP_MAX_KEY_SIZE);

            /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
            std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
            for(int i = hashmap[nBucket] - 1; i >= 0; --i)
            {
                { LOCK(KEY_MUTEX);

                    /* Find the file stream for LRU cache. */
                    std::fstream *pstream;
                    if(!fileCache->Get(i, pstream))
                    {
                        /* Set the new stream pointer. */
                        std::string filename = debug::strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), i);

                        pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                        if(!pstream->is_open())
                        {
                            delete pstream;
                            return debug::error(FUNCTION, "couldn't create hashmap object at: ",
                                filename, " (", strerror(errno), ")");
                        }

                        /* If file not found add to LRU cache. */
                        fileCache->Put(i, pstream);
                    }

                    /* Seek to the hashmap index in file. */
                    pstream->seekg(nFilePos, std::ios::beg);

                    /* Read the bucket binary data from file stream */
                    pstream->read((char*) &vBucket[0], vBucket.size());
                }

                /* Check if this bucket has the key */
                if(std::equal(vBucket.begin() + 13, vBucket.begin() + 13 + vKeyCompressed.size(), vKeyCompressed.begin()))
                {
                    /* Deserialie key and return if found. */
                    DataStream ssKey(vBucket, SER_LLD, DATABASE_VERSION);
                    ssKey >> cKey;

                    /* Debug Output of Sector Key Information. */
                    debug::log(4, FUNCTION, "State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                        " | Length: ", cKey.nLength,
                        " | Bucket ", nBucket,
                        " | Location: ", nFilePos,
                        " | File: ", hashmap[nBucket] - 1,
                        " | Sector File: ", cKey.nSectorFile,
                        " | Sector Size: ", cKey.nSectorSize,
                        " | Sector Start: ", cKey.nSectorStart, "\n",
                        HexStr(vKeyCompressed.begin(), vKeyCompressed.end(), true));

                    return true;
                }
            }

            return false;
        }


        /** Get
         *
         *  Read a key index from the disk hashmaps.
         *  This method iterates all maps to find all keys.
         *
         *  @param[in] vKey The binary data of the key.
         *  @param[out] vKeys The list of keys to return.
         *
         *  @return True if the key was found, false otherwise.
         *
         **/
        bool Get(const std::vector<uint8_t>& vKey, std::vector<SectorKey>& vKeys)
        {
            /* Get the assigned bucket for the hashmap. */
            uint32_t nBucket = GetBucket(vKey);

            /* Get the file binary position. */
            uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

            /* Compress any keys larger than max size. */
            std::vector<uint8_t> vKeyCompressed = vKey;
            CompressKey(vKeyCompressed, HASHMAP_MAX_KEY_SIZE);

            /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
            std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
            for(int i = hashmap[nBucket] - 1; i >= 0; --i)
            {
                { LOCK(KEY_MUTEX);

                    /* Find the file stream for LRU cache. */
                    std::fstream* pstream;
                    if(!fileCache->Get(i, pstream))
                    {
                        std::string filename = debug::strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), i);

                        /* Set the new stream pointer. */
                        pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                        if(!pstream->is_open())
                        {
                            delete pstream;
                            return debug::error(FUNCTION, "couldn't create hashmap object at: ",
                                filename, " (", strerror(errno), ")");
                        }

                        /* If file not found add to LRU cache. */
                        fileCache->Put(i, pstream);
                    }

                    /* Seek to the hashmap index in file. */
                    pstream->seekg (nFilePos, std::ios::beg);

                    /* Read the bucket binary data from file stream */
                    pstream->read((char*) &vBucket[0], vBucket.size());
                }

                /* Check if this bucket has the key */
                if(std::equal(vBucket.begin() + 13, vBucket.begin() + 13 + vKeyCompressed.size(), vKeyCompressed.begin()))
                {
                    /* Deserialize key and return if found. */
                    DataStream ssKey(vBucket, SER_LLD, DATABASE_VERSION);
                    SectorKey cKey;
                    ssKey >> cKey;
                    cKey.vKey = vKey;

                    /* Add key to return vector. */
                    vKeys.push_back(cKey);

                    /* Debug Output of Sector Key Information. */
                    debug::log(4, FUNCTION, "Found State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                        " | Length: ", cKey.nLength,
                        " | Bucket ", nBucket,
                        " | Location: ", nFilePos,
                        " | File: ", hashmap[nBucket] - 1,
                        " | Sector File: ", cKey.nSectorFile,
                        " | Sector Size: ", cKey.nSectorSize,
                        " | Sector Start: ", cKey.nSectorStart, "\n",
                        HexStr(vKeyCompressed.begin(), vKeyCompressed.end(), true));
                }
            }

            return (vKeys.size() > 0);
        }


        /** Put
         *
         *  Write a key to the disk hashmaps.
         *
         *  @param[in] cKey The key object to write.
         *
         *  @return True if the key was written, false otherwise.
         *
         **/
        bool Put(const SectorKey& cKey)
        {
            /* Get the assigned bucket for the hashmap. */
            uint32_t nBucket = GetBucket(cKey.vKey);

            /* Get the file binary position. */
            uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

            /* Compress any keys larger than max size. */
            std::vector<uint8_t> vKeyCompressed = cKey.vKey;
            CompressKey(vKeyCompressed, HASHMAP_MAX_KEY_SIZE);

            /* Handle if not in append mode which will update the key. */
            if(!(nFlags & FLAGS::APPEND))
            {
                /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
                std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
                for(int i = hashmap[nBucket] - 1; i >= 0; --i)
                {
                    { LOCK(KEY_MUTEX);

                        /* Find the file stream for LRU cache. */
                        std::fstream* pstream;
                        if(!fileCache->Get(i, pstream))
                        {
                            std::string filename = debug::strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), i);

                            /* Set the new stream pointer. */
                            pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                            if(!pstream->is_open())
                            {
                                delete pstream;
                                return debug::error(FUNCTION, "couldn't create hashmap object at: ",
                                    filename, " (", strerror(errno), ")");
                            }

                            /* If file not found add to LRU cache. */
                            fileCache->Put(i, pstream);
                        }

                        /* Seek to the hashmap index in file. */
                        pstream->seekg (nFilePos, std::ios::beg);

                        /* Read the bucket binary data from file stream */
                        pstream->read((char*) &vBucket[0], vBucket.size());

                    }

                    /* Check if this bucket has the key */
                    if(std::equal(vBucket.begin() + 13, vBucket.begin() + 13 + vKeyCompressed.size(), vKeyCompressed.begin()))
                    {
                        /* Serialize the key and return if found. */
                        DataStream ssKey(SER_LLD, DATABASE_VERSION);
                        ssKey << cKey;

                        /* Serialize the key into the end of the vector. */
                        ssKey.write((char*)&vKeyCompressed[0], vKeyCompressed.size());

                        { LOCK(KEY_MUTEX);

                            /* Find the file stream for LRU cache. */
                            std::fstream* pstream;
                            if(!fileCache->Get(i, pstream))
                            {
                                std::string filename = debug::strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), i);

                                /* Set the new stream pointer. */
                                pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                                if(!pstream->is_open())
                                {
                                    delete pstream;
                                    return debug::error(FUNCTION, "couldn't create hashmap object at: ",
                                        filename, " (", strerror(errno), ")");
                                }

                                /* If file not found add to LRU cache. */
                                fileCache->Put(i, pstream);
                            }

                            /* Handle the disk writing operations. */
                            pstream->seekp (nFilePos, std::ios::beg);
                            pstream->write((char*)&ssKey.Bytes()[0], ssKey.size());
                            pstream->flush();

                        }

                        /* Debug Output of Sector Key Information. */
                        debug::log(4, FUNCTION, "State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                            " | Length: ", cKey.nLength,
                            " | Bucket ", nBucket,
                            " | Location: ", nFilePos,
                            " | File: ", hashmap[nBucket] - 1,
                            " | Sector File: ", cKey.nSectorFile,
                            " | Sector Size: ", cKey.nSectorSize,
                            " | Sector Start: ", cKey.nSectorStart, "\n",
                            HexStr(vKeyCompressed.begin(), vKeyCompressed.end(), true));

                        /* Signal the cache thread to wake up. */
                        fCacheActive = true;
                        CONDITION.notify_all();

                        return true;
                    }
                }
            }

            /* Create a new disk hashmap object in linked list if it doesn't exist. */
            std::string file = debug::strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), hashmap[nBucket]);
            if(!filesystem::exists(file))
            {
                /* Blank vector to write empty space in new disk file. */
                std::vector<uint8_t> vSpace(HASHMAP_TOTAL_BUCKETS * HASHMAP_KEY_ALLOCATION, 0);

                /* Write the blank data to the new file handle. */
                std::fstream stream(file, std::ios::out | std::ios::binary | std::ios::trunc);
                if(!stream)
                    return debug::error(FUNCTION, strerror(errno));

                stream.write((char*)&vSpace[0], vSpace.size());
                stream.close();

                /* Debug output for monitoring new disk maps. */
                debug::log(0, FUNCTION, "Generated Disk Hash Map ", hashmap[nBucket], " of ", vSpace.size(), " bytes");
            }

            /* Read the State and Size of Sector Header. */
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << cKey;

            /* Serialize the key into the end of the vector. */
            ssKey.write((char*)&vKeyCompressed[0], vKeyCompressed.size());

            { LOCK(KEY_MUTEX);

                /* Find the file stream for LRU cache. */
                std::fstream* pstream;
                if(!fileCache->Get(hashmap[nBucket], pstream))
                {
                    /* Set the new stream pointer. */
                    pstream = new std::fstream(file, std::ios::in | std::ios::out | std::ios::binary);
                    if(!pstream->is_open())
                    {
                        delete pstream;
                        return debug::error(FUNCTION, "Failed to generate file object");
                    }

                    /* If not in cache, add to the LRU. */
                    fileCache->Put(hashmap[nBucket], pstream);
                }

                /* Flush the key file to disk. */
                pstream->seekp (nFilePos, std::ios::beg);
                pstream->write((char*)&ssKey.Bytes()[0], ssKey.size());
                pstream->flush();

                /* Iterate the linked list value in the hashmap. */
                ++hashmap[nBucket];
            }

            /* Debug Output of Sector Key Information. */
            debug::log(4, FUNCTION, "State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                " | Length: ", cKey.nLength,
                " | Bucket ", nBucket,
                " | Location: ", nFilePos,
                " | File: ", hashmap[nBucket] - 1,
                " | Sector File: ", cKey.nSectorFile,
                " | Sector Size: ", cKey.nSectorSize,
                " | Sector Start: ", cKey.nSectorStart,
                " | Key: ",  HexStr(vKeyCompressed.begin(), vKeyCompressed.end()));

            /* Signal the cache thread to wake up. */
            fCacheActive = true;
            CONDITION.notify_all();

            return true;
        }


        /** CacheWriter
         *
         *  Helper Thread to Batch Write to Disk.
         *
         **/
        void CacheWriter()
        {
            std::mutex CONDITION_MUTEX;
            while(!fDestruct.load())
            {

                /* Wait for Database to Initialize. */
                std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
                CONDITION.wait(CONDITION_LOCK, [this]{ return fCacheActive.load() || fDestruct.load(); });

                /* Flush the disk hashmap. */
                std::vector<uint8_t> vDisk;

                /* Lock for hashmap object. */
                { LOCK(KEY_MUTEX);
                    vDisk.insert(vDisk.end(), (uint8_t*)&hashmap[0], (uint8_t*)&hashmap[0] + (4 * hashmap.size()));
                }

                std::string filename = debug::strprintf("%s_hashmap.index", strBaseLocation.c_str());

                /* Create the file handler. */
                std::fstream stream(filename, std::ios::in | std::ios::out | std::ios::binary);
                if(!stream.is_open())
                {
                    debug::error(FUNCTION, "couldn't open file: ",
                        filename, " (", strerror(errno), ")");

                    return;
                }

                stream.seekp(0, std::ios::beg);
                stream.write((char*)&vDisk[0], vDisk.size());
                stream.close();

                /* Verbose logging to show triggered write. */
                debug::log(4, FUNCTION, "Flushed Cache Disk Index");

                fCacheActive = false;
            }
        }


        /** Erase
         *
         *  Erase a key from the disk hashmaps.
         *  TODO: This should be optimized further.
         *
         *  @param[in] vKey the key to erase.
         *
         *  @return True if the key was erased, false otherwise.
         *
         **/
        bool Erase(const std::vector<uint8_t> &vKey)
        {
            /* Get the assigned bucket for the hashmap. */
            uint32_t nBucket = GetBucket(vKey);

            /* Get the file binary position. */
            uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;


            /* Compress any keys larger than max size. */
            std::vector<uint8_t> vKeyCompressed = vKey;
            CompressKey(vKeyCompressed, HASHMAP_MAX_KEY_SIZE);

            /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
            std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
            for(int i = hashmap[nBucket] - 1; i >= 0; --i)
            {
                /* Find the file stream for LRU cache. */
                std::fstream* pstream;
                if(!fileCache->Get(i, pstream))
                {
                    /* Set the new stream pointer. */
                    pstream = new std::fstream(
                      debug::strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), i),
                      std::ios::in | std::ios::out | std::ios::binary);

                    /* If file not found add to LRU cache. */
                    fileCache->Put(i, pstream);
                }

                /* Handle the disk operations. */
                {
                    LOCK(KEY_MUTEX);

                    /* Seek to the hashmap index in file. */
                    pstream->seekg (nFilePos, std::ios::beg);

                    /* Read the bucket binary data from file stream */
                    pstream->read((char*) &vBucket[0], vBucket.size());
                }

                /* Check if this bucket has the key */
                if(std::equal(vBucket.begin() + 13, vBucket.begin() + 13 + vKeyCompressed.size(), vKeyCompressed.begin()))
                {
                    /* Deserialize key and return if found. */
                    DataStream ssKey(vBucket, SER_LLD, DATABASE_VERSION);
                    SectorKey cKey;
                    ssKey >> cKey;

                    /* Handle the disk operations. */
                    {
                        LOCK(KEY_MUTEX);

                        /* Seek to the hashmap index in file. */
                        pstream->seekp (nFilePos, std::ios::beg);

                        /* Read the bucket binary data from file stream */
                        std::vector<uint8_t> vBlank(HASHMAP_KEY_ALLOCATION, 0);
                        pstream->write((char*) &vBlank[0], vBlank.size());
                        pstream->flush();
                    }


                    /* Debug Output of Sector Key Information. */
                    debug::log(4, FUNCTION, "Erased State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                        " | Length: ", cKey.nLength,
                        " | Bucket ", nBucket,
                        " | Location: ", nFilePos,
                        " | File: ", hashmap[nBucket] - 1,
                        " | Sector File: ", cKey.nSectorFile,
                        " | Sector Size: ", cKey.nSectorSize,
                        " | Sector Start: ", cKey.nSectorStart,
                        " | Key: ", HexStr(vKeyCompressed.begin(), vKeyCompressed.end()));
                }
            }

            return true;
        }
    };
}

#endif
