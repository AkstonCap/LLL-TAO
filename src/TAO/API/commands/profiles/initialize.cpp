/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/profiles.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Profiles::Initialize()
    {
        /* Populate our MASTER standard. */
        mapStandards["master"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                return false;
            }
        );


        /* Handle for all CREATE operations. */
        mapFunctions["create"] = Function
        (
            std::bind
            (
                &Profiles::Create,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for all transactions operations. */
        mapFunctions["transactions"] = Function
        (
            std::bind
            (
                &Profiles::Transactions,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );
    }
}
