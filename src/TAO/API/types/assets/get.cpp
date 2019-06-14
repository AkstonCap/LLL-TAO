/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/assets.h>
#include <TAO/API/types/names.h>
#include <TAO/API/include/json.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get the data from a digital asset */
        json::json Assets::Get(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the Register ID. */
            uint256_t hashRegister = 0;

            /* Check whether the caller has provided the asset name parameter. */
            if(params.find("name") != params.end())
            {
                /* If name is provided then use this to deduce the register address */
                hashRegister = Names::ResolveAddress( params, params["name"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetHex(params["address"].get<std::string>());

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-23, "Missing name / address");


            /* Check to see whether the caller has requested a specific data field to return */
            std::string strDataField = "";


            /* Get the asset from the register DB.  We can read it as an Object.
               If this fails then we try to read it as a base State type and assume it was
               created as a raw format asset */
            TAO::Register::Object object;
            if(!LLD::Register->ReadState(hashRegister, object, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-24, "Asset not found");

            /* Only include raw and non-standard object types (assets)*/
            if(object.nType != TAO::Register::REGISTER::APPEND
            && object.nType != TAO::Register::REGISTER::RAW
            && object.nType != TAO::Register::REGISTER::OBJECT)
            {
                throw APIException(-24, "Specified name/address is not an asset.");
            }

            if(object.nType == TAO::Register::REGISTER::OBJECT)
            {
                /* parse object so that the data fields can be accessed */
                if(!object.Parse())
                    throw APIException(-24, "Failed to parse object register");

                /* Only include non standard object registers (assets) */
                if(object.Standard() != TAO::Register::OBJECTS::NONSTANDARD)
                    throw APIException(-24, "Specified name/address is not an asset.");
            }

            /* Check for specific field names. */
            if(params.find("fieldname") != params.end())
            {
                /* First get the fieldname from the response */
                std::string strFieldname =  params["fieldname"].get<std::string>();

                /* Get temp JSON. */
                json::json temp = ObjectToJSON(params, object, hashRegister);
                for(auto it = temp.begin(); it != temp.end(); ++it)
                {
                    /* If this key is not the one that was requested then erase it */
                    if(it.key() == strFieldname)
                        ret[it.key()] = it.value();
                }
            }
            else
                ret = ObjectToJSON(params, object, hashRegister);

            return ret;
        }
    }
}
