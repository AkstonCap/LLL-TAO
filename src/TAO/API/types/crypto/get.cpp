/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/objects.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>
#include <Util/include/encoding.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Returns the public key from the crypto object register for the specified key name, from the specified signature chain. */
        json::json Crypto::Get(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Get the Genesis ID. */
            uint256_t hashGenesis = 0;

            /* Check for specified username / genesis. If no specific genesis or username
             * have been provided then fall back to the active sigchain. */
            if(params.find("genesis") != params.end())
                hashGenesis.SetHex(params["genesis"].get<std::string>());

            /* Check for username. */
            else if(params.find("username") != params.end())
                hashGenesis = TAO::Ledger::SignatureChain::Genesis(params["username"].get<std::string>().c_str());
            
            /* use logged in session. */
            else 
                hashGenesis = users->GetGenesis(users->GetSession(params));
           
            /* Prevent foreign data lookup in client mode */
            if(config::fClient.load() && hashGenesis != users->GetCallersGenesis(params))
                throw APIException(-300, "API can only be used to lookup data for the currently logged in signature chain when running in client mode");

            /* Check genesis exists */
            if(!LLD::Ledger->HasGenesis(hashGenesis))
                throw APIException(-258, "Unknown genesis");

            /* Check the caller included the key name */
            if(params.find("name") == params.end() || params["name"].get<std::string>().empty())
                throw APIException(-88, "Missing name.");
            
            /* Get the requested key name */
            std::string strName = params["name"].get<std::string>();

            /* The address of the crypto object register, which is deterministic based on the genesis */
            TAO::Register::Address hashCrypto = TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);
            
            /* Read the crypto object register */
            TAO::Register::Object crypto;
            if(!LLD::Register->ReadState(hashCrypto, crypto, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-259, "Could not read crypto object register");

            /* Parse the object. */
            if(!crypto.Parse())
                throw APIException(-36, "Failed to parse object register");
            
            /* Check the key exists */
            if(!crypto.CheckName(strName))
                throw APIException(-260, "Invalid key name");

            /* Get List of key names in the crypto object */
            std::vector<std::string> vKeys = crypto.GetFieldNames();        

            /* Get the public key */
            uint256_t hashPublic = crypto.get<uint256_t>(strName);

            /* Populate response JSON */
            ret["name"] = strName;

            /* convert the scheme type to a string */
            switch(hashPublic.GetType())
            {
                case TAO::Ledger::SIGNATURE::FALCON:
                    ret["scheme"] = "FALCON";
                    break;
                case TAO::Ledger::SIGNATURE::BRAINPOOL:
                    ret["scheme"] = "BRAINPOOL";
                    break;
                default:
                    ret["scheme"] = "";

            }
            
            /* Populate the key, base58 encoded */
            ret["key"] = hashPublic == 0 ? "" : encoding::EncodeBase58(hashPublic.GetBytes());

            /* If the caller has requested to filter on a fieldname then filter out the json response to only include that field */
            FilterResponse(params, ret);

            return ret;
        }


        /* Generates private key based on keyname/user/pass/pin and stores it in the keyname slot in the crypto register. */
        json::json Crypto::GetPrivate(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            uint256_t nSession = users->GetSession(params);

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* Check the caller included the key name */
            if(params.find("name") == params.end() || params["name"].get<std::string>().empty())
                throw APIException(-88, "Missing name.");
            
            /* Get the requested key name */
            std::string strName = params["name"].get<std::string>();

            /* Ensure the user has not requested the private key for one of the default keys, only the app or additional 3rd party keys */
            std::set<std::string> setDefaults{"auth", "lisp", "network", "sign", "verify", "cert"};
            if(setDefaults.find(strName) != setDefaults.end())
                throw APIException(-263, "Private key can only be retrieved for app1, app2, app3, and other 3rd-party keys");

            /* The logged in sig chain genesis hash */
            uint256_t hashGenesis = user->Genesis();

            /* The address of the crypto object register, which is deterministic based on the genesis */
            TAO::Register::Address hashCrypto = TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);
            
            /* Read the crypto object register */
            TAO::Register::Object crypto;
            if(!LLD::Register->ReadState(hashCrypto, crypto, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-259, "Could not read crypto object register");

            /* Parse the object. */
            if(!crypto.Parse())
                throw APIException(-36, "Failed to parse object register");
            
            /* Check to see if the key name is valid */
            if(!crypto.CheckName(strName))
                throw APIException(-260, "Invalid key name");
 

            /* Check to see if the the has been generated.  Even though the key is deterministic,  */
            if(crypto.get<uint256_t>(strName) == 0)
                throw APIException(-264, "Key not yet created");

            /* Get the last transaction. */
            uint512_t hashLast;
            if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-138, "No previous transaction found");

            /* Get previous transaction */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::Ledger->ReadTx(hashLast, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-138, "No previous transaction found");

            /* Generate a new transaction hash next using the current credentials so that we can verify them. */
            TAO::Ledger::Transaction tx;
            tx.NextHash(user->Generate(txPrev.nSequence + 1, strPIN, false), txPrev.nNextType);

            /* Check the calculated next hash matches the one on the last transaction in the sig chain. */
            if(txPrev.hashNext != tx.hashNext)
                throw APIException(-139, "Invalid credentials");
                        
            /* Generate the private key */
            uint512_t hashPrivate = user->Generate(strName, 0, strPIN);

            /* Populate the key, hex encoded */
            ret["privatekey"] = hashPrivate.ToString();

            return ret;
        }
    }
}
