/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Register/include/object.h>

#include <unit/catch2/catch.hpp>

TEST_CASE( "Object Register Tests", "[register]" )
{
    using namespace TAO::Register;

    {
        Object object;
        object << std::string("byte") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT8_T) << uint8_t(55)
               << std::string("test") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::STRING) << std::string("this string")
               << std::string("bytes") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::BYTES) << std::vector<uint8_t>(10, 0xff)
               << std::string("balance") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT64_T) << uint64_t(55)
               << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint32_t(0);

        //parse object
        REQUIRE(object.Parse());

        //read test
        uint8_t nByte;
        REQUIRE(object.Read("byte", nByte));

        //check
        REQUIRE(nByte == uint8_t(55));

        //write test
        REQUIRE(object.Write("byte", uint8_t(98)));

        //check
        REQUIRE(object.Read("byte", nByte));

        //check
        REQUIRE(nByte == uint8_t(98));


        //string test
        std::string strTest;
        REQUIRE(object.Read("test", strTest));

        //check
        REQUIRE(strTest == std::string("this string"));

        //test fail
        REQUIRE(!object.Write("test", std::string("fail")));

        //test type fail
        REQUIRE(!object.Write("test", "fail"));

        //test write
        REQUIRE(object.Write("test", std::string("THIS string")));

        //test read
        REQUIRE(object.Read("test", strTest));

        //check
        REQUIRE(strTest == std::string("THIS string"));


        //vector test
        std::vector<uint8_t> vBytes;
        REQUIRE(object.Read("bytes", vBytes));

        //write test
        vBytes[0] = 0x00;
        REQUIRE(object.Write("bytes", vBytes));

        //read test
        std::vector<uint8_t> vCheck;
        REQUIRE(object.Read("bytes", vCheck));

        //check
        REQUIRE(vBytes == vCheck);

        //identifier
        uint32_t nIdentifier;
        REQUIRE(object.Read("identifier", nIdentifier));

        //check
        REQUIRE(nIdentifier == uint64_t(0));

        //check standards
        REQUIRE(object.Standard() == OBJECTS::ACCOUNT);
    }


    {
        Object object;
        object << std::string("balance") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT64_T) << uint64_t(55)
               << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint32_t(0)
               << std::string("supply") << uint8_t(TYPES::UINT64_T) << uint64_t(888888);

        //parse object
        REQUIRE(object.Parse());

        //check standards
        REQUIRE(object.Standard() == OBJECTS::ACCOUNT);
    }


    {
        Object object;
        object << std::string("balance") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT64_T) << uint64_t(55)
               << std::string("current") << uint8_t(TYPES::UINT32_T) << uint32_t(0)
               << std::string("testing") << uint8_t(TYPES::UINT64_T) << uint64_t(888888);

        //parse object
        REQUIRE(object.Parse());

        //check standards
        REQUIRE(object.Standard() == OBJECTS::NONSTANDARD);
    }


    {
        Object object;
        object << std::string("balance") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT64_T) << uint64_t(55)
               << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint32_t(0)
               << std::string("supply") << uint8_t(TYPES::UINT64_T) << uint64_t(888888)
               << std::string("digits") << uint8_t(TYPES::UINT64_T) << uint64_t(100);

        //parse object
        REQUIRE(object.Parse());

        //check standards
        REQUIRE(object.Standard() == OBJECTS::TOKEN);
    }

    {
        Object object;
        object << std::string("balance") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT64_T) << uint64_t(55)
               << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint64_t(0)
               << std::string("supply") << uint8_t(TYPES::UINT64_T) << uint64_t(888888)
               << std::string("digits") << uint8_t(TYPES::UINT64_T) << uint64_t(100);

        //parse object
        REQUIRE(!object.Parse());
    }
}
