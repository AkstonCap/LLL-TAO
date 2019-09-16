/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/finance.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/stake_change.h>
#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Set the stake amount for trust account (add/remove stake). */
        json::json Finance::Stake(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            uint256_t nSession = users->GetSession(params);

            /* Get the user account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* Check for amount parameter. */
            if(params.find("amount") == params.end())
                throw APIException(-46, "Missing amount");

            else if(std::stod(params["amount"].get<std::string>()) < 0)
                throw APIException(-204, "Cannot set stake to a negative amount");

            /* Lock the signature chain. */
            LOCK(users->CREATE_MUTEX);

            /* Check that the sig chain is mature after the last coinbase/coinstake transaction in the chain. */
            CheckMature(user->Genesis());

            /* Get trust account. Any trust account that has completed Genesis will be indexed. */
            TAO::Register::Object trustAccount;

            if(LLD::Register->HasTrust(user->Genesis()))
            {
                /* Trust account is indexed */
                if(!LLD::Register->ReadTrust(user->Genesis(), trustAccount))
                   throw APIException(-75, "Unable to retrieve trust account");

                if(!trustAccount.Parse())
                    throw APIException(-71, "Unable to parse trust account");
            }
            else
            {
                throw APIException(-76, "Cannot set stake for trust account until after Genesis transaction");
            }

            uint64_t nBalancePrev = trustAccount.get<uint64_t>("balance");
            uint64_t nStakePrev = trustAccount.get<uint64_t>("stake");

            /* Retrieve last stake tx hash for the user trust account to include in stake change request */
            uint512_t hashLast;
            if(!LLD::Ledger->ReadStake(user->Genesis(), hashLast))
                throw APIException(-205, "Unable to retrieve last stake");

            /* Get the requested amount to stake. */
            uint64_t nAmount = std::stod(params["amount"].get<std::string>()) * TAO::Ledger::NXS_COIN;

            /* Get the time to expiration (optional). */
            uint64_t nExpires = 0;
            if(params.find("expires") == params.end())
                nExpires = std::stoull(params["expires"].get<std::string>());

            uint64_t nTime = runtime::unifiedtimestamp();

            /* Convert time to expiration to an expiration timestamp (0 = does not expire) */
            if(nExpires > 0)
            {

                if((std::numeric_limits<uint64_t>::max() - nExpires) < nTime) //check for overflow
                    nExpires = std::numeric_limits<uint64_t>::max();

                else
                    nExpires = nExpires + nTime;
            }

            /* Only need to validate amount for add to stake. An amount between zero and current stake balance
             * is an unstake (remove from stake), and cannot set less than zero so cannot attempt to remove
             * more than current stake amount.
             */
            if(nAmount > nStakePrev)
            {
                /* Validate add to stake */
                if((nAmount - nStakePrev) > nBalancePrev)
                    throw APIException(-77, "Insufficient trust account balance to add to stake");
            }

            TAO::Ledger::StakeChange request;
            bool hasPrev = false;

            /* Retrieve any existing stake change request */
            if(LLD::Local->ReadStakeChange(user->Genesis(), request))
                hasPrev = true;

            if(hasPrev && request.nExpires != 0 && (request.nExpires < runtime::unifiedtimestamp()))
            {
                request.SetNull();
                hasPrev = false;

                if(!LLD::Local->EraseStakeChange(user->Genesis()))
                    throw APIException(-206, "Failed to erase expired stake change request");
            }

            /* Set up the stake change request and store for inclusion in next stake block found */
            if(nAmount == nStakePrev)
            {
                if(hasPrev)
                {
                    /* Setting amount to the current stake value (no change) removes the prior stake change request */
                    if(!LLD::Local->EraseStakeChange(user->Genesis()))
                        throw APIException(-207, "Failed to erase previous stake change request");
                }
                else
                    throw APIException(-78, "Stake not changed"); //Request to set stake to current value when no prior request
            }

            else
            {
                request.hashGenesis = user->Genesis();
                request.hashLast = hashLast;
                request.nAmount = nAmount;
                request.nTime = nTime;
                request.nExpires = nExpires;

                if(!LLD::Local->WriteStakeChange(user->Genesis(), request))
                        throw APIException(-208, "Failed to save stake change request");
            }

            /* Build a JSON response object. */
            ret["success"] = true;

            return ret;
        }
    }
}
