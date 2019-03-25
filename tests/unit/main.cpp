#include <Util/include/debug.h>
#include <Util/include/hex.h>

#include <Util/include/runtime.h>

#include <LLC/include/flkey.h>

#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/memory.h>

#include <LLC/include/random.h>

#include <openssl/rand.h>

#include <LLC/aes/aes.h>

#include <TAO/Operation/include/validate.h>

#include <TAO/Operation/include/enum.h>

#include <LLD/include/global.h>

#include <cmath>


std::atomic<uint64_t> nVerified;

std::vector<uint8_t> vchPubKey;

std::vector<uint8_t> vchSignature;

std::vector<uint8_t> vchMessage;

void Verifier()
{
    while(true)
    {
        LLC::FLKey key2;
        key2.SetPubKey(vchPubKey);
        if(!key2.Verify(vchMessage, vchSignature))
            debug::error(FUNCTION, "failed to verify");

        ++nVerified;
    }
}








//claims
//TAO::Operation::OP_CREDIT <txid of your debit> <hash-proof=0xff> <hash-to-b> 100
//---> check state of locking transaction

//TAO::Operation::OP_CREDIT <txid of your debit> <hash-proof=0xff> <hash-to-a> 500


//Exchange would be:
//TAO::Operation::OP::DEBIT <hash-from-a> <0xffffffffffffff> 100
//TAO::Operation::OP::REQUIRE TAO::Operation::OP_DEBIT <identifier> 500

//TAO::Operation::OP::AND TIMESTAMP TAO::Operation::OP::LESSTHAN TAO::Operation::OP::CHAIN::UNIFIED
//TAO::Operation::OP::OR TAO::Operation::OP_CREDIT <%txid%> <hash-proof=0xff> <MATCH::hash-to-a> 100

//first debit:
// TAO::Operation::OP::DEBIT <hash-from-account> <0xffffffff> TAO::Operation::OP::REQUIRE TAO::Operation::OP::CALLER::OPERATIONS TAO::Operation::OP::CONTAINS TAO::Operation::OP::DEBIT <0xffffffffff> <0xffffffffff> 100

//TAO::Operation::OP::REGISTER::IDENTIFIER TAO::Operation::OP::CALLER::DEBIT::FROM TAO::Operation::OP::EQUALS <token-type>

//TODO: need special parsing for specific caller parameters.

//second debit:
// TAO::Operation::OP::DEBIT <hash-from-account> <0xffffffff>> TAO::Operation::OP_VALIDATE <txid>
//---> TAO::Operation::OP_VALIDATE executes TAO::Operation::OP::VALIDATE from txid, if TAO::Operation::OP_REQUIRE satisfied


//return to self
//TAO::Operation::OP_VALIDATE <txid> TAO::Operation::OP_CREDIT <txid> <hash-proof=0xff> <hash-to-a> 100



bool Validate2(uint64_t a, uint64_t b)
{
    uint64_t vRet;
    vRet = pow(a, b);

    return (vRet == 25);
}






int main(int argc, char **argv)
{

    TAO::Register::BaseVM registers;

    TAO::Register::Value test;
    uint256_t hashing = LLC::GetRand256();
    registers.allocate(hashing, test);


    uint256_t random = LLC::GetRand256();
    TAO::Register::Value test2;
    registers.allocate(random, test2);


    TAO::Register::Value test3;
    uint64_t nTest = 48384839483948;
    registers.allocate(nTest, test3);



    uint64_t nTest33;
    registers.deallocate(nTest33, test3);
    assert(nTest == nTest33);


    uint256_t hash333;
    registers.deallocate(hash333, test2);
    assert(hash333 == random);


    uint256_t hashing2;
    registers.deallocate(hashing2, test);
    assert(hashing == hashing2);

    assert(registers.available() == 512);


    TAO::Operation::Stream ssOperation;

    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT32_T << (uint32_t)7u << (uint8_t) TAO::Operation::OP::EXP << (uint8_t) TAO::Operation::OP::TYPES::UINT32_T << (uint32_t)2u << (uint8_t) TAO::Operation::OP::EQUALS << (uint8_t)TAO::Operation::OP::TYPES::UINT32_T << (uint32_t)49u;

    runtime::timer bench;
    bench.Reset();

    runtime::timer func;
    func.Reset();

    uint256_t hashFrom = LLC::GetRand256();
    uint256_t hashTo   = LLC::GetRand256();
    uint64_t  nAmount  = 500;

    TAO::Ledger::Transaction tx;
    tx.nTimestamp  = 989798;
    tx.hashGenesis = LLC::GetRand256();
    tx << (uint8_t)TAO::Operation::OP::DEBIT << hashFrom << hashTo << nAmount;

    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        for(int t = 0; t < 1000000; ++t)
        {
            assert(script.Execute());
            script.Reset();
        }
    }

    uint64_t nTime = bench.ElapsedMicroseconds();

    debug::log(0, "Operations ", nTime, " ", std::dec, 1000000.0 / nTime, " million ops / s");

    uint64_t nA = 5;
    uint64_t nB = 2;

    bench.Reset();
    for(uint64_t i = 0; i <= 13000000; ++i)
    {
        assert(Validate2(nA, nB));
        ssOperation.reset();
    }

    nTime = bench.ElapsedMicroseconds();
    debug::log(0, "Binary ", bench.ElapsedMicroseconds());


    uint256_t hash = LLC::GetRand256();

    uint256_t hash2 = LLC::GetRand256();
    TAO::Register::State state;
    state.hashOwner = LLC::GetRand256();
    state.nType     = 2;
    state << hash2;


    LLD::regDB = new LLD::RegisterDB();
    assert(LLD::regDB->Write(hash, state));

    std::string strName = "colasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfin!!!";
    uint256_t hashRegister = LLC::SK256(std::vector<uint8_t>(strName.begin(), strName.end()));


    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::STRING << strName << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t)TAO::Operation::OP::TYPES::STRING << strName;

    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::STRING << strName << (uint8_t)TAO::Operation::OP::CRYPTO::SK256 << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t)TAO::Operation::OP::TYPES::UINT256_T << hashRegister;

    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }


    uint512_t hashRegister2 = LLC::SK512(strName.begin(), strName.end());

    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::STRING << strName << (uint8_t)TAO::Operation::OP::CRYPTO::SK512 << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t)TAO::Operation::OP::TYPES::UINT512_T << hashRegister2;


    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }





    //////////////COMPARISONS
    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT256_T << hash << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t)TAO::Operation::OP::TYPES::UINT256_T << hash;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT256_T << hash << (uint8_t)TAO::Operation::OP::LESSTHAN << (uint8_t)TAO::Operation::OP::TYPES::UINT256_T << hash + 1;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT256_T << hash << (uint8_t)TAO::Operation::OP::GREATERTHAN << (uint8_t)TAO::Operation::OP::TYPES::UINT256_T << hash - 1;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT256_T << hash << (uint8_t)TAO::Operation::OP::NOTEQUALS << (uint8_t)TAO::Operation::OP::TYPES::UINT256_T << hash2;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT256_T << hash << (uint8_t)TAO::Operation::OP::NOTEQUALS << (uint8_t)TAO::Operation::OP::TYPES::UINT256_T << hash + 1;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::STRING << std::string("is there an atomic bear out there?") << (uint8_t)TAO::Operation::OP::CONTAINS << (uint8_t)TAO::Operation::OP::TYPES::STRING << std::string("bear out");
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::STRING << std::string("is there an atomic bear out there?") << (uint8_t)TAO::Operation::OP::CONTAINS << (uint8_t)TAO::Operation::OP::TYPES::STRING << std::string("is");
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::STRING << std::string("is there an atomic bear out there?") << (uint8_t)TAO::Operation::OP::CONTAINS << (uint8_t)TAO::Operation::OP::TYPES::STRING << std::string("atomic bear out");
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::STRING << std::string("is there an atomic bear out there?") << (uint8_t)TAO::Operation::OP::CONTAINS << (uint8_t)TAO::Operation::OP::TYPES::STRING << std::string("atomic fox");
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(!script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::STRING << std::string("is there an atomic bear out there?") << (uint8_t)TAO::Operation::OP::CONTAINS << (uint8_t)TAO::Operation::OP::TYPES::STRING << std::string("atomic") <<
    (uint8_t)TAO::Operation::OP::AND << (uint8_t)TAO::Operation::OP::TYPES::STRING << std::string("is there an atomic bear out there?") << (uint8_t)TAO::Operation::OP::CONTAINS << (uint8_t)TAO::Operation::OP::TYPES::STRING << std::string("bear");
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::STRING << std::string("is there an atomic bear out there?") << (uint8_t)TAO::Operation::OP::CONTAINS << (uint8_t)TAO::Operation::OP::TYPES::STRING << std::string("fox and bear") <<
    (uint8_t)TAO::Operation::OP::OR << (uint8_t)TAO::Operation::OP::TYPES::STRING << std::string("is there an atomic bear out there?") << (uint8_t)TAO::Operation::OP::CONTAINS << (uint8_t)TAO::Operation::OP::TYPES::STRING << std::string("atomic bear");
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }


    TAO::Operation::Stream ssCompare;
    ssCompare << (uint8_t)TAO::Operation::OP::DEBIT << uint256_t(0) << uint256_t(0) << nAmount;

    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::CALLER::OPERATIONS << (uint8_t)TAO::Operation::OP::CONTAINS << (uint8_t)TAO::Operation::OP::TYPES::BYTES << ssCompare.Bytes();
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }


    ssCompare.SetNull();
    ssCompare << (uint8_t)TAO::Operation::OP::DEBIT << uint256_t(0) << uint256_t(5) << nAmount;

    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::CALLER::OPERATIONS << (uint8_t)TAO::Operation::OP::CONTAINS << (uint8_t)TAO::Operation::OP::TYPES::BYTES << ssCompare.Bytes();
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(!script.Execute());
    }



    /////REGISTERS
    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT256_T << hash << (uint8_t)TAO::Operation::OP::REGISTER::STATE << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t) TAO::Operation::OP::TYPES::UINT256_T << hash2;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT256_T << hash << (uint8_t)TAO::Operation::OP::REGISTER::OWNER << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t) TAO::Operation::OP::TYPES::UINT256_T << state.hashOwner;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT256_T << hash << (uint8_t)TAO::Operation::OP::REGISTER::TIMESTAMP << (uint8_t) TAO::Operation::OP::ADD << (uint8_t) TAO::Operation::OP::TYPES::UINT32_T << 3u << (uint8_t)TAO::Operation::OP::GREATERTHAN << (uint8_t) TAO::Operation::OP::GLOBAL::UNIFIED;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT256_T << hash << (uint8_t)TAO::Operation::OP::REGISTER::TYPE << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t) TAO::Operation::OP::TYPES::UINT64_T << (uint64_t) 2;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }





    ///////LEDGER and CALLER
    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::CALLER::TIMESTAMP << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t) TAO::Operation::OP::TYPES::UINT64_T << (uint64_t) 989798;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::CALLER::GENESIS << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t) TAO::Operation::OP::TYPES::UINT256_T << tx.hashGenesis;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }


    TAO::Ledger::BlockState block;
    block.nTime          = 947384;
    block.nHeight        = 23030;
    block.nMoneySupply   = 39239;
    block.nChannelHeight = 3;
    block.nChainTrust    = 55;

    TAO::Ledger::ChainState::stateBest.store(block);


    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::LEDGER::TIMESTAMP << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t) TAO::Operation::OP::TYPES::UINT64_T << (uint64_t)947384;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::LEDGER::HEIGHT << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t) TAO::Operation::OP::TYPES::UINT64_T << (uint64_t)23030;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::LEDGER::SUPPLY << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t) TAO::Operation::OP::TYPES::UINT64_T << (uint64_t)39239;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }


    ////////////COMPUTATION
    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT32_T << 555u << (uint8_t) TAO::Operation::OP::SUB << (uint8_t) TAO::Operation::OP::TYPES::UINT32_T << 333u << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t)TAO::Operation::OP::TYPES::UINT32_T << 222u;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT32_T << 9234837u << (uint8_t) TAO::Operation::OP::SUB << (uint8_t) TAO::Operation::OP::TYPES::UINT32_T << 384728u << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t)TAO::Operation::OP::TYPES::UINT32_T << 8850109u;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT32_T << 905u << (uint8_t) TAO::Operation::OP::MOD << (uint8_t) TAO::Operation::OP::TYPES::UINT32_T << 30u << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t)TAO::Operation::OP::TYPES::UINT32_T << 5u;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT64_T << (uint64_t)1000000000000 << (uint8_t) TAO::Operation::OP::DIV << (uint8_t) TAO::Operation::OP::TYPES::UINT64_T << (uint64_t)30000 << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t)TAO::Operation::OP::TYPES::UINT32_T << 33333333u;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT64_T << (uint64_t)5 << (uint8_t) TAO::Operation::OP::EXP << (uint8_t) TAO::Operation::OP::TYPES::UINT64_T << (uint64_t)15 << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t)TAO::Operation::OP::TYPES::UINT64_T << (uint64_t)30517578125;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT32_T << 2u << (uint8_t) TAO::Operation::OP::EXP << (uint8_t) TAO::Operation::OP::TYPES::UINT32_T << 8u << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t)TAO::Operation::OP::TYPES::UINT32_T << 256u;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT64_T << (uint64_t)9837 << (uint8_t) TAO::Operation::OP::ADD << (uint8_t) TAO::Operation::OP::TYPES::UINT64_T << (uint64_t)7878 << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t)TAO::Operation::OP::TYPES::UINT32_T << 17715u;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT64_T << (uint64_t)9837 << (uint8_t) TAO::Operation::OP::ADD << (uint8_t) TAO::Operation::OP::TYPES::UINT32_T << 7878u << (uint8_t)TAO::Operation::OP::LESSTHAN << (uint8_t)TAO::Operation::OP::TYPES::UINT256_T << uint256_t(17716);
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }


    //////////////// AND / OR
    ssOperation.SetNull();
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT32_T << 9837u << (uint8_t) TAO::Operation::OP::ADD << (uint8_t) TAO::Operation::OP::TYPES::UINT32_T << 7878u << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t)TAO::Operation::OP::TYPES::UINT32_T << 17715u;
    ssOperation << (uint8_t)TAO::Operation::OP::AND;
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT32_T << 9837u << (uint8_t) TAO::Operation::OP::ADD << (uint8_t) TAO::Operation::OP::TYPES::UINT32_T << 7878u << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t)TAO::Operation::OP::TYPES::UINT32_T << 17715u;
    ssOperation << (uint8_t)TAO::Operation::OP::AND;
    ssOperation << (uint8_t)TAO::Operation::OP::TYPES::UINT32_T << 9837u << (uint8_t) TAO::Operation::OP::ADD << (uint8_t) TAO::Operation::OP::TYPES::UINT32_T << 7878u << (uint8_t)TAO::Operation::OP::EQUALS << (uint8_t)TAO::Operation::OP::TYPES::UINT32_T << 17715u;
    {
        TAO::Operation::Validate script = TAO::Operation::Validate(ssOperation, tx);
        assert(script.Execute());
    }



    return 0;
}
