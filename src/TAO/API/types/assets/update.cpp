/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/templates/datastream.h>

#include <Util/encoding/include/convert.h>
#include <Util/encoding/include/base64.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Update the data in an asset */
        json::json Assets::Update(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            uint256_t nSession = users->GetSession(params);

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* Lock the signature chain. */
            LOCK(users->CREATE_MUTEX);

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!Users::CreateTransaction(user, strPIN, tx))
                throw APIException(-17, "Failed to create transaction");


            /* Get the Register ID. */
            TAO::Register::Address hashRegister ;

            /* Check whether the caller has provided the asset name parameter. */
            if(params.find("name") != params.end())
            {
                /* If name is provided then use this to deduce the register address */
                hashRegister = Names::ResolveAddress(params, params["name"].get<std::string>());
            }

            /* Otherwise try to find the raw base58 encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetBase58(params["address"].get<std::string>());

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-33, "Missing name / address");


            /* Check for format parameter. */
            std::string strFormat = "JSON"; // default to json format if no foramt is specified
            if(params.find("format") != params.end())
                strFormat = params["format"].get<std::string>();

            /* Get the asset from the register DB.  We can read it as an Object.
               If this fails then we try to read it as a base State type and assume it was
               created as a raw format asset */
            TAO::Register::Object asset;
            if(!LLD::Register->ReadState(hashRegister, asset, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-34, "Asset not found");

            /* Check that this is an updatable object, i.e. not a raw / append obejct */
            if(asset.nType != TAO::Register::REGISTER::OBJECT)
                throw APIException(-155, "Raw assets can not be updated");

            /* Ensure that the object is an asset */
            if(!asset.Parse() || asset.Standard() != TAO::Register::OBJECTS::NONSTANDARD)
                throw APIException(-35, "Specified name/address is not an asset");

            /* Declare operation stream to serialize all of the field updates*/
            TAO::Operation::Stream ssOperationStream;

            if(strFormat == "JSON")
            {
                /* Iterate through each field definition */
                for(auto it = params.begin(); it != params.end(); ++it)
                {
                    /* Skip any incoming parameters that are keywords used by this API method*/
                    if(it.key() == "pin"
                    || it.key() == "PIN"
                    || it.key() == "session"
                    || it.key() == "name"
                    || it.key() == "address"
                    || it.key() == "format")
                    {
                        continue;
                    }

                    if(it->is_string())
                    {
                        /* Get the key / value from the JSON*/
                        std::string strDataField = it.key();
                        std::string strValue = it->get<std::string>();

                        /* Check that the data field exists in the asset */
                        uint8_t nType = TAO::Register::TYPES::UNSUPPORTED;
                        if(!asset.Type(strDataField, nType))
                            throw APIException(-156, debug::safe_printstr("Field not found in asset ", strDataField));

                        if(!asset.Check(strDataField, nType, true))
                            throw APIException(-157, debug::safe_printstr("Field not mutable in asset ", strDataField));

                        /* Convert the incoming value to the correct type and write it into the asset object */
                        if(nType == TAO::Register::TYPES::UINT8_T)
                            ssOperationStream << strDataField << uint8_t(TAO::Operation::OP::TYPES::UINT8_T) << uint8_t(stol(strValue));
                        else if(nType == TAO::Register::TYPES::UINT16_T)
                            ssOperationStream << strDataField << uint8_t(TAO::Operation::OP::TYPES::UINT16_T) << uint16_t(stol(strValue));
                        else if(nType == TAO::Register::TYPES::UINT32_T)
                            ssOperationStream << strDataField << uint8_t(TAO::Operation::OP::TYPES::UINT32_T) << uint32_t(stol(strValue));
                        else if(nType == TAO::Register::TYPES::UINT64_T)
                            ssOperationStream << strDataField << uint8_t(TAO::Operation::OP::TYPES::UINT64_T) << uint64_t(stol(strValue));
                        else if(nType == TAO::Register::TYPES::UINT256_T)
                            ssOperationStream << strDataField << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << uint256_t(strValue);
                        else if(nType == TAO::Register::TYPES::UINT512_T)
                            ssOperationStream << strDataField << uint8_t(TAO::Operation::OP::TYPES::UINT512_T) << uint512_t(strValue);
                        else if(nType == TAO::Register::TYPES::UINT1024_T)
                            ssOperationStream << strDataField << uint8_t(TAO::Operation::OP::TYPES::UINT1024_T) << uint1024_t(strValue);
                        else if(nType == TAO::Register::TYPES::STRING)
                        {
                            /* Check that the incoming value is not longer than the current value */
                            size_t nMaxLength = asset.Size(strDataField);
                            if(strValue.length() > nMaxLength)
                                throw APIException(-158, debug::safe_printstr("Value longer than maximum length ", strDataField));

                            /* Ensure that the serialized value is padded out to the max length */
                            strValue.resize(nMaxLength);

                            ssOperationStream << strDataField << uint8_t(TAO::Operation::OP::TYPES::STRING) << strValue;
                        }
                        else if(nType == TAO::Register::TYPES::BYTES)
                        {
                            bool fInvalid = false;
                            std::vector<uint8_t> vchBytes = encoding::DecodeBase64(strValue.c_str(), &fInvalid);


                            if(fInvalid)
                                throw APIException(-5, "Malformed base64 encoding");

                            /* Check that the incoming value is not longer than the current value */
                            size_t nMaxLength = asset.Size(strDataField);
                            if(vchBytes.size() > nMaxLength)
                                throw APIException(-158, debug::safe_printstr("Value longer than maximum length ", strDataField));

                            /* Ensure that the serialized value is padded out to the max length */
                            vchBytes.resize(nMaxLength);

                            ssOperationStream << strDataField << uint8_t(TAO::Operation::OP::TYPES::BYTES) << vchBytes;
                        }
                    }
                    else
                    {
                        throw APIException(-159, "Values must be passed in as strings");
                    }
                }
            }

            /* Create the transaction object script. */
            tx[0] << uint8_t(TAO::Operation::OP::WRITE) << hashRegister << ssOperationStream.Bytes();

            /* Finalize the transaction. */
            BuildAndAccept(tx, users->GetKey(tx.nSequence, strPIN, nSession));

            /* Build a JSON response object. */
            ret["txid"]  = tx.GetHash().ToString();
            ret["address"] = hashRegister.ToString();


            return ret;
        }
    }
}
