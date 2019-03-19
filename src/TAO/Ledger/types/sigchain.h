/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_SIGNATURE_CHAIN_H
#define NEXUS_TAO_LEDGER_TYPES_SIGNATURE_CHAIN_H

#include <string>

#include <LLC/types/uint1024.h>

#include <Util/include/allocators.h>
#include <Util/include/mutex.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** SignatureChain
         *
         *  Base Class for a Signature SignatureChain
         *
         *  Similar to HD wallet systems, but integrated into the Layer 2 of the nexus software stack.
         *  Seed phrase includes at least 128 bits of entropy (8 char username, 8 char password) and pin
         *  to attack a signature chain by dictionary attack.
         *
         */
        class SignatureChain
        {

            /** Secure allocator to represent the username of this signature chain. **/
            const SecureString strUsername;


            /** Secure allocater to represent the password of this signature chain. **/
            const SecureString strPassword;


            /* Internal mutex for caches. */
            mutable std::mutex MUTEX;


            /** Internal sigchain cache (to not exhaust ourselves regenerating the same key). **/
            mutable std::pair<uint32_t, SecureString> pairCache;


            /** Internal genesis hash. **/
            const uint256_t hashGenesis;

        public:

            /** Default constructor. **/
            SignatureChain()
            : strUsername()
            , strPassword()
            , MUTEX()
            , pairCache(std::make_pair(std::numeric_limits<uint32_t>::max(), ""))
            , hashGenesis()
            {

            }

            /** Constructor to generate Keychain
             *
             * @param[in] strUsernameIn The username to seed the signature chain
             * @param[in] strPasswordIn The password to seed the signature chain
             **/
            SignatureChain(const SecureString& strUsernameIn, const SecureString& strPasswordIn)
            : strUsername(strUsernameIn.c_str())
            , strPassword(strPasswordIn.c_str())
            , MUTEX()
            , pairCache(std::make_pair(std::numeric_limits<uint32_t>::max(), ""))
            , hashGenesis(SignatureChain::Genesis(strUsernameIn))
            {
            }


            /** Copy constructor **/
            SignatureChain(const SignatureChain& chain)
            : strUsername(chain.strUsername)
            , strPassword(chain.strPassword)
            , MUTEX()
            , pairCache(chain.pairCache)
            , hashGenesis(chain.hashGenesis)
            {
            }

            /** Move constructor
             *
             * @param[in] strUsernameIn The username to seed the signature chain
             * @param[in] strPasswordIn The password to seed the signature chain
             **/
            SignatureChain(const SignatureChain&& chain)
            : strUsername(chain.strUsername)
            , strPassword(chain.strPassword)
            , MUTEX()
            , pairCache(chain.pairCache)
            , hashGenesis(chain.hashGenesis)
            {
            }


            /** Destructor. **/
            ~SignatureChain()
            {
            }


            /** Genesis
             *
             *  This function is responsible for generating the genesis ID.
             *
             *  @return The 512 bit hash of this key in the series.
             **/
            uint256_t Genesis() const;


            /** Genesis
             *
             *  This function is responsible for generating the genesis ID.
             *
             *  @return The 512 bit hash of this key in the series.
             **/
            static uint256_t Genesis(const SecureString& strUsername);


            /** Generate
             *
             *  This function is responsible for genearting the private key in the keychain of a specific account.
             *  The keychain is a series of keys seeded from a secret phrase and a PIN number.
             *
             *  @param[in] nKeyID The key number in the keychian
             *  @param[in] strSecret The secret phrase to use
             *  @param[in] fCache Use the cache on hand for keys.
             *
             *  @return The 512 bit hash of this key in the series.
             **/
            uint512_t Generate(const uint32_t nKeyID, const SecureString& strSecret, bool fCache = true) const;
        };
    }
}

#endif
