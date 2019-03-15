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
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** Lisp
         *
         *  Lisp API Class.
         *  API that wraps the lispers.net API to provide querying of lisp data
         *
         **/
        class Lisp : public Base
        {
        public:

            /** Default Constructor. **/
            Lisp() { Initialize(); }


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
                return "Lisp";
            }

     
            /** EIDs
             *
             *  Queries the lisp api and returns the EID's for this node
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json EIDs(const json::json& params, bool fHelp);

            

        };

        extern Lisp lisp;

    }

}