/*__________________________________________________________________________________________

        (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

        (c) Copyright The Nexus Developers 2014 - 2021

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/constants.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/validate.h>

#include <TAO/Operation/types/condition.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/constants.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

         /* Commit validation proofs. **/
        bool Validate::Commit(const Contract& contract, const uint512_t& hashTx, const uint32_t nContract, const uint256_t& hashCaller, const uint8_t nFlags)
        {
            /* Check for disk write to add indexes. */
            if(nFlags == TAO::Ledger::FLAGS::BLOCK && config::GetBoolArg("-indexproofs"))
            {
                /* Write a ledger level index if validated. */
                if(!LLD::Ledger->IndexContract(hashTx, nContract, contract.Hash()))
                    return debug::error(FUNCTION, "OP::VALIDATE: failed to write validation contract index");
            }

            /* Check that contract hasn't already been completed. */
            if(LLD::Contract->HasContract(std::make_pair(hashTx, nContract), nFlags))
                return debug::error(FUNCTION, "OP::VALIDATE: cannot validate when already fulfilled");

            /* Write the contract caller. */
            if(!LLD::Contract->WriteContract(std::make_pair(hashTx, nContract), hashCaller, nFlags))
                return debug::error(FUNCTION, "OP::VALIDATE: failed to write contract caller to disk");

            return true;
        }


        /* Verify validation rules and caller. */
        bool Validate::Verify(const Contract& contract, const Contract& condition, uint64_t &nCost)
        {
            /* Reset the condition. */
            condition.Reset();

            /* Check for validation conditions. */
            if(condition.Empty(Contract::CONDITIONS))
                return debug::error(FUNCTION, "OP::VALIDATE: cannot validate with no conditions");

            /* Get the validation type. */
            uint8_t nOP = 0;
            condition >> nOP;

            /* Check for condition. */
            if(nOP != OP::CONDITION)
                return debug::error(FUNCTION, "OP::VALIDATE: incorrect condition op");

            /* Switch based on type. */
            condition >> nOP;
            switch(nOP)
            {
                /* Handle for transfer validation. */
                case OP::TRANSFER:
                {
                    /* Get the address. */
                    uint256_t hashAddress = 0;
                    condition >> hashAddress;

                    /* Get the transfer. */
                    uint256_t hashTransfer = 0;
                    condition >> hashTransfer;

                    /* Check transfer type. */
                    uint8_t nType = 0;
                    condition >> nType;

                    /* Check for claimable. */
                    if(nType != TRANSFER::CLAIM)
                        return debug::error(FUNCTION, "OP::VALIDATE: cannot validate on forced transfer");

                    /* Check that transfer is wildcard for contract version 1 */
                    if(contract.Version() == 1 && hashTransfer != TAO::Register::WILDCARD_ADDRESS)
                        return debug::error(FUNCTION, "OP::VALIDATE: cannot validate without wildcard");

                    /* Check for condition. */
                    if(!condition.End())
                        return debug::error(FUNCTION, "OP::VALIDATE: can't validate with extra conditions");

                    break;
                }

                /* Handle for debit validation. */
                case OP::DEBIT:
                {
                    /* Get the address. */
                    uint256_t hashFrom = 0;
                    condition >> hashFrom;

                    /* Get the transfer. */
                    uint256_t hashTo = 0;
                    condition >> hashTo;

                    /* Get the amount. */
                    uint64_t nAmount = 0;
                    condition >> nAmount;

                    /* Get the reference */
                    uint64_t nReference = 0;
                    condition >> nReference;

                    /* Check that debit is wildcard for contract version 1 */
                    if(contract.Version() == 1 && hashTo != TAO::Register::WILDCARD_ADDRESS)
                        return debug::error(FUNCTION, "OP::VALIDATE: cannot validate without wildcard");

                    /* Check for condition. */
                    if(!condition.End())
                        return debug::error(FUNCTION, "OP::VALIDATE: can't validate with extra conditions");

                    break;
                }

                /* Default is to fail. */
                default:
                    return debug::error(FUNCTION, "invalid validate PRIMITIVE op");
            }

            /* Build the validation script for execution. */
            Condition conditions = Condition(condition, contract);
            if(!conditions.Execute())
                return debug::error(FUNCTION, "OP::VALIDATE: conditions not satisfied");

            /* Assess the fees for the computation limits. */
            if(conditions.nCost > CONDITION_LIMIT_FREE)
                nCost += conditions.nCost - CONDITION_LIMIT_FREE;

            return true;
        }
    }
}
