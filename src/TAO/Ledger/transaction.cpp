/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLC/hash/SK.h>
#include <LLC/hash/macro.h>

#include <LLC/include/flkey.h>
#include <LLC/include/eckey.h>

#include <LLD/include/global.h>

#include <LLP/include/version.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/stake.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/mempool.h>

#include <TAO/Operation/include/enum.h>

#include <Util/templates/datastream.h>
#include <Util/include/hex.h>
#include <Util/include/debug.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Determines if the transaction is a valid transaciton and passes ledger level checks. */
        bool Transaction::Check() const
        {

            /* Checks for coinbase. */
            if(IsCoinbase())
            {
                /* Check the coinbase size. */
                if(ssOperation.size() != 17)
                    return debug::error(FUNCTION, "operation data inconsistent with fixed size rules ", ssOperation.size());
            }

            /* Check for genesis valid numbers. */
            if(hashGenesis == 0)
                return debug::error(FUNCTION, "genesis cannot be zero");

            /* Check for genesis valid numbers. */
            if(hashNext == 0)
                return debug::error(FUNCTION, "nextHash cannot be zero");

            /* Check the timestamp. */
            if(nTimestamp > runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT)
                return debug::error(FUNCTION, "transaction timestamp too far in the future ", nTimestamp);

            /* Check the size constraints of the ledger data. */
            if(ssOperation.size() > 1024) //TODO: implement a constant max size
                return debug::error(FUNCTION, "ledger data outside of maximum size constraints");

            /* Check for empty signatures. */
            if(vchSig.size() == 0)
                return debug::error(FUNCTION, "transaction with empty signature");

            /* Switch based on signature type. */
            switch(nKeyType)
            {
                /* Support for the FALCON signature scheeme. */
                case SIGNATURE::FALCON:
                {
                    /* Create the FL Key object. */
                    LLC::FLKey key;

                    /* Set the public key and verify. */
                    key.SetPubKey(vchPubKey);
                    if(!key.Verify(GetHash().GetBytes(), vchSig))
                        return debug::error(FUNCTION, "invalid transaction signature");

                    break;
                }

                /* Support for the BRAINPOOL signature scheme. */
                case SIGNATURE::BRAINPOOL:
                {
                    /* Create EC Key object. */
                    LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the public key and verify. */
                    key.SetPubKey(vchPubKey);
                    if(!key.Verify(GetHash().GetBytes(), vchSig))
                        return debug::error(FUNCTION, "invalid transaction signature");

                    break;
                }

                default:
                    return debug::error(FUNCTION, "unknown signature type");
            }

            return true;
        }


        /* Accept a transaction object into the main chain. */
        bool Transaction::Accept() const
        {
            /* Accept to memory pool. */
            return mempool.Accept(GetHash(), *this);
        }


        /* Verify a transaction contracts. */
        bool Transaction::Verify() const
        {
            /* Run through all the contracts. */
            for(const auto& contract : vContracts)
                if(!TAO::Register::Verify(contract, TAO::Register::FLAGS::MEMPOOL))
                    return debug::error(FUNCTION, "transaction register layer failed to verify");

            return true;
        }


        /* Build the transaction contracts. */
        bool Transaction::Build()
        {
            /* Run through all the contracts. */
            for(auto& contract : vContracts)
                if(!TAO::Register::Calculate(contract))
                    return debug::error(FUNCTION, "transaction register layer failed to verify");

            return true;
        }


        /* Connect a transaction object to the main chain. */
        bool Transaction::Connect(const uint8_t nFlags) const
        {
            /* Check for first. */
            if(tx.IsFirst())
            {
                //Check for duplicate genesis

                /* Write the Genesis to disk. */
                if((nFlags & TAO::Register::FLAGS::WRITE) && !LLD::legDB->WriteGenesis(tx.hashGenesis, hash))
                    return debug::error(FUNCTION, "failed to write genesis");
            }
            else
            {
                /* Make sure the previous transaction is on disk or mempool. */
                TAO::Ledger::Transaction txPrev;
                if(!LLD::legDB->ReadTx(hashPrevTx, txPrev, nFlags))
                    return debug::error(FUNCTION, "prev transaction not on disk");

                /* Double check sequence numbers here. */
                if(txPrev.nSequence + 1 != nSequence)
                    return debug::error(FUNCTION, "prev transaction incorrect sequence");

                /* Check the previous next hash that is being claimed. */
                if(txPrev.hashNext != PrevHash())
                    return debug::error(FUNCTION, "next hash mismatch with previous transaction");

                /* Check the previous sequence number. */
                if(txPrev.nSequence + 1 != nSequence)
                    return debug::error(FUNCTION, "prev sequence ", txPrev.nSequence, " broken ", nSequence);

                /* Check the previous genesis. */
                if(txPrev.hashGenesis != hashGenesis)
                    return debug::error(FUNCTION,
                        "genesis ", txPrev.hashGenesis.SubString(),
                        " broken ",     tx.hashGenesis.SubString());

                /* Check previous transaction from disk hash. */
                if(txPrev.GetHash() != hashPrevTx) //NOTE: this is being extra paranoid. Consider removing.
                    return debug::error(FUNCTION, "prev transaction prevhash mismatch");

                /* Write specific transaction flags. */
                if(nFlags & TAO::Register::FLAGS::WRITE)
                {
                    /* Check previous transaction next pointer. */
                    if(!txPrev.IsHead())
                        return debug::error(FUNCTION, "prev transaction not head of sigchain");

                    /* Set the previous transactions next hash. */
                    txPrev.hashNextTx = hash;

                    /* Write the next pointer. */
                    if(!LLD::legDB->WriteTx(tx.hashPrevTx, txPrev))
                        return debug::error(FUNCTION, "failed to write last tx");

                    /* Set the proper next pointer. */
                    hashNextTx = STATE::HEAD;
                    if(!LLD::legDB->WriteTx(hash, *this))
                        return debug::error(FUNCTION, "failed to write valid next pointer");
                }

                return true;
            }

            /* Run through all the contracts. */
            for(const auto& contract : vContracts)
                if(!TAO::Operation::Execute(contract, nFlags))
                    return false;

            return true;
        }


        /* Disconnect a transaction object to the main chain. */
        bool Transaction::Disconnect() const
        {
            /* Set the proper next pointer. */
            hashNextTx = STATE::UNCONFIRMED;
            if(!LLD::legDB->WriteTx(GetHash(), *this))
                return debug::error(FUNCTION, "failed to write valid next pointer");

            /* Erase last for genesis. */
            if(IsFirst() && !LLD::legDB->EraseLast(hashGenesis))
                return debug::error(FUNCTION, "failed to erase last hash");
            else
            {
                /* Make sure the previous transaction is on disk. */
                TAO::Ledger::Transaction txPrev;
                if(!LLD::legDB->ReadTx(hashPrevTx, txPrev))
                    return debug::error(FUNCTION, "prev transaction not on disk");

                /* Set the proper next pointer. */
                txPrev.hashNextTx = STATE::HEAD;
                if(!LLD::legDB->WriteTx(hashPrevTx, txPrev))
                    return debug::error(FUNCTION, "failed to write valid next pointer");

                /* Write proper last hash index. */
                if(!LLD::legDB->WriteLast(hashGenesis, hashPrevTx))
                    return debug::error(FUNCTION, "failed to write last hash");

            }

            /* Run through all the contracts. */
            for(const auto& contract : vContracts)
                if(!TAO::Register::Rollback(contract))
                    return false;

            return true;
        }


        /* Determines if the transaction is a coinbase transaction. */
        bool Transaction::IsCoinbase() const
        {
            if(ssOperation.size() == 0)
                return false;

            return ssOperation.get(0) == TAO::Operation::OP::COINBASE;
        }


        /* Determines if the transaction is for a private block. */
        bool Transaction::IsPrivate() const
        {
            if(ssOperation.size() == 0)
                return false;

            return ssOperation.get(0) == TAO::Operation::OP::AUTHORIZE;
        }


        /* Determines if the transaction is a coinstake transaction. */
        bool Transaction::IsTrust() const
        {
            if(ssOperation.size() == 0)
                return false;

            return ssOperation.get(0) == TAO::Operation::OP::TRUST;
        }


        /* Determines if the transaction is at head of chain. */
        bool Transaction::IsHead() const
        {
            return hashNextTx == STATE::HEAD;
        }


        /* Determines if the transaction is confirmed in the chain. */
        bool Transaction::IsConfirmed() const
        {
            return (hashNextTx != STATE::UNCONFIRMED);
        }


        /* Determines if the transaction is a genesis transaction */
        bool Transaction::IsGenesis() const
        {
            if(ssOperation.size() == 0)
                return false;

            return ssOperation.get(0) == TAO::Operation::OP::GENESIS;
        }


        /* Determines if the transaction is a genesis transaction */
        bool Transaction::IsFirst() const
        {
            return (nSequence == 0 && hashPrevTx == 0);
        }


        /* Gets the hash of the transaction object. */
        uint512_t Transaction::GetHash() const
        {
            DataStream ss(SER_GETHASH, nVersion);
            ss << *this;

            return LLC::SK512(ss.begin(), ss.end());
        }


        /* Sets the Next Hash from the key */
        void Transaction::NextHash(const uint512_t& hashSecret, const uint8_t nType)
        {
            /* Get the secret from new key. */
            std::vector<uint8_t> vBytes = hashSecret.GetBytes();
            LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

            /* Switch based on signature type. */
            switch(nType)
            {

                /* Support for the FALCON signature scheeme. */
                case SIGNATURE::FALCON:
                {
                    /* Create the FL Key object. */
                    LLC::FLKey key;

                    /* Set the secret key. */
                    if(!key.SetSecret(vchSecret, true))
                        return;

                    /* Calculate the next hash. */
                    hashNext = LLC::SK256(key.GetPubKey());

                    break;
                }

                /* Support for the BRAINPOOL signature scheme. */
                case SIGNATURE::BRAINPOOL:
                {
                    /* Create EC Key object. */
                    LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the secret key. */
                    if(!key.SetSecret(vchSecret, true))
                        return;

                    /* Calculate the next hash. */
                    hashNext = LLC::SK256(key.GetPubKey());

                    break;
                }

                default:
                {
                    /* Unsupported (this is a failure flag). */
                    hashNext = 0;

                    break;
                }
            }
        }


        /* Gets the nextHash for the previous transaction */
        uint256_t Transaction::PrevHash() const
        {
            return LLC::SK256(vchPubKey);
        }


        /* Signs the transaction with the private key and sets the public key */
        bool Transaction::Sign(const uint512_t& hashSecret)
        {
            /* Get the secret from new key. */
            std::vector<uint8_t> vBytes = hashSecret.GetBytes();
            LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

            /* Switch based on signature type. */
            switch(nKeyType)
            {

                /* Support for the FALCON signature scheeme. */
                case SIGNATURE::FALCON:
                {
                    /* Create the FL Key object. */
                    LLC::FLKey key;

                    /* Set the secret key. */
                    if(!key.SetSecret(vchSecret, true))
                        return false;

                    /* Set the public key. */
                    vchPubKey = key.GetPubKey();

                    /* Sign the hash. */
                    return key.Sign(GetHash().GetBytes(), vchSig);
                }

                /* Support for the BRAINPOOL signature scheme. */
                case SIGNATURE::BRAINPOOL:
                {
                    /* Create EC Key object. */
                    LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the secret key. */
                    if(!key.SetSecret(vchSecret, true))
                        return false;

                    /* Set the public key. */
                    vchPubKey = key.GetPubKey();

                    /* Sign the hash. */
                    return key.Sign(GetHash().GetBytes(), vchSig);
                }
            }

            return false;
        }


        /* Create a transaction string. */
        std::string Transaction::ToString() const
        {
            return debug::safe_printstr
            (
                IsGenesis() ? "Genesis" : "Tritium", "(",
                "nVersion = ", nVersion, ", ",
                "nSequence = ", nSequence, ", ",
                "nTimestamp = ", nTimestamp, ", ",
                "hashNext  = ",  hashNext.ToString().substr(0, 20), ", ",
                "hashPrevTx = ", hashPrevTx.ToString().substr(0, 20), ", ",
                "hashGenesis = ", hashGenesis.ToString().substr(0, 20), ", ",
                "pub = ", HexStr(vchPubKey).substr(0, 20), ", ",
                "sig = ", HexStr(vchSig).substr(0, 20), ", ",
                "hash = ", GetHash().ToString().substr(0, 20), ", ",
                "register.size() = ", ssRegister.size(), ", ",
                "operation.size() = ", ssOperation.size(), ")"
            );
        }


        /* Debug Output. */
        void Transaction::print() const
        {
            debug::log(0, ToString());
        }

        /* Short form of the debug output. */
        std::string Transaction::ToStringShort() const
        {
            std::string str;
            std::string txtype = GetTxTypeString();
            str += debug::safe_printstr(GetHash().ToString(), " ", txtype);
            return str;
        }

        /*  User readable description of the transaction type. */
        std::string Transaction::GetTxTypeString() const
        {
            std::string txtype = "tritium ";
            if(IsCoinbase())
                txtype += "base";
            else if(IsFirst())
                txtype += "first";
            else if(IsTrust())
                txtype += "trust";
            else if(IsGenesis())
                txtype += "genesis";
            else
                txtype += "user";

            return txtype;
        }
    }
}
