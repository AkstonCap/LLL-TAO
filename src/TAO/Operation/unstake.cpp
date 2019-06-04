/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/unstake.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /*  Commit the final state to disk. */
        bool Unstake::Commit(const TAO::Register::State& state, const uint8_t nFlags)
        {
            return LLD::regDB->WriteTrust(state.hashOwner, state);
        }


        /* Move from stake to balance for trust account (unlock stake). */
        bool Unstake::Execute(TAO::Register::Object &trustAccount, const uint64_t nAmount, const uint64_t nTrustPenalty, const uint64_t nTimestamp)
        {
            /* Parse the account object register. */
            if(!trustAccount.Parse())
                return debug::error(FUNCTION, "Failed to parse account object register");

            /* Get account starting values */
            uint64_t nTrustPrev = trustAccount.get<uint64_t>("trust");
            uint64_t nBalancePrev = trustAccount.get<uint64_t>("balance");
            uint64_t nStakePrev = trustAccount.get<uint64_t>("stake");

            if(nAmount > nStakePrev)
                return debug::error(FUNCTION, "cannot unstake more than existing stake balance");

            /* Write the new trust to object register. */
            if(!trustAccount.Write("trust", std::max((nTrustPrev - nTrustPenalty), (uint64_t)0)))
                return debug::error(FUNCTION, "trust could not be written to object register");

            /* Write the new balance to object register. */
            if(!trustAccount.Write("balance", nBalancePrev + nAmount))
                return debug::error(FUNCTION, "balance could not be written to object register");

            /* Write the new stake to object register. */
            if(!trustAccount.Write("stake", nStakePrev - nAmount))
                return debug::error(FUNCTION, "stake could not be written to object register");

            /* Update the state register's timestamp. */
            trustAccount.nModified = nTimestamp;
            trustAccount.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!trustAccount.IsValid())
                return debug::error(FUNCTION, "trust account is in invalid state");

            return true;
        }


        /* Verify trust validation rules and caller. */
        bool Unstake::Verify(const Contract& contract)
        {
            /* Seek read position to first position. */
            contract.Reset();

            /* Get operation byte. */
            uint8_t OP = 0;
            contract >> OP;

            /* Check operation byte. */
            if(OP != OP::UNSTAKE)
                return debug::error(FUNCTION, "called with incorrect OP");

            /* Get the state byte. */
            uint8_t nState = 0; //RESERVED
            contract >>= nState;

            /* Check for the pre-state. */
            if(nState != TAO::Register::STATES::PRESTATE)
                return debug::error(FUNCTION, "register script not in pre-state");

            /* Get the pre-state. */
            TAO::Register::State state;
            contract >>= state;

            /* Check that pre-state is valid. */
            if(!state.IsValid())
                return debug::error(FUNCTION, "pre-state is in invalid state");

            /* Check ownership of register. */
            if(state.hashOwner != contract.Caller())
                return debug::error(FUNCTION, "caller not authorized ", contract.Caller().SubString());

            /* Seek read position to first position. */
            contract.Seek(1);

            return true;
        }
    }
}
