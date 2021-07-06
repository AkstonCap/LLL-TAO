/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/base.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /** Objects
     *
     *  Objects API Class.
     *  Manages the function pointers for all Objects API commands.
     *
     **/
    class Objects : public Base
    {
    public:

        /** Default Constructor. **/
        Objects()
        : Base()
        {
        }


        /** Initialize.
         *
         *  Sets the function pointers for this API.
         *
         **/
        void Initialize() final;


        /** Transfer
         *
         *  Generic method to transfer an object register .
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] nType The object type to transfer.
         *  @param[in] strType The name of the object type, used to format error messages
         *
         *  @return The return object in JSON.
         *
         **/
        static encoding::json Transfer(const encoding::json& params, const uint8_t nType, const std::string& strType);


        /** Claim
         *
         *  Generic method to claim a transferred object regiseter .
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] nType The object type to claim
         *  @param[in] strType The name of the object type, used to format error messages
         *
         *  @return The return object in JSON.
         *
         **/
        static encoding::json Claim(const encoding::json& params, const uint8_t nType, const std::string& strType);


        /** History
         *
         *  Generic method to output the history of an object register and its ownership
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] nType The object type to show history for
         *  @param[in] strType The name of the object type, used to format error messages
         *
         *  @return The return object in JSON.
         *
         **/
        static encoding::json History(const encoding::json& params, const uint8_t nType, const std::string& strType);


        /** List
         *
         *  Generic method to list object registers by sig chain
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] nRegisterType The register type to list
         *  @param[in] nObjectType The object type to list
         *
         *  @return The return object in JSON.
         *
         **/
        static encoding::json List(const encoding::json& params, const uint8_t nRegisterType, const uint8_t nObjectType)
        {
            return encoding::json::array();
        }


        /** ListTransactions
         *
         *  Lists all transactions for a given object register
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        static encoding::json ListTransactions(const encoding::json& params, const bool fHelp);

    };
}
