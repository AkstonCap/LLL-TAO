/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/assets/types/assets.h>
#include <TAO/API/names/types/names.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/json.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get the data from a digital asset */
        json::json Assets::Get(const json::json& params, const bool fHelp)
        {
            json::json ret;

            /* Get the Register ID. */
            TAO::Register::Address hashRegister ;

            /* Check whether the caller has provided the asset name parameter. */
            if(params.find("name") != params.end() && !params["name"].get<std::string>().empty())
            {
                /* If name is provided then use this to deduce the register address */
                hashRegister = Names::ResolveAddress(params, params["name"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetBase58(params["address"].get<std::string>());

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-33, "Missing name / address");


            /* Check to see whether the caller has requested a specific data field to return */
            std::string strDataField = "";


            /* Get the asset from the register DB.  We can read it as an Object.
               If this fails then we try to read it as a base State type and assume it was
               created as a raw format asset */
            TAO::Register::Object object;
            if(!LLD::Register->ReadState(hashRegister, object, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-34, "Asset not found");

            if(config::fClient.load() && object.hashOwner != Commands::Get<Users>()->GetCallersGenesis(params))
                throw APIException(-300, "API can only be used to lookup data for the currently logged in signature chain when running in client mode");

            /* Only include raw and non-standard object types (assets)*/
            if(object.nType != TAO::Register::REGISTER::APPEND
            && object.nType != TAO::Register::REGISTER::RAW
            && object.nType != TAO::Register::REGISTER::OBJECT)
            {
                throw APIException(-35, "Specified name/address is not an asset.");
            }

            if(object.nType == TAO::Register::REGISTER::OBJECT)
            {
                /* parse object so that the data fields can be accessed */
                if(!object.Parse())
                    throw APIException(-36, "Failed to parse object register");

                /* Only include non standard object registers (assets) */
                if(object.Standard() != TAO::Register::OBJECTS::NONSTANDARD)
                    throw APIException(-35, "Specified name/address is not an asset.");
            }

            /* Populate the response JSON */
            ret["owner"]    = TAO::Register::Address(object.hashOwner).ToString();
            ret["created"]  = object.nCreated;
            ret["modified"] = object.nModified;

            json::json data  =TAO::API::ObjectToJSON(params, object, hashRegister);

            /* Copy the asset data in to the response after the type/checksum */
            ret.insert(data.begin(), data.end());

            /* If the caller has requested to filter on a fieldname then filter out the json response to only include that field */
            FilterResponse(params, ret);

            return ret;
        }
    }
}
