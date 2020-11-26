/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>
#include <LLC/hash/SK.h>
#include <LLC/include/random.h>

#include <LLD/include/global.h>
#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/hashmap.h>
#include <LLD/templates/sector.h>

#include <Util/include/debug.h>
#include <Util/include/base64.h>

#include <openssl/rand.h>

#include <LLC/hash/argon2.h>
#include <LLC/hash/SK.h>
#include <LLC/include/flkey.h>
#include <LLC/types/bignum.h>

#include <Util/include/hex.h>

#include <iostream>

#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

#include <TAO/Register/include/create.h>

#include <TAO/Ledger/types/genesis.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/transaction.h>

#include <TAO/Ledger/include/ambassador.h>

#include <Legacy/types/address.h>
#include <Legacy/types/transaction.h>

#include <LLP/templates/ddos.h>
#include <Util/include/runtime.h>

#include <list>
#include <functional>
#include <variant>

#include <Util/include/softfloat.h>
#include <Util/include/filesystem.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/stake.h>

#include <LLP/types/tritium.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/locator.h>

#include <LLD/config/hashmap.h>
#include <LLD/config/sector.h>

class TestDB : public LLD::Templates::StaticDatabase<LLD::BinaryHashMap, LLD::BinaryLRU, LLD::Config::Hashmap>
{
public:
    TestDB(const LLD::Config::Sector& sector, const LLD::Config::Hashmap& keychain)
    : StaticDatabase(sector, keychain)
    {
    }

    ~TestDB()
    {

    }

    bool WriteKey(const uint1024_t& key, const uint1024_t& value)
    {
        return Write(std::make_pair(std::string("key"), key), value);
    }


    bool ReadKey(const uint1024_t& key, uint1024_t &value)
    {
        return Read(std::make_pair(std::string("key"), key), value);
    }


    bool EraseKey(const uint1024_t& key)
    {
        return Erase(std::make_pair(std::string("key"), key));
    }


    bool WriteLast(const uint1024_t& last)
    {
        return Write(std::string("last"), last);
    }

    bool ReadLast(uint1024_t &last)
    {
        return Read(std::string("last"), last);
    }

};

#include <TAO/Ledger/include/genesis_block.h>

const uint256_t hashSeed = 55;

#include <bitset>

#include <LLP/include/global.h>

#include <LLD/cache/template_lru.h>


/* Read an index entry at given bucket crossing file boundaries. */
bool read_index(const uint32_t nBucket, const uint32_t nTotal, const LLD::Config::Hashmap& CONFIG)
{
    const uint32_t INDEX_FILTER_SIZE = 10;

    /* Check we are flushing within range of the hashmap indexes. */
    if(nBucket >= CONFIG.HASHMAP_TOTAL_BUCKETS)
        return debug::error(FUNCTION, "out of range ", VARIABLE(nBucket));

    /* Keep track of how many buckets and bytes we have remaining in this read cycle. */
    uint32_t nRemaining = nTotal;
    uint32_t nIterator  = nBucket;

    /* Calculate the number of bukcets per index file. */
    const uint32_t nTotalBuckets = (CONFIG.HASHMAP_TOTAL_BUCKETS / CONFIG.MAX_FILES_PER_INDEX) + 1;
    const uint32_t nMaxSize = nTotalBuckets * INDEX_FILTER_SIZE;

    /* Adjust our buffer size to fit the total buckets. */
    do
    {
        /* Calculate the file and boundaries we are on with current bucket. */
        const uint32_t nFile = (nIterator * CONFIG.MAX_FILES_PER_INDEX) / CONFIG.HASHMAP_TOTAL_BUCKETS;
        const uint32_t nEnd  = (((nFile + 1) * CONFIG.HASHMAP_TOTAL_BUCKETS) / CONFIG.MAX_FILES_PER_INDEX);

        //const uint32_t nEnd2  = (((nFile + 2) * CONFIG.HASHMAP_TOTAL_BUCKETS) / CONFIG.MAX_FILES_PER_INDEX);
        //const uint64_t nCommon = (nEnd * CONFIG.MAX_FILES_PER_INDEX) / CONFIG.HASHMAP_TOTAL_BUCKETS;

        const uint32_t nBegin = (nFile * CONFIG.HASHMAP_TOTAL_BUCKETS) / CONFIG.MAX_FILES_PER_INDEX;
        const uint32_t nCheck = (nBegin * CONFIG.MAX_FILES_PER_INDEX) / CONFIG.HASHMAP_TOTAL_BUCKETS;

        const bool fAdjust = (nCheck != nFile);

        /* Find our new file position from current bucket and offset. */
        const uint64_t nFilePos      = (INDEX_FILTER_SIZE * (nIterator - (fAdjust ? 1 : 0) -
            ((nFile * CONFIG.HASHMAP_TOTAL_BUCKETS) / CONFIG.MAX_FILES_PER_INDEX)));

        /* Find our new file position from current bucket and offset. */
        //const uint64_t nFilePos2      = (INDEX_FILTER_SIZE * (//(nFile == 0 ? 0 : 1) -
        //    ((nEnd * CONFIG.HASHMAP_TOTAL_BUCKETS) / CONFIG.MAX_FILES_PER_INDEX)));

        /* Find the range (in bytes) we want to read for this index range. */
        const uint32_t nMaxBuckets = std::min(nRemaining, std::max(1u, (nEnd - nIterator)));


        if(nFilePos + INDEX_FILTER_SIZE > nMaxSize)
        {
            debug::warning(fAdjust ? ANSI_COLOR_BRIGHT_YELLOW : "",
                          VARIABLE(nRemaining),  " | ",
                          VARIABLE(nMaxBuckets), " | ",
                          VARIABLE(nFilePos),    " | ",
                          VARIABLE(nFile),       " | ",
                          VARIABLE(nEnd),        " | ",
                          VARIABLE(nBegin),      " | ",
                          VARIABLE(nCheck),      " | ",
                          VARIABLE(nBucket),     " | ",
                          fAdjust ? ANSI_COLOR_RESET : ""
                      );

            return false;
        }

        /* Track our current binary position, remaining buckets to read, and current bucket iterator. */
        nRemaining -= nMaxBuckets;
        nIterator  += nMaxBuckets;
    }
    while(nRemaining > 0); //continue reading into the buffer, with one loop per file adjusting to each boundary

    return true;
}

#include <leveldb/c.h>

#include <Util/mio/mmap.hpp>


/** @Class mstream
 *
 *  RAII wrapper around mio::mmap library.
 *
 *  Allows reading and writing on memory mapped files with an interface that closely resembles std::fstream.
 *  Keeps track of reading and writing iterators and seeks over the memory map rather than through a buffer
 *  and a disk seek. Allows interchangable swap ins and outs for regular buffered io or mmapped io.
 *
 **/
class mstream
{
    /** Internal enumeration flags for mimicking fstream flags. **/
    enum STATE
    {
        BAD  = (1 << 1),
        FAIL = (1 << 2),
        END  = (1 << 3),
    };


    /** Error code driven by mmap mio wrapper. **/
    std::error_code ERROR;


    /** MMAP object from mio wrapper. **/
    mio::mmap_sink MMAP;


    /** Binary position of reading pointer. **/
    uint64_t GET;


    /** Binary position of writing pointer. **/
    uint64_t PUT;


    /** Current status of stream object. **/
    uint8_t STATUS;


    /** Current input flags to set behavior. */
    const uint8_t FLAGS;

public:

    mstream(const std::string& strPath, const uint8_t nFlags)
    : ERROR  ( )
    , MMAP   (mio::make_mmap_sink(strPath, 0, mio::map_entire_file, ERROR))
    , GET    (0)
    , PUT    (0)
    , STATUS (0)
    , FLAGS  (nFlags)
    {
    }

    ~mstream()
    {
        MMAP.sync(ERROR);
    }


    /** is_open
     *
     *  Checks if the mmap is currently open for reading.
     *
     **/
    bool is_open() const
    {
        return MMAP.is_open();
    }


    /** good
     *
     *  Checks if the mmap is in an operable state.
     *
     **/
    bool good() const
    {
        /* Check our status bits first. */
        if(STATUS & STATE::BAD || STATUS & STATE::FAIL || STATUS & STATE::END)
            return false;

        return MMAP.is_open() && MMAP.is_mapped();
    }


    /** bad
     *
     *  Checks if the mmap has failed or reached end of file.
     *
     **/
    bool bad() const
    {
        /* Check our failbits first. */
        if(STATUS & STATE::BAD || STATUS & STATE::FAIL)
            return true;

        /* Check the reverse of good(). */
        return false;
    }


    /** fail
     *
     *  Checks if the mmap has failed at any point.
     *
     **/
    bool fail() const
    {
        /* Check our failbits first. */
        if(STATUS & STATE::FAIL)
            return true;

        return false;
    }


    /** eof
     *
     *  Checks if the end of file has been found.
     *
     **/
    bool eof() const
    {
        /* Check if our end of file flag has been set. */
        if(STATUS & STATE::END)
            return true;

        return false;
    }


    /** ! operator
     *
     *  Check that the current mmap is in good condition.
     *
     **/
    bool operator!(void) const
    {
        return !good();
    }


    /** tellg
     *
     *  Gives current reading position for the memory map
     *
     **/
    uint64_t tellg() const
    {
        return GET;
    }


    /** tellp
     *
     *  Gives current writing position for the memory map
     *
     **/
    uint64_t tellp() const
    {
        return PUT;
    }


    /** seekg
     *
     *  Sets the current reading position for memory map.
     *
     *  @param[in] nPos The binary position in bytes to seek to
     *
     *  @return reference of stream
     *
     **/
    mstream& seekg(const uint64_t nPos)
    {
        /* Check our stream flags. */
        if(!(FLAGS & std::ios::in))
        {
            STATUS |= (STATE::BAD);
            return *this;
        }

        /* Unset the eof bit. */
        STATUS &= ~(STATE::END);
        GET = nPos;

        /* Check we aren't out of bounds. */
        if(GET > MMAP.size())
            STATUS |= (STATE::END);

        return *this;
    }


    /** seekg
     *
     *  Sets the current reading position for memory map.
     *
     *  @param[in] nPos The binary position in bytes to seek to
     *  @param[in] nFlag The flag to adjust seeking behavior.
     *
     *  @return reference of stream
     *
     **/
    mstream& seekg(const uint64_t nPos, const std::ios_base::seekdir nFlag)
    {
        /* Check our stream flags. */
        if(!(FLAGS & std::ios::in))
        {
            STATUS |= (STATE::BAD);
            return *this;
        }

        /* Unset the eof bit. */
        STATUS &= ~(STATE::END);

        /* Handle a seek from beginning. */
        if(nFlag & std::ios::beg)
            GET = nPos;

        /* Handle a seek from the end. */
        if(nFlag & std::ios::end)
            GET = (MMAP.size() - nPos);

        /* Handle a seek from cursor. */
        if(nFlag & std::ios::cur)
            GET += nPos;

        /* Check we aren't out of bounds. */
        if(GET > MMAP.size())
            STATUS |= (STATE::END);

        return *this;
    }


    /** seekp
     *
     *  Sets the current writing position for memory map.
     *
     *  @param[in] nPos The binary position in bytes to seek to
     *
     *  @return reference of stream
     *
     **/
    mstream& seekp(const uint64_t nPos)
    {
        /* Check our stream flags. */
        if(!(FLAGS & std::ios::out))
        {
            STATUS |= (STATE::BAD);
            return *this;
        }

        /* Unset the eof bit. */
        STATUS &= ~(STATE::END);
        PUT = nPos;

        /* Check we aren't out of bounds. */
        if(PUT > MMAP.size())
            STATUS |= (STATE::END);

        return *this;
    }


    /** seekp
     *
     *  Sets the current writing position for memory map.
     *
     *  @param[in] nPos The binary position in bytes to seek to
     *  @param[in] nFlag The flag to adjust seeking behavior.
     *
     *  @return reference of stream
     *
     **/
    mstream& seekp(const uint64_t nPos, const std::ios_base::seekdir nFlag)
    {
        /* Check our stream flags. */
        if(!(FLAGS & std::ios::out))
        {
            STATUS |= (STATE::BAD);
            return *this;
        }

        /* Unset the eof bit. */
        STATUS &= ~(STATE::END);

        /* Handle a seek from beginning. */
        if(nFlag & std::ios::beg)
            PUT = nPos;

        /* Handle a seek from the end. */
        if(nFlag & std::ios::end)
            PUT = (MMAP.size() - nPos);

        /* Handle a seek from cursor. */
        if(nFlag & std::ios::cur)
            PUT += nPos;

        /* Check we aren't out of bounds. */
        if(PUT > MMAP.size())
            STATUS |= (STATE::END);

        return *this;
    }




    /** flush
     *
     *  Flushes buffers to disk from memorymap.
     *
     *  @return reference of stream
     *
     **/
    mstream& flush()
    {
        /* Sync to filesystem. */
        MMAP.sync(ERROR);

        /* Check for errors syncing. */
        if(ERROR)
            STATUS &= STATE::FAIL;

        return *this;
    }


    /** write
     *
     *  Writes a series of bytes into memory mapped file
     *
     *  @param[in] pBuffer The starting address of buffer to write.
     *  @param[in] nSize The size to write to memory map.
     *
     *  @return reference of stream
     *
     **/
    mstream& write(const char* pBuffer, const uint64_t nSize)
    {
        /* Check our stream flags. */
        if(!(FLAGS & std::ios::out))
        {
            STATUS |= (STATE::BAD);
            return *this;
        }

        /* Check if we will write to the end of file. */
        if(PUT + nSize > MMAP.size())
        {
            STATUS |= (STATE::END);
            return *this;
        }

        /* Copy the input buffer into memory map. */
        std::copy((uint8_t*)pBuffer, (uint8_t*)pBuffer + nSize, (uint8_t*)&MMAP[PUT]);

        /* Check for errors. */
        if(ERROR)
            STATUS |= STATE::FAIL;

        return *this;
    }


    /** read
     *
     *  Reads a series of bytes from memory mapped file
     *
     *  @param[in] pBuffer The starting address of buffer to read.
     *  @param[in] nSize The size to read from memory map.
     *
     *  @return reference of stream
     *
     **/
    mstream& read(char* pBuffer, const uint64_t nSize)
    {
        /* Check our stream flags. */
        if(!(FLAGS & std::ios::in))
        {
            STATUS |= (STATE::BAD);
            return *this;
        }

        /* Check if we will write to the end of file. */
        if(GET + nSize > MMAP.size())
        {
            STATUS |= (STATE::END);
            return *this;
        }

        /* Copy the memory mapped buffer into return buffer. */
        std::copy((uint8_t*)&MMAP[GET], (uint8_t*)&MMAP[GET] + nSize, (uint8_t*)pBuffer);

        /* Check for errors. */
        if(ERROR)
            STATUS |= STATE::FAIL;

        return *this;
    }
};


/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{
    config::ParseParameters(argc, argv);

    LLP::Initialize();

    //config::nVerbose.store(4);
    config::mapArgs["-datadir"] = "/database";

    const std::string strDB = "load8";

    std::string strIndex = config::mapArgs["-datadir"] + "/" + strDB + "/";

    if(config::GetBoolArg("-reset", false))
    {
        debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "Deleting data directory ", strIndex, ANSI_COLOR_RESET);
        filesystem::remove_directories(strIndex);
    }

    std::vector<uint1024_t> vFirst;


    bool fNew = false;

    std::string strKeys = config::mapArgs["-datadir"] + "/keys";
    if(!filesystem::exists(strKeys))
    {
        /* Create 10k new keys. */
        std::vector<uint1024_t> vKeys;
        for(int i = 0; i < 10000; ++i)
            vKeys.push_back(LLC::GetRand1024());

        /* Create a new file for next writes. */
        std::fstream stream(strKeys, std::ios::out | std::ios::binary | std::ios::trunc);
        stream.write((char*)&vKeys[0], vKeys.size() * sizeof(vKeys[0]));
        stream.close();

        debug::log(0, "Created new keys file.");
        vFirst = vKeys;

        fNew = true;
    }
    else
    {
        /* Create 10k new keys. */
        std::vector<uint1024_t> vKeys(10000, 0);

        /* Create a new file for next writes. */
        //std::fstream stream(strKeys, std::ios::out | std::ios::binary | std::ios::in);
        //stream.read((char*)&vKeys[0], vKeys.size() * sizeof(vKeys[0]));
        //stream.close();

        runtime::stopwatch swMap;
        if(config::GetBoolArg("-fstream"))
        {
            swMap.start();
            std::fstream stream(strKeys, std::ios::out | std::ios::binary | std::ios::in);
            stream.read((char*)&vKeys[0], vKeys.size() * sizeof(vKeys[0]));
            stream.close();
            swMap.stop();

            debug::log(0, "fstream completed in ", swMap.ElapsedMicroseconds());
        }
        else
        {
            swMap.start();
            mstream stream2(strKeys, std::ios::in | std::ios::out);
            if(!stream2.read((char*)&vKeys[0], vKeys.size() * sizeof(vKeys[0])))
                return debug::error("Failed to write data for mmap");

            if(!stream2)
                return debug::error("failbit set");

            swMap.stop();

            debug::log(0, "MMap completed in ", swMap.ElapsedMicroseconds());
        }

        vFirst = vKeys;

        debug::log(0, "[A] ", vFirst.begin()->SubString());
        debug::log(0, "[B] ", vFirst.back().SubString());
    }

    return 0;


    leveldb_t *db;
    leveldb_options_t *options;
    leveldb_readoptions_t *roptions;
    leveldb_writeoptions_t *woptions;
    char *err = NULL;

    /******************************************/
    /* OPEN */

    options = leveldb_options_create();
    leveldb_options_set_create_if_missing(options, 1);

    db = leveldb_open(options, "/database/leveldb1", &err);

    if (err != NULL) {
        return debug::error("Failed to open LEVELDB instance");
    }

    /* reset error var */
    leveldb_free(err); err = NULL;



    //build our base configuration
    LLD::Config::Base BASE =
        LLD::Config::Base(strDB, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE | LLD::FLAGS::APPEND);

    //build our sector configuration
    LLD::Config::Sector SECTOR      = LLD::Config::Sector(BASE);
    SECTOR.MAX_SECTOR_FILE_STREAMS  = 16;
    SECTOR.MAX_SECTOR_BUFFER_SIZE   = 1024 * 1024 * 4; //4 MB write buffer
    SECTOR.MAX_SECTOR_CACHE_SIZE    = 256; //4 MB of cache available

    //build our hashmap configuration
    LLD::Config::Hashmap CONFIG     = LLD::Config::Hashmap(BASE);
    CONFIG.HASHMAP_TOTAL_BUCKETS    = 10000000;
    CONFIG.MAX_FILES_PER_HASHMAP    = 4;
    CONFIG.MAX_FILES_PER_INDEX      = 10;
    CONFIG.MAX_HASHMAPS             = 50;
    CONFIG.MIN_LINEAR_PROBES        = 1;
    CONFIG.MAX_LINEAR_PROBES        = 1024;
    CONFIG.MAX_HASHMAP_FILE_STREAMS = 200;
    CONFIG.MAX_INDEX_FILE_STREAMS   = 10;
    CONFIG.PRIMARY_BLOOM_HASHES     = 7;
    CONFIG.PRIMARY_BLOOM_ACCURACY   = 300;
    CONFIG.SECONDARY_BLOOM_BITS     = 13;
    CONFIG.SECONDARY_BLOOM_HASHES   = 7;
    CONFIG.QUICK_INIT               = !config::GetBoolArg("-audit", false);

    TestDB* bloom = new TestDB(SECTOR, CONFIG);

    runtime::stopwatch swElapsed, swReaders;

    runtime::stopwatch swElapsed1, swReaders1;

    uint32_t nTotalWritten = 0, nTotalRead = 0;;

    uint32_t nTotalWritten1 = 0, nTotalRead1 = 0;;


    {
        debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "[LLD] BASE LEVEL TEST:", ANSI_COLOR_RESET);

        runtime::stopwatch swTimer;
        swTimer.start();
        swElapsed.start();

        uint32_t nTotal = 0;
        if(fNew)
        {
            for(const auto& nBucket : vFirst)
            {
                ++nTotalWritten;

                ++nTotal;
                if(!bloom->WriteKey(nBucket, nBucket))
                    return debug::error(FUNCTION, "failed to write ", VARIABLE(nTotal));
            }


            swElapsed.stop();
            swTimer.stop();

            uint64_t nElapsed = swTimer.ElapsedMicroseconds();
            debug::log(0,  "[LLD] ", vFirst.size() / 1000, "k records written in ", nElapsed,
                ANSI_COLOR_BRIGHT_YELLOW, " (", std::fixed, (1000000.0 * vFirst.size()) / nElapsed, " writes/s)", ANSI_COLOR_RESET);

        }


        uint1024_t hashKey = 0;

        swTimer.reset();
        swTimer.start();

        swReaders.start();

        nTotal = 0;
        for(const auto& nBucket : vFirst)
        {
            ++nTotal;
            ++nTotalRead;

            if(!bloom->ReadKey(nBucket, hashKey))
                return debug::error("Failed to read ", nBucket.SubString(), " total ", nTotal);
        }
        swTimer.stop();
        swReaders.stop();

        uint64_t nElapsed = swTimer.ElapsedMicroseconds();
        debug::log(0, "[LLD] ", vFirst.size() / 1000, "k records read in ", nElapsed,
            ANSI_COLOR_BRIGHT_YELLOW, " (", std::fixed, (1000000.0 * vFirst.size()) / nElapsed, " read/s)", ANSI_COLOR_RESET);
    }



    {
        debug::log(0, ANSI_COLOR_BRIGHT_RED, "[LEVELDB] BASE LEVEL TEST:", ANSI_COLOR_RESET);

        runtime::stopwatch swTimer;
        swTimer.start();
        swElapsed1.start();

        woptions = leveldb_writeoptions_create();

        uint32_t nTotal = 0;
        if(fNew)
        {

            for(const auto& nBucket : vFirst)
            {
                ++nTotalWritten1;

                ++nTotal;

                /******************************************/
                /* WRITE */
                std::string strData = nBucket.ToString();
                leveldb_put(db, woptions, strData.c_str(), strData.size(), strData.c_str(), strData.size(), &err);

                if (err != NULL) {
                  return debug::error("[LEVELDB] Write Failed for ", nBucket.ToString());
                }
            }

            leveldb_free(err); err = NULL;

            swElapsed1.stop();
            swTimer.stop();

            uint64_t nElapsed = swTimer.ElapsedMicroseconds();
            debug::log(0, "[LEVELDB] ", vFirst.size() / 1000, "k records written in ", nElapsed,
                ANSI_COLOR_BRIGHT_RED, " (", std::fixed, (1000000.0 * vFirst.size()) / nElapsed, " writes/s)", ANSI_COLOR_RESET);

        }

        uint1024_t hashKey = 0;

        swTimer.reset();
        swTimer.start();

        swReaders1.start();


        roptions = leveldb_readoptions_create();

        nTotal = 0;
        for(const auto& nBucket : vFirst)
        {
            ++nTotal;
            ++nTotalRead1;
            /******************************************/
            /* READ */

            size_t read_len;
            std::string strKey = nBucket.ToString();
            char* strRead;
            strRead = leveldb_get(db, roptions, strKey.c_str(), strKey.size(), &read_len, &err);

            if (err != NULL) {
             return debug::error("[LEVELDB] Read Failed at ", nBucket.ToString(), " value ", strRead);
            }

            leveldb_free(err); err = NULL;
        }
        swTimer.stop();
        swReaders1.stop();

        uint64_t nElapsed = swTimer.ElapsedMicroseconds();
        debug::log(0, "[LEVELDB] ", vFirst.size() / 1000, "k records read in ", nElapsed,
            ANSI_COLOR_BRIGHT_RED, " (", std::fixed, (1000000.0 * vFirst.size()) / nElapsed, " read/s)", ANSI_COLOR_RESET);

        //if(!bloom->EraseKey(vKeys[0]))
        //    return debug::error("failed to erase ", vKeys[0].SubString());

        //if(bloom->ReadKey(vKeys[0], hashKey))
        //    return debug::error("Failed to erase ", vKeys[0].SubString());
    }


    for(int n = 0; n < config::GetArg("-tests", 1); ++n)
    {
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Generating Keys for test: ", n, "/", config::GetArg("-tests", 1), ANSI_COLOR_RESET);

        std::vector<uint1024_t> vKeys;
        for(int i = 0; i < config::GetArg("-total", 1); ++i)
            vKeys.push_back(LLC::GetRand1024());


        runtime::stopwatch swTimer;
        swTimer.start();
        swElapsed.start();

        uint32_t nTotal = 0;
        for(const auto& nBucket : vKeys)
        {
            ++nTotalWritten;

            ++nTotal;
            if(!bloom->WriteKey(nBucket, nBucket))
                return debug::error(FUNCTION, "failed to write ", VARIABLE(nTotal));
        }


        swElapsed.stop();
        swTimer.stop();

        uint64_t nElapsed = swTimer.ElapsedMicroseconds();
        debug::log(0,  "[LLD] ", vKeys.size() / 1000, "k records written in ", nElapsed,
            ANSI_COLOR_BRIGHT_YELLOW, " (", std::fixed, (1000000.0 * vKeys.size()) / nElapsed, " writes/s)", ANSI_COLOR_RESET);

        uint1024_t hashKey = 0;

        swTimer.reset();
        swTimer.start();

        swReaders.start();

        nTotal = 0;
        for(const auto& nBucket : vKeys)
        {
            ++nTotal;
            ++nTotalRead;

            if(!bloom->ReadKey(nBucket, hashKey))
                return debug::error("Failed to read ", nBucket.SubString(), " total ", nTotal);
        }
        swTimer.stop();
        swReaders.stop();

        nElapsed = swTimer.ElapsedMicroseconds();
        debug::log(0, "[LLD] ", vKeys.size() / 1000, "k records read in ", nElapsed,
            ANSI_COLOR_BRIGHT_YELLOW, " (", std::fixed, (1000000.0 * vKeys.size()) / nElapsed, " read/s)", ANSI_COLOR_RESET);

        //if(!bloom->EraseKey(vKeys[0]))
        //    return debug::error("failed to erase ", vKeys[0].SubString());

        //if(bloom->ReadKey(vKeys[0], hashKey))
        //    return debug::error("Failed to erase ", vKeys[0].SubString());
    }


    {
        /******************************************/
        /* CLOSE */

        runtime::stopwatch swClose;
        swClose.start();
        swElapsed.start();
        delete bloom;
        swClose.stop();
        swElapsed.stop();

        debug::log(0, "[LLD] Closed in ", swClose.ElapsedMilliseconds(), " ms");
    }


    for(int n = 0; n < config::GetArg("-tests", 1); ++n)
    {
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Generating Keys for test: ", n, "/", config::GetArg("-tests", 1), ANSI_COLOR_RESET);

        std::vector<uint1024_t> vKeys;
        for(int i = 0; i < config::GetArg("-total", 1); ++i)
            vKeys.push_back(LLC::GetRand1024());

        {
            runtime::stopwatch swTimer;
            swTimer.start();
            swElapsed1.start();

            woptions = leveldb_writeoptions_create();

            uint32_t nTotal = 0;
            for(const auto& nBucket : vKeys)
            {
                ++nTotalWritten1;

                ++nTotal;

                /******************************************/
                /* WRITE */
                std::string strData = nBucket.ToString();
                leveldb_put(db, woptions, strData.c_str(), strData.size(), strData.c_str(), strData.size(), &err);

                if (err != NULL) {
                  return debug::error("[LEVELDB] Write Failed for ", nBucket.ToString());
                }
            }

            leveldb_free(err); err = NULL;


            swElapsed1.stop();
            swTimer.stop();

            uint64_t nElapsed = swTimer.ElapsedMicroseconds();
            debug::log(0, "[LEVELDB] ", vKeys.size() / 1000, "k records written in ", nElapsed,
                ANSI_COLOR_BRIGHT_RED, " (", std::fixed, (1000000.0 * vKeys.size()) / nElapsed, " writes/s)", ANSI_COLOR_RESET);

            uint1024_t hashKey = 0;

            swTimer.reset();
            swTimer.start();

            swReaders1.start();


            roptions = leveldb_readoptions_create();

            nTotal = 0;
            for(const auto& nBucket : vKeys)
            {
                ++nTotal;
                ++nTotalRead1;
                /******************************************/
                /* READ */

                size_t read_len;
                std::string strKey = nBucket.ToString();
                char* strRead;
                strRead = leveldb_get(db, roptions, strKey.c_str(), strKey.size(), &read_len, &err);

                if (err != NULL) {
                 return debug::error("[LEVELDB] Read Failed at ", nBucket.ToString(), " value ", strRead);
                }

                leveldb_free(err); err = NULL;
            }
            swTimer.stop();
            swReaders1.stop();

            nElapsed = swTimer.ElapsedMicroseconds();
            debug::log(0, "[LEVELDB] ", vKeys.size() / 1000, "k records read in ", nElapsed,
                ANSI_COLOR_BRIGHT_RED, " (", std::fixed, (1000000.0 * vKeys.size()) / nElapsed, " read/s)", ANSI_COLOR_RESET);

            //if(!bloom->EraseKey(vKeys[0]))
            //    return debug::error("failed to erase ", vKeys[0].SubString());

            //if(bloom->ReadKey(vKeys[0], hashKey))
            //    return debug::error("Failed to erase ", vKeys[0].SubString());
        }

    }

    {
        /******************************************/
        /* CLOSE */

        runtime::stopwatch swClose;
        swClose.start();
        swElapsed1.start();
        leveldb_close(db);
        swClose.stop();
        swElapsed1.stop();

        debug::log(0, "[LEVELDB] Closed in ", swClose.ElapsedMilliseconds(), " ms");
    }


    {
        uint64_t nElapsed = swElapsed1.ElapsedMicroseconds();
        uint64_t nMinutes = nElapsed / 60000000;
        uint64_t nSeconds = (nElapsed / 1000000) - (nMinutes * 60);

        debug::log(0, ANSI_COLOR_BRIGHT_RED, "[LEVELDB] Completed Writing ", nTotalWritten1, " Keys in ",
            nMinutes, " minutes ", nSeconds, " seconds (", std::fixed, (1000000.0 * nTotalWritten1) / nElapsed, " writes/s)");
    }

    {
        uint64_t nElapsed = swReaders1.ElapsedMicroseconds();
        uint64_t nMinutes = nElapsed / 60000000;
        uint64_t nSeconds = (nElapsed / 1000000) - (nMinutes * 60);

        debug::log(0, ANSI_COLOR_BRIGHT_RED, "[LEVELDB] Completed Reading ", nTotalRead1, " Keys in ",
            nMinutes, " minutes ", nSeconds, " seconds (", std::fixed, (1000000.0 * nTotalRead1) / nElapsed, " reads/s)");
    }


    {
        uint64_t nElapsed = swElapsed.ElapsedMicroseconds();
        uint64_t nMinutes = nElapsed / 60000000;
        uint64_t nSeconds = (nElapsed / 1000000) - (nMinutes * 60);

        debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "[LLD] Completed Writing ", nTotalWritten, " Keys in ",
            nMinutes, " minutes ", nSeconds, " seconds (", std::fixed, (1000000.0 * nTotalWritten) / nElapsed, " writes/s)");
    }

    {
        uint64_t nElapsed = swReaders.ElapsedMicroseconds();
        uint64_t nMinutes = nElapsed / 60000000;
        uint64_t nSeconds = (nElapsed / 1000000) - (nMinutes * 60);

        debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "[LLD] Completed Reading ", nTotalRead, " Keys in ",
            nMinutes, " minutes ", nSeconds, " seconds (", std::fixed, (1000000.0 * nTotalRead) / nElapsed, " reads/s)");
    }







    return 0;
}
