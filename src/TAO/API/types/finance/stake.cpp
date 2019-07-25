/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/API/types/finance.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/stake.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/names.h>
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
            SecureString strPIN = users->GetPin(params);

            /* Get the session to be used for this API call */
            uint64_t nSession = users->GetSession(params);

            /* Get the user account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* Check for amount parameter. */
            if(params.find("amount") == params.end())
                throw APIException(-46, "Missing amount.");

            /* Lock the signature chain. */
            LOCK(users->CREATE_MUTEX);

            /* Check that the account is unlocked for creating transactions */
            if(!users->CanTransact())
                throw APIException(-16, "Account has not been unlocked for transactions.");

            /* Get trust account. Any trust account that has completed Genesis will be indexed. */
            TAO::Register::Object trustAccount;

            if(LLD::Register->HasTrust(user->Genesis()))
            {
                /* Trust account is indexed */
                if(!LLD::Register->ReadTrust(user->Genesis(), trustAccount))
                   throw APIException(-75, "Unable to retrieve trust account.");

                if(!trustAccount.Parse())
                    throw APIException(-71, "Unable to parse trust account.");
            }
            else
            {
                throw APIException(-76, "Cannot set stake for trust account until after Genesis transaction");
            }

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-17, "Failed to create transaction.");

            uint64_t nBalancePrev = trustAccount.get<uint64_t>("balance");
            uint64_t nStakePrev = trustAccount.get<uint64_t>("stake");

            /* Get the amount to debit. */
            uint64_t nAmount = std::stod(params["amount"].get<std::string>()) * TAO::Ledger::NXS_COIN;
            if(nAmount > nStakePrev)
            {
                /* Adding to stake from balance */
                if((nAmount - nStakePrev) > nBalancePrev)
                    throw APIException(-77, "Insufficient trust account balance to add to stake");

                /* Set the transaction payload for stake operation */
                uint64_t nAddStake = nAmount - nStakePrev;

                tx[0] << uint8_t(TAO::Operation::OP::STAKE) << nAddStake;
            }
            else if(nAmount < nStakePrev)
            {
                /* Removing from stake to balance */
                uint64_t nRemoveStake = nStakePrev - nAmount;

                uint64_t nTrustPrev = trustAccount.get<uint64_t>("trust");
                uint64_t nTrustPenalty = TAO::Ledger::GetUnstakePenalty(nTrustPrev, nStakePrev, nAmount, user->Genesis());

                tx[0] << uint8_t(TAO::Operation::OP::UNSTAKE) << nRemoveStake << nTrustPenalty;
            }
            else
                throw APIException(-78, "Stake not changed");

            /* Add the fee */
            AddFee(tx);

            /* Execute the operations layer. */
            if(!tx.Build())
                throw APIException(-44, "Transaction failed to build");

            /* Sign the transaction. */
            if(!tx.Sign(users->GetKey(tx.nSequence, strPIN, nSession)))
                throw APIException(-31, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw APIException(-32, "Failed to accept");

            /* Build a JSON response object. */
            ret["txid"] = tx.GetHash().ToString();

            return ret;
        }
    }
}
