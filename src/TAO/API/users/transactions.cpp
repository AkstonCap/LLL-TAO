/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/users.h>
#include <TAO/API/include/utils.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get a user's account. */
        json::json Users::Transactions(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Get the Genesis ID. */
            uint256_t hashGenesis = 0;

            /* Watch for destination genesis. */
            if(params.find("genesis") != params.end())
                hashGenesis.SetHex(params["genesis"].get<std::string>());
            else if(params.find("username") != params.end())
                hashGenesis = TAO::Ledger::SignatureChain::Genesis(params["username"].get<std::string>().c_str());
            else if(!config::fAPISessions && mapSessions.count(0))
            {
                /* If no specific genesis or username have been provided then fall back to the active sig chain */
                hashGenesis = mapSessions[0]->Genesis();
            }
            else
                throw APIException(-25, "Missing Genesis or Username");

            /* Check for paged parameter. */
            uint32_t nPage = 0;
            if(params.find("page") != params.end())
                nPage = std::stoul(params["page"].get<std::string>().c_str());

            /* Check for username parameter. */
            uint32_t nLimit = 100;
            if(params.find("limit") != params.end())
                nLimit = std::stoul(params["limit"].get<std::string>().c_str());

            /* Get verbose levels. */
            uint32_t nVerbose = 2; // default to level 2
            if(params.find("verbose") != params.end())
                nVerbose = std::stoul(params["verbose"].get<std::string>().c_str());

            /* Get the last transaction. */
            uint512_t hashLast = 0;
            if(!LLD::legDB->ReadLast(hashGenesis, hashLast))
                throw APIException(-28, "No transactions found");

            /* Loop until genesis. */
            uint32_t nTotal = 0;
            while(hashLast != 0)
            {
                /* Get the current page. */
                uint32_t nCurrentPage = nTotal / nLimit;

                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::legDB->ReadTx(hashLast, tx))
                    throw APIException(-28, "Failed to read transaction");

                /* Set the next last. */
                hashLast = tx.hashPrevTx;
                ++nTotal;

                /* Check the paged data. */
                if(nCurrentPage < nPage)
                    continue;

                if(nCurrentPage > nPage)
                    break;

                if( nTotal > nLimit)
                    break;

                TAO::Ledger::BlockState blockState;
                /* Read the block state from the the ledger DB using the transaction hash index */
                if(!LLD::legDB->ReadBlock(tx.GetHash(), blockState))
                    throw APIException(-25, "Block not found");

                json::json obj = TAO::API::TransactionToJSON( tx, blockState, nVerbose);

                ret.push_back(obj);
            }

            return ret;
        }
    }
}
