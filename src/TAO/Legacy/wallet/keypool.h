/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
        
            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_KEYPOOL_H
#define NEXUS_LEGACY_WALLET_KEYPOOL_H

#include <set>
#include <vector>

namespace Legacy
{

    namespace Wallet
    {
        /* forward declarations */    
        class CKeyPoolEntry;
        class CWallet;


        /** @class CKeyPool
         *
         *  Implementation for managing a key pool, backed by the wallet database
         *  of the wallet containing the pool.
         *
         *  A CKeyPool instance contains the pool indexes of the CKeyPoolEntry values
         *  (public keys) stored in the wallet database, and supports adding/retrieving 
         *  them as needed.
         *
         *  Keys may also be reserved for potential use, then kept (removed from pool) 
         *  if actually used, or returned (unreserved) if not used.
         *
         *  The containing wallet manages and stores the private keys associated
         *  with these pool entries. They are not part of key pool itself.
         *
         *  Key pool usage requires the containing wallet be file backed, because
         *  the wallet database is used to store the actual key pool entries. If
         *  the containing wallet is not file backed, using the key pool collapses
         *  into an empty pool of limited use.
         *
         **/
        class CKeyPool
        {
        public:
            /** Defines the default number of keys contained by a key pool **/
            static const int DEFAULT_KEY_POOL_SIZE = 100;

        private:
            /** 
             *  A set containing the key pool index values for all the keys in the pool. 
             *  The actual keys pool entries are stored in the wallet database and
             *  retrieved using these index values.
             **/ 
            std::set<int64_t> setKeyPool;


            /** The wallet containing this key pool **/
            CWallet* pWallet;


        public:
            /** Constructor
             *
             *  Initializes a key pool associated with a given wallet.
             *
             *  @param[in] pWalletIn The wallet containing this key pool
             *
             **/
            CKeyPool(CWallet* pWalletIn) : pWallet(pWalletIn) {}


            /** Copy constructor deleted. No copy allowed **/
            CKeyPool(const CKeyPool&) = delete;


            /** Copy assignment operator deleted. No copy allowed **/
            CKeyPool& operator= (const CKeyPool &rhs) = delete;


            /** NewKeyPool
             *
             *  Clears any existing keys in the pool and the wallet database
             *  and generates a completely new key set.
             *
             *  New key pool will have KeyPool::DEFAULT_KEY_POOL_SIZE entries
             *  unless the value is overridden by startup arguments.
             *
             *  @return false if wallet locked and unable to add keys, true otherwise
             *
             **/
            bool NewKeyPool();


            /** TopUpKeyPool
             *
             *  Adds keys to key pool to top up the number of entries. Does
             *  nothing if key pool already full.
             *
             *  Fills pool to KeyPool::DEFAULT_KEY_POOL_SIZE entries
             *  unless the value is overridden by startup arguments.
             *
             *  @return false if wallet locked and unable to add keys, true otherwise
             *
             **/
            bool TopUpKeyPool();


            /** ClearKeyPool
             *
             *  Empties the current key pool. 
             *
             **/
            inline void ClearKeyPool() 
            {
                setKeyPool.clear();
            }


            /** AddKey
             *
             *  Manually adds a key pool entry. This only adds the entry to the pool.
             *  If used, a separate call to CWallet::GenerateNewKey is required to
             *  create the key and record the private key in the wallet database. 
             *  Otherwise the key pool entry added here is useless.
             *
             *  @param[in] keypoolEntry The key pool entry to add
             *
             *  @return pool index of the added entry, or -1 if not successful
             *
             **/
            int64_t AddKey(const CKeyPoolEntry& keypoolEntry);


            /** GetKeyPoolSize
             *
             *  Retrieves the current number of keys in the pool
             *
             *  @return key pool size
             *
             **/
            inline int GetKeyPoolSize() const
            {
                return setKeyPool.size();
            }


            /** GetKeyFromPool
             *
             *  Extracts a key from the key pool. This both reserves and keeps the key,
             *  removing it from the pool.
             *
             *  @param[out] key The value (public key) of the key pool entry extracted
             *
             *  @param[in] fUseDefaultWhenEmpty true  = Use the wallet default key if the pool is empty
             *                                  false = Generate a new key if the pool is empty
             *
             *  @return true if the public key value was successfully extracted
             *
             **/
            bool GetKeyFromPool(std::vector<uint8_t> &key, const bool fUseDefaultWhenEmpty=true);


            /** ReserveKeyFromPool
             *
             *  Reserves a key pool entry out of this key pool. After reserving it, the
             *  key pool entry is unavailable for other use. 
             *
             *  Reserved keys can be returned to the pool by calling ReturnKey() or removed 
             *  entirely by calling KeepKey(). Shutting down and restarting has the same 
             *  effect as ReturnKey().
             *
             *  @param[out] nPoolIndex The pool index of the reserved key
             *
             *  @param[out] keypoolEntry The reserved key pool entry 
             *
             *  @see KeepKey() ReturnKey()
             *
             **/
            void ReserveKeyFromPool(int64_t& nPoolIndex, CKeyPoolEntry& keypoolEntry);


            /** KeepKey
             *
             *  Marks a reserved key as used, removing it from the key pool.
             *
             *  @param[in] nPoolIndex The pool index of the reserved key
             *
             **/
            void KeepKey(const int64_t nPoolIndex);


            /** ReturnKey
             *
             *  Returns a reserved key to the key pool. After call, it is no longer 
             *  reserved and available again in the pool for other use.
             *
             *  @param[in] nPoolIndex The pool index of the reserved key
             *
             **/
            void ReturnKey(const int64_t nPoolIndex);


            /** GetOldestKeyPoolTime
             *
             *  Retrieves the creation time for the pool's oldest entry.
             *
             *  @return timestamp of oldest key pool entry in the pool
             *
             **/
            int64_t GetOldestKeyPoolTime();

        };

}
}
