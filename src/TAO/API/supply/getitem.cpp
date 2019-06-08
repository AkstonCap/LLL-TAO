/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/supply.h>
#include <TAO/API/include/utils.h>

#include <LLD/include/global.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Get's the description of an item. */
        json::json Supply::GetItem(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the Register ID. */
            uint256_t hashRegister = 0;

            /* Check whether the caller has provided the asset name parameter. */
            if(params.find("name") != params.end())
            {
                /* If name is provided then use this to deduce the register address */
                hashRegister = AddressFromName( params, params["name"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetHex(params["address"].get<std::string>());

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-23, "Missing name / address");

            /* Get the history. */
            TAO::Register::State state;
            if(!LLD::Register->ReadState(hashRegister, state, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-24, "No state found");

            /* Build the response JSON. */
            //ret["version"]  = state.nVersion;
            //ret["type"]     = state.nType;
            ret["address"]    = hashRegister.ToString();
            ret["created"]   = state.nCreated;
            ret["modified"]  = state.nModified;
            ret["owner"]     = state.hashOwner.ToString();

            while(!state.end())
            {
                /* If the data type is string. */
                std::string data;
                state >> data;

                //ret["checksum"] = state.hashChecksum;
                ret["data"] = data;
            }

            return ret;
        }
    }
}
