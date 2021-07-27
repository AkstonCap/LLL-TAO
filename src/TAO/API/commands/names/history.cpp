/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/names.h>
#include <TAO/API/objects/types/objects.h>

#include <TAO/Register/include/enum.h>

/* Global TAO namespace. */
namespace TAO
{
    /* API Layer namespace. */
    namespace API
    {
        /* History of a name */
        encoding::json Names::NameHistory(const encoding::json& params, const bool fHelp)
        {
            return Objects::History(params, TAO::Register::OBJECTS::NAME, std::string("Name"));
        }

        /* History of a namespace */
        encoding::json Names::NamespaceHistory(const encoding::json& params, const bool fHelp)
        {
            return Objects::History(params, TAO::Register::OBJECTS::NAMESPACE, std::string("Namespace"));
        }
    }
}
