/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>
#include <LLD/include/global.h>

#include <Legacy/include/create.h>

#include <Legacy/types/coinbase.h>

#include <Legacy/include/enum.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/rollback.h>
#include <TAO/Register/include/create.h>
#include <TAO/Register/include/reserved.h>
#include <TAO/Register/types/address.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/types/genesis.h>

#include <unit/catch2/catch.hpp>

TEST_CASE("UTXO Unit Tests", "[UTXO]")
{
    using namespace TAO::Register;
    using namespace TAO::Operation;

    {
        //reserve key from temp wallet
        Legacy::ReserveKey* pReserveKey = new Legacy::ReserveKey(&Legacy::Wallet::GetInstance());

        //dummy coinbase
        Legacy::Coinbase coinbase;

        //make the coinbase tx
        Legacy::Transaction tx;
        REQUIRE(Legacy::CreateCoinbase(*pReserveKey, coinbase, 1, 0, 7, tx));

        //add the data into input script
        tx.vin[0].scriptSig << uint32_t(555);

        //get inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(tx.FetchInputs(inputs));

        //get best
        TAO::Ledger::BlockState state = TAO::Ledger::ChainState::stateBest.load();

        //conect tx
        REQUIRE(tx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        //add to wallet
        Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(tx, TAO::Ledger::ChainState::stateGenesis, true);

        //check balance
        REQUIRE(Legacy::Wallet::GetInstance().GetBalance() == 1000000);
    }


    //create a tritium transaction
    TAO::Ledger::BlockState state;
    state.nHeight = 150;

    REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

    //create a trust register from inputs spent on coinbase
    {
        uint256_t hashTrust    = TAO::Register::Address(TAO::Register::Address::ACCOUNT);
        uint256_t hashGenesis  = TAO::Ledger::Genesis(LLC::GetRand256(), true);

        uint512_t hashCoinbaseTx = 0;
        uint512_t hashLastTrust = LLC::GetRand512();
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();
            tx.hashNextTx  = TAO::Ledger::STATE::HEAD;

            //payload (hashGenesis, coinbase reward, extra nonce)
            tx[0] << uint8_t(OP::COINBASE) << hashGenesis << uint64_t(5000) << (uint64_t)0;

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //write index
            REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), state.GetHash()));

            //set the hash
            hashCoinbaseTx = tx.GetHash();
        }


        //Create a account register
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object account = CreateAccount(0);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashTrust << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check trust account initial values
            {
                Object trust;
                REQUIRE(LLD::Register->ReadState(hashTrust, trust));

                //parse register
                REQUIRE(trust.Parse());

                //check register
                REQUIRE(trust.get<uint64_t>("balance") == 0);
                REQUIRE(trust.get<uint256_t>("token")  == 0);
            }
        }


        //Add balance to trust account by crediting from Coinbase tx
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::CREDIT) << hashCoinbaseTx << uint32_t(0) << hashTrust << hashGenesis << uint64_t(5000);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check register values
            {
                Object trust;
                REQUIRE(LLD::Register->ReadState(hashTrust, trust));

                //parse register
                REQUIRE(trust.Parse());

                //check balance (claimed Coinbase amount added to balance)
                REQUIRE(trust.get<uint64_t>("balance") == 5000);
                REQUIRE(trust.get<uint256_t>("token") == 0);
            }
        }
    }
}
