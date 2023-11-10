/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/address.h>

#include <TAO/Ledger/include/enum.h>

#include <set>
#include <map>
#include <string>

/* Global TAO namespace. */
namespace TAO::Register
{
    /** Hold list of reserved object register values. **/
    const std::map<std::string, std::pair<std::string, bool>> RESERVED =
    {
        /* Used to hold token blance. */
        { "balance",  std::make_pair("uint64", true)    },

        /* Used to identify a specific token. */
        { "token",    std::make_pair("uint256", false)  },

        /* Used to set validation script in object register. */
        { "require",  std::make_pair("disabled", false) },

        /* Used for token domain parameters for max supply. */
        { "supply",   std::make_pair("uint64", false)   },

        /* Used to determine decimals of a token. */
        { "decimals", std::make_pair("uint8", false)    },

        /* Used to determine collective trust in a register. */
        { "trust",    std::make_pair("uint64", true)    },

        /* Used to determine total stake for a register. */
        { "stake",    std::make_pair("uint64", true)    },

        /* Used for internal object register system memory. */
        { "system",   std::make_pair("disabled", false) },

        /* Used for internal object register names. */
        { "name",     std::make_pair("string", false)   }
    };


    /** Reserved
     *
     *  Object register keys that are reserved
     *
     *  @param[in] strValue The value to check.
     *
     *  @return True if value is object register reserved value.
     *
     **/
    __attribute__((const)) inline bool Reserved(const std::string& strValue)
    {
        return RESERVED.count(strValue);
    }


    /** Reserved
     *
     *  Object register keys that are reserved
     *
     *  @param[in] strValue The value to check.
     *
     *  @return True if value is object register reserved value.
     *
     **/
    __attribute__((const)) inline bool Reserved(const std::string& strValue, const std::string& strType, const bool fMutable = false)
    {
        /* Check that is in the reserved map. */
        if(!RESERVED.count(strValue))
            return false;

        return (RESERVED.at(strValue) != std::make_pair(strType, fMutable));
    }


    /** Reserved
     *
     *  System reserved values for system registers.
     *
     *  @param[in] hashAddress The register address to check.
     *
     *  @return True if value is system reserved value.
     *
     **/
    __attribute__((const)) inline bool Reserved(const Address& hashAddress)
    {
        /* Check for valid address values. */
        if(!hashAddress.IsValid())
            return true;

        return hashAddress >= uint256_t(SYSTEM::RESERVED) && hashAddress <= uint256_t(SYSTEM::LIMIT);
    }


    /** Range
     *
     *  Check if a register type is using a valid range
     *
     *  @param[in] nType The type byte to check
     *
     *  @return True if value is in acceptable range.
     *
     **/
    inline bool Range(const uint8_t nType)
    {
        return (nType > REGISTER::RESERVED && nType < REGISTER::SYSTEM);
    }


    /** Name
     *
     *  Contains the list of reserved names we want to check against.
     *
     **/
    namespace NAME
    {
        /** Hold list of reserved global names. **/
        const std::set<std::string> RESERVED =
        {
            /* native token name. */
            "NXS",
            "nxs",
            "NEXUS",
            "nexus"
        };

        /** Reserved
         *
         *  Object register keys that are reserved
         *
         *  @param[in] strValue The value to check.
         *
         *  @return True if value is object register reserved value.
         *
         **/
        __attribute__((const)) inline bool Reserved(const std::string& strValue)
        {
            return std::find(RESERVED.begin(), RESERVED.end(), strValue) != RESERVED.end();
        }
    }


    /** Namespace
     *
     *  Contains the list of reserved namespaces we want to check against.
     *
     **/
    namespace NAMESPACE
    {
        /** Hold list of reserved global names. **/
        const std::set<std::string> RESERVED =
        {
            /* Global namspace name has to be reserved */
            TAO::Register::NAMESPACE::GLOBAL,

            /* reserved to avoid confusion or scam attempts. */
             "nxs",
             "nexus.io",
             "nexus.com",
             "nexusearth.com",

        };

        /** Reserved
         *
         *  Object register keys that are reserved
         *
         *  @param[in] strValue The value to check.
         *
         *  @return True if value is object register reserved value.
         *
         **/
        __attribute__((const)) inline bool Reserved(const std::string& strValue)
        {
            /* For namespaces the reserved word can't exist anywhere in the name.  */
            return std::any_of(RESERVED.begin(), RESERVED.end(),
                [&](std::string strReserved)
                {
                    return strValue.find(strReserved) != std::string::npos;
                }
            );
        }
    }
}
