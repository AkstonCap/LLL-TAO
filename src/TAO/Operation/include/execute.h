/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_OPERATION_INCLUDE_EXECUTE_H
#define NEXUS_TAO_OPERATION_INCLUDE_EXECUTE_H

#include <LLC/types/uint1024.h>

#include <TAO/Operation/include/stream.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/operations.h>

#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/types/transaction.h>

#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        //TODO: stress test and try to break operations transactions. Detect if parameters are sent in wrong order giving deliberate failures

        /** Execute
         *
         *  Executes a given operation byte sequence.
         *
         *  @param[in] regDB The register database to execute on.
         *  @param[in] hashOwner The owner executing the register batch.
         *
         *  @return True if operations executed successfully, false otherwise.
         *
         **/
        inline bool Execute(TAO::Ledger::Transaction& tx, uint8_t nFlags)
        {
            /* Get the caller. */
            uint256_t hashOwner = tx.hashGenesis;

            /* Start the stream at the beginning. */
            tx.ssOperation.seek(0, STREAM::BEGIN);

            /* Start the register stream at the beginning. */
            tx.ssRegister.seek(0, STREAM::BEGIN);


            /* Make sure no exceptions are thrown. */
            try
            {

                /* Loop through the operations tx.ssOperation. */
                while(!tx.ssOperation.end())
                {
                    uint8_t OPERATION;
                    tx.ssOperation >> OPERATION;

                    /* Check the current opcode. */
                    switch(OPERATION)
                    {

                        /* Record a new state to the register. */
                        case OP::WRITE:
                        {
                            /* Get the Address of the Register. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* Deserialize the register from tx.ssOperation. */
                            std::vector<uint8_t> vchData;
                            tx.ssOperation >> vchData;

                            /* Execute the operation method. */
                            if(!Write(hashAddress, vchData, hashOwner, nFlags, tx))
                                return false;

                            break;
                        }


                        /* Append a new state to the register. */
                        case OP::APPEND:
                        {
                            /* Get the Address of the Register. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* Deserialize the register from tx.ssOperation. */
                            std::vector<uint8_t> vchData;
                            tx.ssOperation >> vchData;

                            /* Execute the operation method. */
                            if(!Append(hashAddress, vchData, hashOwner, nFlags, tx))
                                return false;

                            break;
                        }


                        /* Create a new register. */
                        case OP::REGISTER:
                        {
                            /* Extract the address from the tx.ssOperation. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* Extract the register type from tx.ssOperation. */
                            uint8_t nType;
                            tx.ssOperation >> nType;

                            /* Extract the register data from the tx.ssOperation. */
                            std::vector<uint8_t> vchData;
                            tx.ssOperation >> vchData;

                            /* Execute the operation method. */
                            if(!Register(hashAddress, nType, vchData, hashOwner, nFlags, tx))
                                return false;

                            break;
                        }


                        /* Transfer ownership of a register to another signature chain. */
                        case OP::TRANSFER:
                        {
                            /* Extract the address from the tx.ssOperation. */
                            uint256_t hashAddress;
                            tx.ssOperation >> hashAddress;

                            /* Read the register transfer recipient. */
                            uint256_t hashTransfer;
                            tx.ssOperation >> hashTransfer;

                            /* Execute the operation method. */
                            if(!Transfer(hashAddress, hashTransfer, hashOwner, nFlags, tx))
                                return false;

                            break;
                        }


                        /* Debit tokens from an account you own. */
                        case OP::DEBIT:
                        {
                            uint256_t hashAddress; //the register address debit is being sent from. Hard reject if this register isn't account id
                            tx.ssOperation >> hashAddress;

                            uint256_t hashTransfer;   //the register address debit is being sent to. Hard reject if this register isn't an account id
                            tx.ssOperation >> hashTransfer;

                            uint64_t  nAmount;  //the amount to be transfered
                            tx.ssOperation >> nAmount;

                            /* Execute the operation method. */
                            if(!Debit(hashAddress, hashTransfer, nAmount, hashOwner, nFlags, tx))
                                return false;

                            break;
                        }


                        /* Credit tokens to an account you own. */
                        case OP::CREDIT:
                        {
                            /* The transaction that this credit is claiming. */
                            uint512_t hashTx;
                            tx.ssOperation >> hashTx;

                            /* The proof this credit is using to make claims. */
                            uint256_t hashProof;
                            tx.ssOperation >> hashProof;

                            /* The account that is being credited. */
                            uint256_t hashAccount;
                            tx.ssOperation >> hashAccount;

                            /* The total to be credited. */
                            uint64_t  nCredit;
                            tx.ssOperation >> nCredit;

                            /* Execute the operation method. */
                            if(!Credit(hashTx, hashProof, hashAccount, nCredit, hashOwner, nFlags, tx))
                                return false;

                            break;
                        }


                        /* Coinbase operation. Creates an account if none exists. */
                        case OP::COINBASE:
                        {
                            /* Ensure that it as beginning of the tx.ssOperation. */
                            if(!tx.ssOperation.begin())
                                return debug::error(FUNCTION, "coinbase opeartion has to be first");

                            /* The total to be credited. */
                            uint64_t  nCredit;
                            tx.ssOperation >> nCredit;

                            /* Ensure that it as end of tx.ssOperation. TODO: coinbase should be followed by ambassador and developer scripts */
                            if(!tx.ssOperation.end())
                                return debug::error(FUNCTION, "coinbase can't have extra data");

                            break;
                        }


                        /* Coinstake operation. Requires an account. */
                        case OP::TRUST:
                        {
                            /* Ensure that it as beginning of the tx.ssOperation. */
                            if(!tx.ssOperation.begin())
                                return debug::error(FUNCTION, "trust opeartion has to be first");

                            /* The account that is being staked. */
                            uint256_t hashAccount;
                            tx.ssOperation >> hashAccount;

                            /* The previous trust block. */
                            uint1024_t hashLastTrust;
                            tx.ssOperation >> hashLastTrust;

                            /* Previous trust sequence number. */
                            uint32_t nSequence;
                            tx.ssOperation >> nSequence;

                            /* The trust calculated. */
                            uint64_t nTrust;
                            tx.ssOperation >> nTrust;

                            /* The total to be staked. */
                            uint64_t  nStake;
                            tx.ssOperation >> nStake;

                            /* Execute the operation method. */
                            if(!Trust(hashAccount, hashLastTrust, nSequence, nTrust, nStake, hashOwner, nFlags, tx))
                                return false;

                            /* Ensure that it as end of tx.ssOperation. TODO: coinbase should be followed by ambassador and developer scripts */
                            if(!tx.ssOperation.end())
                                return debug::error(FUNCTION, "trust can't have extra data");

                            break;
                        }


                        /* Authorize a transaction to happen with a temporal proof. */
                        case OP::AUTHORIZE:
                        {
                            /* The transaction that you are authorizing. */
                            uint512_t hashTx;
                            tx.ssOperation >> hashTx;

                            /* The proof you are using that you have rights. */
                            uint256_t hashProof;
                            tx.ssOperation >> hashProof;

                            /* Execute the operation method. */
                            if(!Authorize(hashTx, hashProof, hashOwner, nFlags, tx))
                                return false;

                            break;
                        }


                        case OP::VOTE:
                        {
                            //voting mechanism. OP_VOTE can be to any random number. Something that can be regarded as a vote for thie hashOWner
                            //consider how to index this from API to OP_READ the votes without having to parse too deep into the register layer
                            //OP_VOTE doesn't need to change states. IT could be a vote read only object register

                            break;
                        }

                        /*
                        case OP_EXCHANGE:
                        {
                            //exchange contracts validation register. hashProof in credits can be used as an exchange medium if OP_DEBIT is to
                            //an exchange object register which holds the token identifier and total in exchange for deposited amount.
                            //OP_DEBIT goes to exchange object and sets its state. another OP_DEBIT from the other token locks this contract in
                            //hash proof for OP_CREDIT on each side allows the OP_DEBIT from opposite token to be claimed
                        }
                        */

                        case OP::SIGNATURE:
                        {
                            //a transaction proof that designates the hashOwner or genesisID has signed published data
                            //>> vchData. to prove this data was signed by being published. This can be a hash if needed.

                            break;
                        }
                    }
                }
            }
            catch(std::runtime_error& e)
            {
                return debug::error(FUNCTION, "exception encountered ", e.what());
            }

            /* If nothing failed, return true for evaluation. */
            return true;
        }
    }
}

#endif
