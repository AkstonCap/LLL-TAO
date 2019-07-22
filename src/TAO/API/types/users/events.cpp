/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/json.h>
#include <TAO/API/types/users.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/names.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/transaction.h>

#include <Util/include/debug.h>

#include <vector>


namespace TAO
{
    namespace API
    {

        // TODO:
        // get identifer from notification (you need to add identifier to notification JSON)
        // check event processor config to see if we have an account or register address configured for identifier X
        // if so then use that to process this debit...
        //2 = jack:savings OR 2=asdfasdfsdfsdf
        // if they have configured a register address then set hashTo from that address
        // if they have configured an account name then we need to generate the register address from that name
        //  - register address is namespacehash:token:name
        //  - namespacehash is argon2 hash of username
        //  - Look at Names::ResolveAddress() method as you can probably use that

        /*  Background thread to handle/suppress sigchain notifications. */
        void Users::EventsThread()
        {
            /* Auto-login feature if configured. */
            try
            {
                if(config::GetBoolArg("-autologin") && !config::fMultiuser.load())
                {
                    /* Check for username and password. */
                    if(config::GetArg("-username", "") != ""
                    && config::GetArg("-password", "") != ""
                    && config::GetArg("-pin", "")      != "")
                    {
                        /* Create the sigchain. */
                        memory::encrypted_ptr<TAO::Ledger::SignatureChain> user =
                            new TAO::Ledger::SignatureChain(config::GetArg("-username", "").c_str(), config::GetArg("-password", "").c_str());

                        /* Get the genesis ID. */
                        uint256_t hashGenesis = user->Genesis();

                        /* Check for duplicates in ledger db. */
                        TAO::Ledger::Transaction txPrev;

                        /* Get the last transaction. */
                        uint512_t hashLast;
                        if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                        {
                            user.free();
                            throw APIException(-138, "No previous transaction found");
                        }

                        /* Get previous transaction */
                        if(!LLD::Ledger->ReadTx(hashLast, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                        {
                            user.free();
                            throw APIException(-138, "No previous transaction found");
                        }

                        /* Genesis Transaction. */
                        TAO::Ledger::Transaction tx;
                        tx.NextHash(user->Generate(txPrev.nSequence + 1, config::GetArg("-pin", "").c_str(), false), txPrev.nNextType);

                        /* Check for consistency. */
                        if(txPrev.hashNext != tx.hashNext)
                        {
                            user.free();
                            throw APIException(-139, "Invalid credentials");
                        }

                        /* Setup the account. */
                        {
                            LOCK(MUTEX);
                            mapSessions.emplace(0, std::move(user));
                        }

                        /* Extract the PIN. */
                        if(!pActivePIN.IsNull())
                            pActivePIN.free();

                        /* Set account to unlocked. */
                        pActivePIN = new TAO::Ledger::PinUnlock(
                            config::GetArg("-pin", "").c_str(), TAO::Ledger::PinUnlock::UnlockActions::ALL);
                    }
                }
            }
            catch(const APIException& e)
            {
                debug::error(FUNCTION, e.what());
            }

            /* Loop the events processing thread until shutdown. */
            while(!fShutdown.load())
            {
                /* Wait for the events processing thread to be woken up (such as a login) */
                std::unique_lock<std::mutex> lk(EVENTS_MUTEX);
                CONDITION.wait_for(lk, std::chrono::milliseconds(5000), [this]{ return fEvent.load() || fShutdown.load();});

                /* Check for a shutdown event. */
                if(fShutdown.load())
                    return;

                try
                {
                    /* Ensure that the user is logged, in, wallet unlocked, and able to transact. */
                    if(!LoggedIn() || Locked() || !CanTransact())
                        continue;

                    /* Get the session to be used for this API call */
                    json::json params;
                    uint64_t nSession = users->GetSession(params);

                    /* Get the account. */
                    memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
                    if(!user)
                        throw APIException(-10, "Invalid session ID");

                    /* Lock the signature chain. */
                    LOCK(user->CREATE_MUTEX);

                    /* Set the hash genesis for this user. */
                    uint256_t hashGenesis = user->Genesis();

                    /* Set the genesis id for the user. */
                    params["genesis"] = hashGenesis.ToString();

                    /* Get the PIN to be used for this API call */
                    SecureString strPIN = users->GetPin(params);

                    /* Retrieve user's default NXS account. */
                    std::string strAccount = "default";
                    TAO::Register::Object account;
                    if(!TAO::Register::GetNameRegister(hashGenesis, strAccount, account))
                        throw APIException(-63, "Could not retrieve default NXS account to credit");

                    /* Get the list of outstanding contracts. */
                    std::vector<std::pair<std::shared_ptr<TAO::Ledger::Transaction>, uint32_t>> vContracts;
                    GetOutstanding(hashGenesis, vContracts);

                    /* The transaction hash. */
                    uint512_t hashTx;

                    /* hash from, hash to, and amount for operations. */
                    uint256_t hashFrom;
                    uint256_t hashTo;


                    uint64_t nAmount = 0;
                    uint32_t nOut = 0;

                    /* Create the transaction output. */
                    TAO::Ledger::Transaction txout;
                    if(!TAO::Ledger::CreateTransaction(user, strPIN, txout))
                        throw APIException(-17, "Failed to create transaction");

                    /* Loop through each contract in the notification queue. */
                    for(const auto& contract : vContracts)
                    {
                        /* Get a reference to the contract */
                        const TAO::Operation::Contract& refContract = (*contract.first)[contract.second]; 

                        /* Set the transaction hash. */
                        hashTx = refContract.Hash();

                        /* Get the maturity for this transaction. */
                        bool fMature = LLD::Ledger->ReadMature(hashTx);

                        /* Reset the contract operation stream. */
                        refContract.Reset();

                        /* Get the opcode. */
                        uint8_t OPERATION;
                        refContract >> OPERATION;

                        /* Check the opcodes for debit, coinbase or transfers. */
                        switch (OPERATION)
                        {
                            /* Check for Debits. */
                            case Operation::OP::DEBIT:
                            {
                                /* Set to and from hashes and amount. */
                                refContract >> hashFrom;
                                refContract >> hashTo;
                                refContract >> nAmount;

                                /* Submit the payload object. */
                                txout[nOut] << uint8_t(TAO::Operation::OP::CREDIT);
                                txout[nOut] << hashTx << contract.second;
                                txout[nOut] << hashTo << hashFrom;
                                txout[nOut] << nAmount;

                                /* Increment the contract ID. */
                                ++nOut;

                                /* Log debug message. */
                                debug::log(0, FUNCTION, "Matching DEBIT with CREDIT");

                                break;
                            }

                            /* Check for Coinbases. */
                            case Operation::OP::COINBASE:
                            {
                                /* Check that the coinbase is mature and ready to be credited. */
                                if(!fMature)
                                {
                                    debug::error(FUNCTION, "Immature coinbase.");
                                    continue;
                                }

                                /* Set the genesis hash and the amount. */
                                refContract >> hashFrom;
                                refContract >> nAmount;

                                /* Get the address that this name register is pointing to. */
                                hashTo = account.get<uint256_t>("address");

                                /* Submit the payload object. */
                                txout[nOut] << uint8_t(TAO::Operation::OP::CREDIT);
                                txout[nOut] << hashTx << contract.second;
                                txout[nOut] << hashTo << hashFrom;
                                txout[nOut] << nAmount;

                                /* Increment the contract ID. */
                                ++nOut;

                                /* Log debug message. */
                                debug::log(0, FUNCTION, "Matching COINBASE with CREDIT");

                                break;
                            }

                            /* Check for Transfers. */
                            case Operation::OP::TRANSFER:
                            {
                                /* Get the address of the asset being transfered from the transaction. */
                                refContract >> hashFrom;

                                /* Get the genesis hash (recipient) of the transfer. */
                                refContract >> hashTo;

                                /* Read the force transfer flag */
                                uint8_t nType = 0;
                                refContract >> nType;

                                /* Ensure this wasn't a forced transfer (which requires no Claim) */
                                if(nType == TAO::Operation::TRANSFER::FORCE)
                                    continue;

                                /* Submit the payload object. */
                                txout[nOut] << uint8_t(TAO::Operation::OP::CLAIM);
                                txout[nOut] << hashTx << contract.second;
                                txout[nOut] << hashFrom;

                                /* Increment the contract ID. */
                                ++nOut;

                                /* Log debug message. */
                                debug::log(0, FUNCTION, "Matching TRANSFER with CLAIM");

                                break;
                            }
                            default:
                                break;
                        }
                    }

                    /* If any of the notifications have been matched, execute the operations layer and sign the transaction. */
                    if(nOut)
                    {
                        /* Execute the operations layer. */
                        if(!txout.Build())
                            throw APIException(-30, "Failed to build register pre-states");

                        /* Sign the transaction. */
                        if(!txout.Sign(users->GetKey(txout.nSequence, strPIN, users->GetSession(params))))
                            throw APIException(-31, "Ledger failed to sign transaction");

                        /* Execute the operations layer. */
                        if(!TAO::Ledger::mempool.Accept(txout))
                            throw APIException(-32, "Failed to accept");
                    }
                }
                catch(const APIException& e)
                {
                    debug::error(FUNCTION, e.what());
                }

                /* Reset the events flag. */
                fEvent = false;
            }
        }


        /*  Notifies the events processor that an event has occurred so it
         *  can check and update it's state. */
        void Users::NotifyEvent()
        {
            fEvent = true;
            CONDITION.notify_one();
        }
    }
}
