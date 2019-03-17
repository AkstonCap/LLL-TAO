/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/API/include/accounts.h>

#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/include/create.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        //TODO: have the authorization system build a SHA256 hash and salt on the client side as the AUTH hash.

        /* Login to a user account. */
        json::json Accounts::Login(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Check for username parameter. */
            if(params.find("username") == params.end())
                throw APIException(-23, "Missing Username");

            /* Check for password parameter. */
            if(params.find("password") == params.end())
                throw APIException(-24, "Missing Password");

            /* Check for pin parameter. */
            if(params.find("pin") == params.end())
                throw APIException(-24, "Missing PIN");

            /* Create the sigchain. */
            TAO::Ledger::SignatureChain* user = new TAO::Ledger::SignatureChain(params["username"].get<std::string>().c_str(), params["password"].get<std::string>().c_str());

            /* Get the genesis ID. */
            uint256_t hashGenesis = user->Genesis();

            /* Check for duplicates in ledger db. */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::legDB->HasGenesis(hashGenesis))
            {
                /* Check the memory pool and compare hashes. */
                if(!TAO::Ledger::mempool.Has(hashGenesis))
                {
                    delete user;
                    user = nullptr;

                    throw APIException(-26, "Account doesn't exists");
                }

                /* Get the memory pool tranasction. */
                if(!TAO::Ledger::mempool.Get(hashGenesis, txPrev))
                    throw APIException(-26, "Couldn't get transaction");
            }
            else
            {
                /* Get the last transaction. */
                uint512_t hashLast;
                if(!LLD::legDB->ReadLast(hashGenesis, hashLast))
                    throw APIException(-27, "No previous transaction found");

                /* Get previous transaction */
                TAO::Ledger::Transaction txPrev;
                if(!LLD::legDB->ReadTx(hashLast, txPrev))
                    throw APIException(-27, "No previous transaction found");
            }

            /* Genesis Transaction. */
            TAO::Ledger::Transaction tx;
            tx.NextHash(user->Generate(txPrev.nSequence + 1, params["pin"].get<std::string>().c_str()));

            /* Check for consistency. */
            if(txPrev.hashNext != tx.hashNext)
                throw APIException(-28, "Invalid credentials");

            /* Check the sessions. */
            for(auto session = mapSessions.begin(); session != mapSessions.end(); ++ session)
            {
                if(hashGenesis == session->second->Genesis())
                {
                    delete user;
                    user = nullptr;

                    ret["genesis"] = hashGenesis.ToString();
                    ret["session"] = session->first;

                    return ret;
                }
            }

            /* Set the return value. */
            uint64_t nSession = LLC::GetRand();
            ret["genesis"] = hashGenesis.ToString();
            ret["session"] = nSession;

            /* Setup the account. */
            mapSessions[nSession] = user;

            return ret;
        }
    }
}
