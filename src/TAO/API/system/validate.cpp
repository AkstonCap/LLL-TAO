/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/system/types/system.h>
#include <Util/include/debug.h>

#include <Legacy/types/script.h>
#include <Legacy/wallet/wallet.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/json.h>

#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Validates a register / legacy address */
        json::json System::Validate(const json::json& params, bool fHelp)
        {
            json::json jsonRet;

            /* Check for address parameter. */
            if(params.find("address") == params.end() )
                throw APIException(-105, "Missing address");

            /* Extract the address, which will either be a legacy address or a sig chain account address */
            std::string strAddress = params["address"].get<std::string>();

            /* Legacy address */
            Legacy::NexusAddress address(strAddress);

            /* The decoded register address */
            TAO::Register::Address hashAddress;

            /* Decode the address string */
            hashAddress.SetBase58(strAddress);

            /* Populate address into response */
            jsonRet["address"] = strAddress;

            /* handle recipient being a register address */
            if(hashAddress.IsValid() )
            {
                /* Check to see if this is a legacy address */
                if(hashAddress.IsLegacy())
                {
                    /* Set the valid flag in the response */
                    jsonRet["is_valid"] = true;

                    /* Set type in response */
                    jsonRet["type"] = "LEGACY";

                    #ifndef NO_WALLET
                    if(Legacy::Wallet::GetInstance().HaveKey(address) || Legacy::Wallet::GetInstance().HaveScript(address.GetHash256()))
                        jsonRet["is_mine"] = true;
                    else
                        jsonRet["is_mine"] = false;
                    #endif
                }
                else
                {
                    /* Get the state. We only consider an address valid if the state exists in the register database*/
                    TAO::Register::Object state;
                    if(LLD::Register->ReadState(hashAddress, state, TAO::Ledger::FLAGS::LOOKUP))
                    {
                        /* Set the valid flag in the response */
                        jsonRet["is_valid"] = true;

                        /* Set the register type */
                        jsonRet["type"]    = RegisterType(state.nType);

                        /* Check if address is owned by current user */
                        uint256_t hashGenesis = users->GetCallersGenesis(params);
                        if(hashGenesis != 0)
                        {
                            if(state.hashOwner == hashGenesis)
                                jsonRet["is_mine"] = true;
                            else
                                jsonRet["is_mine"] = false;
                        }

                        /* If it is an object register then parse it to add the object_type */
                        if(state.nType == TAO::Register::REGISTER::OBJECT)
                        {
                            /* parse object so that the data fields can be accessed */
                            if(state.Parse())
                                jsonRet["object_type"] = ObjectType(state.Standard());
                            else
                                jsonRet["is_valid"] = false;
                        }
                    }
                    else
                    {
                        /* Set the valid flag in the response */
                        jsonRet["is_valid"] = false;
                    }
                }
            }
            else
            {
                /* Set the valid flag in the response */
                jsonRet["is_valid"] = false;
            }

            return jsonRet;
        }

    }
}
