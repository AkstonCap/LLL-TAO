/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/rollback.h>
#include <TAO/Register/include/create.h>

#include <TAO/Ledger/types/transaction.h>

#include <unit/catch2/catch.hpp>

TEST_CASE( "Validate Primitive Tests", "[operation]" )
{
    using namespace TAO::Register;
    using namespace TAO::Operation;


    //check a debit from token
    {

        //create object
        uint256_t hashToken     = LLC::GetRand256();
        uint256_t hashAccount   = LLC::GetRand256();
        uint256_t hashGenesis   = LLC::GetRand256();

        uint256_t hashToken2    = LLC::GetRand256();
        uint256_t hashAccount2  = LLC::GetRand256();
        uint256_t hashGenesis2  = LLC::GetRand256();

        //make first sigchain accounts and tokens.
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object token = CreateToken(hashToken, 1000, 100);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashToken << uint8_t(REGISTER::OBJECT) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }

        //make second account and token
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis2;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object token = CreateToken(hashToken2, 1000, 100);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashToken2 << uint8_t(REGISTER::OBJECT) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }


        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object account = CreateAccount(hashToken2);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }


        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis2;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object account = CreateAccount(hashToken);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount2 << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }



        //check wildcard sending and conditions
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << ~uint256_t(0) << uint64_t(500);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check for error
            std::string error = debug::GetLastError();
            REQUIRE(error.find("cannot debit to wildcard with no conditions") != std::string::npos);
        }




        //build a debit by condition
        uint512_t hashTx;
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << ~uint256_t(0) << uint64_t(500);
            tx[0] <= uint8_t(OP::CALLER::GENESIS) <= uint8_t(OP::EQUALS) <= uint8_t(OP::TYPES::UINT256_T) <= hashGenesis;

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //get tx hash
            hashTx = tx.GetHash();

            //write transaction to disk
            REQUIRE(LLD::Ledger->WriteTx(hashTx, tx));
        }


        //test failed conditions
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis2;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::CREDIT) << hashTx << uint32_t(0) << hashAccount2 << hashToken << uint64_t(500);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check for error
            std::string error = debug::GetLastError();
            printf("ERROR: %s\n", error.c_str());
            
            REQUIRE(error.find("OP::CREDIT: conditions not satisfied") != std::string::npos);
        }


        //execute the condition
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::CREDIT) << hashTx << uint32_t(0) << hashToken << hashToken << uint64_t(500);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }


        //try to double-spend
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::CREDIT) << hashTx << uint32_t(0) << hashToken << hashToken << uint64_t(500);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check for error
            std::string error = debug::GetLastError();
            REQUIRE(error.find("credit is already claimed") != std::string::npos);
        }


        //check that all values are reflected properly
        {
            TAO::Register::Object object;
            REQUIRE(LLD::Register->ReadState(hashToken, object));

            REQUIRE(object.Parse());

            REQUIRE(object.get<uint64_t>("balance") == 1000);

        }





        //test DEX exchange between tokens
        //build a debit by condition
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << ~uint256_t(0) << uint64_t(500) << uint8_t(OP::CONDITION);


            //build condition requirement
            TAO::Operation::Stream compare;
            compare << uint8_t(OP::DEBIT) << uint256_t(0) << hashAccount << uint64_t(200);

            //conditions
            tx[0] <= uint8_t(OP::GROUP);
            tx[0] <= uint8_t(OP::CALLER::OPERATIONS) <= uint8_t(OP::CONTAINS) <= uint8_t(OP::TYPES::BYTES) <= compare.Bytes();
            tx[0] <= uint8_t(OP::AND);
            tx[0] <= uint8_t(OP::CALLER::PRESTATE::VALUE) <= std::string("token");
            tx[0] <= uint8_t(OP::EQUALS) <= uint8_t(OP::TYPES::UINT256_T) <= hashToken2;
            tx[0] <= uint8_t(OP::UNGROUP);

            tx[0] <= uint8_t(OP::OR);

            tx[0] <= uint8_t(OP::CALLER::GENESIS) <= uint8_t(OP::EQUALS) <= uint8_t(OP::TYPES::UINT256_T) <= hashGenesis;


            //get tx hash
            hashTx = tx.GetHash();

            //write transaction to disk
            REQUIRE(LLD::Ledger->WriteTx(hashTx, tx));

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }



        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis2;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::CREDIT) << hashTx << uint32_t(0) << hashAccount2 << hashToken << uint64_t(500);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //write transaction to disk
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            printf("Attempting double spending....\n");
            REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check for error
            std::string error = debug::GetLastError();

            printf("ERROR: %s\n", error.c_str());
            //REQUIRE(error.find("credit is already claimed") != std::string::npos);
        }



        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis2;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken2 << hashAccount << uint64_t(200) << uint8_t(OP::VALIDATE) << hashTx << uint32_t(0);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //write transaction to disk
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        }


        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis2;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken2 << hashAccount << uint64_t(200) << uint8_t(OP::VALIDATE) << hashTx << uint32_t(0);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //write transaction to disk
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        }
















    }
}
