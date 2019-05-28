/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/credit.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Commit the final state to disk. */
        bool Credit::Commit(const TAO::Register::Object& account, const uint256_t& hashAddress,
            const uint256_t& hashProof, const uint512_t& hashTx, const uint32_t nContract, const uint8_t nFlags)
        {
            /* Check if this transfer is already claimed. */
            if(LLD::legDB->HasProof(hashProof, std::make_pair(hashTx, nContract), nFlags))
                return debug::error(FUNCTION, "credit is already claimed");

            /* Write the claimed proof. */
            if(!LLD::legDB->WriteProof(hashProof, std::make_pair(hashTx, nContract), nFlags))
                return debug::error(FUNCTION, "failed to write credit proof");

            /* Write the new register's state. */
            return LLD::regDB->WriteState(hashAddress, account, nFlags);
        }


        /* Commits funds from an account to an account */
        bool Credit::Execute(TAO::Register::Object &account, const uint64_t nAmount, const uint64_t nTimestamp)
        {
            /* Parse the account object register. */
            if(!account.Parse())
                return debug::error(FUNCTION, "failed to parse account object register");

            /* Check that we are crediting to an account object register. */
            if(account.Base() != TAO::Register::OBJECTS::ACCOUNT)
                return debug::error(FUNCTION, "cannot credit to a non-account base object");

            /* Write the new balance to object register. */
            if(!account.Write("balance", account.get<uint64_t>("balance") + nAmount))
                return debug::error(FUNCTION, "balance could not be written to object register");

            /* Update the state register's timestamp. */
            account.nModified = nTimestamp;
            account.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!account.IsValid())
                return debug::error(FUNCTION, "memory address is in invalid state");

            return true;
        }


        /* Verify claim validation rules and caller. */
        bool Credit::Verify(const Contract& debit, const Contract& credit, uint64_t &nAmount)
        {
            /* Extract current contract. */
            credit.Reset();

            /* Get operation byte. */
            uint8_t OP = 0;
            credit >> OP;

            /* Check operation byte. */
            if(OP != OP::CREDIT)
                return debug::error(FUNCTION, "called with incorrect OP");

            /* Seek past transaction-id. */
            credit.Seek(65);

            /* Get the proof hash. */
            uint256_t hashProof = 0;
            credit >> hashProof;

            /* Get the byte from pre-state. */
            uint8_t nState = 0;
            credit >>= nState;

            /* Check for the pre-state. */
            if(nState != TAO::Register::STATES::PRESTATE)
                return debug::error(FUNCTION, "register credit not in pre-state");

            /* Read pre-states. */
            TAO::Register::Object account;
            credit >>= account;

            /* Check credit account */
            if(credit.hashCaller != account.hashOwner)
                return debug::error(FUNCTION, "no write permissions for caller ", credit.hashCaller.SubString());

            /* Parse the account. */
            if(!account.Parse())
                return debug::error(FUNCTION, "failed to parse account to");

            /* Seek claim read position to first. */
            debit.Reset();

            /* Get operation byte. */
            OP = 0;
            debit >> OP;

            /* Check that prev is coinbase. */
            if(OP == OP::COINBASE)
            {
                /* Check that the proof is the genesis. */
                if(hashProof != credit.hashCaller)
                    return debug::error(FUNCTION, "proof for coinbase needs to be genesis");

                /* Extract the coinbase public-id. */
                uint256_t hashPublic = 0;
                debit  >> hashPublic;

                /* Make sure the claimed account is the debited account. */
                if(hashPublic != credit.hashCaller)
                    return debug::error(FUNCTION, "cannot claim coinbase from different sigchain");

                /* Get the coinbase amount. */
                debit >> nAmount;

                /* Check the identifier. */
                if(account.get<uint256_t>("identifier") != 0)
                    return debug::error(FUNCTION, "credit disabled for coinbase of non-native token");

                return true;
            }

            /* Check that prev is debit. */
            else if(OP != OP::DEBIT)
                return debug::error(FUNCTION, "tx claim is not a debit");

            /* Get the hashFrom */
            uint256_t hashFrom = 0;
            debit  >> hashFrom;

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashFrom))
                return debug::error(FUNCTION, "cannot credit register with reserved address");

            /* Get the byte from pre-state. */
            nState = 0;
            debit >>= nState;

            /* Check for the pre-state. */
            if(nState != TAO::Register::STATES::PRESTATE)
                return debug::error(FUNCTION, "register debit not in pre-state");

            /* Read pre-states. */
            TAO::Register::Object accountFrom;
            debit >>= accountFrom;

            /* Parse the account. */
            if(!accountFrom.Parse())
                return debug::error(FUNCTION, "failed to parse account from");

            /* Check debit hashFrom is a base object account. */
            if(accountFrom.Base() != TAO::Register::OBJECTS::ACCOUNT)
                return debug::error(FUNCTION, "debit from must have a base account object");

            /* Check token identifiers. */
            if(accountFrom.get<uint256_t>("identifier") != account.get<uint256_t>("identifier"))
                return debug::error(FUNCTION, "credit can't be of different identifier");

            /* Get the hashTo. */
            uint256_t hashTo = 0;
            debit  >> hashTo;

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashTo))
                return debug::error(FUNCTION, "cannot credit register with reserved address");

            /* Get the byte from pre-state. */
            nState = 0;
            debit >= nState; //NOTE: this is deserializing from ssProof

            /* Check for the pre-state. */
            if(nState != TAO::Register::STATES::PRESTATE)
                return debug::error(FUNCTION, "register debit not in pre-state");

            /* Read pre-states. */
            TAO::Register::State stateTo;
            debit >= stateTo;

            /* Read the to account that credit is operating on. */
            uint256_t hashAccount;
            credit >> hashAccount;

            /* Handle a return to self. */
            if(hashFrom == hashAccount)
            {
                /* Get the debit amount. */
                debit >> nAmount;

                return true;
            }

            /* Credits specific to account objects. */
            if(stateTo.nType == TAO::Register::REGISTER::OBJECT)
            {
                /* Check the proof as being the caller. */
                if(hashProof != hashFrom)
                    return debug::error(FUNCTION, "proof must equal from for credit");



                /* Check that account to matches. */
                if(hashTo != hashAccount)
                    return debug::error(FUNCTION, "debit to account mismatch with credit to account");

                /* Get the debit amount. */
                uint64_t nDebit = 0;
                debit >> nDebit;

            }
            else if(!TAO::Register::Range(stateTo.nType))
                return debug::error(FUNCTION, "debit to is out of range");



                /* Get the state register of this register's owner. */
                TAO::Register::Object token;
                if(!LLD::regDB->ReadState(stateTo.hashOwner, tokenOwner, nFlags))
                    return debug::error(FUNCTION, "credit from raw object can't be without owner");

                /* Parse the owner object register. */
                if(!tokenOwner.Parse())
                    return debug::error(FUNCTION, "failed to parse owner object register");

                /* Check that owner is of a valid type. */
                if(tokenOwner.Standard() != TAO::Register::OBJECTS::TOKEN)
                    return debug::error(FUNCTION, "owner object is not a token");


            /* Get the byte from pre-state. */
            nState = 0;
            credit >= nState; //NOTE: this is deserializing from ssProof

            /* Check for the pre-state. */
            if(nState != TAO::Register::STATES::PRESTATE)
                return debug::error(FUNCTION, "register debit not in pre-state");

            /* Read pre-states. */
            TAO::Register::Object proof;
            credit >= proof;  //NOTE: this is deserializing from ssProof

            /* Parse object register. */
            if(!proof.Parse())
                return debug::error(FUNCTION, "failed to parse proof object register");

            /* Compare the last timestamp update to transaction timestamp. */
            if(proof.nModified > credit.nTimestamp)
                return debug::error(FUNCTION, "temporal proof is stale");

            /* Check that owner is of a valid type. */
            if(proof.Standard() != TAO::Register::OBJECTS::ACCOUNT)
                return debug::error(FUNCTION, "owner object is not a token");

            /* Check the ownership of proof register. */
            if(proof.hashOwner != credit.hashCaller)
                return debug::error(FUNCTION, "not authorized to use this temporal proof");

            /* Check that the token indetifier matches token identifier. */
            if(proof.get<uint256_t>("identifier") != token.get<uint256_t>("identifier"))
                return debug::error(FUNCTION, "account proof identifier not token identifier");

            /* Get the total amount of the debit. */
            uint64_t nDebit = 0;
            debit >> nDebit;

            /* Get the total tokens to be distributed. */
            uint64_t nPartial = (proof.get<uint64_t>("balance") * nDebit) / token.get<uint64_t>("supply");

            //NOTE: ISSUE here, temporal proofs can't be used if post timestamped. This prevents double spending,
            //but it doesn't prevent coins from getting locked if a temporal proof has been changed. Possible to
            //check tokens to ensure tokens aren't moved if a proof is able to be claimed.
            //find a way to unlock the unspent tokens possibly with validation regitser.
            //We might need to check back the history to find timestamp before proof to use previous state.
            //This will work for now, but is not production ready.

            /* Check that account from is a standard object. */
            if(accountFrom.Standard() != TAO::Register::OBJECTS::ACCOUNT)
                return debug::error(FUNCTION, "account from object register is non-standard type");

            /* Check that the debit to credit identifiers match. */
            if(account.get<uint256_t>("identifier") != accountFrom.get<uint256_t>("identifier"))
                return debug::error(FUNCTION, "credit can't be of different identifier");

            return true;
        }
    }
}
