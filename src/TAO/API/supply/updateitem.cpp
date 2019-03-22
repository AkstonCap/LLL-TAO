/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/accounts.h>
#include <TAO/API/include/supply.h>

#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/verify.h>
#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <LLC/include/random.h>
#include <LLD/include/global.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Submits an item. */
        json::json Supply::UpdateItem(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Check for pin parameter. */
            SecureString strPIN;
            bool fNeedPin = accounts.Locked();

            if( fNeedPin && params.find("pin") == params.end() )
                throw APIException(-25, "Missing PIN");
            else if( fNeedPin)
                strPIN = params["pin"].get<std::string>().c_str();
            else
                strPIN = accounts.GetActivePin();   

            /* Check for session parameter. */
            uint64_t nSession = 0;
            bool fNeedSession = !accounts.LoggedIn();

            if(fNeedSession && params.find("session") == params.end())
                throw APIException(-25, "Missing Session ID");
            else if(fNeedSession)
                nSession = std::stoull(params["session"].get<std::string>());

            /* Check for address parameter. */
            if(params.find("address") == params.end())
                throw APIException(-25, "Missing Address");

            /* Check for data parameter. */
            if(params.find("data") == params.end())
                throw APIException(-25, "Missing data");

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = accounts.GetAccount(nSession);
            if(!user)
                throw APIException(-25, "Invalid session ID");

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-25, "Failed to create transaction");

            /* Submit the transaction payload. */
            uint256_t hashRegister;
            hashRegister.SetHex(params["address"].get<std::string>());

            /* Test the payload feature. */
            DataStream ssData(SER_REGISTER, 1);
            ssData << params["data"].get<std::string>();

            /* Submit the payload object. */
            tx << (uint8_t)TAO::Operation::OP::APPEND << hashRegister << ssData.Bytes();

            /* Execute the operations layer. */
            if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE))
                throw APIException(-26, "Operations failed to execute");

            /* Sign the transaction. */
            if(!tx.Sign(accounts.GetKey(tx.nSequence, strPIN, nSession)))
                throw APIException(-26, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw APIException(-26, "Failed to accept");

            /* Build a JSON response object. */
            ret["txid"]  = tx.GetHash().ToString();
            ret["address"] = hashRegister.ToString();

            return ret;
        }
    }
}
