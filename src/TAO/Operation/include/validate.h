/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_INCLUDE_VALIDATE_H
#define NEXUS_TAO_OPERATION_INCLUDE_VALIDATE_H

#include <TAO/Operation/include/stream.h>

#include <TAO/Register/types/basevm.h>
#include <TAO/Register/types/value.h>

#include <TAO/Ledger/types/transaction.h>

namespace TAO
{

    namespace Operation
    {

        /** Validate
         *
         *  An object to handle the executing of validation scripts.
         *
         **/
        class Validate : public TAO::Register::BaseVM
        {

            /** Computational limits for validation script. **/
            int32_t nLimits;


            /** Reference of the stream that script exists on. **/
            const Stream& ssOperations;


            /** Reference of the transaction executing script. **/
            const TAO::Ledger::Transaction& tx;


            /** The stream position this script starts on. **/
            const uint64_t nStreamPos;


        public:

            /** Default constructor. **/
            Validate(const Stream& ssOperationIn, const TAO::Ledger::Transaction& txIn, int32_t nLimitsIn = 2048);


            /** Copy constructor. **/
            Validate(const Validate& in);


            /** Reset
             *
             *  Reset the validation script for re-executing.
             *
             **/
            void Reset();


            /** Execute
             *
             *  Execute the validation script.
             *
             **/
            bool Execute();


            /** GetValue
             *
             *  Get a value from the register virtual machine.
             *
             **/
            bool GetValue(TAO::Register::Value& vRet);
        };
    }
}

#endif
