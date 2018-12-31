/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_TYPES_TRANSACTION_H
#define NEXUS_TAO_LEDGER_TYPES_TRANSACTION_H

#include <vector>

#include <LLC/types/uint1024.h>

#include <Util/include/runtime.h>
#include <Util/templates/serialize.h>

namespace TAO::Ledger
{
    /** Tritium Transaction.
     *
     *  State of a tritium specific transaction.
     *
     **/
    class Transaction //transaction header size is 144 bytes
    {
    public:

        /** The transaction version. **/
        uint32_t nVersion;


        /** The sequence identifier. **/
        uint32_t nSequence;


        /** The transaction timestamp. **/
        uint64_t nTimestamp;


        /** The nextHash which can claim the signature chain. */
        uint256_t hashNext;


        /** The genesis ID hash. **/
        uint256_t hashGenesis;


        /** The previous transaction. **/
        uint512_t hashPrevTx;


        /** The data to be recorded in the ledger. **/
        std::vector<uint8_t> vchOperations;


        //memory only, to be disposed once fully locked into the chain behind a checkpoint
        //this is for the segregated keys from transaction data.
        std::vector<uint8_t> vchPubKey;
        std::vector<uint8_t> vchSig;


        //flag to determine if tx has been connected
        bool fConnected;


        //memory only read position
        uint32_t nReadPos;


        //serialization methods
        IMPLEMENT_SERIALIZE
        (

            READWRITE(this->nVersion);
            READWRITE(nSequence);
            READWRITE(nTimestamp);
            READWRITE(hashNext);

            if(!(nSerType & SER_GENESISHASH)) //genesis hash is not serialized
                READWRITE(hashGenesis);

            READWRITE(hashPrevTx);
            READWRITE(vchOperations);
            READWRITE(vchPubKey);

            if(!(nSerType & SER_GETHASH))
            {
                READWRITE(vchSig);
            }

            READWRITE(fConnected);
        )


        /** Default Constructor. **/
        Transaction()
        : nVersion(1)
        , nSequence(0)
        , nTimestamp(runtime::unifiedtimestamp())
        , hashNext(0)
        , hashGenesis(0)
        , hashPrevTx(0)
        , fConnected(false)
        , nReadPos(0) {}


        /** read
         *
         *  Reads raw data from the stream
         *
         *  @param[in] pch The pointer to beginning of memory to write
         *
         *  @param[in] nSize The total number of bytes to read
         *
         **/
        Transaction& read(char* pch, int nSize)
        {
            /* Check size constraints. */
            if(nReadPos + nSize > vchOperations.size())
                throw std::runtime_error(debug::strprintf(FUNCTION "reached end of stream %u", __PRETTY_FUNCTION__, nReadPos));

            /* Copy the bytes into tmp object. */
            std::copy((uint8_t*)&vchOperations[nReadPos], (uint8_t*)&vchOperations[nReadPos] + nSize, (uint8_t*)pch);

            /* Iterate the read position. */
            nReadPos += nSize;

            return *this;
        }


        /** write
         *
         *  Writes data into the stream
         *
         *  @param[in] pch The pointer to beginning of memory to write
         *
         *  @param[in] nSize The total number of bytes to copy
         *
         **/
        Transaction& write(const char* pch, int nSize)
        {
            /* Push the obj bytes into the vector. */
            vchOperations.insert(vchOperations.end(), (uint8_t*)pch, (uint8_t*)pch + nSize);

            return *this;
        }


        /** Operator Overload <<
         *
         *  Serializes data into vchOperations
         *
         *  @param[in] obj The object to serialize into ledger data
         *
         **/
        template<typename Type> Transaction& operator<<(const Type& obj)
        {
            /* Serialize to the stream. */
            ::Serialize(*this, obj, SER_OPERATIONS, nVersion); //temp versinos for now

            return (*this);
        }


        /** Operator Overload >>
         *
         *  Serializes data into vchOperations
         *
         *  @param[out] obj The object to de-serialize from ledger data
         *
         **/
        template<typename Type> Transaction& operator>>(Type& obj)
        {
            /* Unserialize from the stream. */
            ::Unserialize(*this, obj, SER_OPERATIONS, nVersion);
            return (*this);
        }


        /** IsValid
         *
         *  Determines if the transaction is a valid transaciton and passes ledger level checks.
         *
         *  @return true if transaction is valid.
         *
         **/
        bool IsValid() const;


        /** Is Coinbase
         *
         *  Determines if the transaction is a coinbase transaction.
         *
         *  @return true if transaction is a coinbase.
         *
         **/
        bool IsCoinbase() const;


        /** Is Trust
         *
         *  Determines if the transaction is a trust transaction.
         *
         *  @return true if transaction is a coinbase.
         *
         **/
        bool IsTrust() const;


        /** IsGenesis
         *
         *  Determines if the transaction is a genesis transaction
         *
         *  @return true if transaction is genesis
         *
         **/
        bool IsGenesis() const;


        /** GetHash
         *
         *  Gets the hash of the transaction object.
         *
         *  @return 512-bit unsigned integer of hash.
         *
         **/
        uint512_t GetHash() const;


        /** Genesis
         *
         *  Gets the hash of the genesis transaction
         *
         *  @return 256-bit unsigned integer of hash.
         *
         **/
        uint256_t Genesis() const;


        /** NextHash
         *
         *  Sets the Next Hash from the key
         *
         *  @param[in] hashSecret The secret phrase to generate the keys.
         *
         **/
        void NextHash(uint512_t hashSecret);


        /** PrevHash
         *
         *  Gets the nextHash from the previous transaction
         *
         *  @return 256-bit hash of previous transaction
         *
         **/
        uint256_t PrevHash() const;


        /** Sign
         *
         *  Signs the transaction with the private key and sets the public key
         *
         *  @param[in] hashSecret The secret phrase to generate the keys.
         *
         **/
         bool Sign(uint512_t hashSecret);



         /** Print
          *
          * Prints the object to the console.
          *
          **/
         void print() const;

    };
}

#endif
