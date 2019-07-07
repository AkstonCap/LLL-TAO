/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Default Constructor. */
        Contract::Contract()
        : ssOperation()
        , ssCondition()
        , ssRegister()
        , ptx(nullptr)
        , nFees(0)
        {
        }


        /* Copy Constructor. */
        Contract::Contract(const Contract& contract)
        : ssOperation(contract.ssOperation)
        , ssCondition(contract.ssCondition)
        , ssRegister(contract.ssRegister)
        , ptx(contract.ptx)
        , nFees(contract.nFees)
        {
        }


        /* Move Constructor. */
        Contract::Contract(const Contract&& contract)
        : ssOperation(contract.ssOperation)
        , ssCondition(contract.ssCondition)
        , ssRegister(contract.ssRegister)
        , ptx(contract.ptx)
        , nFees(contract.nFees)
        {
        }


        /* Assignment Operator */
        Contract& Contract::operator=(const Contract& contract)
        {
            /* Set the main streams. */
            ssOperation = contract.ssOperation;
            ssCondition = contract.ssCondition;
            ssRegister  = contract.ssRegister;

            /* Set the transaction reference. */
            ptx         = contract.ptx;

            /* Set the fees. */
            nFees       = contract.nFees;

            return *this;
        }


        /* Bind the contract to a transaction. */
        void Contract::Bind(const TAO::Ledger::Transaction* tx) const
        {
            ptx = const_cast<TAO::Ledger::Transaction*>(tx);
        }


        /* Get the primitive operation. */
        uint8_t Contract::Primitive() const
        {
            /* Sanity checks. */
            if(ssOperation.size() == 0)
                throw debug::exception(FUNCTION, "cannot get primitive when empty");

            /* Return first byte. */
            return ssOperation.get(0);
        }


        /*  Get the fees for contract. */
        const int64_t& Contract::Fees() const
        {
            return nFees;
        }


        /* Get this contract's execution time. */
        const uint64_t& Contract::Timestamp() const
        {
            /* Check for nullptr. */
            if(!ptx)
                throw debug::exception(FUNCTION, "timestamp access for nullptr");

            return ptx->nTimestamp;
        }


        /* Get this contract's caller */
        const uint256_t& Contract::Caller() const
        {
            /* Check for nullptr. */
            if(!ptx)
                throw debug::exception(FUNCTION, "caller access for nullptr");

            return ptx->hashGenesis;
        }


        /* Get the hash of calling tx */
        const uint512_t Contract::Hash() const
        {
            /* Check for nullptr. */
            if(!ptx)
                throw debug::exception(FUNCTION, "hash access for nullptr");

            //TODO: optimize with keeping txid cached
            return ptx->GetHash();
        }


        /* Get the value of the contract if valid */
        bool Contract::Value(uint64_t &nValue) const
        {
            /* Reset the contract. */
            ssOperation.seek(0, STREAM::BEGIN);

            /* Set value. */
            nValue = 0;

            /* Get the operation code.*/
            uint8_t nOP = 0;
            ssOperation >> nOP;

            /* Switch for validate or condition. */
            switch(nOP)
            {
                /* Check for condition. */
                case OP::CONDITION:
                {
                    /* Get next op. */
                    ssOperation >> nOP;

                    break;
                }

                /* Check for validate. */
                case OP::VALIDATE:
                {
                    /* Skip over on validate. */
                    ssOperation.seek(68);

                    /* Get next op. */
                    ssOperation >> nOP;

                    break;
                }
            }

            /* Switch for validate or condition. */
            switch(nOP)
            {
                /* Check for condition. */
                case OP::DEBIT:
                {
                    /* Skip over from and to. */
                    ssOperation.seek(64);

                    /* Get value. */
                    ssOperation >> nValue;

                    break;
                }

                /* Check for condition. */
                case OP::CREDIT:
                {
                    /* Skip over unrelated data. */
                    ssOperation.seek(132);

                    /* Get value. */
                    ssOperation >> nValue;

                    break;
                }

                /* Check for validate. */
                case OP::COINBASE:
                {
                    /* Skip over genesis. */
                    ssOperation.seek(32);

                    /* Get value. */
                    ssOperation >> nValue;

                    break;
                }
            }

            /* Reset before return. */
            ssOperation.seek(0, STREAM::BEGIN);

            return (nValue > 0);
        }


        /* Detect if the stream is empty.*/
        bool Contract::Empty(const uint8_t nFlags) const
        {
            /* Return flag. */
            bool fRet = false;

            /* Check the operations. */
            if(nFlags & OPERATIONS)
                fRet = (fRet || ssOperation.size() == 0);

            /* Check the operations. */
            if(nFlags & CONDITIONS)
                fRet = (fRet || ssCondition.size() == 0);

            /* Check the operations. */
            if(nFlags & REGISTERS)
                fRet = (fRet || ssRegister.size() == 0);

            return fRet;
        }


        /* Reset the internal stream read pointers.*/
        void Contract::Reset(const uint8_t nFlags) const
        {
            /* Check the operations. */
            if(nFlags & OPERATIONS)
                ssOperation.seek(0, STREAM::BEGIN);

            /* Check the operations. */
            if(nFlags & CONDITIONS)
                ssCondition.seek(0, STREAM::BEGIN);

            /* Check the operations. */
            if(nFlags & REGISTERS)
                ssRegister.seek(0, STREAM::BEGIN);
        }


        /* Clears all contract data*/
        void Contract::Clear(const uint8_t nFlags)
        {
            /* Check the operations. */
            if(nFlags & OPERATIONS)
                ssOperation.SetNull();

            /* Check the operations. */
            if(nFlags & CONDITIONS)
                ssCondition.SetNull();

            /* Check the operations. */
            if(nFlags & REGISTERS)
                ssRegister.SetNull();
        }


        /* Get's a size from internal stream. */
        uint64_t Contract::ReadCompactSize(const uint8_t nFlags) const
        {
            /* We don't use masks here, becuase it needs to be exclusive to the stream. */
            switch(nFlags)
            {
                case OPERATIONS:
                    return ::ReadCompactSize(ssOperation);

                case CONDITIONS:
                    return ::ReadCompactSize(ssCondition);

                case REGISTERS:
                    return ::ReadCompactSize(ssRegister);
            }

            return 0;
        }


        /* End of the internal stream.*/
        bool Contract::End(const uint8_t nFlags) const
        {
            /* Return flag. */
            bool fRet = false;

            /* Check the operations. */
            if(nFlags & OPERATIONS)
                fRet = (fRet || ssOperation.end());

            /* Check the operations. */
            if(nFlags & CONDITIONS)
                fRet = (fRet || ssCondition.end());

            /* Check the operations. */
            if(nFlags & REGISTERS)
                fRet = (fRet || ssRegister.end());

            return fRet;
        }


        /* Get the raw operation bytes from the contract.*/
        const std::vector<uint8_t>& Contract::Operations() const
        {
            return ssOperation.Bytes();
        }


        /* Seek the internal operation stream read pointers.*/
        void Contract::Seek(const uint32_t nPos, const uint8_t nFlags, const uint8_t nType) const
        {
            /* Check the operations. */
            if(nFlags & OPERATIONS)
                ssOperation.seek(nPos, nType);

            /* Check the operations. */
            if(nFlags & CONDITIONS)
                ssCondition.seek(nPos, nType);

            /* Check the operations. */
            if(nFlags & REGISTERS)
                ssRegister.seek(nPos, nType);
        }


        /* Seek the internal operation stream read pointers.*/
        void Contract::Rewind(const uint32_t nPos, const uint8_t nFlags) const
        {
            /* Check the operations. */
            if(nFlags & OPERATIONS)
                ssOperation.seek(int32_t(-1 * nPos));

            /* Check the operations. */
            if(nFlags & CONDITIONS)
                ssCondition.seek(int32_t(-1 * nPos));

            /* Check the operations. */
            if(nFlags & REGISTERS)
                ssRegister.seek(int32_t(-1 * nPos));
        }
    }
}
