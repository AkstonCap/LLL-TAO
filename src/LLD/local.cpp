/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/types/local.h>

#include <TAO/Ledger/types/transaction.h>

namespace LLD
{

    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    LocalDB::LocalDB(const uint8_t nFlagsIn, const uint32_t nBucketsIn, const uint32_t nCacheIn)
    : SectorDatabase(std::string("_LOCAL")
    , nFlagsIn
    , nBucketsIn
    , nCacheIn)
    {
    }


    /** Default Destructor **/
    LocalDB::~LocalDB()
    {
    }


    /* Writes the txid of the transaction that modified the given register. */
    bool LocalDB::WriteIndex(const uint256_t& hashAddress, const std::pair<uint512_t, uint64_t>& pairIndex)
    {
        return Write(std::make_pair(std::string("index"), hashAddress), pairIndex);
    }


    /* Reads the txid of the transaction that modified the given register. */
    bool LocalDB::ReadIndex(const uint256_t& hashAddress, std::pair<uint512_t, uint64_t> &pairIndex)
    {
        return Read(std::make_pair(std::string("index"), hashAddress), pairIndex);
    }

    /* Writes the timestamp of a register's local disk cache expiration. */
    bool LocalDB::WriteExpiration(const uint256_t& hashAddress, const uint64_t nTimestamp)
    {
        return Write(std::make_pair(std::string("expires"), hashAddress), nTimestamp);
    }


    /* Reads the timestamp of a register's local disk cache expiration. */
    bool LocalDB::ReadExpiration(const uint256_t& hashAddress, uint64_t &nTimestamp)
    {
        return Read(std::make_pair(std::string("expires"), hashAddress), nTimestamp);
    }

    /* Writes a stake change request indexed by genesis. */
    bool LocalDB::WriteStakeChange(const uint256_t& hashGenesis, const TAO::Ledger::StakeChange& stakeChange)
    {
        return Write(std::make_pair(std::string("stakechange"), hashGenesis), stakeChange);
    }


    /* Reads a stake change request for a sig chain genesis. */
    bool LocalDB::ReadStakeChange(const uint256_t& hashGenesis, TAO::Ledger::StakeChange &stakeChange)
    {
        return Read(std::make_pair(std::string("stakechange"), hashGenesis), stakeChange);
    }


    /* Removes a recorded stake change request. */
    bool LocalDB::EraseStakeChange(const uint256_t& hashGenesis)
    {
        return Erase(std::make_pair(std::string("stakechange"), hashGenesis));
    }


    /* Writes a notification suppression record */
    bool LocalDB::WriteSuppressNotification(const uint512_t& hashTx, const uint32_t nContract, const uint64_t &nTimestamp)
    {
        return Write(std::make_tuple(std::string("suppress"), hashTx, nContract), nTimestamp);
    }


    /* Reads a notification suppression record */
    bool LocalDB::ReadSuppressNotification(const uint512_t& hashTx, const uint32_t nContract, uint64_t &nTimestamp)
    {
        return Read(std::make_tuple(std::string("suppress"), hashTx, nContract), nTimestamp);
    }


    /* Removes a suppressed notification record */
    bool LocalDB::EraseSuppressNotification(const uint512_t& hashTx, const uint32_t nContract)
    {
        return Erase(std::make_tuple(std::string("suppress"), hashTx, nContract));
    }


    /* Writes a username - genesis hash pair to the local database. */
    bool LocalDB::WriteFirst(const SecureString& strUsername, const uint256_t& hashGenesis)
    {
        std::vector<uint8_t> vKey(strUsername.begin(), strUsername.end());
        return Write(std::make_pair(std::string("genesis"), vKey), hashGenesis);
    }


    /* Reads a genesis hash from the local database for a given username */
    bool LocalDB::ReadFirst(const SecureString& strUsername, uint256_t &hashGenesis)
    {
        std::vector<uint8_t> vKey(strUsername.begin(), strUsername.end());
        return Read(std::make_pair(std::string("genesis"), vKey), hashGenesis);
    }


    /* Writes session data to the local database. */
    bool LocalDB::WriteSession(const uint256_t& hashGenesis, const std::vector<uint8_t>& vchData)
    {
        return Write(std::make_pair(std::string("session"), hashGenesis), vchData);
    }


    /* Reads session data from the local database */
    bool LocalDB::ReadSession(const uint256_t& hashGenesis, std::vector<uint8_t>& vchData)
    {
        return Read(std::make_pair(std::string("session"), hashGenesis), vchData);
    }


    /* Deletes session data from the local database fort he given session ID. */
    bool LocalDB::EraseSession(const uint256_t& hashGenesis)
    {
        return Erase(std::make_pair(std::string("session"), hashGenesis));
    }


    /* Determines whether the local DB contains session data for the given session ID */
    bool LocalDB::HasSession(const uint256_t& hashGenesis)
    {
        return Exists(std::make_pair(std::string("session"), hashGenesis));
    }


    /* Check if a record exists for a table. */
    bool LocalDB::HasRecord(const std::string& strTable, const std::string& strKey)
    {
        return Exists(std::make_tuple(std::string("record.proof"), strKey, strTable));
    }


    /* Erase a record from a table. */
    bool LocalDB::EraseRecord(const std::string& strTable, const std::string& strKey)
    {
        return Erase(std::make_tuple(std::string("record.proof"), strKey, strTable));
    }


    /* Push a new record to a given table. */
    bool LocalDB::PushRecord(const std::string& strTable, const std::string& strKey, const std::string& strValue)
    {
        /* Check for already existing order. */
        if(HasRecord(strTable, strKey))
            return false;

        /* Get our current sequence number. */
        uint32_t nSequence = 0;

        /* Read our sequences from disk. */
        Read(std::make_pair(std::string("record.sequence"), strTable), nSequence);

        /* Add our indexing entry by owner sequence number. */
        if(!Write(std::make_tuple(std::string("record.index"), nSequence, strTable), std::make_pair(strKey, strValue)))
            return false;

        /* Write our new events sequence to disk. */
        if(!Write(std::make_pair(std::string("record.sequence"), strTable), ++nSequence))
            return false;

        /* Write our order proof. */
        if(!Write(std::make_tuple(std::string("record.proof"), strKey, strTable)))
            return false;

        return true;
    }


    /* List the current records for a given table. */
    bool LocalDB::ListRecords(const std::string& strTable, std::vector<std::pair<std::string, std::string>> &vRecords)
    {
        /* Loop until we have failed. */
        uint32_t nSequence = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Use this to keep a local record of our pairs. */
            std::pair<std::string, std::string> pairRecord;

            /* Read our current record. */
            if(!Read(std::make_tuple(std::string("record.index"), nSequence++, strTable), pairRecord))
                break;

            /* Check for transfer keys. */
            if(!HasRecord(strTable, pairRecord.first))
                continue; //NOTE: we skip over transfer keys

            /* Push our record to return value. */
            vRecords.push_back(pairRecord);
        }

        return !vRecords.empty();
    }

}
