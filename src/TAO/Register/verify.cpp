/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/verify.h>
#include <TAO/Register/include/enum.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /* Verify the pre-states of a register to current network state. */
        bool Verify(const TAO::Operation::Contract& contract,
                    std::map<uint256_t, TAO::Register::State>& mapStates, const uint8_t nFlags)
        {
            /* Reset the contract streams. */
            contract.Reset();

            /* Make sure no exceptions are thrown. */
            try
            {
                /* Get the contract OP. */
                uint8_t OP = 0;
                contract >> OP;

                /* Check the current opcode. */
                switch(OP)
                {
                    /* Check pre-state to database. */
                    case TAO::Operation::OP::WRITE:
                    {
                        /* Get the Address of the Register. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::WRITE: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        State state;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            state = mapStates[hashAddress];

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::WRITE: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != state)
                            return debug::error(FUNCTION, "OP::WRITE: pre-state verification failed");

                        /* Check contract account */
                        if(contract.Caller() != prestate.hashOwner)
                            return debug::error(FUNCTION, "OP::WRITE: not authorized ", contract.Caller().SubString());

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = state;

                        return true;
                    }


                    /* Check pre-state to database. */
                    case TAO::Operation::OP::APPEND:
                    {
                        /* Get the Address of the Register. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::APPEND: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        State state;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            state = mapStates[hashAddress];

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::APPEND: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != state)
                            return debug::error(FUNCTION, "OP::APPEND: pre-state verification failed");

                        /* Check contract account */
                        if(contract.Caller() != prestate.hashOwner)
                            return debug::error(FUNCTION, "OP::APPEND: not authorized ", contract.Caller().SubString());

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = state;

                        return true;
                    }


                    /* Create does not have a pre-state. */
                    case TAO::Operation::OP::CREATE:
                    {
                        break;
                    }


                    /* Transfer ownership of a register to another signature chain. */
                    case TAO::Operation::OP::TRANSFER:
                    {
                        /* Get the Address of the Register. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::TRANSFER: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        State state;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            state = mapStates[hashAddress];

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::TRANSFER: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != state)
                            return debug::error(FUNCTION, "OP::TRANSFER: pre-state verification failed");

                        /* Check contract account */
                        if(contract.Caller() != prestate.hashOwner)
                            return debug::error(FUNCTION, "OP::TRANSFER: not authorized ", contract.Caller().SubString());

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = state;

                        break;
                    }


                    /* Transfer ownership of a register to another signature chain. */
                    case TAO::Operation::OP::CLAIM:
                    {
                        /* Seek to address location. */
                        contract.Seek(69);

                        /* Get the address to update. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::CLAIM: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        State state;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            state = mapStates[hashAddress];

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::CLAIM: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != state)
                            return debug::error(FUNCTION, "OP::CLAIM: pre-state verification failed");

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = state;

                        break;
                    }


                    /* Coinbase operation. Creates an account if none exists. */
                    case TAO::Operation::OP::COINBASE:
                    {
                        /* Seek through coinbase data. */
                        contract.Seek(49);

                        break;
                    }


                    /* Coinstake operation. Requires an account. */
                    case TAO::Operation::OP::TRUST:
                    {
                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::TRUST: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        State state;
                        if(mapStates.count(contract.Caller())) //TODO: check here for duplicate register address (could have a collision between genesis and address)
                            state = mapStates[contract.Caller()];

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadTrust(contract.Caller(), state))
                            return debug::error(FUNCTION, "OP::TRUST: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != state)
                            return debug::error(FUNCTION, "OP::TRUST: pre-state verification failed");

                        /* Check contract account */
                        if(contract.Caller() != prestate.hashOwner)
                            return debug::error(FUNCTION, "OP::TRUST: not authorized ", contract.Caller().SubString());

                        /* Write the state to memory map. */
                        mapStates[contract.Caller()] = state;

                        break;
                    }


                    /* Coinstake operation. Requires an account. */
                    case TAO::Operation::OP::GENESIS:
                    {
                        /* Get last trust block. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::GENESIS: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        State state;
                        if(mapStates.count(contract.Caller()))
                            state = mapStates[contract.Caller()];

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::GENESIS: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != state)
                            return debug::error(FUNCTION, "OP::GENESIS: pre-state verification failed");

                        /* Check contract account */
                        if(contract.Caller() != prestate.hashOwner)
                            return debug::error(FUNCTION, "OP::GENESIS: not authorized ", contract.Caller().SubString());

                        /* Write the state to memory map. */
                        mapStates[contract.Caller()] = state;

                        break;
                    }


                    /* Move funds from trust account balance to stake. */
                    case TAO::Operation::OP::STAKE:
                    {
                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::STAKE: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        State state;
                        if(mapStates.count(contract.Caller())) //TODO: check here for duplicate register address (could have a collision between genesis and address)
                            state = mapStates[contract.Caller()];

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadTrust(contract.Caller(), state))
                            return debug::error(FUNCTION, "OP::STAKE: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != state)
                            return debug::error(FUNCTION, "OP::STAKE: pre-state verification failed");

                        /* Check contract account */
                        if(contract.Caller() != prestate.hashOwner)
                            return debug::error(FUNCTION, "OP::STAKE: not authorized ", contract.Caller().SubString());

                        /* Write the state to memory map. */
                        mapStates[contract.Caller()] = state;

                        break;
                    }


                    /* Move funds from trust account stake to balance. */
                    case TAO::Operation::OP::UNSTAKE:
                    {
                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::UNSTAKE: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        State state;
                        if(mapStates.count(contract.Caller())) //TODO: check here for duplicate register address (could have a collision between genesis and address)
                            state = mapStates[contract.Caller()];

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadTrust(contract.Caller(), state))
                            return debug::error(FUNCTION, "OP::UNSTAKE: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != state)
                            return debug::error(FUNCTION, "OP::UNSTAKE: pre-state verification failed");

                        /* Check contract account */
                        if(contract.Caller() != prestate.hashOwner)
                            return debug::error(FUNCTION, "OP::UNSTAKE: not authorized ", contract.Caller().SubString());

                        /* Write the state to memory map. */
                        mapStates[contract.Caller()] = state;

                        break;
                    }


                    /* Debit tokens from an account you own. */
                    case TAO::Operation::OP::DEBIT:
                    {
                        /* Get last trust block. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::DEBIT: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        State state;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            state = mapStates[hashAddress];

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::DEBIT: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != state)
                            return debug::error(FUNCTION, "OP::DEBIT: pre-state verification failed");

                        /* Check contract account */
                        if(contract.Caller() != prestate.hashOwner)
                            return debug::error(FUNCTION, "OP::DEBIT: not authorized ", contract.Caller().SubString());

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = state;

                        break;
                    }


                    /* Credit tokens to an account you own. */
                    case TAO::Operation::OP::CREDIT:
                    {
                        /* Extract the transaction from contract. */
                        uint512_t hashTx = 0;
                        contract >> hashTx;

                        /* Extract the contract-id. */
                        uint32_t nContract = 0;
                        contract >> nContract;

                        /* Get the transfer address. */
                        uint256_t hashAddress = 0;
                        contract >> hashAddress;

                        /* Verify the first register code. */
                        uint8_t nState = 0;
                        contract >>= nState;

                        /* Check the state is prestate. */
                        if(nState != STATES::PRESTATE)
                            return debug::error(FUNCTION, "OP::CREDIT: register state not in pre-state");

                        /* Verify the register's prestate. */
                        State prestate;
                        contract >>= prestate;

                        /* Check temporary memory states first. */
                        State state;
                        if(mapStates.find(hashAddress) != mapStates.end())
                            state = mapStates[hashAddress];

                        /* Read the register from database. */
                        else if(!LLD::Register->ReadState(hashAddress, state, nFlags))
                            return debug::error(FUNCTION, "OP::CREDIT: failed to read pre-state");

                        /* Check that the checksums match. */
                        if(prestate != state)
                            return debug::error(FUNCTION, "OP::CREDIT: pre-state verification failed");

                        /* Check contract account */
                        if(contract.Caller() != prestate.hashOwner)
                            return debug::error(FUNCTION, "OP::CREDIT: not authorized ", contract.Caller().SubString());

                        /* Write the state to memory map. */
                        mapStates[hashAddress] = state;

                        break;
                    }
                }
            }
            catch(const std::exception& e)
            {
                return debug::error(FUNCTION, "exception encountered ", e.what());
            }

            /* If nothing failed, return true for evaluation. */
            return true;
        }
    }
}
