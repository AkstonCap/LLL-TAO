/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/accounts.h>
#include <TAO/API/include/supply.h>

#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/verify.h>
#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/types/sigchain.h>

#include <LLC/include/random.h>
#include <LLD/include/global.h>

namespace TAO::API
{
    /* Declaration of the api */
    Supply supply;


    /* Standard initialization function. */
    void Supply::Initialize()
    {
        mapFunctions["getitem"]             = Function(std::bind(&Supply::GetItem,    this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["transfer"]            = Function(std::bind(&Supply::Transfer,   this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["createitem"]          = Function(std::bind(&Supply::CreateItem, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["appenditem"]          = Function(std::bind(&Supply::AppendItem, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["history"]             = Function(std::bind(&Supply::History,    this, std::placeholders::_1, std::placeholders::_2));
    }


    /* Get's the description of an item. */
    json::json Supply::GetItem(const json::json& params, bool fHelp)
    {
        json::json ret;

        /* Check for username parameter. */
        if(params.find("address") == params.end())
            throw APIException(-23, "Missing memory address");

        /* Get the Register ID. */
        uint256_t hashRegister;
        hashRegister.SetHex(params["address"]);

        /* Get the history. */
        TAO::Register::State state;
        if(!LLD::regDB->ReadState(hashRegister, state))
            throw APIException(-24, "No state found");

        /* Build the response JSON. */
        //ret["version"]  = state.nVersion;
        //ret["type"]     = state.nType;
        ret["updated"]  = state.nTimestamp;
        ret["owner"]    = state.hashOwner.ToString();

        while(!state.end())
        {
            /* If the data type is string. */
            std::string data;
            state >> data;

            //ret["checksum"] = state.hashChecksum;
            ret["states"].push_back(data);
        }

        return ret;
    }


    /* Transfers an item. */
    json::json Supply::Transfer(const json::json& params, bool fHelp)
    {
        json::json ret;

        /* Check for pin parameter. */
        if(params.find("pin") == params.end())
            throw APIException(-25, "Missing PIN");

        /* Check for username parameter. */
        if(params.find("session") == params.end())
            throw APIException(-25, "Missing Session ID");

        /* Check for id parameter. */
        if(params.find("address") == params.end())
            throw APIException(-25, "Missing register ID");

        /* Check for id parameter. */
        if(params.find("destination") == params.end())
            throw APIException(-25, "Missing Destination");

        /* Watch for destination genesis. */
        uint256_t hashTo;
        hashTo.SetHex(params["destination"].get<std::string>());
        if(!LLD::legDB->HasGenesis(hashTo))
            throw APIException(-25, "Destination doesn't exist");

        /* Get the session. */
        uint64_t nSession = std::stoull(params["session"].get<std::string>());

        /* Get the Genesis ID. */
        uint256_t hashGenesis = accounts.GetGenesis(nSession);

        /* Get the last transaction. */
        uint512_t hashLast;
        if(!LLD::locDB->ReadLast(hashGenesis, hashLast))
            throw APIException(-28, "No transactions found");

        /* Get previous transaction */
        TAO::Ledger::Transaction txPrev;
        if(!LLD::legDB->ReadTx(hashLast, txPrev))
            throw APIException(-29, "Failed to read previous transaction");

        /* Build new transaction object. */
        TAO::Ledger::Transaction tx;
        tx.nSequence   = txPrev.nSequence + 1;
        tx.hashGenesis = txPrev.hashGenesis;
        tx.hashPrevTx  = hashLast;
        tx.NextHash(accounts.GetKey(tx.nSequence + 1, params["pin"].get<std::string>().c_str(), nSession));

        /* Submit the transaction payload. */
        uint256_t hashRegister;
        hashRegister.SetHex(params["address"].get<std::string>());

        /* Submit the payload object. */
        tx << (uint8_t)TAO::Operation::OP::TRANSFER << hashRegister << hashTo;

        /* Execute the operations layer. */
        if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE))
            throw APIException(-26, "Operations failed to execute");

        /* Verify the register layer. */
        if(!TAO::Register::Verify(tx))
            throw APIException(-26, "Registers failed to verify");

        /* Sign the transaction. */
        if(!tx.Sign(accounts.GetKey(tx.nSequence, params["pin"].get<std::string>().c_str(), nSession)))
            throw APIException(-26, "Ledger failed to sign transaction");

        /* Check that the transaction is valid. */
        if(!tx.IsValid())
            throw APIException(-26, "Ledger Transaction Invalid");

        /* Write transaction to local database. */
        LLD::legDB->WriteTx(tx.GetHash(), tx);
        LLD::locDB->WriteLast(hashGenesis, tx.GetHash());

        /* Execute the operations layer. */
        if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::WRITE))
            throw APIException(-26, "Operations failed to execute");

        /* Build a JSON response object. */
        ret["txid"]  = tx.GetHash().ToString();
        ret["address"] = hashRegister.ToString();

        return ret;
    }


    /* Submits an item. */
    json::json Supply::CreateItem(const json::json& params, bool fHelp)
    {
        json::json ret;

        /* Check for pin parameter. */
        if(params.find("pin") == params.end())
            throw APIException(-25, "Missing PIN");

        /* Check for username parameter. */
        if(params.find("session") == params.end())
            throw APIException(-25, "Missing Session ID");

        /* Check for data parameter. */
        if(params.find("data") == params.end())
            throw APIException(-25, "Missing data");

        /* Get the session. */
        uint64_t nSession = std::stoull(params["session"].get<std::string>());

        /* Get the Genesis ID. */
        uint256_t hashGenesis = accounts.GetGenesis(nSession);

        /* Get the last transaction. */
        uint512_t hashLast;
        if(!LLD::locDB->ReadLast(hashGenesis, hashLast))
            throw APIException(-28, "No transactions found");

        /* Get previous transaction */
        TAO::Ledger::Transaction txPrev;
        if(!LLD::legDB->ReadTx(hashLast, txPrev))
            throw APIException(-29, "Failed to read previous transaction");

        /* Build new transaction object. */
        TAO::Ledger::Transaction tx;
        tx.nSequence   = txPrev.nSequence + 1;
        tx.hashGenesis = txPrev.hashGenesis;
        tx.hashPrevTx  = hashLast;
        tx.NextHash(accounts.GetKey(tx.nSequence + 1, params["pin"].get<std::string>().c_str(), nSession));

        /* Submit the transaction payload. */
        uint256_t hashRegister = LLC::GetRand256();

        /* Test the payload feature. */
        DataStream ssData(SER_REGISTER, 1);
        ssData << params["data"].get<std::string>();

        /* Submit the payload object. */
        tx << (uint8_t)TAO::Operation::OP::REGISTER << hashRegister << (uint8_t)TAO::Register::OBJECT::APPEND << static_cast<std::vector<uint8_t>>(ssData);

        /* Execute the operations layer. */
        if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE))
            throw APIException(-26, "Operations failed to execute");

        /* Verify the register layer. */
        if(!TAO::Register::Verify(tx))
            throw APIException(-26, "Registers failed to verify");

        /* Sign the transaction. */
        if(!tx.Sign(accounts.GetKey(tx.nSequence, params["pin"].get<std::string>().c_str(), nSession)))
            throw APIException(-26, "Ledger failed to sign transaction");

        /* Check that the transaction is valid. */
        if(!tx.IsValid())
            throw APIException(-26, "Ledger Transaction Invalid");

        /* Write transaction to local database. */
        LLD::legDB->WriteTx(tx.GetHash(), tx);
        LLD::locDB->WriteLast(hashGenesis, tx.GetHash());

        /* Execute the operations layer. */
        if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::WRITE))
            throw APIException(-26, "Operations failed to execute");

        /* Build a JSON response object. */
        ret["txid"]  = tx.GetHash().ToString();
        ret["address"] = hashRegister.ToString();

        return ret;
    }


    /* Submits an item. */
    json::json Supply::AppendItem(const json::json& params, bool fHelp)
    {
        json::json ret;

        /* Check for pin parameter. */
        if(params.find("pin") == params.end())
            throw APIException(-25, "Missing PIN");

        /* Check for username parameter. */
        if(params.find("session") == params.end())
            throw APIException(-25, "Missing Session ID");

        /* Check for address parameter. */
        if(params.find("address") == params.end())
            throw APIException(-25, "Missing Address");

        /* Check for data parameter. */
        if(params.find("data") == params.end())
            throw APIException(-25, "Missing data");

        /* Get the session. */
        uint64_t nSession = std::stoull(params["session"].get<std::string>());

        /* Get the Genesis ID. */
        uint256_t hashGenesis = accounts.GetGenesis(nSession);

        /* Get the last transaction. */
        uint512_t hashLast;
        if(!LLD::locDB->ReadLast(hashGenesis, hashLast))
            throw APIException(-28, "No transactions found");

        /* Get previous transaction */
        TAO::Ledger::Transaction txPrev;
        if(!LLD::legDB->ReadTx(hashLast, txPrev))
            throw APIException(-29, "Failed to read previous transaction");

        /* Build new transaction object. */
        TAO::Ledger::Transaction tx;
        tx.nSequence   = txPrev.nSequence + 1;
        tx.hashGenesis = txPrev.hashGenesis;
        tx.hashPrevTx  = hashLast;
        tx.NextHash(accounts.GetKey(tx.nSequence + 1, params["pin"].get<std::string>().c_str(), nSession));

        /* Submit the transaction payload. */
        uint256_t hashRegister;
        hashRegister.SetHex(params["address"].get<std::string>());

        /* Test the payload feature. */
        DataStream ssData(SER_REGISTER, 1);
        ssData << params["data"].get<std::string>();

        /* Submit the payload object. */
        tx << (uint8_t)TAO::Operation::OP::APPEND << hashRegister << static_cast<std::vector<uint8_t>>(ssData);

        /* Execute the operations layer. */
        if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE))
            throw APIException(-26, "Operations failed to execute");

        /* Verify the register layer. */
        if(!TAO::Register::Verify(tx))
            throw APIException(-26, "Registers failed to verify");

        /* Sign the transaction. */
        if(!tx.Sign(accounts.GetKey(tx.nSequence, params["pin"].get<std::string>().c_str(), nSession)))
            throw APIException(-26, "Ledger failed to sign transaction");

        /* Check that the transaction is valid. */
        if(!tx.IsValid())
            throw APIException(-26, "Ledger Transaction Invalid");

        /* Write transaction to local database. */
        LLD::legDB->WriteTx(tx.GetHash(), tx);
        LLD::locDB->WriteLast(hashGenesis, tx.GetHash());

        /* Execute the operations layer. */
        if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::WRITE))
            throw APIException(-26, "Operations failed to execute");

        /* Build a JSON response object. */
        ret["txid"]  = tx.GetHash().ToString();
        ret["address"] = hashRegister.ToString();

        return ret;
    }


    /* Gets the history of an item. */
    json::json Supply::History(const json::json& params, bool fHelp)
    {
        json::json ret;

        /* Check for username parameter. */
        if(params.find("address") == params.end())
            throw APIException(-23, "Missing memory address");

        /* Get the Register ID. */
        uint256_t hashRegister;
        hashRegister.SetHex(params["address"].get<std::string>());

        /* Get the history. */
        std::vector<TAO::Register::State> states;
        if(!LLD::regDB->GetStates(hashRegister, states))
            throw APIException(-24, "No states found");

        /* Build the response JSON. */
        for(auto & state : states)
        {
            json::json obj;
            obj["version"]  = state.nVersion;
            obj["owner"]    = state.hashOwner.ToString();
            obj["updated"]  = state.nTimestamp;

            while(!state.end())
            {
                /* If the data type is string. */
                std::string data;
                state >> data;

                //ret["checksum"] = state.hashChecksum;
                obj["states"].push_back(data);
            }

            ret.push_back(obj);
        }

        return ret;
    }
}
