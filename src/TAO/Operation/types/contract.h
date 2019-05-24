/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_TYPES_CONTRACT_H
#define NEXUS_TAO_OPERATION_TYPES_CONTRACT_H

#include <TAO/Operation/types/stream.h>

#include <TAO/Register/types/stream.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /** Contract
         *
         *  Contains the operation stream, and register pre-states and post states.
         *
         **/
        class Contract
        {
            /** Contract operation stream. **/
            TAO::Operation::Stream ssOperation;


            /** Contract register stream. **/
            TAO::Register::Stream  ssRegister;


        public:

            /** The contract caller genesis-id. **/
            uint256_t hashCaller;


            /** The timestamp contract was created. **/
            uint64_t nTimestamp;


            /** Default Constructor. **/
            Contract()
            : ssOperation()
            , ssRegister()
            , hashCaller(0)
            , nTimestamp(runtime::unifiedtimestamp())
            {
            }


            /** Reset
             *
             *  Reset the internal stream read pointers.
             *
             **/
            void Reset()
            {
                /* Set the operation stream to beginning. */
                ssOperation.seek(0, STREAM::BEGIN);

                /* Set the register stream to beginning. */
                ssRegister.seek(0, STREAM::BEGIN);
            }


            /** Operator Overload <<
             *
             *  Serializes data into vchOperations
             *
             *  @param[in] obj The object to serialize into ledger data
             *
             **/
            template<typename Type>
            Contract& operator<<(const Type& obj)
            {
                /* Serialize to the stream. */
                ssOperation << obj;

                return (*this);
            }


            /** Operator Overload <<
             *
             *  Serializes data into vchOperations
             *
             *  @param[in] obj The object to serialize into ledger data
             *
             **/
            template<typename Type>
            const Contract& operator>>(Type& obj) const
            {
                /* Serialize to the stream. */
                ssOperation >> obj;

                return (*this);
            }


            /** Operator Overload <=
             *
             *  Serializes data into ssRegister
             *
             *  @param[in] obj The object to serialize into ledger data
             *
             **/
            template<typename Type>
            Contract& operator<<=(const Type& obj)
            {
                /* Serialize to the stream. */
                ssRegister << obj;

                return (*this);
            }


            /** Operator Overload >=
             *
             *  Serializes data into ssRegister
             *
             *  @param[in] obj The object to serialize into ledger data
             *
             **/
            template<typename Type>
            const Contract& operator>>=(Type& obj) const
            {
                /* Serialize to the stream. */
                ssRegister >> obj;

                return (*this);
            }
        };
    }
}

#endif
