/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_INCLUDE_LEDGER_H
#define NEXUS_LLD_INCLUDE_LEDGER_H

#include <LLC/types/uint1024.h>

#include <LLD/include/version.h>
#include <LLD/templates/sector.h>

#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/hashmap.h>

#include <TAO/Register/include/state.h>
#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Register/include/enum.h>


namespace LLD
{


    class LedgerDB : public SectorDatabase<BinaryHashMap, BinaryLRU>
    {
        std::mutex MEMORY_MUTEX;

        std::map< std::pair<uint256_t, uint512_t>, uint32_t > mapProofs;

    public:
        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        LedgerDB(uint8_t nFlagsIn = FLAGS::CREATE | FLAGS::WRITE)
        : SectorDatabase("ledger", nFlagsIn) { }


        /** Write Best Chain.
         *
         *  Writes the best chain pointer to the ledger DB.
         *
         *  @param[in] hashBest The best chain hash to write.
         *
         *  @return True if the write was successful.
         *
         **/
        bool WriteBestChain(uint1024_t hashBest)
        {
            return Write(std::string("hashbestchain"), hashBest);
        }


        /** Read Best Chain.
         *
         *  Reads the best chain pointer from the ledger DB.
         *
         *  @param[out] hashBest The best chain hash to read.
         *
         *  @return True if the read was successful.
         *
         **/
        bool ReadBestChain(uint1024_t& hashBest)
        {
            return Read(std::string("hashbestchain"), hashBest);
        }


        /** Write Tx.
         *
         *  Writes a transaction to the ledger DB.
         *
         *  @param[in] hashTransaction The txid of transaction to write.
         *  @param[in] tx The transaction object to write.
         *
         *  @return True if the transaction was successfully written.
         *
         **/
        bool WriteTx(uint512_t hashTransaction, TAO::Ledger::Transaction tx)
        {
            return Write(hashTransaction, tx);
        }


        /** Read Tx.
         *
         *  Reads a transaction from the ledger DB.
         *
         *  @param[in] hashTransaction The txid of transaction to read.
         *  @param[in] tx The transaction object to read.
         *
         *  @return True if the transaction was successfully read.
         *
         **/
        bool ReadTx(uint512_t hashTransaction, TAO::Ledger::Transaction& tx)
        {
            return Read(hashTransaction, tx);
        }


        /** Erase Tx.
         *
         *  Erases a transaction from the ledger DB.
         *
         *  @param[in] hashTransaction The txid of transaction to erase.
         *
         *  @return True if the transaction was successfully erased.
         *
         **/
        bool EraseTx(uint512_t hashTransaction)
        {
            return Erase(hashTransaction);
        }


        /** Has Tx.
         *
         *  Checks LedgerDB if a transaction exists.
         *
         *  @param[in] hashTransaction The txid of transaction to check.
         *
         *  @return True if the transaction exists.
         *
         **/
        bool HasTx(uint512_t hashTransaction)
        {
            return Exists(hashTransaction);
        }


        /** Write Last.
         *
         *  Writes the last txid of sigchain to disk indexed by genesis.
         *
         *  @param[in] hashGenesis The genesis hash to write.
         *  @param[in] hashLast The last hash (txid) to write
         *
         *  @return True if the last was successfully written.
         *
         **/
        bool WriteLast(uint256_t hashGenesis, uint512_t hashLast)
        {
            return Write(std::make_pair(std::string("last"), hashGenesis), hashLast);
        }


        /** Read Last.
         *
         *  Reads the last txid of sigchain to disk indexed by genesis.
         *
         *  @param[in] hashGenesis The genesis hash to read.
         *  @param[in] hashLast The last hash (txid) to read
         *
         *  @return True if the last was successfully read.
         *
         **/
        bool ReadLast(uint256_t hashGenesis, uint512_t& hashLast)
        {
            return Read(std::make_pair(std::string("last"), hashGenesis), hashLast);
        }


        /** Write Proof.
         *
         *  Writes a proof to disk. Proofs are used to keep track of spent temporal proofs.
         *
         *  @param[in] hashProof The proof that is being spent.
         *  @param[in] hashTransaction The transaction hash that proof is being spent for.
         *  @param[in] nFlags Flags to detect if in memory mode (MEMPOOL) or disk mode (WRITE)
         *
         *  @return True if the last was successfully written.
         *
         **/
        bool WriteProof(uint256_t hashProof, uint512_t hashTransaction, uint8_t nFlags = TAO::Register::FLAGS::WRITE)
        {
            /* Memory mode for pre-database commits. */
            if(nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                LOCK(MEMORY_MUTEX);

                /* Write the new proof state. */
                mapProofs[std::make_pair(hashProof, hashTransaction)] = 0;
                return true;
            }
            else
            {
                LOCK(MEMORY_MUTEX);

                /* Erase memory proof if they exist. */
                if(mapProofs.count(std::make_pair(hashProof, hashTransaction)))
                    mapProofs.erase(std::make_pair(hashProof, hashTransaction));
            }

            return Write(std::make_pair(hashProof, hashTransaction));
        }


        /** Has Proof.
         *
         *  Checks if a proof exists. Proofs are used to keep track of spent temporal proofs.
         *
         *  @param[in] hashProof The proof that is being spent.
         *  @param[in] hashTransaction The transaction hash that proof is being spent for.
         *  @param[in] nFlags Flags to detect if in memory mode (MEMPOOL) or disk mode (WRITE)
         *
         *  @return True if the last was successfully read.
         *
         **/
        bool HasProof(uint256_t hashProof, uint512_t hashTransaction, uint8_t nFlags = TAO::Register::FLAGS::WRITE)
        {
            /* Memory mode for pre-database commits. */
            if(nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                LOCK(MEMORY_MUTEX);

                /* If exists in memory, return true. */
                if(mapProofs.count(std::make_pair(hashProof, hashTransaction)))
                    return true;
            }

            return Exists(std::make_pair(hashProof, hashTransaction));
        }


        /** Write Block
         *
         *  Writes a block state object to disk.
         *
         *  @param[in] hashBlock The block hash to write as.
         *  @param[in] state The block state object to write.
         *
         *  @return True if the write was successful.
         *
         **/
        bool WriteBlock(uint1024_t hashBlock, TAO::Ledger::BlockState state)
        {
            return Write(hashBlock, state);
        }


        /** Read Block
         *
         *  Reads a block state object from disk.
         *
         *  @param[in] hashBlock The block hash to read.
         *  @param[in] state The block state object to read.
         *
         *  @return True if the read was successful.
         *
         **/
        bool ReadBlock(uint1024_t hashBlock, TAO::Ledger::BlockState& state)
        {
            return Read(hashBlock, state);
        }


        /** Has Genesis.
         *
         *  Checks if a genesis transaction exists.
         *
         *  @param[in] hashGenesis The genesis ID to check for.
         *
         *  @return True if the genesis exists.
         *
         **/
        bool HasGenesis(uint256_t hashGenesis)
        {
            return Exists(std::make_pair(std::string("genesis"), hashGenesis));
        }


        /** Write Genesis.
         *
         *  Writes a genesis transaction-id to disk.
         *
         *  @param[in] hashGenesis The genesis ID to write for.
         *  @param[in] hashTransaction The transaction-id to write for.
         *
         *  @return True if the genesis exists.
         *
         **/
        bool WriteGenesis(uint256_t hashGenesis, uint512_t hashTransaction)
        {
            return Write(std::make_pair(std::string("genesis"), hashGenesis), hashTransaction);
        }


        /** Read Genesis.
         *
         *  Reads a genesis transaction-id from disk.
         *
         *  @param[in] hashGenesis The genesis ID to read for.
         *  @param[out] hashTransaction The transaction-id to read for.
         *
         *  @return True if the genesis was read..
         *
         **/
        bool ReadGenesis(uint256_t hashGenesis, uint512_t& hashTransaction)
        {
            return Read(std::make_pair(std::string("genesis"), hashGenesis), hashTransaction);
        }
    };
}

#endif
