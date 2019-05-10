/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>

#include <TAO/Register/include/system.h>

#include <TAO/Ledger/include/constants.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Commits funds from a coinbase transaction. */
        bool Trust(const uint512_t& hashLastTrust, const uint64_t nTrustScore, const uint8_t nFlags, TAO::Ledger::Transaction &tx)
        {
            /* Read the register from the database. */
            TAO::Register::Object account;
            TAO::Register::Object sys;

            /* Write pre-states. */
            if((nFlags & TAO::Register::FLAGS::PRESTATE))
            {
                /* Set the register pre-states. */
                {
                    if(!LLD::regDB->ReadTrust(tx.hashGenesis, account))
                        return debug::error(FUNCTION, "trust address doesn't exist ", tx.hashGenesis.ToString());

                    tx.ssRegister << uint8_t(TAO::Register::STATES::PRESTATE) << account;
                }

                /* Set the system pre-states. */
                {
                    if(!LLD::regDB->ReadState(uint256_t(TAO::Register::SYSTEM::TRUST), sys))
                        return debug::error(FUNCTION, "system register address doesn't exist ", uint256_t(TAO::Register::SYSTEM::TRUST).ToString());

                    tx.ssSystem << uint8_t(TAO::Register::STATES::PRESTATE) << sys;
                }
            }

            /* Get pre-states on write. */
            if(nFlags & TAO::Register::FLAGS::WRITE  || nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                {
                    /* Get the state byte. */
                    uint8_t nState = 0; //RESERVED
                    tx.ssRegister >> nState;

                    /* Check for the pre-state. */
                    if(nState != TAO::Register::STATES::PRESTATE)
                        return debug::error(FUNCTION, "register script not in pre-state");

                    /* Get the pre-state. */
                    tx.ssRegister >> account;
                }

                {
                    /* Get the state byte. */
                    uint8_t nState = 0; //RESERVED
                    tx.ssSystem >> nState;

                    /* Check for the pre-state. */
                    if(nState != TAO::Register::STATES::PRESTATE)
                        return debug::error(FUNCTION, "register script not in pre-state");

                    /* Get the pre-state. */
                    tx.ssSystem >> sys;
                }
            }

            /* Check ownership of register. */
            if(account.hashOwner != tx.hashGenesis)
                return debug::error(FUNCTION, tx.hashGenesis.ToString(), " caller not authorized to debit from register");

            /* Parse the account object register. */
            if(!account.Parse())
                return debug::error(FUNCTION, "failed to parse account object register");

            /* Calculate the new trust score. */
            uint64_t nTrustPrev = account.get<uint64_t>("trust");

            /* Write the new trust to object register. */
            if(!account.Write("trust", nTrustScore))
                return debug::error(FUNCTION, "trust could not be written to object register");

            /* Get the interest payment. */
            uint64_t nBalance = account.get<uint64_t>("balance");

            //calculate stake reward and add it to nBalance

            /* Write the new balance to object register. */
            if(!account.Write("balance", nBalance))
                return debug::error(FUNCTION, "balance could not be written to object register");

            /* Update the state register's timestamp. */
            account.nTimestamp = tx.nTimestamp;
            account.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!account.IsValid())
                return debug::error(FUNCTION, "trust address ", tx.hashGenesis.ToString(), " is in invalid state");

            /* Parse the system object register. */
            if(!sys.Parse())
                return debug::error(FUNCTION, "failed to parse system object register");

            /* Write the system values. */
            if(!sys.Write("trust", sys.get<uint64_t>("trust") + int64_t(nTrustScore - nTrustPrev)))
                return debug::error(FUNCTION, "could not write new system register value.");

            /* Update the system register's timestamp. */
            sys.nTimestamp = tx.nTimestamp;
            sys.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!sys.IsValid())
                return debug::error(FUNCTION, "system memory address ", uint256_t(TAO::Register::SYSTEM::TRUST).ToString(), " is in invalid state");

            /* Write post-state checksum. */
            if((nFlags & TAO::Register::FLAGS::POSTSTATE))
            {
                tx.ssRegister << uint8_t(TAO::Register::STATES::POSTSTATE) << account.GetHash();
                tx.ssSystem   << uint8_t(TAO::Register::STATES::POSTSTATE) << sys.GetHash();
            }

            /* Verify the post-state checksum. */
            if(nFlags & TAO::Register::FLAGS::WRITE || nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                /* Check register post-state checksum. */
                {
                    /* Get the state byte. */
                    uint8_t nState = 0; //RESERVED
                    tx.ssRegister >> nState;

                    /* Check for the pre-state. */
                    if(nState != TAO::Register::STATES::POSTSTATE)
                        return debug::error(FUNCTION, "register script not in post-state");

                    /* Get the post state checksum. */
                    uint64_t nChecksum;
                    tx.ssRegister >> nChecksum;

                    /* Check for matching post states. */
                    if(nChecksum != account.GetHash())
                        return debug::error(FUNCTION, "register script has invalid post-state");
                }

                /* Check system post-state checksum. */
                {
                    /* Get the state byte. */
                    uint8_t nState = 0; //RESERVED
                    tx.ssSystem >> nState;

                    /* Check for the pre-state. */
                    if(nState != TAO::Register::STATES::POSTSTATE)
                        return debug::error(FUNCTION, "system register script not in post-state");

                    /* Get the post state checksum. */
                    uint64_t nChecksum;
                    tx.ssSystem >> nChecksum;

                    /* Check for matching post states. */
                    if(nChecksum != sys.GetHash())
                        return debug::error(FUNCTION, "system register script has invalid post-state");
                }

                /* Update the register database with the index. */
                if((nFlags & TAO::Register::FLAGS::WRITE))
                {
                    /* Write the register to the database. */
                    if(!LLD::regDB->WriteState(uint256_t(TAO::Register::SYSTEM::TRUST), sys))
                        return debug::error(FUNCTION, "failed to write new state");

                    /* Write the register to the database. */
                    if(!LLD::regDB->WriteTrust(tx.hashGenesis, account))
                        return debug::error(FUNCTION, "failed to write new state");
                }

            }

            return true;
        }
    }
}
