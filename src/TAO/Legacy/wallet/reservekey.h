/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
        
			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_WALLET_RESERVEKEY_H
#define NEXUS_TAO_LEGACY_WALLET_RESERVEKEY_H

#include <vector>

#include <TAO/Legacy/wallet/keypool.h>
#include <TAO/Legacy/wallet/wallet.h>

namespace Legacy
{
    
    /* forward declaration */
    class CWallet;

    /** @class CReserveKey
     *
     *  Holds the public key value of a key reserved in the key pool but not
     *  yet used.  Supports either returning the key to the pool or keeping it 
     *  for use. 
     *
     **/
    class CReserveKey
    {
    protected:
        /** Wallet containing the key pool where we want to reserve keys **/
        CWallet& wallet;

        /** The key pool index of a reserved key in the key pool, or -1 if no key reserved **/
        int64_t nPoolIndex;

        /** The public key value of the reserved key. Empty if no key reserved. **/
        std::vector<uint8_t> vchPubKey;


    public:
        /** Constructor
         *
         *  Initializes reserve key from wallet pointer. 
         *  
         *  @param[in] pWalletIn The wallet where keys will be reserved, cannot be nullptr
         *
         *  @deprecated supported for backward compatability
         *
         **/
        CReserveKey(CWallet* pwalletIn) :
            nIndex(-1),
            wallet(*pWalletIn)
        { }


        /** Constructor
         *
         *  Initializes reserve key from wallet reference.
         * 
         *  @param[in] walletIn The wallet where keys will be reserved
         *
         **/
        CReserveKey(CWallet& walletIn) :
            nIndex(-1),
            wallet(walletIn)
        { }


        /** Destructor
         *
         *  Returns any reserved key to the key pool.
         *
         **/
        ~CReserveKey()
        {
            if (!fShutdown)
                ReturnKey();
        }


        /* If you copy a CReserveKey that has a key reserved, then keep/return it in one copy, the other  
         * copy becomes invalid but could still be used. For example, it would be a problem if KeepKey() 
         * were called on one copy, then ReturnKey() called on the other. The key would be put back into 
         * the key pool even though it had been used. All of this is why copy disallowed.                
         */

        /** Copy constructor deleted. No copy allowed **/
        CReserveKey(const CReserveKey&) = delete;


        /** Copy assignment operator deleted. No copy allowed **/
        CReserveKey& operator= (const CReserveKey &rhs) = delete;


        /** GetReservedKey
         *
         *  Retrieves the public key value for the currently reserved key. 
         *  If none yet reserved, will first reserve a new key from the key pool of the associated wallet.
         *
         *  @return the public key value of the reserved key
         *
         **/
        std::vector<uint8_t> GetReservedKey();


        /** KeepKey
         *
         *  Marks the reserved key as used, removing it from the key pool.
         *  Must call GetReservedKey first to reserve a key, or this does nothing.
         *
         **/
        void KeepKey();


        /** ReturnKey
         *
         *  Returns a reserved key to the key pool. After call, it is no longer reserved.
         *
         **/
        void ReturnKey();

    };

}

#endif
