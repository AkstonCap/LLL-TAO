/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLC_INCLUDE_X509_CERT_H
#define NEXUS_LLC_INCLUDE_X509_CERT_H

#include <openssl/pem.h>
#include <openssl/x509v3.h>

namespace LLC
{

    class Certificate
    {
    public:
        Certificate();

        ~Certificate();

    private:

        bool create_cert(X509 **px509, EVP_PKEY **pkey, uint32_t nBits, uint32_t nSerial, uint32_t nDays);


        void free_cert(X509 *px509, EVP_PKEY *pkey);


        /* The OpenSSl x509 certificate object. */
        X509 *px509;

        /* The OpenSSL key object */
        EVP_PKEY *pkey;


    }




}

#endif
