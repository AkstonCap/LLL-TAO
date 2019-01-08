/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/verify.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/types/mempool.h>



namespace TAO::Ledger
{
    Mempool mempool;

    /* Add a transaction to the memory pool without validation checks. */
    bool Mempool::AddUnchecked(TAO::Ledger::Transaction tx)
    {
        LOCK(MUTEX);

        /* Get the transaction hash. */
        uint512_t hash = tx.GetHash();

        /* Check the mempool. */
        if(mapLedger.count(hash))
            return debug::error(FUNCTION "%s already exists", tx.GetHash().ToString().substr(0, 20).c_str());

        /* Add to the map. */
        mapLedger[hash] = tx;

        return true;
    }


    /* Accepts a transaction with validation rules. */
    bool Mempool::Accept(TAO::Ledger::Transaction tx)
    {
        LOCK(MUTEX);

        /* Check the mempool. */
        uint512_t hash = tx.GetHash();
        if(mapLedger.count(hash))
            return debug::error(FUNCTION "%s already exists", tx.GetHash().ToString().substr(0, 20).c_str());

        /* The next hash that is being claimed. */
        uint256_t hashClaim = tx.PrevHash();
        if(mapPrevHashes.count(hashClaim))
            return debug::error(FUNCTION "trying to claim spent next hash", hashClaim.ToString().substr(0, 20).c_str());

        /* Check for duplicate coinbase or coinstake. */
        if(tx.IsCoinbase())
            return debug::error(FUNCTION "coinbase %s not accepted in pool", tx.GetHash().ToString().substr(0, 20).c_str());

        /* Check for duplicate coinbase or coinstake. */
        if(tx.IsTrust())
            return debug::error(FUNCTION "trust %s not accepted in pool", tx.GetHash().ToString().substr(0, 20).c_str());

        /* Check for duplicate coinbase or coinstake. */
        if(tx.nTimestamp > runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT)
            return debug::error(FUNCTION "tx %s too far in the future", tx.GetHash().ToString().substr(0, 20).c_str());

        /* Check that the transaction is in a valid state. */
        if(!tx.IsValid())
            return debug::error(FUNCTION "%s is invalid", tx.GetHash().ToString().substr(0, 20).c_str());

        /* Verify the Ledger Pre-States. */
        if(!TAO::Register::Verify(tx))
            return debug::error(FUNCTION "%s register verification failed", tx.GetHash().ToString().substr(0, 20).c_str());

        /* Calculate the future potential states. */
        if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::MEMPOOL))
            return debug::error(FUNCTION "%s operations execution failed", tx.GetHash().ToString().substr(0, 20).c_str());

        /* Add to the map. */
        mapLedger[hash] = tx;

        /* Debug output. */
        debug::log(2, FUNCTION "%s ACCEPTED", hash.ToString().substr(0, 20).c_str());

        return true;
    }


    /* Checks if a transaction exists. */
    bool Mempool::Has(uint512_t hashTx)
    {
        LOCK(MUTEX);

        return mapLedger.count(hashTx);
    }


    /* Remove a transaction from pool. */
    bool Mempool::Remove(uint512_t hashTx)
    {
        LOCK(MUTEX);

        /* Find the transaction in pool. */
        TAO::Ledger::Transaction tx;
        if(mapLedger.count(hashTx))
        {
            tx = mapLedger[hashTx];

            /* Erase from the memory map. */
            mapPrevHashes.erase(tx.PrevHash());
            mapLedger.erase(hashTx);

            return true;
        }

        return false;
    }


    /* Gets a transaction from mempool */
    bool Mempool::Get(uint512_t hashTx, TAO::Ledger::Transaction& tx)
    {
        LOCK(MUTEX);

        /* Fail if not found. */
        if(!mapLedger.count(hashTx))
            return false;

        /* Find the object. */
        tx = mapLedger[hashTx];

        return true;
    }
}
