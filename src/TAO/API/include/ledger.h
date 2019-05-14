/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_API_INCLUDE_LEDGER_H
#define NEXUS_TAO_API_INCLUDE_LEDGER_H

#include <TAO/API/types/base.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** Ledger
         *
         *  Ledger API Class.
         *  Lower level API to interact directly with registers.
         *
         **/
        class Ledger : public Base
        {
        public:

            /** Default Constructor. **/
            Ledger()
            {
                Initialize();
            }


            /** Initialize.
             *
             *  Sets the function pointers for this API.
             *
             **/
            void Initialize() final;


            /** GetName
             *
             *  Returns the name of this API.
             *
             **/
            std::string GetName() const final
            {
                return "Ledger";
            }


            /** Create
             *
             *  Creates a register with given RAW state.
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Create(const json::json& params, bool fHelp);


            /** BlockHash
             *
             *  Retrieves the blockhash for the given height.
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json BlockHash(const json::json& params, bool fHelp);


            /** Block
             *
             *  Retrieves the block data for a given hash or height.
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Block(const json::json& params, bool fHelp);


            /** Blocks
             *
             *  Retrieves the block data for a sequential range of blocks
             *  starting at a given hash or height.
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Blocks(const json::json& params, bool fHelp);


            /** Transaction
             *
             *  Retrieves the transaction data for a given hash.
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Transaction(const json::json& params, bool fHelp);


            /** MiningInfo
            *
            *  Returns an object containing mining-related information
            *
            *  @param[in] params The parameters from the API call.
            *  @param[in] fHelp Trigger for help data.
            *
            *  @return The return object in JSON.
            *
            **/
            json::json MiningInfo(const json::json& params, bool fHelp);

        };

        extern Ledger ledger;
    }
}

#endif
