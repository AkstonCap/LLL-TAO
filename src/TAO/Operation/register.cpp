/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>

#include <TAO/Register/include/state.h>
#include <TAO/Register/include/enum.h>
#include <TAO/Register/objects/token.h>
#include <TAO/Register/objects/account.h>

namespace TAO::Operation
{

    /* Creates a new register if it doesn't exist. */
    bool Register(uint256_t hashAddress, uint8_t nType, std::vector<uint8_t> vchData, uint256_t hashCaller, uint8_t nFlags, TAO::Register::Stream &ssRegister)
    {
        /* Check that the register doesn't exist yet. */
        if(LLD::regDB->HasState(hashAddress))
            return debug::error(FUNCTION "cannot allocate register of same memory address %s", __PRETTY_FUNCTION__, hashAddress.ToString().c_str());

        /* Set the owner of this register. */
        TAO::Register::State state;
        state.nVersion  = 1;
        state.nType     = nType;
        state.hashOwner = hashCaller;

        /* Check register types specific rules. */
        switch(nType)
        {

            case TAO::Register::OBJECT::ACCOUNT:
            {
                /* Create an account object. */
                TAO::Register::Account acct;

                /* Setup a stream to deserialize the data. */
                DataStream ssData(vchData, SER_REGISTER, state.nVersion);
                ssData >> acct;

                /* Check that the size is correct. */
                if(vchData.size() != acct.GetSerializeSize(SER_REGISTER, state.nVersion))
                    return debug::error(FUNCTION "unexpected account register size %u", __PRETTY_FUNCTION__, vchData.size());

                /* Check the account version. */
                if(acct.nVersion != 1)
                    return debug::error(FUNCTION "unexpected account version %u", __PRETTY_FUNCTION__, acct.nVersion);

                /* Check the account balance. */
                if(acct.nBalance != 0)
                    return debug::error(FUNCTION "account can't be created with non-zero balance", __PRETTY_FUNCTION__, acct.nBalance);

                /* Check that token identifier hasn't been claimed. */
                if(acct.nIdentifier != 0 && !LLD::regDB->HasIdentifier(acct.nIdentifier, nFlags))
                    return debug::error(FUNCTION "account can't be created with no identifier %u", __PRETTY_FUNCTION__, acct.nIdentifier);

                break;
            }

            case TAO::Register::OBJECT::TOKEN:
            {
                /* Create an account object. */
                TAO::Register::Token token;

                /* Setup a stream to deserialize the data. */
                DataStream ssData(vchData, SER_REGISTER, state.nVersion);
                ssData >> token;

                /* Check that the size is correct. */
                if(vchData.size() != token.GetSerializeSize(SER_REGISTER, state.nVersion))
                    return debug::error(FUNCTION "unexpected token register size %u", __PRETTY_FUNCTION__, vchData.size());

                /* Check the account version. */
                if(token.nVersion != 1)
                    return debug::error(FUNCTION "unexpected token version %u", __PRETTY_FUNCTION__, token.nVersion);

                /* Check that token identifier hasn't been claimed. */
                if(token.nIdentifier == 0 || LLD::regDB->HasIdentifier(token.nIdentifier, nFlags))
                    return debug::error(FUNCTION "token can't be created with reserved identifier %u", __PRETTY_FUNCTION__, token.nIdentifier);

                /* Check that the current supply and max supply are the same. */
                if(token.nMaxSupply != token.nCurrentSupply)
                    return debug::error(FUNCTION "token current supply and max supply can't mismatch", __PRETTY_FUNCTION__);

                /* Write the new identifier to database. */
                if(!LLD::regDB->WriteIdentifier(token.nIdentifier, hashAddress, nFlags))
                    return debug::error(FUNCTION "failed to commit token register identifier to disk", __PRETTY_FUNCTION__);

                break;
            }
        }

        /* Set the state from binary data. */
        state.SetState(vchData);

        /* Check the state change is correct. */
        if(!state.IsValid())
            return debug::error(FUNCTION "memory address %s is in invalid state", __PRETTY_FUNCTION__, hashAddress.ToString().c_str());

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
            if((nFlags & TAO::Register::FLAGS::WRITE) && !LLD::regDB->WriteState(hashAddress, state))
                return debug::error(FUNCTION "failed to write new state", __PRETTY_FUNCTION__);
        }

        return true;
    }
}
