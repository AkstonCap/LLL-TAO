/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/state.h>
#include <TAO/Register/objects/account.h>

namespace TAO::Operation
{

    /* Authorizes funds from an account to an account */
    bool Debit(uint256_t hashFrom, uint256_t hashTo, uint64_t nAmount, uint256_t hashCaller, uint8_t nFlags, TAO::Register::Stream &ssRegister)
    {
        /* Read the register from the database. */
        TAO::Register::State state;

        /* Write pre-states. */
        if((nFlags & TAO::Register::FLAGS::PRESTATE))
        {
            if(!LLD::regDB->ReadState(hashFrom, state))
                return debug::error(FUNCTION "register address doesn't exist %s", __PRETTY_FUNCTION__, hashFrom.ToString().c_str());

            ssRegister << (uint8_t)TAO::Register::STATES::PRESTATE << state;
        }

        /* Get pre-states on write. */
        if(nFlags & TAO::Register::FLAGS::WRITE  || nFlags & TAO::Register::FLAGS::MEMPOOL)
        {
            /* Get the state byte. */
            uint8_t nState = 0; //RESERVED
            ssRegister >> nState;

            /* Check for the pre-state. */
            if(nState != TAO::Register::STATES::PRESTATE)
                return debug::error(FUNCTION "register script not in pre-state", __PRETTY_FUNCTION__);

            /* Get the pre-state. */
            ssRegister >> state;
        }

        /* Check ownership of register. */
        if(state.hashOwner != hashCaller)
            return debug::error(FUNCTION "%s caller not authorized to debit from register", __PRETTY_FUNCTION__, hashCaller.ToString().c_str());

        /* Skip all non account registers for now. */
        if(state.nType != TAO::Register::OBJECT::ACCOUNT)
            return debug::error(FUNCTION "%s is not an account object", __PRETTY_FUNCTION__, hashFrom.ToString().c_str());

        /* Get the account object from register. */
        TAO::Register::Account account;
        state >> account;

        /* Check the balance of the from account. */
        if(nAmount > account.nBalance)
            return debug::error(FUNCTION "%s doesn't have sufficient balance", __PRETTY_FUNCTION__, hashFrom.ToString().c_str());

        /* Change the state of account register. */
        account.nBalance -= nAmount;

        /* Clear the state of register. */
        state.ClearState();
        state << account;

        /* Check that the register is in a valid state. */
        if(!state.IsValid())
            return debug::error(FUNCTION "memory address %s is in invalid state", __PRETTY_FUNCTION__, hashFrom.ToString().c_str());

        /* Write post-state checksum. */
        if((nFlags & TAO::Register::FLAGS::POSTSTATE))
            ssRegister << (uint8_t)TAO::Register::STATES::POSTSTATE << state.GetHash();

        /* Verify the post-state checksum. */
        if(nFlags & TAO::Register::FLAGS::WRITE || nFlags & TAO::Register::FLAGS::MEMPOOL)
        {
            /* Get the state byte. */
            uint8_t nState = 0; //RESERVED
            ssRegister >> nState;

            /* Check for the pre-state. */
            if(nState != TAO::Register::STATES::POSTSTATE)
                return debug::error(FUNCTION "register script not in post-state", __PRETTY_FUNCTION__);

            /* Get the post state checksum. */
            uint64_t nChecksum;
            ssRegister >> nChecksum;

            /* Check for matching post states. */
            if(nChecksum != state.GetHash())
                return debug::error(FUNCTION "register script has invalid post-state", __PRETTY_FUNCTION__);

            /* Write the register to the database. */
            if((nFlags & TAO::Register::FLAGS::WRITE) && !LLD::regDB->WriteState(hashFrom, state))
                return debug::error(FUNCTION "failed to write new state", __PRETTY_FUNCTION__);
        }

        return true;
    }
}
