/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <Legacy/include/ambassador.h>
#include <Legacy/types/script.h>

namespace Legacy
{

    /** Binary Data of Each Developer Address for Signature Verification of Exchange Transaction in Accept Block. **/
    std::vector<uint8_t> AMBASSADOR_SCRIPT_SIGNATURES[13];
    std::vector<uint8_t> DEVELOPER_SCRIPT_SIGNATURES[13];


    /** Binary Data of Each Developer Address for Signature Verification of Exchange Transaction in Accept Block. **/
    std::vector<uint8_t> AMBASSADOR_SCRIPT_SIGNATURES_RECYCLED[13];
    std::vector<uint8_t> DEVELOPER_SCRIPT_SIGNATURES_RECYCLED[13];


    /** Initialize the scripts. **/
    void InitializeScripts()
    {
        CScript scriptSig;
        for(int i = 0; i < 13; i++)
        {
            /* Set the script byte code for ambassador addresses old. */
            scriptSig.clear();
            scriptSig.SetNexusAddress(AMBASSADOR_ADDRESSES[i]);
            AMBASSADOR_SCRIPT_SIGNATURES[i] = (std::vector<unsigned char>)scriptSig;

            /* Set the script byte code for ambassador addresses new. */
            scriptSig.clear();
            scriptSig.SetNexusAddress(AMBASSADOR_ADDRESSES_RECYCLED[i]);
            AMBASSADOR_SCRIPT_SIGNATURES_RECYCLED[i] = (std::vector<unsigned char>)scriptSig;

            /* Set the script byte code for developer addresses old. */
            scriptSig.clear();
            scriptSig.SetNexusAddress(DEVELOPER_ADDRESSES[i]);
            DEVELOPER_SCRIPT_SIGNATURES[i] = (std::vector<unsigned char>)scriptSig;

            /* Set the script byte code for developer addresses new. */
            scriptSig.clear();
            scriptSig.SetNexusAddress(DEVELOPER_ADDRESSES_RECYCLED[i]);
            DEVELOPER_SCRIPT_SIGNATURES_RECYCLED[i] = (std::vector<unsigned char>)scriptSig;
        }
    }

}
