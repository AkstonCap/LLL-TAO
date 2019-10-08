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
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/types/transaction.h>

#include <TAO/Ledger/include/ambassador.h>

#include <Legacy/types/address.h>
#include <Legacy/types/transaction.h>

#include <LLP/templates/ddos.h>
#include <Util/include/runtime.h>

#include <list>
#include <variant>


class TestDB : public LLD::SectorDatabase<LLD::BinaryHashMap, LLD::BinaryLRU>
{
public:
    TestDB()
    : SectorDatabase("testdb"
    , LLD::FLAGS::CREATE | LLD::FLAGS::FORCE
    , 256 * 256 * 64
    , 1024 * 1024 * 4)
    {
    }

    ~TestDB()
    {

    }

    bool WriteLast(const uint1024_t& last)
    {
        return Write(std::string("last"), last);
    }


    bool ReadLast(uint1024_t& last)
    {
        return Read(std::string("last"), last);
    }


    bool WriteHash(const uint1024_t& hash)
    {
        return Write(std::make_pair(std::string("hash"), hash), hash, "hash");
    }

    bool WriteHash2(const uint1024_t& hash, const uint1024_t& hash2)
    {
        return Write(std::make_pair(std::string("hash2"), hash), hash2, "hash");
    }

    bool IndexHash(const uint1024_t& hash)
    {
        return Index(std::make_pair(std::string("hash2"), hash), std::make_pair(std::string("hash"), hash));
    }


    bool ReadHash(const uint1024_t& hash, uint1024_t& hash2)
    {
        return Read(std::make_pair(std::string("hash"), hash), hash2);
    }


    bool ReadHash2(const uint1024_t& hash, uint1024_t& hash2)
    {
        return Read(std::make_pair(std::string("hash2"), hash), hash2);
    }


    bool EraseHash(const uint1024_t& hash)
    {
        return Erase(std::make_pair(std::string("hash"), hash));
    }
};


/*
Hash Tables:

Set max tables per timestamp.

Keep disk index of all timestamps in memory.

Keep caches of Disk Index files (LRU) for low memory footprint

Check the timestamp range of bucket whether to iterate forwards or backwards


_hashmap.000.0000
_name.shard.file
  t0 t1 t2
  |  |  |

  timestamp each hashmap file if specified
  keep indexes in TemplateLRU

  search from nTimestamp < timestamp[nShard][nHashmap]

*/


class Test
{
public:

    Test()
    {

    }

    /** Subscribed
     *
     *  Determine if a node is subscribed to receive relay message.
     *
     **/
    template<typename MessageType>
    bool Subscribed(const MessageType& message)
    {
        return debug::error("lower order function ", message);
    }
};


class Test2 : public Test
{
public:

    Test2()
    {

    }

    /** Subscribed
     *
     *  Determine if a node is subscribed to receive relay message.
     *
     **/
    bool Subscribed(const uint16_t& message)
    {
        return debug::error("Higher order function ", message);
    }
};


#include <Util/math/softfloat.h>


class precision64_t
{
    void set(double a)
    {
        std::copy((uint8_t*)&a, (uint8_t*)&a + 8, (uint8_t*)&value);
    }

protected:

    float64_t value;

public:

    precision64_t()
    {
    }

    precision64_t(const float64_t& a)
    : value(a)
    {
    }

    precision64_t(const double& a)
    {
        set(a);
    }

    precision64_t& operator=(const double& a)
    {
        set(a);

        return *this;
    }

    operator double()
    {
        double ret = 0;
        std::copy((uint8_t*)&value, (uint8_t*)&value + 8, (uint8_t*)&ret);

        return ret;
    }

    precision64_t operator*(const precision64_t& b)
    {
        return precision64_t(f64_mul(value, b.value));
    }

    precision64_t operator/(const precision64_t& b)
    {
        return precision64_t(f64_div(value, b.value));
    }

    precision64_t operator+(const precision64_t& b)
    {
        return precision64_t(f64_add(value, b.value));
    }

    precision64_t operator-(const precision64_t& b)
    {
        return precision64_t(f64_sub(value, b.value));
    }

    bool operator<(const precision64_t& b) const
    {
        return f64_lt(value, b.value);
    }

    bool operator>(const precision64_t& b) const
    {
        return !f64_eq(value, b.value) && !f64_lt(value, b.value);
    }

    bool operator==(const precision64_t& b) const
    {
        return f64_eq(value, b.value);
    }

    bool operator==(const double& b)
    {
        return f64_eq(value, precision64_t(b).value);
    }

    bool operator <=(const precision64_t& b)
    {
        return f64_lt(value, b.value) || f64_eq(value, b.value);
    }

    bool operator >=(const precision64_t& b)
    {
        return !f64_lt(value, b.value) || f64_eq(value, b.value);
    }
};

#include <Util/math/fdlibm.h>

#include <math.h>

/* These values reflect the Three Decay Equations for Miners, Ambassadors, and Developers. */
const double decay[3][3] =
{
    {50.0, -0.00000110, 1.000},
    {10.0, -0.00000055, 1.000},
    {01.0, -0.00000059, 0.032}
};


/* Get the Total Amount to be Released at a given Minute since the NETWORK_TIMELOCK. */
uint64_t GetSubsidy(const uint32_t nMinutes, const uint8_t nType)
{
    return (((decay[nType][0] * exp(decay[nType][1] * nMinutes)) + decay[nType][2]) * (500000));
}

/* Get the Total Amount to be Released at a given Minute since the NETWORK_TIMELOCK. */
uint64_t GetSubsidy2(const uint32_t nMinutes, const uint8_t nType)
{
    return (((decay[nType][0] * exp2(decay[nType][1] * nMinutes)) + decay[nType][2]) * (500000));
}

/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{
    for(int i = 0; i < 1000000; ++i)
    {
        if(GetSubsidy(i, 0) != GetSubsidy2(i, 0))
        {
            debug::error("FAILED AT ", i, "Base ", GetSubsidy(i, 0), " Check ", GetSubsidy2(i, 0));

            //return 0;
        }

        if(i % 10000 == 0)
            debug::log(0, "Iterated ", i);
    }

    double x = 8.3923929234232;
    double y = 3.28234233828382;

    double r = x * y;

    precision64_t x1 = x;
    //precision64_t x1;
    //x1.set(x);

    precision64_t y1 = y;

    precision64_t r1 = x1 * y1;

    bool fEquals = (x == x1);
    debug::log(0, "Equals ", fEquals ? "YES" : "NO");

    double dPow = std::pow(x, y);

    double dPow2 = pow(x, y);

    printf("POW %.15f\n", dPow);
    printf("POW %.15f\n", dPow2);

    printf("Value is %0.15f\n", r);
    printf("Value is %.15f\n", double(r1));

    precision64_t dPow1 = std::pow(x1, y1);

    printf("dPOW %.15f\n", double(dPow1));

    return 0;


    //debug::log(0, "Chain Age ", GetChainAge(time(NULL)));
    uint32_t nTotals = 1000000;

    runtime::timer timer;
    timer.Start();
    for(int i = 0; i < nTotals; i++)
    {
        if(i % 10000 == 0)
            printf("%u\n", i);

        TAO::Ledger::GetSubsidy(i, 0);
    }
    uint64_t nElapsed = timer.ElapsedMilliseconds();

    debug::log(0, "Elapsed ", nElapsed, " ms");

    return 0;

    Legacy::LegacyBlock block;
    block.nTime = 49339439;
    block.nBits = 34934;
    block.nVersion = 444;

    block.print();


    //runtime::timer timer;
    timer.Start();
    Legacy::LegacyBlock block2 = std::move(block);

    uint64_t nTotal = timer.ElapsedNanoseconds();

    debug::log(0, "Took ", nTotal, " nanoseconds");

    block2.print();

    return 0;

    uint64_t nTest = 555;

    nTest = std::max(nTest, uint64_t(888));

    debug::log(0, "Test ", nTest);

    TestDB* testDB = new TestDB();

    uint1024_t hash = LLC::GetRand();



    debug::log(0, "Write Hash");
    debug::log(0, "Hash ", hash.Get64());
    testDB->WriteHash(hash);

    debug::log(0, "Index Hash");
    if(!testDB->IndexHash(hash))
        return debug::error("failed to index");

    testDB->TxnBegin();

    debug::log(0, "Read Hash");
    uint1024_t hashTest;
    testDB->ReadHash(hash, hashTest);

    debug::log(0, "Hash ", hashTest.Get64());

    uint1024_t hashTest3 = hash + 1;
    testDB->WriteHash2(hash, hashTest3);

    testDB->TxnCheckpoint();
    testDB->TxnCommit();

    {
        debug::log(0, "Read Hash");
        uint1024_t hashTest4;
        testDB->ReadHash2(hash, hashTest4);

        debug::log(0, "Hash New ", hashTest4.Get64());
    }

    {
        debug::log(0, "Read Hash");
        uint1024_t hashTest4;
        testDB->ReadHash(hash, hashTest4);

        debug::log(0, "Hash New ", hashTest4.Get64());
    }

    return 0;

    for(int t = 0; t < 1; ++t)
    {
        uint1024_t last = 0;
        testDB->ReadLast(last);

        debug::log(0, "Last is ", last.Get64());

        runtime::timer timer;
        timer.Start();
        for(uint1024_t i = 1 ; i < last; ++i)
        {
            if(i.Get64() % 10000 == 0)
            {
                uint64_t nElapsed = timer.ElapsedMilliseconds();
                debug::log(0, "Read ", i.Get64(), " Total Records in ", nElapsed, " ms (", i.Get64() * 1000 / nElapsed, " per / s)");
            }

            uint1024_t hash;
            testDB->ReadHash(i, hash);
        }


        timer.Reset();
        uint32_t nTotal = 0;
        for(uint1024_t i = last + 1 ; i < last + 100000; ++i)
        {
            if(++nTotal % 10000 == 0)
            {
                uint64_t nElapsed = timer.ElapsedMilliseconds();
                debug::log(0, "Wrote ", nTotal, " Total Records in ", nElapsed, " ms (", nTotal * 1000 / nElapsed, " per / s)");
            }

            testDB->WriteHash(i);
        }

        testDB->WriteLast(last + 100000);

    }

    delete testDB;



    return 0;
}
