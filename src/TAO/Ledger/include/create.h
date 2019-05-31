/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_CREATE_H
#define NEXUS_TAO_LEDGER_INCLUDE_CREATE_H

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/allocators.h>

#include <condition_variable>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** Condition variable for private blocks. */
        extern std::condition_variable PRIVATE_CONDITION;


        /** CreateTransaction
         *
         *  Create a new transaction object from signature chain.
         *
         *  @param[in] user The signature chain to generate this tx
         *  @param[in] pin The pin number to generate with.
         *  @param[out] tx The traansaction object being created
         *
         **/
        bool CreateTransaction(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& pin, TAO::Ledger::Transaction& tx);


        /** AddTransactions
         *
         *  Gets a list of transactions from memory pool for current block.
         *
         *  @param[out] block The block to add the transactions to.
         *
         **/
        void AddTransactions(TAO::Ledger::TritiumBlock& block);


        /** CreateBlock
         *
         *  Create a new block object from the chain.
         *
         *  For Proof of Stake (channel 0), this method does not populate the producer coinstake operations, sign the producer, or add transactions 
         *  to the block. The stake minter must perform these steps so it can account for differences between Trust or Genesis stake.
         *
         *  @param[in] user The signature chain to generate this block
         *  @param[in] pin The pin number to generate with.
         *  @param[in] nChannel The channel to create block for.
         *  @param[out] block The block object being created.
         *  @param[in] nExtraNonce An extra nonce to use for double iterating.
         *
         **/
        bool CreateBlock(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& pin, uint32_t nChannel, TAO::Ledger::TritiumBlock& block, uint64_t nExtraNonce = 0);


        /** CreateGenesis
         *
         *  Creates the genesis block
         *
         **/
        bool CreateGenesis();


        /** ThreadGenerator
         *
         *  Handles the creation of a private block chain.
         *  Only executes when a transaction is broadcast.
         *
         **/
        void ThreadGenerator();
    }
}

#endif
