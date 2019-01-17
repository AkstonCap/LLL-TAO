/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/key.h>

#include <Legacy/types/legacy.h>
#include <Legacy/types/script.h>
#include <Legacy/include/ambassador.h>
#include <Legacy/include/constants.h>
#include <Legacy/include/evaluate.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/checkpoints.h>
#include <TAO/Ledger/include/chainstate.h>

#include <Util/include/args.h>
#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace Legacy
{

    /* Verify the Signature is Valid for Last 2 Coinbase Tx Outputs. */
    bool VerifyAddress(const std::vector<uint8_t> script, const std::vector<uint8_t> sig)
    {
        if(script.size() != 37)
            return debug::error(FUNCTION, "Script Size not 37 Bytes");

        for(int32_t i = 0; i < 37; i++)
            if(script[i] != sig[i])
                return false;

        return true;
    }

    /* Compare Two Vectors Element by Element. */
    bool VerifyAddressList(const std::vector<uint8_t> script, const std::vector<uint8_t> sigs[13])
    {
        for(int32_t i = 0; i < 13; i++)
            if(VerifyAddress(script, sigs[i]))
                return true;

        return false;
    }

    /* For debugging Purposes seeing block state data dump */
    std::string LegacyBlock::ToString() const
    {
        return debug::strprintf("Block("
            VALUE("hash")     " = %s "
            VALUE("nVersion") " = %u, "
            VALUE("hashPrevBlock") " = %s, "
            VALUE("hashMerkleRoot") " = %s, "
            VALUE("nChannel") " = %u, "
            VALUE("nHeight") " = %u, "
            VALUE("nBits") " = %u, "
            VALUE("nNonce") " = %" PRIu64 ", "
            VALUE("nTime") " = %u, "
            VALUE("vchBlockSig") " = %s, "
            VALUE("vtx.size()") " = %u)",
        GetHash().ToString().substr(0, 20).c_str(), nVersion, hashPrevBlock.ToString().substr(0, 20).c_str(), hashMerkleRoot.ToString().substr(0, 20).c_str(), nChannel, nHeight, nBits, nNonce, nTime, HexStr(vchBlockSig.begin(), vchBlockSig.end()).c_str(), vtx.size());
    }


    /* For debugging purposes, printing the block to stdout */
    void LegacyBlock::print() const
    {
        debug::log(0, ToString());
    }


    /* Checks if a block is valid if not connected to chain. */
    bool LegacyBlock::Check() const
    {
        /* Check the Size limits of the Current Block. */
        if (::GetSerializeSize(*this, SER_NETWORK, LLP::PROTOCOL_VERSION) > TAO::Ledger::MAX_BLOCK_SIZE)
            return debug::error(FUNCTION, "size ",
                ::GetSerializeSize(*this, SER_NETWORK, LLP::PROTOCOL_VERSION), " limits failed ", TAO::Ledger::MAX_BLOCK_SIZE);


        /* Make sure the Block was Created within Active Channel. */
        if (GetChannel() > 2)
            return debug::error(FUNCTION, "channel out of Range.");


        /* Check that the time was within range. */
        if (GetBlockTime() > runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT)
            return debug::error(FUNCTION, "block timestamp too far in the future");


        /* Do not allow blocks to be accepted above the current block version. */
        if(nVersion == 0 || nVersion > (config::fTestNet ?
            TAO::Ledger::TESTNET_BLOCK_CURRENT_VERSION : TAO::Ledger::NETWORK_BLOCK_CURRENT_VERSION))
            return debug::error(FUNCTION, "invalid block version");


        /* Only allow POS blocks in Version 4. */
        if(IsProofOfStake() && nVersion < 4)
            return debug::error(FUNCTION, "proof-of-stake rejected until version 4");


        /* Check the Proof of Work Claims. */
        if (IsProofOfWork() && !VerifyWork())
            return debug::error(FUNCTION, "invalid proof of work");


        /* Check the Network Launch Time-Lock. */
        if (nHeight > 0 && GetBlockTime() <=
            (config::fTestNet ? TAO::Ledger::NEXUS_TESTNET_TIMELOCK : TAO::Ledger::NEXUS_NETWORK_TIMELOCK))
            return debug::error(FUNCTION, "block created before network time-lock");


        /* Check the Current Channel Time-Lock. */
        if (nHeight > 0 && GetBlockTime() < (config::fTestNet ?
            TAO::Ledger::CHANNEL_TESTNET_TIMELOCK[GetChannel()] :
            TAO::Ledger::CHANNEL_NETWORK_TIMELOCK[GetChannel()]))
            return debug::error(FUNCTION, "block created before channel time-lock, please wait ",
                (config::fTestNet ?
                TAO::Ledger::CHANNEL_TESTNET_TIMELOCK[GetChannel()] :
                TAO::Ledger::CHANNEL_NETWORK_TIMELOCK[GetChannel()]) - runtime::unifiedtimestamp(), " seconds");


        /* Check the Current Version Block Time-Lock. Allow Version (Current -1) Blocks for 1 Hour after Time Lock. */
        if (nVersion > 1 && nVersion == (config::fTestNet ?
            TAO::Ledger::TESTNET_BLOCK_CURRENT_VERSION - 1 :
            TAO::Ledger::NETWORK_BLOCK_CURRENT_VERSION - 1) &&
            (GetBlockTime() - 3600) > (config::fTestNet ?
            TAO::Ledger::TESTNET_VERSION_TIMELOCK[TAO::Ledger::TESTNET_BLOCK_CURRENT_VERSION - 2] :
            TAO::Ledger::NETWORK_VERSION_TIMELOCK[TAO::Ledger::NETWORK_BLOCK_CURRENT_VERSION - 2]))
            return debug::error(FUNCTION, "version ", nVersion, " blocks have been obsolete for ",
                (runtime::unifiedtimestamp() - (config::fTestNet ?
                TAO::Ledger::TESTNET_VERSION_TIMELOCK[TAO::Ledger::TESTNET_BLOCK_CURRENT_VERSION - 2] :
                TAO::Ledger::NETWORK_VERSION_TIMELOCK[TAO::Ledger::TESTNET_BLOCK_CURRENT_VERSION - 2])), " seconds");


        /* Check the Current Version Block Time-Lock. */
        if (nVersion >= (config::fTestNet ?
            TAO::Ledger::TESTNET_BLOCK_CURRENT_VERSION :
            TAO::Ledger::NETWORK_BLOCK_CURRENT_VERSION) && GetBlockTime() <=
            (config::fTestNet ? TAO::Ledger::TESTNET_VERSION_TIMELOCK[TAO::Ledger::TESTNET_BLOCK_CURRENT_VERSION - 2] :
            TAO::Ledger::NETWORK_VERSION_TIMELOCK[TAO::Ledger::NETWORK_BLOCK_CURRENT_VERSION - 2]))
            return debug::error(FUNCTION, "version ", nVersion, " blocks are not accepted for ",
                (runtime::unifiedtimestamp() - (config::fTestNet ?
                TAO::Ledger::TESTNET_VERSION_TIMELOCK[TAO::Ledger::TESTNET_BLOCK_CURRENT_VERSION - 2] :
                TAO::Ledger::NETWORK_VERSION_TIMELOCK[TAO::Ledger::NETWORK_BLOCK_CURRENT_VERSION - 2])), " seconds");


        /* Check the Required Mining Outputs. */
        if (IsProofOfWork())
        {
            uint32_t nSize = vtx[0].vout.size();

            /* Check the Coinbase Tx Size. */
            if(nSize < 3)
                return debug::error(FUNCTION, "coinbase too small");

            /* Check the ambassador and developer addresses. */
            if(!config::fTestNet)
            {
                /* Check the ambassador signatures. */
                if (!VerifyAddressList(vtx[0].vout[nSize - 2].scriptPubKey,
                    (nVersion < 5) ? AMBASSADOR_SCRIPT_SIGNATURES : AMBASSADOR_SCRIPT_SIGNATURES_RECYCLED))
                    return debug::error(FUNCTION, "block ", nHeight, " ambassador signatures invalid");

                /* Check the developer signatures. */
                if (!VerifyAddressList(vtx[0].vout[nSize - 1].scriptPubKey,
                    (nVersion < 5) ? DEVELOPER_SCRIPT_SIGNATURES : DEVELOPER_SCRIPT_SIGNATURES_RECYCLED))
                    return debug::error(FUNCTION, "block ", nHeight, " developer signatures invalid");
            }
        }


        /* Check the Coinbase Transaction is First, with no repetitions. */
        if (vtx.empty() || (!vtx[0].IsCoinBase() && IsProofOfWork()))
            return debug::error(FUNCTION, "first tx is not coinbase for proof of work");


        /* Check the Coinstake Transaction is First, with no repetitions. */
        if (vtx.empty() || (!vtx[0].IsCoinStake() && nChannel == 0))
            return debug::error(FUNCTION, "first tx is not coinstake for proof of stake");


        /* Check for duplicate Coinbase / Coinstake Transactions. */
        for (uint32_t i = 1; i < vtx.size(); i++)
            if (vtx[i].IsCoinBase() || vtx[i].IsCoinStake())
                return debug::error(FUNCTION, "more than one coinbase / coinstake");


        /* Check coinbase/coinstake timestamp is at least 20 minutes before block time */
        if (GetBlockTime() > (uint64_t)vtx[0].nTime + ((nVersion < 4) ? 1200 : 3600))
            return debug::error(FUNCTION, "coinbase/coinstake timestamp is too early");

        /* Ensure the Block is for Proof of Stake Only. */
        if(IsProofOfStake())
        {
            /* Check for nNonce zero. */
            if(nHeight > 2392970 && nNonce == 0)
                return debug::error(FUNCTION, "stake cannot have nonce of 0");

            /* Check the Coinstake Time is before Unified Timestamp. */
            if(vtx[0].nTime > (runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT))
                return debug::error(FUNCTION, "coinstake too far in Future.");

            /* Make Sure Coinstake Transaction is First. */
            if (!vtx[0].IsCoinStake())
                return debug::error(FUNCTION, "first transaction non-coinstake ", vtx[0].GetHash().ToString());

            /* Make Sure Coinstake Transaction Time is Before Block. */
            if (vtx[0].nTime > nTime)
                return debug::error(FUNCTION, "coinstake ahead of block time");

        }


        /* Check for duplicate txid's */
        std::set<uint512_t> uniqueTx;


        /* Get the hashes for the merkle root. */
        std::vector<uint512_t> vHashes;


        /* Get the signature operations for legacy tx's. */
        uint32_t nSigOps = 0;


        /* Check all the transactions. */
        for(auto & tx : vtx)
        {
            /* Insert txid into set to check for duplicates. */
            uniqueTx.insert(tx.GetHash());

            /* Push back this hash for merkle root. */
            vHashes.push_back(tx.GetHash());

            /* Check the transaction timestamp. */
            if(GetBlockTime() < (uint64_t) tx.nTime)
                return debug::error(FUNCTION, "block timestamp earlier than transaction timestamp");

            /* Check the transaction for validitity. */
            if(!tx.CheckTransaction())
                return debug::error(FUNCTION, "check transaction failed.");

            /* Calculate the signature operations. */
            nSigOps += tx.GetLegacySigOpCount();
        }


        /* Check for duplicate txid's. */
        if (uniqueTx.size() != vtx.size())
            return debug::error(FUNCTION, "duplicate transaction");


        /* Check the signature operations for legacy. */
        if (nSigOps > TAO::Ledger::MAX_BLOCK_SIGOPS)
            return debug::error(FUNCTION, "out-of-bounds SigOpCount");


        /* Check the merkle root. */
        if (hashMerkleRoot != BuildMerkleTree(vHashes))
            return debug::error(FUNCTION, "hashMerkleRoot mismatch");


        /* Get the key from the producer. */
        if(nHeight > 0)
        {
            /* Get a vector for the solver solutions. */
            std::vector<std::vector<uint8_t> > vSolutions;

            /* Switch for the transaction type. */
            TransactionType whichType;
            if (!Solver(vtx[0].vout[0].scriptPubKey, whichType, vSolutions))
                return debug::error(FUNCTION, "solver failed for check signature.");

            /* Check for public key type. */
            if (whichType == TX_PUBKEY)
            {
                /* Get the public key. */
                const std::vector<uint8_t>& vchPubKey = vSolutions[0];

                /* Get the key object. */
                LLC::ECKey key;
                if (!key.SetPubKey(vchPubKey))
                    return false;

                /* Check the Block Signature. */
                if (!VerifySignature(key))
                    return debug::error(FUNCTION, "bad block signature");
            }
        }

        return true;
    }


    /* Accept a block into the chain. */
    bool LegacyBlock::Accept()
    {
        /* Read leger DB for duplicate block. */
        TAO::Ledger::BlockState state;
        if(LLD::legDB->ReadBlock(GetHash(), state))
            return debug::error(FUNCTION, "block state already exists");


        /* Read leger DB for previous block. */
        TAO::Ledger::BlockState statePrev;
        if(!LLD::legDB->ReadBlock(hashPrevBlock, statePrev))
            return debug::error(FUNCTION, "previous block state not found");


        /* Check the Height of Block to Previous Block. */
        if(statePrev.nHeight + 1 != nHeight)
            return debug::error(FUNCTION, "incorrect block height.");


        /* Get the proof hash for this block. */
        uint1024_t hash = (nVersion < 5 ? GetHash() : GetChannel() == 0 ? StakeHash() : ProofHash());


        /* Get the target hash for this block. */
        uint1024_t hashTarget = LLC::CBigNum().SetCompact(nBits).getuint1024();


        /* Verbose logging of proof and target. */
        debug::log(2, "  proof:  ", hash.ToString().substr(0, 30));


        /* Channel switched output. */
        if(GetChannel() == 1)
            debug::log(2, "  prime cluster verified of size ", TAO::Ledger::GetDifficulty(nBits, 1));
        else
            debug::log(2, "  target: ", hashTarget.ToString().substr(0, 30));


        /* Check that the nBits match the current Difficulty. **/
        if (nBits != TAO::Ledger::GetNextTargetRequired(statePrev, GetChannel()))
            return debug::error(FUNCTION, "incorrect proof-of-work/proof-of-stake");


        /* Check That Block timestamp is not before previous block. */
        //if (GetBlockTime() <= statePrev.GetBlockTime())
        //    return debug::error(FUNCTION, "block's timestamp too early Block: ", GetBlockTime(), " Prev: ",
        //     statePrev.GetBlockTime());


        /* Check that Block is Descendant of Hardened Checkpoints. */
        if(!TAO::Ledger::ChainState::Synchronizing() && !TAO::Ledger::IsDescendant(statePrev))
            return debug::error(FUNCTION, "not descendant of last checkpoint");


        /* Check the block proof of work rewards. */
        if(IsProofOfWork() && nVersion >= 3)
        {
            uint32_t nSize = vtx[0].vout.size();

            /* Add up the Miner Rewards from Coinbase Tx Outputs. */
            uint64_t nMiningReward = 0;
            for(int32_t nIndex = 0; nIndex < nSize - 2; nIndex++)
                nMiningReward += vtx[0].vout[nIndex].nValue;

            /* Check that the Mining Reward Matches the Coinbase Calculations. */
            if (nMiningReward != TAO::Ledger::GetCoinbaseReward(statePrev, GetChannel(), 0))
                return debug::error(FUNCTION, "miner reward mismatch ",
                    nMiningReward, " to ", TAO::Ledger::GetCoinbaseReward(statePrev, GetChannel(), 0));

            /* Check that the Ambassador Reward Matches the Coinbase Calculations. */
            if (vtx[0].vout[nSize - 2].nValue != TAO::Ledger::GetCoinbaseReward(statePrev, GetChannel(), 1))
                return debug::error(FUNCTION, "ambassador reward mismatch ",
                    vtx[0].vout[nSize - 2].nValue, " to ", TAO::Ledger::GetCoinbaseReward(statePrev, GetChannel(), 1));

            /* Check that the Developer Reward Matches the Coinbase Calculations. */
            if (vtx[0].vout[nSize - 1].nValue != TAO::Ledger::GetCoinbaseReward(statePrev, GetChannel(), 2))
                return debug::error(FUNCTION, "developer reward mismatch ",
                    vtx[0].vout[nSize - 1].nValue, " to ", TAO::Ledger::GetCoinbaseReward(statePrev, GetChannel(), 2));

        }
        else if (IsProofOfStake())
        {
            /* Check that the Coinbase / CoinstakeTimstamp is after Previous Block. */
            if (vtx[0].nTime < statePrev.GetBlockTime())
                return debug::error(FUNCTION, "coinstake transaction too early");

            /* Check the claimed stake limits are met. */
            if(nVersion >= 5 && !CheckStake())
                return debug::error(FUNCTION, "invalid proof of stake");
        }

        /* Check that Transactions are Finalized. */
        for(const auto & tx : vtx)
            if (!tx.IsFinal(nHeight, GetBlockTime()))
                return debug::error(FUNCTION, "contains a non-final transaction");

        return true;
    }


    /* Check the proof of stake calculations. */
    bool LegacyBlock::CheckStake() const
    {
        return true;
    }
}
