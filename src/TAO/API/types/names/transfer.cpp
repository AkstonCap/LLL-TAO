/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/names.h>
#include <TAO/API/types/objects.h>

#include <TAO/Register/include/enum.h>

/* Global TAO namespace. */
namespace TAO
{
    /* API Layer namespace. */
    namespace API
    {
        /* Transfer a name. */
        json::json Names::TransferName(const json::json& params, bool fHelp)
        {
            return Objects::Transfer(params, TAO::Register::OBJECTS::NAME, std::string("Name"));
        }

        /* Transfer a namespace. */
        json::json Names::TransferNamespace(const json::json& params, bool fHelp)
        {
            return Objects::Transfer(params, TAO::Register::OBJECTS::NAMESPACE, std::string("Namespace"));
        }
    }
}
