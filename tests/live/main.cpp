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

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/types/condition.h>

/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{
    uint256_t hashGenesis  = TAO::Ledger::Genesis(LLC::GetRand256(), true);
    uint256_t hashAsset  = TAO::Register::Address(TAO::Register::Address::OBJECT);

    using namespace TAO::Operation;

    std::vector<std::pair<uint16_t, uint64_t>> vWarnings;

    {
        TAO::Operation::Contract contract;
        contract << uint8_t(OP::TRANSFER) << hashAsset << hashGenesis << uint8_t(TAO::Operation::TRANSFER::CLAIM);

        contract <= uint8_t(OP::CONTRACT::OPERATIONS) <= uint8_t(OP::SUBDATA) <= uint16_t(1) <= uint16_t(32); //hashFrom
        contract <= uint8_t(OP::NOTEQUALS); //if the proof is not the hashFrom we can assume it is a split dividend payment
        contract <= uint8_t(0xd3)   <= uint8_t(OP::SUBDATA) <= uint16_t(101) <= uint16_t(32);  //hashProof

        if(!TAO::Operation::Condition::Verify(contract, vWarnings))
            debug::error("Validate Error");
        else
            debug::error("Validate Success");
    }

    {
        TAO::Operation::Contract contract;
        contract << uint8_t(OP::TRANSFER) << hashAsset << hashGenesis << uint8_t(TAO::Operation::TRANSFER::CLAIM);

        contract <= uint8_t(OP::CONTRACT::OPERATIONS) <= uint8_t(OP::SUBDATA) <= uint16_t(32); //hashFrom
        contract <= uint8_t(OP::NOTEQUALS); //if the proof is not the hashFrom we can assume it is a split dividend payment
        contract <= uint8_t(OP::CALLER::OPERATIONS)   <= uint8_t(OP::SUBDATA) <= uint16_t(101) <= uint16_t(32);  //hashProof

        if(!TAO::Operation::Condition::Verify(contract, vWarnings))
            debug::error("Validate Error");
        else
            debug::error("Validate Success");
    }

    {
        TAO::Operation::Contract contract;
        contract << uint8_t(OP::TRANSFER) << hashAsset << hashGenesis << uint8_t(TAO::Operation::TRANSFER::CLAIM);
    
        //contract <= uint8_t(OP::GROUP);
        contract <= (uint8_t)OP::TYPES::UINT64_T <= uint64_t(0) <= (uint8_t) OP::SUB <= (uint8_t)OP::TYPES::UINT64_T <= uint64_t(100) <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 222u;
        //contract <= uint8_t(OP::UNGROUP);

        contract <= uint8_t(OP::AND);

        contract <= (uint8_t)OP::TYPES::UINT64_T <= std::numeric_limits<uint64_t>::max() <= (uint8_t) OP::ADD <= (uint8_t)OP::TYPES::UINT64_T <= uint64_t(100) <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 222u;
        
        contract <= uint8_t(OP::OR);

        /* 5 + 5 = 10 */
        contract <= uint8_t(OP::GROUP);
        contract <= uint8_t(OP::TYPES::UINT64_T) <= uint64_t(5) <= uint8_t(OP::ADD) <= uint8_t(OP::TYPES::UINT64_T) <= uint64_t(5);
        contract <= uint8_t(OP::NOTEQUALS);
        contract <= uint8_t(OP::TYPES::UINT64_T) <= uint64_t(5);
        contract <= uint8_t(OP::UNGROUP);

        if(!TAO::Operation::Condition::Verify(contract, vWarnings))
            debug::error("Validate Error");
        else
            debug::error("Validate Success");

        TAO::Operation::Contract caller;

        /* Create 1024 bytes if data */
        std::vector<uint8_t> vData(1024);

        /* Fill with 0x02 */
        std::fill(vData.begin(), vData.end(), 0x02);

        Condition condition(contract, caller);
        if(!condition.Execute())
            debug::error("Execute Error");
        else
            debug::error("Execute Success");
    }

    {
        TAO::Operation::Contract contract;
        contract << uint8_t(OP::TRANSFER) << hashAsset << hashGenesis << uint8_t(TAO::Operation::TRANSFER::CLAIM);
    
        contract <= uint8_t(OP::CONTRACT::TIMESTAMP) <= uint8_t(OP::ADD) <= uint8_t(OP::TYPES::UINT64_T) <= uint64_t(5);
        contract <= uint8_t(OP::GREATERTHAN) <= uint8_t(OP::CALLER::TIMESTAMP);

        if(!TAO::Operation::Condition::Verify(contract, vWarnings))
            debug::error("Validate Error");
        else
            debug::error("Validate Success");
    }




    return 0;
}
