/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/types/merkle.h>
#include <TAO/Ledger/types/state.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* The default constructor. */
        MerkleTx::MerkleTx()
        : Transaction   ( )
        , hashBlock     (0)
        , vMerkleBranch ( )
        , nIndex        (0)
        {
        }


        /* Copy constructor. */
        MerkleTx::MerkleTx(const MerkleTx& tx)
        : Transaction   (tx)
        , hashBlock     (tx.hashBlock)
        , vMerkleBranch (tx.vMerkleBranch)
        , nIndex        (tx.nIndex)
        {
        }


        /* Move constructor. */
        MerkleTx::MerkleTx(MerkleTx&& tx) noexcept
        : Transaction   (std::move(tx))
        , hashBlock     (std::move(tx.hashBlock))
        , vMerkleBranch (std::move(tx.vMerkleBranch))
        , nIndex        (std::move(tx.nIndex))
        {
        }


        /* Copy constructor. */
        MerkleTx::MerkleTx(const Transaction& tx)
        : Transaction   (tx)
        , hashBlock     (0)
        , vMerkleBranch ( )
        , nIndex        (0)
        {
        }


        /* Move constructor. */
        MerkleTx::MerkleTx(Transaction&& tx) noexcept
        : Transaction   (std::move(tx))
        , hashBlock     (0)
        , vMerkleBranch ( )
        , nIndex        (0)
        {
        }


        /* Copy assignment. */
        MerkleTx& MerkleTx::operator=(const MerkleTx& tx)
        {
            vContracts    = tx.vContracts;
            nVersion      = tx.nVersion;
            nSequence     = tx.nSequence;
            nTimestamp    = tx.nTimestamp;
            hashNext      = tx.hashNext;
            hashRecovery  = tx.hashRecovery;
            hashGenesis   = tx.hashGenesis;
            hashPrevTx    = tx.hashPrevTx;
            nKeyType      = tx.nKeyType;
            nNextType     = tx.nNextType;
            vchPubKey     = tx.vchPubKey;
            vchSig        = tx.vchSig;

            hashBlock     = tx.hashBlock;
            vMerkleBranch = tx.vMerkleBranch;
            nIndex        = tx.nIndex;

            return *this;
        }


        /* Move assignment. */
        MerkleTx& MerkleTx::operator=(MerkleTx&& tx) noexcept
        {
            vContracts    = std::move(tx.vContracts);
            nVersion      = std::move(tx.nVersion);
            nSequence     = std::move(tx.nSequence);
            nTimestamp    = std::move(tx.nTimestamp);
            hashNext      = std::move(tx.hashNext);
            hashRecovery  = std::move(tx.hashRecovery);
            hashGenesis   = std::move(tx.hashGenesis);
            hashPrevTx    = std::move(tx.hashPrevTx);
            nKeyType      = std::move(tx.nKeyType);
            nNextType     = std::move(tx.nNextType);
            vchPubKey     = std::move(tx.vchPubKey);
            vchSig        = std::move(tx.vchSig);

            hashBlock     = std::move(tx.hashBlock);
            vMerkleBranch = std::move(tx.vMerkleBranch);
            nIndex        = std::move(tx.nIndex);

            return *this;
        }


        /* Default Destructor */
        MerkleTx::~MerkleTx()
        {
        }


        /* Checks if this transaction has a valid merkle path.*/
        bool MerkleTx::Check(const uint512_t& hashMerkleRoot) const
        {
            /* Generate merkle root from merkle branch. */
            uint512_t hashMerkleCheck = TAO::Ledger::BlockState::CheckMerkleBranch(GetHash(), vMerkleBranch, 3);

            return hashMerkleRoot == hashMerkleCheck;
        }


        /* Builds a merkle branch from block state. */
        bool MerkleTx::BuildMerkleBranch(const BlockState& state)
        {
            /* Cache this txid. */
            uint512_t hash = GetHash();

            /* Find the index of this transaction. */
            for(nIndex = 0; nIndex < state.vtx.size(); ++nIndex)
                if(state.vtx[nIndex].second == hash)
                    break;

            /* Check for valid index. */
            if(nIndex == state.vtx.size())
                return debug::error(FUNCTION, "transaction not found");

            /* Build merkle branch. */
            vMerkleBranch = state.GetMerkleBranch(state.vtx, nIndex);

            /* NOTE: extra expensive check for testing, consider removing in production */
            if(state.hashMerkleRoot != Block::CheckMerkleBranch(hash, vMerkleBranch, nIndex))
                return debug::error(FUNCTION, "merkle root mismatch");

            return true;
        }
    }
}
