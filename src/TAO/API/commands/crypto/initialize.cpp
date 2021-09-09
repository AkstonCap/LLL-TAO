/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/crypto.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Crypto::Initialize()
    {
        mapFunctions["list/keys"]           = Function(std::bind(&Crypto::List, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["create/key"]          = Function(std::bind(&Crypto::Create, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["create/certificate"]  = Function(std::bind(&Crypto::CreateCertificate, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["get/key"]             = Function(std::bind(&Crypto::Get, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["get/publickey"]       = Function(std::bind(&Crypto::GetPublic, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["get/privatekey"]      = Function(std::bind(&Crypto::GetPrivate, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["get/certificate"]     = Function(std::bind(&Crypto::GetCertificate, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["change/scheme"]       = Function(std::bind(&Crypto::ChangeScheme,this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["encrypt/data"]        = Function(std::bind(&Crypto::Encrypt,this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["decrypt/data"]        = Function(std::bind(&Crypto::Decrypt,this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["sign/data"]           = Function(std::bind(&Crypto::Sign,this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["verify/signature"]    = Function(std::bind(&Crypto::Verify,this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["verify/certificate"]  = Function(std::bind(&Crypto::VerifyCertificate,this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["get/hash"]            = Function(std::bind(&Crypto::GetHash,this, std::placeholders::_1, std::placeholders::_2));
    }
}
