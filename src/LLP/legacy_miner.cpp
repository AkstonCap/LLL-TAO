/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/


#include <LLP/types/legacy_miner.h>
#include <LLP/templates/events.h>
#include <LLP/templates/ddos.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/supply.h>

#include <Legacy/include/create.h>
#include <Legacy/types/legacy.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/reservekey.h>

#include <Util/include/convert.h>


namespace LLP
{

    /** Default Constructor **/
    LegacyMiner::LegacyMiner()
    : BaseMiner()
    , pMiningKey(nullptr)
    {
        pMiningKey = new Legacy::ReserveKey(&Legacy::Wallet::GetInstance());
    }


    /** Constructor **/
    LegacyMiner::LegacyMiner(const Socket& SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS)
    : BaseMiner(SOCKET_IN, DDOS_IN, isDDOS)
    , pMiningKey(nullptr)
    {
        pMiningKey = new Legacy::ReserveKey(&Legacy::Wallet::GetInstance());
    }


    /** Constructor **/
    LegacyMiner::LegacyMiner(DDOS_Filter* DDOS_IN, bool isDDOS)
    : BaseMiner(DDOS_IN, isDDOS)
    , pMiningKey(nullptr)
    {
        pMiningKey = new Legacy::ReserveKey(&Legacy::Wallet::GetInstance());
    }


    /** Default Destructor **/
    LegacyMiner::~LegacyMiner()
    {
        if(pMiningKey)
            delete pMiningKey;
    }


    /** new_block
     *
     *  Adds a new block to the map.
     *
     **/
     TAO::Ledger::Block *LegacyMiner::new_block()
     {
         /*  make a copy of the base block before making the hash  unique for this requst*/
         uint1024_t proof_hash;
         uint32_t s = static_cast<uint32_t>(mapBlocks.size());
         Legacy::LegacyBlock *pBlock = new Legacy::LegacyBlock();

         /* Update base block member variables */
         TAO::Ledger::Block *pBase = static_cast<TAO::Ledger::Block *>(pBlock);
         *pBase = *pBaseBlock;


         /* We need to make the block hash unique for each subsribed miner so that they are not
             duplicating their work.  To achieve this we take a copy of pBaseblock and then modify
             the scriptSig to be unique for each subscriber, before rebuilding the merkle tree.

             We need to drop into this for loop at least once to set the unique hash, but we will iterate
             indefinitely for the prime channel until the generated hash meets the min prime origins
             and is less than 1024 bits*/
         for(uint32_t i = s; ; ++i)
         {
             pBlock->vtx[0].vin[0].scriptSig = (Legacy::Script() <<  (uint64_t)((i+1) * 513513512151));

             /* Rebuild the merkle tree. */
             std::vector<uint512_t> vMerkleTree;
             for(const auto& tx : pBlock->vtx)
                 vMerkleTree.push_back(tx.GetHash());


             pBlock->hashMerkleRoot = pBlock->BuildMerkleTree(vMerkleTree);

             /* Update the time. */
             pBlock->UpdateTime();

             /* skip if not prime channel or version less than 5 */
             if(nChannel != 1 || pBlock->nVersion >= 5)
                 break;

             proof_hash = pBlock->ProofHash();

             /* exit loop when the block is above minimum prime origins and less than
                 1024-bit hashes */
             if(proof_hash > TAO::Ledger::bnPrimeMinOrigins.getuint1024()
             && !proof_hash.high_bits(0x80000000))
                 break;
         }

         debug::log(2, FUNCTION, "***** Mining LLP: Created new Block ",
             pBlock->hashMerkleRoot.ToString().substr(0, 20));

         /* Store the new block in the memory map of recent blocks being worked on. */
         mapBlocks[pBlock->hashMerkleRoot] = pBlock;

         /* Return a pointer to the heap memory */
         return pBlock;
     }


    /*  Creates the derived block for the base miner class. */
    bool LegacyMiner::create_base_block()
    {
        Legacy::LegacyBlock *pDerived = new Legacy::LegacyBlock();

        /*create a new base block */
        if(!Legacy::CreateLegacyBlock(*pMiningKey, pCoinbaseTx, nChannel, 1, *pDerived))
        {
            debug::error(FUNCTION, "Failed to create a new block.");
            delete pDerived;
            return false;
        }

        pBaseBlock = static_cast<TAO::Ledger::Block *>(pDerived);

        return true;
    }


    /** validates the block for the derived miner class. **/
    bool LegacyMiner::validate_block(const uint512_t &merkle_root)
    {
        Legacy::LegacyBlock *pBlock = dynamic_cast<Legacy::LegacyBlock *>(mapBlocks[merkle_root]);

        /* Check the Proof of Work for submitted block. */
        if(!Legacy::CheckWork(*pBlock, Legacy::Wallet::GetInstance()))
        {
            debug::log(2, "***** Mining LLP: Invalid Work for block ", merkle_root.ToString().substr(0, 20));

            return false;
        }

        return true;
    }


     /** validates the block for the derived miner class. **/
     bool LegacyMiner::sign_block(uint64_t nonce, const uint512_t &merkle_root)
     {
         //Legacy::LegacyBlock *pBlock = dynamic_cast<Legacy::LegacyBlock *>(mapBlocks[merkle_root]);

         //pBlock->nNonce = nonce;
         //pBlock->UpdateTime();
         //pBlock->print();

         //if(!Legacy::SignBlock(*pBlock, Legacy::Wallet::GetInstance()))
         //{
        //     debug::log(2, "***** Mining LLP: Unable to Sign block ", merkle_root.ToString().substr(0, 20));

             return false;
         //}

         /* Tell the wallet to keep this key */
         //pMiningKey->KeepKey();

        // return true;
     }


     /*  Determines if the mining wallet is unlocked. */
     bool LegacyMiner::is_locked()
     {
         return Legacy::Wallet::GetInstance().IsLocked();
     }


}
