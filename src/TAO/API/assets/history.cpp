/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/assets.h>
#include <TAO/API/include/utils.h>

#include <LLD/include/global.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* History of an asset and its ownership */
        json::json Assets::History(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the Register ID. */
            uint256_t hashRegister = 0;

            /* Check whether the caller has provided the asset name parameter. */
            if(params.find("name") != params.end())
            {
                /* If name is provided then use this to deduce the register address */
                hashRegister = RegisterAddressFromName( params, "asset", params["name"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetHex(params["address"]);

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-23, "Missing memory address");

            /* Get the history. */
            std::vector<TAO::Register::State> states;
            if(!LLD::regDB->GetStates(hashRegister, states))
                throw APIException(-24, "No states found");

            /* Build the response JSON. */
            for(const auto& state : states)
            {
                json::json obj;
                obj["owner"]      = state.hashOwner.ToString();
                obj["timestamp"]  = state.nTimestamp;

                ret.push_back(obj);
            }

            return ret;
        }
    }
}
