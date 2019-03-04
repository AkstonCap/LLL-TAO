/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_CHECKPOINTS_H
#define NEXUS_TAO_LEDGER_INCLUDE_CHECKPOINTS_H

#include <map>

#include <LLC/types/uint1024.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** IsNewTimespan
         *
         *  Check if the new block triggers a new Checkpoint timespan.
         *
         *  @param[in] state The state object to check from.
         *
         *  @returns true if a new timespan has elapsed
         *
         **/
        bool IsNewTimespan(const BlockState& state);


        /** IsDescendant
         *
         *  Check that the checkpoint is a Descendant of previous Checkpoint.
         *
         *  @param[in] state The state object to check from.
         *
         *  @returns true if a block is a descendant.
         *
         **/
        bool IsDescendant(const BlockState& state);


        /** HardenCheckpoint
         *
         *  Harden a checkpoint into the checkpoint chain.
         *
         *  @param[in] state The state object to check from.
         *
         *  @returns true if a checkpoint was hardened.
         *
         **/
        bool HardenCheckpoint(const BlockState& state);

    }
}


#endif
