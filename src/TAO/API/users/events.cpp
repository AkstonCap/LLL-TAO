/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/global.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/include/debug.h>


namespace TAO
{
    namespace API
    {

        /*  Background thread to handle/suppress sigchain notifications. */
        void Users::EventsThread()
        {
            uint512_t hashTx;
            uint256_t hashFrom;
            uint256_t hashTo;
            uint64_t nSession = 0;

            /* Loop the events processing thread until shutdown. */
            while(!fShutdown.load())
            {

                /* Wait for the events processing thread to be woken up (such as a login) */
                std::unique_lock<std::mutex> lk(EVENTS_MUTEX);
                CONDITION.wait_for(lk, std::chrono::milliseconds(1000), [this]{ return fEvent.load() || fShutdown.load();});

                /* Check for a shutdown event. */
                if(fShutdown.load())
                    return;

                /* Ensure that the user is logged, in, wallet unlocked, and able to transact. */
                if(!LoggedIn() || Locked() || !CanTransact())
                    continue;

                /* Get the session to be used for this API call */
                json::json params;
                nSession = users->GetSession(params);

                /* Get the account. */
                memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
                if(!user)
                    throw APIException(-25, "Invalid session ID");


                /* Set the genesis id for the user. */
                params["genesis"] = user->Genesis().ToString();

                /* Get the PIN to be used for this API call */
                SecureString strPIN = users->GetPin(params);

                try
                {
                    json::json ret = Notifications(params, false);

                    /* Print the JSON value for the notifications processed. */
                    debug::log(0, FUNCTION, "\n", ret.dump(4));

                    for(const auto& notification : ret)
                    {
                        if(notification["operation"][0]["OP"] == "DEBIT")
                        {
                            /* Create the transaction. */
                            TAO::Ledger::Transaction tx;
                            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                                throw APIException(-25, "Failed to create transaction");

                            /* Set the transaction, to and from hashes. */
                            hashTx.SetHex(notification["hash"]);
                            hashFrom.SetHex(notification["operation"][0]["address_from"]);
                            hashTo.SetHex(notification["operation"][0]["address_to"]);


                            // TODO:
                            // get identifer from notification (you need to add identifier to notification JSON)
                            // check event processor config to see if we have an account or register address configured for identifier X
                            // if so then use that to process this debit...
                            //2 = jack:savings OR 2=asdfasdfsdfsdf
                            // if they have configured a register address then set hashTo from that address
                            // if they have configured an account name then we need to generate the register address from that name
                            //  - register address is namespacehash:token:name
                            //  - namespacehash is argon2 hash of username
                            //  - Look at RegisterAddressFromName() method as you can probably use that


                            /* Submit the payload object. */
                            //tx[0] << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << hashFrom << hashTo << nAmount;

                            /* Execute the operations layer. */
                            if(!tx.Build())
                                throw APIException(-26, "Operations failed to execute");

                            /* Sign the transaction. */
                            if(!tx.Sign(users->GetKey(tx.nSequence, strPIN, users->GetSession(params))))
                                throw APIException(-26, "Ledger failed to sign transaction");

                            /* Execute the operations layer. */
                            if(!TAO::Ledger::mempool.Accept(tx))
                                throw APIException(-26, "Failed to accept");
                        }
                        else if(notification["operation"][0]["OP"] == "COINBASE")
                        {

                        }
                        else if(notification["operation"][0]["OP"] == "TRANSFER")
                        {
                            /* Create the transaction. */
                            TAO::Ledger::Transaction tx;
                            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                                throw APIException(-25, "Failed to create transaction");

                            /* Set the transaction hash for the claim. */
                            hashTx.SetHex(notification["hash"]);

                            /* Submit the payload object. */
                            tx[0] << uint8_t(TAO::Operation::OP::CLAIM) << hashTx;

                            /* Execute the operations layer. */
                            if(!tx.Build())
                                throw APIException(-26, "Operations failed to execute");

                            /* Sign the transaction. */
                            if(!tx.Sign(users->GetKey(tx.nSequence, strPIN, users->GetSession(params))))
                                throw APIException(-26, "Ledger failed to sign transaction");

                            /* Execute the operations layer. */
                            if(!TAO::Ledger::mempool.Accept(tx))
                                throw APIException(-26, "Failed to accept");
                        }
                    }

                }
                catch(const APIException& e)
                {
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
