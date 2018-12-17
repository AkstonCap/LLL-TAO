/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/rpc.h>
#include <Util/include/json.h>
#include <Util/include/runtime.h>
#include <LLP/include/version.h>
//#include <TAO/Ledger/include/global.h>

namespace TAO::API
{


    json::json RPC::GetInfo(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() != 0)
            return std::string(
                "getinfo"
                " - Returns an object containing various state info.");

        json::json obj;
        obj["version"] = LLP::strProtocolName; //PS TODO
        obj["protocolversion"] = LLP::PROTOCOL_VERSION;
        /*obj.push_back(std::make_pair("walletversion", pwalletMain->GetVersion()));
        obj.push_back(std::make_pair("balance",       ValueFromAmount(pwalletMain->GetBalance())));
        obj.push_back(std::make_pair("newmint",       ValueFromAmount(pwalletMain->GetNewMint())));
        obj.push_back(std::make_pair("stake",         ValueFromAmount(pwalletMain->GetStake())));
    */

    //   double dPercent = ((double)Core::dTrustWeight + (double)Core::dBlockWeight) / 37.5;
    //   obj.push_back(std::make_pair("interestweight", (double)Core::dInterestRate * 100.0));
    //   obj.push_back(std::make_pair("stakeweight",    dPercent * 100.0));
    //   obj.push_back(std::make_pair("trustweight",    (double)Core::dTrustWeight * 100.0 / 17.5));
    //   obj.push_back(std::make_pair("blockweight",    (double)Core::dBlockWeight * 100.0  / 20.0));
    //   obj.push_back(std::make_pair("txtotal",        (int)pwalletMain->mapWallet.size()));


    //   obj.push_back(std::make_pair("blocks",        (int)Core::nBestHeight));

        obj["timestamp"] =  (int)runtime::UnifiedTimestamp();

    //   obj.push_back(std::make_pair("connections",   (int)::vNodes.size()));
    //    obj.push_back(std::make_pair("proxy",         (fUseProxy ? addrProxy.ToStringIPPort() : string())));
    //    obj.push_back(std::make_pair("ip",            addrSeenByPeer.ToStringIP()));

    //    obj.push_back(std::make_pair("testnet",       fTestNet));
    //    obj.push_back(std::make_pair("keypoololdest", (boost::int64_t)pwalletMain->GetOldestKeyPoolTime()));
    //    obj.push_back(std::make_pair("keypoolsize",   pwalletMain->GetKeyPoolSize()));
    //   obj.push_back(std::make_pair("paytxfee", Core::nTransactionFee / Core::COIN));
    //    if (pwalletMain->IsCrypted())
    //        obj.push_back(std::make_pair("unlocked_until", (boost::int64_t)nWalletUnlockTime / 1000));
    //    obj.push_back(std::make_pair("errors",        Core::GetWarnings("statusbar")));
        
        
        return obj;
    }


    /*
    Value getmininginfo(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 0)
            throw runtime_error(
                "getmininginfo"
                "Returns an object containing mining-related information.");

        uint64 nPrimePS = 0;
        if(!Core::pindexBest || Core::pindexBest->GetBlockHash() != Core::hashGenesisBlock)
        {
            double nPrimeAverageDifficulty = 0.0;
            unsigned int nPrimeAverageTime = 0;
            unsigned int nPrimeTimeConstant = 2480;
            int nTotal = 0;
            const Core::CBlockIndex* pindex = Core::GetLastChannelIndex(Core::pindexBest, 1);
            for(; (nTotal < 1440 && pindex->pprev); nTotal ++) {

                nPrimeAverageTime += (pindex->GetBlockTime() - Core::GetLastChannelIndex(pindex->pprev, 1)->GetBlockTime());
                nPrimeAverageDifficulty += (Core::GetDifficulty(pindex->nBits, 1));

                pindex = Core::GetLastChannelIndex(pindex->pprev, 1);
            }
            nPrimeAverageDifficulty /= nTotal;
            nPrimeAverageTime /= nTotal;
            nPrimePS = (nPrimeTimeConstant / nPrimeAverageTime) * std::pow(50.0, (nPrimeAverageDifficulty - 3.0));
        }

        // Hash
        int nHTotal = 0;
        unsigned int nHashAverageTime = 0;
        double nHashAverageDifficulty = 0.0;
        uint64 nTimeConstant = 276758250000;
        const Core::CBlockIndex* hindex = Core::GetLastChannelIndex(Core::pindexBest, 2);
        for(;  (nHTotal < 1440 && hindex->pprev); nHTotal ++) {

            nHashAverageTime += (hindex->GetBlockTime() - Core::GetLastChannelIndex(hindex->pprev, 2)->GetBlockTime());
            nHashAverageDifficulty += (Core::GetDifficulty(hindex->nBits, 2));

            hindex = Core::GetLastChannelIndex(hindex->pprev, 2);
        }
        nHashAverageDifficulty /= nHTotal;
        nHashAverageTime /= nHTotal;

        uint64 nHashRate = (nTimeConstant / nHashAverageTime) * nHashAverageDifficulty;


        Object obj;
        obj.push_back(Pair("blocks",        (int)Core::nBestHeight));
        obj.push_back(Pair("timestamp", (int)runtime::UnifiedTimestamp()));

        obj.push_back(Pair("currentblocksize",(uint64_t)Core::nLastBlockSize));
        obj.push_back(Pair("currentblocktx",(uint64_t)Core::nLastBlockTx));

        const Core::CBlockIndex* pindexCPU = Core::GetLastChannelIndex(Core::pindexBest, 1);
        const Core::CBlockIndex* pindexGPU = Core::GetLastChannelIndex(Core::pindexBest, 2);
        obj.push_back(Pair("primeDifficulty",       Core::GetDifficulty(Core::GetNextTargetRequired(Core::pindexBest, 1, false), 1)));
        obj.push_back(Pair("hashDifficulty",        Core::GetDifficulty(Core::GetNextTargetRequired(Core::pindexBest, 2, false), 2)));
        obj.push_back(Pair("primeReserve",           ValueFromAmount(pindexCPU->nReleasedReserve[0])));
        obj.push_back(Pair("hashReserve",            ValueFromAmount(pindexGPU->nReleasedReserve[0])));
        obj.push_back(Pair("primeValue",               ValueFromAmount(Core::GetCoinbaseReward(Core::pindexBest, 1, 0))));
        obj.push_back(Pair("hashValue",                ValueFromAmount(Core::GetCoinbaseReward(Core::pindexBest, 2, 0))));
        obj.push_back(Pair("pooledtx",              (boost::uint64_t)Core::mempool.size()));
        obj.push_back(Pair("primesPerSecond",         (boost::uint64_t)nPrimePS));
        obj.push_back(Pair("hashPerSecond",         (boost::uint64_t)nHashRate));

        if(GetBoolArg("-mining", false))
        {
            obj.push_back(Pair("totalConnections", LLP::MINING_LLP->TotalConnections()));
        }


        return obj;
    }
    */

}