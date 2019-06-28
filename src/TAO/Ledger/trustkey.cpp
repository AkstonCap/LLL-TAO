/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <cmath>

#include <Legacy/types/legacy.h>
#include <Legacy/types/transaction.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/trust.h>
#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/types/trustkey.h>

#include <Util/include/debug.h>
#include <Util/include/runtime.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {


        /* Set the Trust Key data to Null (uninitialized) values. */
        void TrustKey::SetNull()
        {
            vchPubKey.clear();

            nVersion         = 1;
            hashGenesisBlock = 0;
            hashGenesisTx    = 0;
            nGenesisTime     = 0;
            hashLastBlock    = 0;
            nLastBlockTime   = 0;
            nStakeRate       = 0;

            return;
        }

        /* Retrieve how old the Trust Key is at a given point in time. */
        uint64_t TrustKey::Age(const uint64_t nTime) const
        {
            if (nTime < nGenesisTime)
                return 0;

            return (uint64_t)(nTime - nGenesisTime);
        }


        /* Retrieve the time since this Trust Key last generated a Proof of Stake block. */
        uint64_t TrustKey::BlockAge(const TAO::Ledger::BlockState& state) const
        {
            /* Check the index for the last block. */
            TAO::Ledger::BlockState stateLast = state;
            if(!GetLastTrust(*this, stateLast))
                return debug::error(FUNCTION, "last trust block not found");

            /* Block Age is Time to Previous Block's Time. */
            return state.GetBlockTime() - stateLast.GetBlockTime();
        }


        /* Determine if a key is expired at a given point in time. */
        bool TrustKey::Expired(const TAO::Ledger::BlockState& state) const
        {
            if (BlockAge(state) > (config::fTestNet ? TRUST_KEY_TIMESPAN_TESTNET : TRUST_KEY_TIMESPAN))
                return true;

            return false;
        }


        /* Check the Genesis Transaction of this Trust Key. */
        bool TrustKey::CheckGenesis(const TAO::Ledger::BlockState& block) const
        {
            /* Invalid if Null. */
            if(IsNull())
                return false;

            /* Trust Keys must be created from only Proof of Stake Blocks. */
            if(!block.IsProofOfStake())
                return debug::error(FUNCTION, "genesis has to be proof of stake");

            /* Trust Key Timestamp must be the same as Genesis Key Block Timestamp. */
            if(nGenesisTime != block.GetBlockTime())
                return debug::error(FUNCTION, "genesis time mismatch");

            /* Genesis Key Transaction must match Trust Key Genesis Hash. */
            if(block.vtx[0].second != hashGenesisTx)
                return debug::error(FUNCTION, "genesis coinstake hash mismatch");

            /* Check the genesis block hash. */
            if(block.GetHash() != hashGenesisBlock)
                return debug::error(FUNCTION, "genesis hash mismatch");

            return true;
        }


        /* Retrieves the legacy staking rate for the trust score defined within a given Proof of Stake legacy block. */
        double TrustKey::StakeRate(const Legacy::LegacyBlock& block, const uint32_t nTime) const
        {
            return StakeRate(block.vtx[0], block.nVersion, nTime);
        }


        /* Retrieves the legacy staking rate for the trust score defined within a given Proof of Stake block state. */
        double TrustKey::StakeRate(const TAO::Ledger::BlockState& block, const uint32_t nTime) const
        {
            /* Retrieve the coinstake transaction for the block state. */
            Legacy::Transaction coinstakeTx;
            if(!LLD::Legacy->ReadTx(block.vtx[0].second, coinstakeTx))
                return debug::error(FUNCTION, "Failed to read coinstake from legacy DB");

            return StakeRate(coinstakeTx, block.nVersion, nTime);
        }


        /* Retrieves the staking rate (ie, minting rate or interest rate) of the trust key for a given PoS block state. */
        double TrustKey::StakeRate(const Legacy::Transaction& coinstakeTx, const uint32_t nVersion, const uint32_t nTime) const
        {
            static const double LOG10 = log(10); // Constant for use in calculations

            /* Use appropriate settings for Testnet or Mainnet */
            static const uint32_t nMaxTrustScore = config::fTestNet ? TAO::Ledger::TRUST_SCORE_MAX_TESTNET : TAO::Ledger::TRUST_SCORE_MAX;

            if(!coinstakeTx.IsCoinStake())
                return debug::error(FUNCTION, "Invalid coinstake transaction for block state");

            /* Genesis interest rate is 0.5% */
            if(coinstakeTx.IsGenesis())
                return 0.005;

            if(nVersion < 4)
                return debug::error(FUNCTION, "Invalid Proof of Stake block version ", nVersion);

            uint32_t nTrustScore = 0;

            /* Block version 4 trust score is age of trust key */
            if(nVersion == 4)
                nTrustScore = (nTime - nGenesisTime);

            /* Block version 5+ is the trust score of the key encoded within the coinstakeTx */
            else if(!coinstakeTx.TrustScore(nTrustScore))
                return 0.0; //this will trigger an interest rate failure

            double nTrustScoreRatio = (double)nTrustScore / (double)nMaxTrustScore;
            return std::min(0.03, (0.025 * log((9.0 * nTrustScoreRatio) + 1.0) / LOG10) + 0.005);
        }


        /* Generate a string representation of this Trust Key. */
        std::string TrustKey::ToString()
        {
            uint576_t cKey;
            cKey.SetBytes(vchPubKey);

            return debug::safe_printstr(
                "hash=", GetHash().ToString(), ", ",
                "key=", cKey.ToString(), ", ",
                "genesis=", hashGenesisBlock.ToString(), ", ",
                "tx=", hashGenesisTx.ToString(), ", ",
                "time=", nGenesisTime, ", ",
                "age=", Age(runtime::unifiedtimestamp()), ", "
            );
        }


        /* Print a string representation of this Trust Key to the debug log. */
        void TrustKey::Print()
        {
            debug::log(0,
                "TrustKey(", ToString(), ")"
            );
        }
    }
}
