/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <Legacy/include/evaluate.h>

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/types/tritium.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands.h>
#include <TAO/API/types/indexing.h>
#include <TAO/API/types/transaction.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/transaction.h>

#include <Util/include/mutex.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/include/string.h>

#include <functional>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Queue to handle dispatch requests. */
    util::atomic::lock_unique_ptr<std::queue<uint1024_t>> Indexing::DISPATCH;


    /* Thread for running dispatch. */
    std::thread Indexing::EVENTS_THREAD;


    /* Condition variable to wake up the indexing thread. */
    std::condition_variable Indexing::CONDITION;


    /* Queue to handle dispatch requests. */
    util::atomic::lock_unique_ptr<std::queue<uint256_t>> Indexing::INITIALIZE;


    /* Thread for running dispatch. */
    std::thread Indexing::INITIALIZE_THREAD;


    /* Condition variable to wake up the indexing thread. */
    std::condition_variable Indexing::INITIALIZE_CONDITION;


    /* Set to track active indexing entries. */
    std::set<std::string> Indexing::REGISTERED;


    /* Mutex around registration. */
    std::mutex Indexing::REGISTERED_MUTEX;


    /* Initializes the current indexing systems. */
    void Indexing::Initialize()
    {
        /* Read our list of active login sessions. */

        /* Initialize our thread objects now. */
        Indexing::DISPATCH      = util::atomic::lock_unique_ptr<std::queue<uint1024_t>>(new std::queue<uint1024_t>());
        Indexing::EVENTS_THREAD = std::thread(&Indexing::Manager);


        /* Initialize our indexing thread now. */
        Indexing::INITIALIZE    = util::atomic::lock_unique_ptr<std::queue<uint256_t>>(new std::queue<uint256_t>());
        Indexing::INITIALIZE_THREAD = std::thread(&Indexing::InitializeThread);
    }


    /* Initialize a user's indexing entries. */
    void Indexing::Initialize(const uint256_t& hashGenesis)
    {
        /* Add our genesis to initialize ordering. */
        INITIALIZE->push(hashGenesis);
        INITIALIZE_CONDITION.notify_all();
    }


    /* Checks current events against transaction history to ensure we are up to date. */
    void Indexing::RefreshEvents()
    {
        /* Our list of transactions to read. */
        std::vector<TAO::Ledger::Transaction> vtx;

        /* Start a timer to track. */
        runtime::timer timer;
        timer.Start();

        /* Check if we have a last entry. */
        uint512_t hashLast;

        /* See if we need to bootstrap indexes. */
        const bool fIndexed =
            LLD::Logical->ReadLastIndex(hashLast);

        /* Handle first key if needed. */
        if(!fIndexed)
        {
            /* Grab our starting txid by quick read. */
            if(LLD::Ledger->BatchRead("tx", vtx, 1))
                hashLast = vtx[0].GetHash();

            debug::log(0, FUNCTION, "Initializing indexing tx ", hashLast.SubString());
        }

        /* Keep track of our total count. */
        uint32_t nScannedCount = 0;

        /* Start our scan. */
        debug::log(0, FUNCTION, "Scanning from tx ", hashLast.SubString());
        while(!config::fShutdown.load())
        {
            /* Read the next batch of inventory. */
            std::vector<TAO::Ledger::Transaction> vtx;
            if(!LLD::Ledger->BatchRead(hashLast, "tx", vtx, 1000, fIndexed))
                break;

            /* Loop through found transactions. */
            for(const auto& tx : vtx)
            {
                /* Check that our transactions are in the main chain. */
                if(!tx.IsConfirmed())
                    continue;

                /* Iterate the transaction contracts. */
                for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
                {
                    /* Grab contract reference. */
                    const TAO::Operation::Contract& rContract = tx[nContract];

                    {
                        LOCK(REGISTERED_MUTEX);

                        /* Loop through registered commands. */
                        for(const auto& strCommands : REGISTERED)
                        {
                            debug::log(3, FUNCTION, "Dispatching for ", VARIABLE(strCommands), " | ", VARIABLE(nContract));
                            Commands::Instance(strCommands)->Index(rContract, nContract);
                        }
                    }
                }

                /* Update the scanned count for meters. */
                ++nScannedCount;

                /* Meter for output. */
                if(nScannedCount % 100000 == 0)
                {
                    /* Get the time it took to rescan. */
                    uint32_t nElapsedSeconds = timer.Elapsed();
                    debug::log(0, FUNCTION, "Processed ", nScannedCount, " in ", nElapsedSeconds, " seconds (",
                        std::fixed, (double)(nScannedCount / (nElapsedSeconds > 0 ? nElapsedSeconds : 1 )), " tx/s)");
                }
            }

            /* Set hash Last. */
            hashLast = vtx.back().GetHash();

            /* Check for end. */
            if(vtx.size() != 1000)
                break;
        }

        /* Write our last index now. */
        LLD::Logical->WriteLastIndex(hashLast);

        debug::log(0, FUNCTION, "Complated scanning ", nScannedCount, " tx in ", timer.Elapsed(), " seconds");
    }


    /*  Index a new block hash to relay thread.*/
    void Indexing::PushBlock(const uint1024_t& hashBlock)
    {
        DISPATCH->push(hashBlock);
        CONDITION.notify_one();
    }


    /* Handle relays of all events for LLP when processing block. */
    void Indexing::Manager()
    {
        /* Refresh our events. */
        RefreshEvents();

        /* Main loop controlled by condition variable. */
        std::mutex CONDITION_MUTEX;
        while(!config::fShutdown.load())
        {
            /* Wait for entries in the queue. */
            std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
            CONDITION.wait(CONDITION_LOCK,
            []
            {
                /* Check for shutdown. */
                if(config::fShutdown.load())
                    return true;

                return Indexing::DISPATCH->size() != 0;
            });

            /* Check for shutdown. */
            if(config::fShutdown.load())
                return;

            /* Start a stopwatch. */
            runtime::stopwatch swTimer;
            swTimer.start();

            /* Grab the next entry in the queue. */
            const uint1024_t hashBlock = DISPATCH->front();
            DISPATCH->pop();

            /* Get block and read transaction history. */
            TAO::Ledger::BlockState oBlock;
            if(!LLD::Ledger->ReadBlock(hashBlock, oBlock))
                continue;

            /* Loop through our transactions. */
            for(const auto& rtx : oBlock.vtx)
            {
                /* Get our txid. */
                const uint512_t hashTx = rtx.second;

                /* Check if handling legacy or tritium. */
                if(hashTx.GetType() == TAO::Ledger::TRITIUM)
                {
                    /* Make sure the transaction is on disk. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::Ledger->ReadTx(hashTx, tx))
                        continue;

                    /* Build our local sigchain events indexes. */
                    index_transaction(hashTx, tx);

                    /* Iterate the transaction contracts. */
                    for(uint32_t nContract = 0; nContract < tx.Size(); nContract++)
                    {
                        /* Grab contract reference. */
                        const TAO::Operation::Contract& rContract = tx[nContract];

                        {
                            LOCK(REGISTERED_MUTEX);

                            /* Loop through registered commands. */
                            for(const auto& strCommands : REGISTERED)
                            {
                                debug::log(3, FUNCTION, "Dispatching for ", VARIABLE(strCommands), " | ", VARIABLE(nContract));
                                Commands::Instance(strCommands)->Index(rContract, nContract);
                            }
                        }
                    }

                    /* Write our last index now. */
                    if(!LLD::Logical->WriteLastIndex(hashTx))
                        continue;
                }

                /* Check for legacy transaction type. */
                if(hashTx.GetType() == TAO::Ledger::LEGACY)
                {
                    /* Make sure the transaction is on disk. */
                    Legacy::Transaction tx;
                    if(!LLD::Legacy->ReadTx(hashTx, tx))
                        continue;

                    /* Loop thgrough the available outputs. */
                    for(const auto txout : tx.vout)
                    {
                        /* Extract our register address. */
                        uint256_t hashTo;
                        if(Legacy::ExtractRegister(txout.scriptPubKey, hashTo))
                        {
                            /* Read the owner of register. (check this for MEMPOOL, too) */
                            TAO::Register::State state;
                            if(!LLD::Register->ReadState(hashTo, state))
                                continue;

                            /* Check if owner is authenticated. */
                            if(Authentication::Active(state.hashOwner))
                            {

                            }
                        }
                    }
                }
            }

            //TODO: check for legacy transaction events for sigchain events.


        }
    }


    /* Default destructor. */
    void Indexing::InitializeThread()
    {
        /* Track our current genesis that we are initializing. */
        uint256_t hashGenesis = 0;

        /* Main loop controlled by condition variable. */
        std::mutex CONDITION_MUTEX;
        while(!config::fShutdown.load())
        {
            /* Cleanup our previous indexing session by setting our status. */
            if(hashGenesis != 0)
            {
                /* Track our ledger database sequence. */
                uint32_t nLedgerSequence = 0;
                LLD::Ledger->ReadSequence(hashGenesis, nLedgerSequence);

                /* Track our logical database sequence. */
                uint32_t nLogicalSequence = 0;
                LLD::Logical->ReadLastEvent(hashGenesis, nLogicalSequence);

                /* Set our indexing status to ready now. */
                Authentication::Ready(hashGenesis);

                /* Debug output to track our sequences. */
                debug::log(2, FUNCTION, "Completed building indexes at ", VARIABLE(nLedgerSequence), " | ", VARIABLE(nLogicalSequence), " for genesis=", hashGenesis.SubString());

                /* Reset the genesis-id now. */
                hashGenesis = 0;

                continue;
            }

            /* Wait for entries in the queue. */
            std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
            INITIALIZE_CONDITION.wait(CONDITION_LOCK,
            [&]
            {
                /* Check for shutdown. */
                if(config::fShutdown.load())
                    return true;

                return Indexing::INITIALIZE->size() != 0;
            });

            /* Check for shutdown. */
            if(config::fShutdown.load())
                return;

            /* Check that we have items in the queue. */
            if(Indexing::INITIALIZE->empty())
                continue;

            /* Get the current genesis-id to initialize for. */
            hashGenesis = INITIALIZE->front();
            INITIALIZE->pop();

            /* Sync the sigchain if an active client before building our indexes. */
            if(config::fClient.load())
            {
                /* Check for genesis. */
                if(LLP::TRITIUM_SERVER)
                {
                    /* Find an active connection to sync from. */
                    std::shared_ptr<LLP::TritiumNode> pNode = LLP::TRITIUM_SERVER->GetConnection();
                    if(pNode != nullptr)
                    {
                        debug::log(1, FUNCTION, "CLIENT MODE: Synchronizing client");

                        /* Get the last txid in sigchain. */
                        uint512_t hashLast;
                        LLD::Ledger->ReadLast(hashGenesis, hashLast); //NOTE: we don't care if it fails here, because zero means begin

                        /* Request the sig chain. */
                        debug::log(1, FUNCTION, "CLIENT MODE: Requesting LIST::SIGCHAIN for ", hashGenesis.SubString());

                        LLP::TritiumNode::BlockingMessage(30000, pNode.get(), LLP::TritiumNode::ACTION::LIST, uint8_t(LLP::TritiumNode::TYPES::SIGCHAIN), hashGenesis, hashLast);

                        debug::log(1, FUNCTION, "CLIENT MODE: LIST::SIGCHAIN received for ", hashGenesis.SubString());

                        /* Grab list of notifications. */
                        pNode->PushMessage(LLP::TritiumNode::ACTION::LIST, uint8_t(LLP::TritiumNode::TYPES::NOTIFICATION), hashGenesis);
                        pNode->PushMessage(LLP::TritiumNode::ACTION::LIST, uint8_t(LLP::TritiumNode::SPECIFIER::LEGACY), uint8_t(LLP::TritiumNode::TYPES::NOTIFICATION), hashGenesis);
                    }
                    else
                        debug::error(FUNCTION, "no connections available...");
                }
            }

            /* Check our current last hash from ledger layer. */
            uint512_t hashLedger;
            if(!LLD::Ledger->ReadLast(hashGenesis, hashLedger, TAO::Ledger::FLAGS::MEMPOOL))
            {
                debug::log(0, FUNCTION, "No indexes for genesis=", hashGenesis.SubString());
                continue;
            }

            /* Check that our last indexing entries match. */
            uint512_t hashLogical = 0;
            if(!LLD::Logical->ReadLast(hashGenesis, hashLogical) || hashLedger != hashLogical)
            {
                /* Check if our logical hash is indexed so we know logical database is behind. */
                if(LLD::Ledger->HasIndex(hashLogical) || hashLogical == 0)
                {
                    debug::log(2, FUNCTION, "Buiding indexes for genesis=", hashGenesis.SubString());

                    /* Build list of transaction hashes. */
                    std::vector<uint512_t> vHashes;

                    /* Read all transactions from our last index. */
                    uint512_t hash = hashLedger;
                    while(hash != hashLogical && !config::fShutdown.load())
                    {
                        /* Read the transaction from the ledger database. */
                        TAO::Ledger::Transaction tx;
                        if(!LLD::Ledger->ReadTx(hash, tx, TAO::Ledger::FLAGS::MEMPOOL))
                        {
                            debug::warning(FUNCTION, "check for ", hashGenesis.SubString(), " failed at ", VARIABLE(hash.SubString()));
                            break;
                        }

                        /* Push transaction to list. */
                        vHashes.push_back(hash); //this will warm up the LLD cache if available, or remain low footprint if not

                        /* Check for first. */
                        if(tx.IsFirst())
                            break;

                        /* Set hash to previous hash. */
                        hash = tx.hashPrevTx;
                    }

                    /* Reverse iterate our list of entries. */
                    for(auto hash = vHashes.rbegin(); hash != vHashes.rend(); ++hash)
                    {
                        /* Read the transaction from the ledger database. */
                        TAO::Ledger::Transaction tx;
                        if(!LLD::Ledger->ReadTx(*hash, tx, TAO::Ledger::FLAGS::MEMPOOL))
                        {
                            debug::warning(FUNCTION, "index for ", hashGenesis.SubString(), " failed at ", VARIABLE(hash->SubString()));
                            break;
                        }

                        /* Build an API transaction. */
                        TAO::API::Transaction tIndex =
                            TAO::API::Transaction(tx);

                        /* Index the transaction to the database. */
                        if(!tIndex.Index(*hash))
                            debug::warning(FUNCTION, "failed to index ", VARIABLE(hash->SubString()));
                    }
                }

                /* Otherwise logical database is ahead and we need to re-broadcast. */
                else
                {
                    debug::log(2, FUNCTION, "Rebroadcasting indexes for genesis=", hashGenesis.SubString());

                    /* Build list of transaction hashes. */
                    std::vector<uint512_t> vHashes;

                    /* Read all transactions from our last index. */
                    uint512_t hash = hashLogical;
                    while(!config::fShutdown.load())
                    {
                        /* Read the transaction from the ledger database. */
                        TAO::API::Transaction tx;
                        if(!LLD::Logical->ReadTx(hash, tx))
                        {
                            debug::warning(FUNCTION, "check for ", hashGenesis.SubString(), " failed at ", VARIABLE(hash.SubString()));
                            break;
                        }

                        /* Push transaction to list. */
                        vHashes.push_back(hash); //this will warm up the LLD cache if available, or remain low footprint if not

                        /* Check for first. */
                        if(tx.IsFirst() || LLD::Ledger->HasIndex(tx.hashPrevTx))
                            break;

                        /* Set hash to previous hash. */
                        hash = tx.hashPrevTx;
                    }

                    /* Reverse iterate our list of entries. */
                    for(auto hash = vHashes.rbegin(); hash != vHashes.rend(); ++hash)
                    {
                        /* Read the transaction from the ledger database. */
                        TAO::API::Transaction tx;
                        if(!LLD::Logical->ReadTx(*hash, tx))
                        {
                            debug::warning(FUNCTION, "check for ", hashGenesis.SubString(), " failed at ", VARIABLE(hash->SubString()));
                            continue;
                        }

                        /* Execute the operations layer. */
                        if(!TAO::Ledger::mempool.Accept(tx))
                        {
                            debug::warning(FUNCTION, "accept for ", hash->SubString(), " failed");
                            continue;
                        }

                        /* Log that tx was rebroadcast. */
                        debug::log(2, FUNCTION, "Re-Broadcasted ", hash->SubString(), " to network");
                    }
                }
            }

            /* Read our last sequence. */
            uint32_t nSequence = 0;
            LLD::Logical->ReadLastEvent(hashGenesis, nSequence);

            /* Debug output so w4e can track our events indexes. */
            debug::log(2, FUNCTION, "Building events indexes from ", nSequence, " for genesis=", hashGenesis.SubString());

            /* Loop through our ledger level events. */
            TAO::Ledger::Transaction tNext;
            while(LLD::Ledger->ReadEvent(hashGenesis, nSequence, tNext))
            {
                /* Check for shutdown. */
                if(config::fShutdown.load())
                    break;

                /* Cache our current event's txid. */
                const uint512_t hashEvent = tNext.GetHash();

                /* Check all the tx contracts. */
                for(uint32_t nContract = 0; nContract < tNext.Size(); nContract++)
                {
                    /* Grab reference of our contract. */
                    const TAO::Operation::Contract& rContract = tNext[nContract];

                    /* Skip to our primitive. */
                    rContract.SeekToPrimitive();

                    /* Check the contract's primitive. */
                    uint8_t nOP = 0;
                    rContract >> nOP;
                    switch(nOP)
                    {
                        case TAO::Operation::OP::TRANSFER:
                        case TAO::Operation::OP::DEBIT:
                        {
                            /* Get our register address. */
                            TAO::Register::Address hashAddress;
                            rContract >> hashAddress;

                            /* Deserialize recipient from contract. */
                            TAO::Register::Address hashRecipient;
                            rContract >> hashRecipient;

                            /* Special check when handling a DEBIT. */
                            if(nOP == TAO::Operation::OP::DEBIT)
                            {
                                /* Skip over partials as this is handled seperate. */
                                if(hashRecipient.IsObject())
                                    continue;

                                /* Read the owner of register. (check this for MEMPOOL, too) */
                                TAO::Register::State oRegister;
                                if(!LLD::Register->ReadState(hashRecipient, oRegister))
                                    continue;

                                /* Set our hash to based on owner. */
                                hashRecipient = oRegister.hashOwner;
                            }

                            /* Ensure this is correct recipient. */
                            if(hashRecipient != hashGenesis)
                                continue;

                            /* Push to unclaimed indexes if processing incoming transfer. */
                            if(nOP == TAO::Operation::OP::TRANSFER)
                            {
                                /* Write incoming transfer as unclaimed. */
                                if(!LLD::Logical->PushUnclaimed(hashRecipient, hashAddress))
                                {
                                    debug::warning(FUNCTION, "PushUnclaimed (",
                                        (nOP == TAO::Operation::OP::TRANSFER ? "TRANSFER) " : "DEBIT) "),
                                        "failed to write: ", hashEvent.SubString(), " | ", VARIABLE(nContract));

                                    continue;
                                }
                            }

                            /* Check for duplicate events. */
                            if(LLD::Logical->HasEvent(hashEvent, nContract))
                            {
                                /* For duplicate events we need to increment events index. */
                                LLD::Logical->IncrementLastEvent(hashRecipient);
                                continue;
                            }

                            /* Write our events to database. */
                            if(!LLD::Logical->PushEvent(hashRecipient, hashEvent, nContract))
                            {
                                debug::warning(FUNCTION, "PushEvent (",
                                    (nOP == TAO::Operation::OP::TRANSFER ? "TRANSFER) " : "DEBIT) "),
                                    "failed to write: ", hashEvent.SubString(), " | ", VARIABLE(nContract));

                                continue;
                            }

                            /* Increment our sequence. */
                            if(!LLD::Logical->IncrementLastEvent(hashRecipient))
                            {
                                debug::warning(FUNCTION, "IncrementLastEvent (",
                                    (nOP == TAO::Operation::OP::TRANSFER ? "TRANSFER) " : "DEBIT) "),
                                    "failed to write: ", hashEvent.SubString(), " | ", VARIABLE(nContract));

                                continue;
                            }


                            debug::log(3, FUNCTION, (nOP == TAO::Operation::OP::TRANSFER ? "TRANSFER: " : "DEBIT: "),
                                "for genesis ", hashRecipient.SubString());

                            break;
                        }

                        case TAO::Operation::OP::COINBASE:
                        {
                            /* Get the genesis. */
                            uint256_t hashRecipient;
                            rContract >> hashRecipient;

                            /* Ensure this is correct recipient. */
                            if(hashRecipient != hashGenesis)
                                continue;

                            /* Check for duplicate events. */
                            if(LLD::Logical->HasEvent(hashEvent, nContract))
                            {
                                /* For duplicate events we need to increment events index. */
                                LLD::Logical->IncrementLastEvent(hashRecipient);
                                continue;
                            }

                            /* Write our events to database. */
                            if(!LLD::Logical->PushEvent(hashRecipient, hashEvent, nContract))
                            {
                                debug::warning(FUNCTION, "PushEvent (COINBASE) failed to write: ", hashEvent.SubString(), " | ", VARIABLE(nContract));
                                continue;
                            }

                            /* Increment our sequence. */
                            if(!LLD::Logical->IncrementLastEvent(hashRecipient))
                            {
                                debug::warning(FUNCTION, "IncrementLastEvent (COINBASE) failed to write");
                                continue;
                            }

                            debug::log(3, FUNCTION, "COINBASE: for genesis ", hashRecipient.SubString());

                            break;
                        }
                    }
                }

                ++nSequence;
            }
        }
    }


    /* Default destructor. */
    void Indexing::Shutdown()
    {
        /* Cleanup our initialization thread. */
        INITIALIZE_CONDITION.notify_all();
        if(INITIALIZE_THREAD.joinable())
            INITIALIZE_THREAD.join();

        /* Cleanup our dispatch thread. */
        CONDITION.notify_all();
        if(EVENTS_THREAD.joinable())
            EVENTS_THREAD.join();

        /* Clear open registrations. */
        {
            LOCK(REGISTERED_MUTEX);
            REGISTERED.clear();
        }
    }


    /* Index list of user level indexing entries. */
    void Indexing::index_transaction(const uint512_t& hash, const TAO::Ledger::Transaction& tx)
    {
        /* Check if we need to index the main sigchain. */
        if(Authentication::Active(tx.hashGenesis))
        {
            /* Build an API transaction. */
            TAO::API::Transaction tIndex =
                TAO::API::Transaction(tx);

            /* Index the transaction to the database. */
            if(!tIndex.Index(hash))
                debug::warning(FUNCTION, "failed to index ", VARIABLE(hash.SubString()));
        }

        /* Check all the tx contracts. */
        for(uint32_t nContract = 0; nContract < tx.Size(); nContract++)
        {
            /* Grab reference of our contract. */
            const TAO::Operation::Contract& rContract = tx[nContract];

            /* Skip to our primitive. */
            rContract.SeekToPrimitive();

            /* Check the contract's primitive. */
            uint8_t nOP = 0;
            rContract >> nOP;
            switch(nOP)
            {
                case TAO::Operation::OP::TRANSFER:
                case TAO::Operation::OP::DEBIT:
                {
                    /* Get the register address. */
                    TAO::Register::Address hashAddress;
                    rContract >> hashAddress;

                    /* Deserialize recipient from contract. */
                    TAO::Register::Address hashRecipient;
                    rContract >> hashRecipient;

                    /* Special check when handling a DEBIT. */
                    if(nOP == TAO::Operation::OP::DEBIT)
                    {
                        /* Skip over partials as this is handled seperate. */
                        if(hashRecipient.IsObject())
                            continue;

                        /* Read the owner of register. (check this for MEMPOOL, too) */
                        TAO::Register::State oRegister;
                        if(!LLD::Register->ReadState(hashRecipient, oRegister))
                            continue;

                        /* Set our hash to based on owner. */
                        hashRecipient = oRegister.hashOwner;

                        /* Check for active debit from with contract. */
                        if(Authentication::Active(tx.hashGenesis))
                        {
                            /* Write our events to database. */
                            if(!LLD::Logical->PushContract(tx.hashGenesis, hash, nContract))
                                continue;
                        }
                    }

                    /* Check if we need to build index for this contract. */
                    if(Authentication::Active(hashRecipient))
                    {
                        /* Check for local sessions. */
                        //if(SESSIONS->at(hashRecipient).Type() != Authentication::Session::LOCAL)
                        //    continue;

                        /* Push to unclaimed indexes if processing incoming transfer. */
                        if(nOP == TAO::Operation::OP::TRANSFER && !LLD::Logical->PushUnclaimed(hashRecipient, hashAddress))
                            continue;

                        /* Write our events to database. */
                        if(!LLD::Logical->PushEvent(hashRecipient, hash, nContract))
                            continue;

                        /* Increment our sequence. */
                        if(!LLD::Logical->IncrementLastEvent(hashRecipient))
                            continue;
                    }

                    debug::log(2, FUNCTION, (nOP == TAO::Operation::OP::TRANSFER ? "TRANSFER: " : "DEBIT: "),
                        "for genesis ", hashRecipient.SubString());

                    break;
                }

                case TAO::Operation::OP::COINBASE:
                {
                    /* Get the genesis. */
                    uint256_t hashRecipient;
                    rContract >> hashRecipient;

                    /* Check if we need to build index for this contract. */
                    if(Authentication::Active(hashRecipient))
                    {
                        /* Check for local sessions. */
                        //if(SESSIONS->at(hashRecipient).Type() != Authentication::Session::LOCAL)
                        //    continue;

                        /* Write our events to database. */
                        if(!LLD::Logical->PushEvent(hashRecipient, hash, nContract))
                            continue;

                        /* We don't increment our events index for miner coinbase contract. */
                        if(hashRecipient == tx.hashGenesis)
                            continue;

                        /* Increment our sequence. */
                        if(!LLD::Logical->IncrementLastEvent(hashRecipient))
                            continue;

                        debug::log(2, FUNCTION, "COINBASE: for genesis ", hashRecipient.SubString());
                    }

                    break;
                }

                //we want to tell clients or indexes that something was claimed for clients
                case TAO::Operation::OP::CLAIM:
                case TAO::Operation::OP::CREDIT:
                {

                    //check the txid to an active session
                    //if(LLD::Logical->ReadTx(hashDependant, rTx))
                    //{

                    //}

                    break;
                }
            }
        }
    }
}
