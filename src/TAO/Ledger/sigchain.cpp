/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLC/hash/SK.h>
#include <LLC/hash/macro.h>

#include <LLC/include/argon2.h>
#include <LLC/include/flkey.h>
#include <LLC/include/eckey.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/genesis.h>

#include <TAO/Ledger/include/enum.h>

#include <TAO/Register/include/names.h>
#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Copy Constructor */
        SignatureChain::SignatureChain(const SignatureChain& sigchain)
        : strUsername (sigchain.strUsername.c_str())
        , strPassword (sigchain.strPassword.c_str())
        , MUTEX       ( )
        , cacheKeys   (sigchain.cacheKeys)
        , hashGenesis (sigchain.hashGenesis)
        {
        }


        /** Move Constructor **/
        SignatureChain::SignatureChain(SignatureChain&& sigchain) noexcept
        : strUsername (std::move(sigchain.strUsername.c_str()))
        , strPassword (std::move(sigchain.strPassword.c_str()))
        , MUTEX       ( )
        , cacheKeys   (std::move(sigchain.cacheKeys))
        , hashGenesis (std::move(sigchain.hashGenesis))
        {
        }


        /** Destructor. **/
        SignatureChain::~SignatureChain()
        {
        }


        /* Constructor to generate Keychain */
        SignatureChain::SignatureChain(const SecureString& strUsernameIn, const SecureString& strPasswordIn)
        : strUsername (strUsernameIn.c_str())
        , strPassword (strPasswordIn.c_str())
        , MUTEX       ( )
        , cacheKeys   (5)
        , hashGenesis (SignatureChain::Genesis(strUsernameIn))
        {
        }


        /* This function is responsible for returning the genesis ID.*/
        uint256_t SignatureChain::Genesis() const
        {
            return hashGenesis;
        }


        /* This function is responsible for generating the genesis ID.*/
        uint256_t SignatureChain::Genesis(const SecureString& strUsername)
        {
            /* The Genesis hash to return */
            uint256_t hashGenesis;

            /* Check the local DB first */
            if(LLD::Local->ReadGenesis(strUsername, hashGenesis))
                return hashGenesis;

            /* Generate the Secret Phrase */
            std::vector<uint8_t> vUsername(strUsername.begin(), strUsername.end());

            /* Default salt */
            std::vector<uint8_t> vSalt(16);

            /* Empty vector used for Argon2 call */
            std::vector<uint8_t> vEmpty;

            /* Argon2 hash the secret */
            hashGenesis = LLC::Argon2_256(vUsername, vSalt, vEmpty, 12, (1 << 16));
            
            /* Set the genesis type byte. */
            hashGenesis.SetType(TAO::Ledger::GenesisType());

            /* Cache this username-genesis pair in the local db*/
            LLD::Local->WriteGenesis(strUsername, hashGenesis);

            return hashGenesis;
        }


        /*
         *  This function is responsible for genearting the private key in the keychain of a specific account.
         *  The keychain is a series of keys seeded from a secret phrase and a PIN number.
         */
        uint512_t SignatureChain::Generate(const uint32_t nKeyID, const SecureString& strSecret, bool fCache) const
        {
            /* key used to identify this private key in the key cache */
            std::tuple<SecureString, SecureString, uint32_t> cacheKey = std::make_tuple(strPassword, strSecret, nKeyID);

            /* If caching is requested, check to see if we have already cached this private key, to stop exhaustive 
               hash key generation */
            if(fCache)
            {
                LOCK(MUTEX);
                
                /* Check the cache */
                if(cacheKeys.Has(cacheKey))
                {
                    /* Retreive the private key hash from the cache */
                    SecureString strKey;
                    cacheKeys.Get(cacheKey, strKey);

                    /* Create the hash key object. */
                    uint512_t hashKey;

                    /* Get the bytes from secure allocator. */
                    std::vector<uint8_t> vBytes(strKey.begin(), strKey.end());

                    /* Set the bytes of return value. */
                    hashKey.SetBytes(vBytes);

                    return hashKey;
                }
            }

            /* Generate the Secret Phrase */
            std::vector<uint8_t> vUsername(strUsername.begin(), strUsername.end());
            vUsername.insert(vUsername.end(), (uint8_t*)&nKeyID, (uint8_t*)&nKeyID + sizeof(nKeyID));

            /* Set to minimum salt limits. */
            if(vUsername.size() < 8)
                vUsername.resize(8);

            /* Generate the Secret Phrase */
            std::vector<uint8_t> vPassword(strPassword.begin(), strPassword.end());
            vPassword.insert(vPassword.end(), (uint8_t*)&nKeyID, (uint8_t*)&nKeyID + sizeof(nKeyID));

            /* Generate the secret data. */
            std::vector<uint8_t> vSecret(strSecret.begin(), strSecret.end());
            vSecret.insert(vSecret.end(), (uint8_t*)&nKeyID, (uint8_t*)&nKeyID + sizeof(nKeyID));

            /* Argon2 hash the secret */
            uint512_t hashKey = LLC::Argon2_512(vPassword, vUsername, vSecret, 
                            std::max(1u, uint32_t(config::GetArg("-argon2", 12))), 
                            uint32_t(1 << std::max(4u, uint32_t(config::GetArg("-argon2_memory", 16)))));

            /* Add the private key to the cache. */
            if(fCache)
            {
                LOCK(MUTEX);

                cacheKeys.Put(cacheKey, SecureString(hashKey.begin(), hashKey.end()));
            }

            return hashKey;
        }


        /* This function is responsible for generating the private key in the sigchain with a specific password and pin.
        *  This version should be used when changing the password and/or pin */
        uint512_t SignatureChain::Generate(const uint32_t nKeyID, const SecureString& strPassword, const SecureString& strSecret) const
        {
            /* Generate the Secret Phrase */
            std::vector<uint8_t> vUsername(strUsername.begin(), strUsername.end());
            vUsername.insert(vUsername.end(), (uint8_t*)&nKeyID, (uint8_t*)&nKeyID + sizeof(nKeyID));

            /* Set to minimum salt limits. */
            if(vUsername.size() < 8)
                vUsername.resize(8);

            /* Generate the Secret Phrase */
            std::vector<uint8_t> vPassword(strPassword.begin(), strPassword.end());
            vPassword.insert(vPassword.end(), (uint8_t*)&nKeyID, (uint8_t*)&nKeyID + sizeof(nKeyID));

            /* Generate the secret data. */
            std::vector<uint8_t> vSecret(strSecret.begin(), strSecret.end());
            vSecret.insert(vSecret.end(), (uint8_t*)&nKeyID, (uint8_t*)&nKeyID + sizeof(nKeyID));


            /* Argon2 hash the secret */
            uint512_t hashKey = LLC::Argon2_512(vPassword, vUsername, vSecret, 
                            std::max(1u, uint32_t(config::GetArg("-argon2", 12))), 
                            uint32_t(1 << std::max(4u, uint32_t(config::GetArg("-argon2_memory", 16)))));


            return hashKey;
        }


        /*
         *  This function is responsible for genearting the private key in the keychain of a specific account.
         *  The keychain is a series of keys seeded from a secret phrase and a PIN number.
         */
        uint512_t SignatureChain::Generate(const std::string& strType, const uint32_t nKeyID, const SecureString& strSecret) const
        {
            /* Generate the Secret Phrase */
            std::vector<uint8_t> vUsername(strUsername.begin(), strUsername.end());
            vUsername.insert(vUsername.end(), (uint8_t*)&nKeyID, (uint8_t*)&nKeyID + sizeof(nKeyID));

            /* Set to minimum salt limits. */
            if(vUsername.size() < 8)
                vUsername.resize(8);

            /* Generate the Secret Phrase */
            std::vector<uint8_t> vPassword(strPassword.begin(), strPassword.end());
            vPassword.insert(vPassword.end(), (uint8_t*)&nKeyID, (uint8_t*)&nKeyID + sizeof(nKeyID));

            /* Generate the secret data. */
            std::vector<uint8_t> vSecret(strSecret.begin(), strSecret.end());
            vSecret.insert(vSecret.end(), (uint8_t*)&nKeyID, (uint8_t*)&nKeyID + sizeof(nKeyID));

            /* Seed secret data with the key type. */
            vSecret.insert(vSecret.end(), strType.begin(), strType.end());

            /* Argon2 hash the secret */
            uint512_t hashKey = LLC::Argon2_512(vPassword, vUsername, vSecret, 
                            std::max(1u, uint32_t(config::GetArg("-argon2", 12))), 
                            uint32_t(1 << std::max(4u, uint32_t(config::GetArg("-argon2_memory", 16)))));

            return hashKey;
        }


        /* This function is responsible for generating a private key from a seed phrase.  By comparison to the other Generate
         *  functions, this version using far stronger argon2 hashing since the only data input into the hashing function is
         *  the seed phrase itself. */
        uint512_t SignatureChain::Generate(const SecureString& strSecret) const
        {
            /* Generate the Secret Phrase */
            std::vector<uint8_t> vSecret(strSecret.begin(), strSecret.end());

            /* Argon2 hash the secret */
            uint512_t hashKey = LLC::Argon2_512(vSecret);

            /* Set the key type */
            hashKey.SetType(TAO::Ledger::GenesisType());

            return hashKey;
        }


        /* This function generates a public key generated from random seed phrase. */
        std::vector<uint8_t> SignatureChain::Key(const std::string& strType, const uint32_t nKeyID, const SecureString& strSecret, const uint8_t nType) const
        {
            /* The public key bytes */
            std::vector<uint8_t> vchPubKey;

            /* Get the private key. */
            uint512_t hashSecret = Generate(strType, nKeyID, strSecret);

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
                    if(!key.SetSecret(vchSecret))
                        throw debug::exception(FUNCTION, "failed to set falcon secret key");

                    /* Set the key bytes to return */
                    vchPubKey = key.GetPubKey();

                    break;
                }

                /* Support for the BRAINPOOL signature scheme. */
                case SIGNATURE::BRAINPOOL:
                {
                    /* Create EC Key object. */
                    LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the secret key. */
                    if(!key.SetSecret(vchSecret, true))
                        throw debug::exception(FUNCTION, "failed to set brainpool secret key");

                    /* Set the key bytes to return */
                    vchPubKey = key.GetPubKey();

                    break;

                }
            }

            /* return the public key */
            return vchPubKey;
        }

        /* This function generates a hash of a public key generated from random seed phrase. */
        uint256_t SignatureChain::KeyHash(const std::string& strType, const uint32_t nKeyID, const SecureString& strSecret, const uint8_t nType) const
        {
            /* Generate the public key */
            std::vector<uint8_t> vchPubKey = Key(strType, nKeyID, strSecret, nType);
            
            /* Calculate the key hash. */
            uint256_t hashRet = LLC::SK256(vchPubKey);

            /* Set the leading byte. */
            hashRet.SetType(nType);

            return hashRet;

        }


        /* This function generates a hash of a public key generated from a recovery seed phrase. */
        uint256_t SignatureChain::RecoveryHash(const SecureString& strRecovery, const uint8_t nType) const
        {
            /* The hashed public key to return*/
            uint256_t hashRet = 0;

            /* Timer to track how long it takes to generate the recovery hash private key from the seed. */
            runtime::timer timer;
            timer.Reset();

            /* Get the private key. */
            uint512_t hashSecret = Generate(strRecovery);

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
                    if(!key.SetSecret(vchSecret))
                        throw debug::exception(FUNCTION, "failed to set falcon secret key");

                    /* Calculate the hash of the public key. */
                    hashRet = LLC::SK256(key.GetPubKey());

                    break;
                }

                /* Support for the BRAINPOOL signature scheme. */
                case SIGNATURE::BRAINPOOL:
                {
                    /* Create EC Key object. */
                    LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the secret key. */
                    if(!key.SetSecret(vchSecret, true))
                        throw debug::exception(FUNCTION, "failed to set brainpool secret key");

                    /* Calculate the hash of the public key. */
                    hashRet = LLC::SK256(key.GetPubKey());

                    break;

                }
                default:
                    throw debug::exception(FUNCTION, "Unknown signature key type");

            }

            debug::log(0, FUNCTION, "Recovery Hash ", hashRet.SubString(), " generated in ", timer.Elapsed(), " seconds");

            return hashRet;
        }


        /* Returns the username for this sig chain */
        const SecureString& SignatureChain::UserName() const
        {
            return strUsername;
        }


        /* Special method for encrypting specific data types inside class. */
        void SignatureChain::Encrypt()
        {
            encrypt(strUsername);
            encrypt(strPassword);
            encrypt(cacheKeys);
            encrypt(hashGenesis);
        }


        /* Generates a signature for the data, using the specified crypto key from the crypto object register. */
        bool SignatureChain::Sign(const std::string& strKey, const std::vector<uint8_t>& vchData, const uint512_t& hashSecret,
                                  std::vector<uint8_t>& vchPubKey, std::vector<uint8_t>& vchSig) const
        {
            /* The crypto register object */
            TAO::Register::Object crypto;

            /* Get the crypto register. This is needed so that we can determine the key type used to generate the public key */
            TAO::Register::Address hashCrypto = TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);
            if(!LLD::Register->ReadState(hashCrypto, crypto, TAO::Ledger::FLAGS::MEMPOOL))
                return debug::error(FUNCTION, "Could not sign - missing crypto register");

            /* Parse the object. */
            if(!crypto.Parse())
                return debug::error(FUNCTION, "failed to parse crypto register");

            /* Check that the requested key is in the crypto register */
            if(!crypto.CheckName(strKey))
                return debug::error(FUNCTION, "Key type not found in crypto register: ", strKey);

            /* Get the encryption key type from the hash of the public key */
            uint8_t nType = crypto.get<uint256_t>(strKey).GetType();

            /* call the Sign method with the retrieved type */
            return Sign(nType, vchData, hashSecret, vchPubKey, vchSig);

        }
        
        /* Generates a signature for the data, using the specified crypto key type. */
        bool SignatureChain::Sign(const uint8_t& nKeyType, const std::vector<uint8_t>& vchData, const uint512_t& hashSecret,
                                  std::vector<uint8_t>& vchPubKey, std::vector<uint8_t>& vchSig) const
        {
            /* Get the secret from new key. */
            std::vector<uint8_t> vBytes = hashSecret.GetBytes();
            LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

            /* Switch based on signature type. */
            switch(nKeyType)
            {
                /* Support for the FALCON signature scheme. */
                case SIGNATURE::FALCON:
                {
                    /* Create the FL Key object. */
                    LLC::FLKey key;

                    /* Set the secret key. */
                    if(!key.SetSecret(vchSecret))
                        throw debug::exception(FUNCTION, "failed to set falcon secret key");

                    /* Generate the public key */
                    vchPubKey = key.GetPubKey();

                    /* Generate the signature */
                    if(!key.Sign(vchData, vchSig))
                        throw debug::exception(FUNCTION, "failed to sign data");

                    break;

                }

                /* Support for the BRAINPOOL signature scheme. */
                case SIGNATURE::BRAINPOOL:
                {
                    /* Create EC Key object. */
                    LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the secret key. */
                    if(!key.SetSecret(vchSecret, true))
                        throw debug::exception(FUNCTION, "failed to set brainpool secret key");

                    /* Generate the public key */
                    vchPubKey = key.GetPubKey();

                    /* Generate the signature */
                    if(!key.Sign(vchData, vchSig))
                        throw debug::exception(FUNCTION, "failed to sign data");

                    break;
                }
                default:
                {
                    throw debug::exception(FUNCTION, "unknown crypto signature scheme");
                }
            }

            /* Return success */
            return true;

        }


        /* Verifies a signature for the data, as well as verifying that the hashed public key matches the 
        *  specified key from the crypto object register */
        bool SignatureChain::Verify(const uint256_t hashGenesis, const std::string& strKey, const std::vector<uint8_t>& vchData, 
                    const std::vector<uint8_t>& vchPubKey, const std::vector<uint8_t>& vchSig)
        {
            /* Derive the object register address. */
            TAO::Register::Address hashCrypto =
                TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

            /* Get the crypto register. */
            TAO::Register::Object crypto;
            if(!LLD::Register->ReadState(hashCrypto, crypto, TAO::Ledger::FLAGS::LOOKUP))
                return debug::error(FUNCTION, "Missing crypto register");

            /* Parse the object. */
            if(!crypto.Parse())
                return debug::drop(FUNCTION, "failed to parse crypto register");

            /* Check that the requested key is in the crypto register */
            if(!crypto.CheckName(strKey))
                return debug::error(FUNCTION, "Key type not found in crypto register: ", strKey);

            /* Check the authorization hash. */
            uint256_t hashCheck = crypto.get<uint256_t>(strKey);

            /* Check that the hashed public key exists in the register*/
            if(hashCheck == 0)
                return debug::error(FUNCTION, "Public key hash not found in crypto register: ", strKey);

            /* Get the encryption key type from the hash of the public key */
            uint8_t nType = hashCheck.GetType();

            /* Grab hash of incoming pubkey and set its type. */
            uint256_t hashPubKey = LLC::SK256(vchPubKey);
            hashPubKey.SetType(nType);

            /* Check the public key to expected authorization key. */
            if(hashPubKey != hashCheck)
                return debug::error(FUNCTION, "Invalid public key");

            /* Switch based on signature type. */
            switch(nType)
            {
                /* Support for the FALCON signature scheeme. */
                case TAO::Ledger::SIGNATURE::FALCON:
                {
                    /* Create the FL Key object. */
                    LLC::FLKey key;

                    /* Set the public key and verify. */
                    key.SetPubKey(vchPubKey);
                    if(!key.Verify(vchData, vchSig))
                        return debug::error(FUNCTION, "Invalid transaction signature");

                    break;
                }

                /* Support for the BRAINPOOL signature scheme. */
                case TAO::Ledger::SIGNATURE::BRAINPOOL:
                {
                    /* Create EC Key object. */
                    LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the public key and verify. */
                    key.SetPubKey(vchPubKey);
                    if(!key.Verify(vchData, vchSig))
                        return debug::error(FUNCTION, "Invalid transaction signature");

                    break;
                }

                default:
                    return debug::error(FUNCTION, "Unknown signature scheme");

            }

            /* Verified! */
            return true;
        }

    }
}
