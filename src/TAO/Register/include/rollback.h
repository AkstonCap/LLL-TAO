/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_REGISTER_INCLUDE_ROLLBACK_H
#define NEXUS_TAO_REGISTER_INCLUDE_ROLLBACK_H

#include <TAO/Ledger/types/transaction.h>

namespace TAO::Register
{

    /** Rollback
     *
     *  Rollback the current network state to register pre-states
     *
     *  @param[in] tx The transaction to verify pre-states with.
     *
     *  @return true if verified correctly.
     *
     **/
    bool Rollback(TAO::Ledger::Transaction tx);

}

#endif
