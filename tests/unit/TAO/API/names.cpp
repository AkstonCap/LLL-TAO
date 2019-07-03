/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include "util.h"

#include <LLC/include/random.h>

#include <unit/catch2/catch.hpp>

#include <LLD/include/global.h>

#include <TAO/Ledger/types/transaction.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/create.h>
#include <TAO/Register/include/names.h>

#include <TAO/API/types/names.h>
#include <TAO/API/include/global.h>

#include <Util/include/convert.h>
#include <Util/include/base64.h>

/* Declare names / hashes in global scope so that we can reuse them for the update/get */
std::string strName = "NAME" +std::to_string(LLC::GetRand());
uint256_t hashName = 0;
uint512_t hashNameTransfer = 0;
std::string strNamespace = "NAMESPACE" +std::to_string(LLC::GetRand());
uint256_t hashNamespace = 0;
uint512_t hashNamespaceTransfer = 0;

uint256_t hashRegisterAddress = LLC::GetRand256();

TEST_CASE( "Test Names API - create namespace", "[names/create/namespace]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /*  fail with missing pin (only relevant for multiuser)*/
    if(config::fMultiuser.load())
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;

        /* Invoke the API */
        ret = APICall("names/create/namespace", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -129);
    }

    /* fail with missing session (only relevant for multiuser)*/
    if(config::fMultiuser.load())
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;

        /* Invoke the API */
        ret = APICall("names/create/namespace", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -12);
    }

    /* fail with missing name */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;

        /* Invoke the API */
        ret = APICall("names/create/namespace", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -88);
    }

    /* fail with invalid chars in name name */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = "not.allowed";

        /* Invoke the API */
        ret = APICall("names/create/namespace", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -162);
    }

    /* fail with invalid chars in name name */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = "not:allowed";

        /* Invoke the API */
        ret = APICall("names/create/namespace", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -162);
    }

    /* success case */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = strNamespace;
        

        /* Invoke the API */
        ret = APICall("names/create/namespace", params);

        /* Check response and validate fields */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());

        /* Grab the namespace hash for later use */
        hashNamespace.SetHex(result["address"].get<std::string>());
    }

    /* fail with already exists */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = strNamespace;

        /* Invoke the API */
        ret = APICall("names/create/namespace", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -90);
    }

}

TEST_CASE( "Test Names API - get namespace", "[names/get/namespace]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /* fail with missing name */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;

        /* Invoke the API */
        ret = APICall("names/get/namespace", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -93);
    }

    /* fail with invalid name */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = "not a namespace";

        /* Invoke the API */
        ret = APICall("names/get/namespace", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -94);
    }

    /* success case */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = strNamespace;
        

        /* Invoke the API */
        ret = APICall("names/get/namespace", params);

        /* Check response and validate fields */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("owner") != result.end());
        REQUIRE(result.find("created") != result.end());
        REQUIRE(result.find("address") != result.end());
        REQUIRE(result.find("name") != result.end());
    }

}

TEST_CASE( "Test Names API - transfer namespace", "[names/transfer/namespace]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);
    InitializeUser(USERNAME2, PASSWORD, PIN, GENESIS2, SESSION2);

    /* fail with missing username / destination */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;

        /* Invoke the API */
        ret = APICall("names/transfer/namespace", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -112);
    }

    /* fail with invalid destination */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["destination"] = LLC::GetRand256().GetHex(); 

        /* Invoke the API */
        ret = APICall("names/transfer/namespace", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -113);
    }

    /* fail with invalid username */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["username"] = "not a user"; 

        /* Invoke the API */
        ret = APICall("names/transfer/namespace", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -113);
    }

    /* fail with missing name / address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["destination"] = GENESIS2.GetHex(); 

        /* Invoke the API */
        ret = APICall("names/transfer/namespace", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -33);
    }

    /* fail with invalid name  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = "invalid name";
        params["destination"] = GENESIS2.GetHex();

        /* Invoke the API */
        ret = APICall("names/transfer/namespace", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -114);
    }

    /* fail with invalid address  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = LLC::GetRand256().GetHex();
        params["destination"] = GENESIS2.GetHex();

        /* Invoke the API */
        ret = APICall("names/transfer/namespace", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -114);
    }

    /* Success case */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = hashNamespace.GetHex();
        params["destination"] = GENESIS2.GetHex();

        /* Invoke the API */
        ret = APICall("names/transfer/namespace", params);
         REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* Check all of the fields */
        REQUIRE(result.find("txid") != result.end());
        
        /* Grab the transfer txid so that we can use it for a claim */
        hashNamespaceTransfer.SetHex(result["txid"].get<std::string>());

    }

}

TEST_CASE( "Test Names API - claim namespace", "[names/claim/namespace]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);
    InitializeUser(USERNAME2, PASSWORD, PIN, GENESIS2, SESSION2);

    /* fail with missing txid */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["pin"] = PIN;

        /* Invoke the API */
        ret = APICall("names/claim/namespace", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -50);
    }

    /* fail with invalid txid */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["pin"] = PIN;
        params["txid"] = LLC::GetRand512().GetHex();

        /* Invoke the API */
        ret = APICall("names/claim/namespace", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -99);
    }

    /* Success case */
    {

        {
            /* Create a namespace and persis the tx to the DB */
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = GENESIS1;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            strNamespace = "NAMESPACE" +std::to_string(LLC::GetRand());
            hashNamespace = TAO::Register::Address(strNamespace, TAO::Register::Address::NAMESPACE);
            TAO::Register::Object namespaceObject = TAO::Register::CreateNamespace(strNamespace);

            //payload
            tx[0] << uint8_t(TAO::Operation::OP::CREATE) << hashNamespace << uint8_t(TAO::Register::REGISTER::OBJECT) << namespaceObject.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }
        {
            /* Create the transfer transaction so that it is persisted to the DB */
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = GENESIS1;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();
            tx.hashNextTx  = TAO::Ledger::STATE::HEAD;

            //payload
            tx[0] << uint8_t(TAO::Operation::OP::TRANSFER) << hashNamespace << GENESIS2 << uint8_t(TAO::Operation::TRANSFER::CLAIM);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
            

            hashNamespaceTransfer = tx.GetHash();
        }

        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["pin"] = PIN;
        params["txid"] = hashNamespaceTransfer.GetHex();

        /* Invoke the API */
        ret = APICall("names/claim/namespace", params);
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* Check all of the fields */
        REQUIRE(result.find("txid") != result.end());
        

    }
}

TEST_CASE( "Test Names API - list namespace history", "[names/list/namespace/history]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);
    InitializeUser(USERNAME2, PASSWORD, PIN, GENESIS2, SESSION2);

    /* fail with missing name / address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;

        /* Invoke the API */
        ret = APICall("names/list/namespace/history", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -105);
    }

    /* fail with invalid name  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["name"] = "not a namspace";

        /* Invoke the API */
        ret = APICall("names/list/namespace/history", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -106);
    }

    /* fail with invalid address  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["address"] = LLC::GetRand256().GetHex();

        /* Invoke the API */
        ret = APICall("names/list/namespace/history", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -106);
    }



    /* success case by address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["address"] = hashNamespace.GetHex();

        /* Invoke the API */
        ret = APICall("names/list/namespace/history", params);

        /* Check response  */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* history should be an array */
        REQUIRE(result.is_array());

        /* Check all of the fields */
        for( const auto& entry : result)
        {
            REQUIRE(entry.find("type") != entry.end());
            std::string strType = entry["type"].get<std::string>();
            REQUIRE((strType == "CREATE" || strType == "MODIFY" || strType == "TRANSFER" || strType == "CLAIM"));
            REQUIRE(entry.find("owner") != entry.end());
            REQUIRE(entry.find("modified") != entry.end());
            REQUIRE(entry.find("checksum") != entry.end());
            REQUIRE(entry.find("address") != entry.end());
            REQUIRE(entry.find("name") != entry.end());

            /* Check the name at each point */
            std::string strName = entry["name"].get<std::string>();
            REQUIRE(strName == strNamespace);
            
        }
        
    }

}

TEST_CASE( "Test Names API - create name", "[names/create/name]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /*  fail with missing pin (only relevant for multiuser)*/
    if(config::fMultiuser.load())
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;

        /* Invoke the API */
        ret = APICall("names/create/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -129);
    }

    /* fail with missing session (only relevant for multiuser)*/
    if(config::fMultiuser.load())
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["pin"] = PIN;

        /* Invoke the API */
        ret = APICall("names/create/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -12);
    }

    /* fail with missing name */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;

        /* Invoke the API */
        ret = APICall("names/create/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -88);
    }

    /* fail with invalid register_address  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = strName;
        params["register_address"] = "notaregisteraddress";

        /* Invoke the API */
        ret = APICall("names/create/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -89);
    }

    /* fail with invalid chars in name */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = "not:allowed";
        params["register_address"] = hashRegisterAddress.GetHex();

        /* Invoke the API */
        ret = APICall("names/create/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -161);
    }

    /* fail with invalid namespace  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = strName +".notanamespace";
        params["register_address"] = hashRegisterAddress.GetHex();
     
        /* Invoke the API */
        ret = APICall("names/create/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -95);
    }

    /* fail with not owner of namespace  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = strName +"." +strNamespace;
        params["register_address"] = hashRegisterAddress.GetHex();
     
        /* Invoke the API */
        ret = APICall("names/create/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -96);
    }

    /* success case in local namespace*/
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = strName;
        params["register_address"] = hashRegisterAddress.GetHex();

        /* Invoke the API */
        ret = APICall("names/create/name", params);

        /* Check response and validate fields */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());

        /* Grab the name hash for later use */
        hashName.SetHex(result["address"].get<std::string>());
    }

    /* success case in global namespace (SESSION2 owns strNamespace)*/
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["pin"] = PIN;
        params["name"] = strName +"." +strNamespace;
        params["register_address"] = hashRegisterAddress.GetHex();
        
        /* Invoke the API */
        ret = APICall("names/create/name", params);

        /* Check response and validate fields */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());

        /* Grab the name hash for later use */
        hashName.SetHex(result["address"].get<std::string>());
    }
}


TEST_CASE( "Test Names API - get name", "[names/get/name]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    /* fail with missing name / register address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;

        /* Invoke the API */
        ret = APICall("names/get/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -91);
    }

    /* fail with invalid name */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["name"] = "not a name";

        /* Invoke the API */
        ret = APICall("names/get/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -101);
    }
    

    /* fail with invalid register_address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["register_address"] = LLC::GetRand256().GetHex();

        /* Invoke the API */
        ret = APICall("names/get/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -92);
    }

    /* success case by local name*/
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["name"] = strName;

        /* Invoke the API */
        ret = APICall("names/get/name", params);

        /* Check response and validate fields */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("owner") != result.end());
        REQUIRE(result.find("created") != result.end());
        REQUIRE(result.find("address") != result.end());
        REQUIRE(result.find("name") != result.end());
        REQUIRE(result.find("namespace") != result.end());
        REQUIRE(result.find("register_address") != result.end());
    }

    /* success case by namespace name*/
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["name"] = strName + "." +strNamespace;

        /* Invoke the API */
        ret = APICall("names/get/name", params);

        /* Check response and validate fields */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("owner") != result.end());
        REQUIRE(result.find("created") != result.end());
        REQUIRE(result.find("address") != result.end());
        REQUIRE(result.find("name") != result.end());
        REQUIRE(result.find("namespace") != result.end());
        REQUIRE(result.find("register_address") != result.end());
    }

    /* success case by register address*/
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1; // session is required so that we know which sig chain to search
        params["register_address"] = hashRegisterAddress.GetHex();

        /* Invoke the API */
        ret = APICall("names/get/name", params);

        /* Check response and validate fields */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        REQUIRE(result.find("owner") != result.end());
        REQUIRE(result.find("created") != result.end());
        REQUIRE(result.find("address") != result.end());
        REQUIRE(result.find("name") != result.end());
        REQUIRE(result.find("namespace") != result.end());
        REQUIRE(result.find("register_address") != result.end());
    }

}

TEST_CASE( "Test Names API - update name", "[names/update/name]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);

    hashRegisterAddress = LLC::GetRand256();

    /* fail with missing register_address  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = hashName.GetHex();

        /* Invoke the API */
        ret = APICall("names/update/name", params); 

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -103);
    }

    /* fail with missing name / address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["register_address"] = hashRegisterAddress.GetHex();

        /* Invoke the API */
        ret = APICall("names/update/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -33);
    }

    /* fail with missing invalid name  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = "invalid name";
        params["register_address"] = hashRegisterAddress.GetHex();

        /* Invoke the API */
        ret = APICall("names/update/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -101);
    }

    /* fail with missing invalid address  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = LLC::GetRand256().GetHex();
        params["register_address"] = hashRegisterAddress.GetHex();

        /* Invoke the API */
        ret = APICall("names/update/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -102);
    }

    /* Success case */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["pin"] = PIN;
        params["address"] = hashName.GetHex();
        params["register_address"] = hashRegisterAddress.GetHex();

        /* Invoke the API */
        ret = APICall("names/update/name", params);

        /* Check the result */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* Check all of the fields */
        REQUIRE(result.find("txid") != result.end());
        REQUIRE(result.find("address") != result.end());

        /* Retrieve the item and check the value */
        ret = APICall("names/get/name", params);

        /* Check the result */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* Check all of the fields */
        REQUIRE(result.find("register_address") != result.end());
        REQUIRE(result["register_address"].get<std::string>() == hashRegisterAddress.GetHex());
    }
}

TEST_CASE( "Test Names API - transfer name", "[names/transfer/name]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);
    InitializeUser(USERNAME2, PASSWORD, PIN, GENESIS2, SESSION2);

    /* fail with missing username / destination */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;

        /* Invoke the API */
        ret = APICall("names/transfer/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -112);
    }

    /* fail with invalid destination */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["destination"] = LLC::GetRand256().GetHex(); 

        /* Invoke the API */
        ret = APICall("names/transfer/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -113);
    }

    /* fail with invalid username */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["username"] = "not a user"; 

        /* Invoke the API */
        ret = APICall("names/transfer/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -113);
    }

    /* fail with missing name / address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["destination"] = GENESIS2.GetHex(); 

        /* Invoke the API */
        ret = APICall("names/transfer/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -33);
    }

    /* fail with invalid name  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = "invalid name";
        params["destination"] = GENESIS2.GetHex();

        /* Invoke the API */
        ret = APICall("names/transfer/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -101);
    }

    /* fail with invalid address  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["address"] = LLC::GetRand256().GetHex();
        params["destination"] = GENESIS2.GetHex();

        /* Invoke the API */
        ret = APICall("names/transfer/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -114);
    }

    /* Fail with not being able to transfer a local name */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION1;
        params["pin"] = PIN;
        params["name"] = strName;
        params["destination"] = GENESIS2.GetHex();

        /* Invoke the API */
        ret = APICall("names/transfer/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -116);
    }

    /* Success case (the name.namespace name was created under session2 so transfer to session1 / genesis1)*/
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["pin"] = PIN;
        params["name"] = strName +"." +strNamespace;;
        params["destination"] = GENESIS1.GetHex();

        /* Invoke the API */
        ret = APICall("names/transfer/name", params);
         REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* Check all of the fields */
        REQUIRE(result.find("txid") != result.end());
        
        /* Grab the transfer txid so that we can use it for a claim */
        hashNameTransfer.SetHex(result["txid"].get<std::string>());

    }

}


TEST_CASE( "Test Names API - claim name", "[names/claim/name]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);
    InitializeUser(USERNAME2, PASSWORD, PIN, GENESIS2, SESSION2);

    /* fail with missing txid */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["pin"] = PIN;

        /* Invoke the API */
        ret = APICall("names/claim/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -50);
    }

    /* fail with invalid txid */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["pin"] = PIN;
        params["txid"] = LLC::GetRand512().GetHex();

        /* Invoke the API */
        ret = APICall("names/claim/name", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -99);
    }

    /* Success case */
    {

        {
            /* Create a namespace and persis the tx to the DB */
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = GENESIS1;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            strNamespace = "NAMESPACE" +std::to_string(LLC::GetRand());
            hashNamespace = TAO::Register::Address(strNamespace, TAO::Register::Address::NAMESPACE);
            TAO::Register::Object namespaceObject = TAO::Register::CreateNamespace(strNamespace);

            //payload
            tx[0] << uint8_t(TAO::Operation::OP::CREATE) << hashNamespace << uint8_t(TAO::Register::REGISTER::OBJECT) << namespaceObject.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }

        {
            /* Create a name in the above namesapce and persis the tx to the DB */
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = GENESIS1;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            //create name object in the namespace
            strName = "name." + strNamespace ;

            /* Get the address of the name register based on the namespace and name */
            TAO::Register::GetNameAddress(hashNamespace, "name", hashName);
            
            //payload
            tx[0] = TAO::API::Names::CreateName(GENESIS1, strName, hashRegisterAddress);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        }

        {
            /* Create the transfer transaction so that it is persisted to the DB */
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = GENESIS1;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();
            tx.hashNextTx  = TAO::Ledger::STATE::HEAD;

            //payload
            tx[0] << uint8_t(TAO::Operation::OP::TRANSFER) << hashName << GENESIS2 << uint8_t(TAO::Operation::TRANSFER::CLAIM);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
            

            hashNameTransfer = tx.GetHash();
        }

        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["pin"] = PIN;
        params["txid"] = hashNameTransfer.GetHex();

        /* Invoke the API */
        ret = APICall("names/claim/name", params);
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* Check all of the fields */
        REQUIRE(result.find("txid") != result.end());
        

    }
}

TEST_CASE( "Test Names API - list name history", "[names/list/name/history]")
{
    /* Declare variables shared across test cases */
    json::json params;
    json::json ret;
    json::json result;
    json::json error;

    /* Ensure user is created and logged in for testing */
    InitializeUser(USERNAME1, PASSWORD, PIN, GENESIS1, SESSION1);
    InitializeUser(USERNAME2, PASSWORD, PIN, GENESIS2, SESSION2);

    /* fail with missing name / address */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;

        /* Invoke the API */
        ret = APICall("names/list/name/history", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -105);
    }

    /* fail with invalid name  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["name"] = "not a name";

        /* Invoke the API */
        ret = APICall("names/list/name/history", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -101);
    }

    /* fail with invalid address  */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["address"] = LLC::GetRand256().GetHex();

        /* Invoke the API */
        ret = APICall("names/list/name/history", params);

        /* Check response is an error and validate error code */
        REQUIRE(ret.find("error") != ret.end());
        REQUIRE(ret["error"]["code"].get<int32_t>() == -106);
    }


    /* success case by name */
    {
        /* Build the parameters to pass to the API */
        params.clear();
        params["session"] = SESSION2;
        params["name"] = strName;

        /* Invoke the API */
        ret = APICall("names/list/name/history", params);

        /* Check response  */
        REQUIRE(ret.find("result") != ret.end());
        result = ret["result"];

        /* history should be an array */
        REQUIRE(result.is_array());

        /* Check all of the fields */
        for( const auto& entry : result)
        {
            REQUIRE(entry.find("type") != entry.end());
            std::string strType = entry["type"].get<std::string>();
            REQUIRE((strType == "CREATE" || strType == "MODIFY" || strType == "TRANSFER" || strType == "CLAIM"));
            REQUIRE(entry.find("owner") != entry.end());
            REQUIRE(entry.find("modified") != entry.end());
            REQUIRE(entry.find("checksum") != entry.end());
            REQUIRE(entry.find("address") != entry.end());
            REQUIRE(entry.find("name") != entry.end());

            /* Check the name at each point */
            std::string strName = entry["name"].get<std::string>();
            REQUIRE(strName == strName);
            
        }
        
    }

}