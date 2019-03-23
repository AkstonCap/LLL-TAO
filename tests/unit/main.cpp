#include <Util/include/debug.h>
#include <Util/include/hex.h>

#include <Util/include/runtime.h>

#include <LLC/include/flkey.h>

#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/memory.h>

#include <LLC/include/random.h>

#include <openssl/rand.h>

#include <LLC/aes/aes.h>

#include <TAO/Operation/include/stream.h>

#include <LLD/include/global.h>

#include <math.h>


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


struct OP
{
    struct TYPES
    {
        enum
        {
            RESERVED    = 0x00,
            UINT8_T     = 0x01,
            UINT16_T    = 0x02,
            UINT32_T    = 0x03,
            UINT64_T    = 0x04,
            UINT256_T   = 0x05,
            UINT512_T   = 0x06,
            UINT1024_T  = 0x07,
            STRING      = 0x08
        };
    };

    enum
    {
        //RESERVED 0x08 - 0x0f
        EQUALS      = 0x10,
        LESSTHAN    = 0x11,
        GREATERTHAN = 0x12,
        NOTEQUALS   = 0x13,


        //RESERVED to 0x1f
        ADD         = 0x20,
        SUB         = 0x21,
        DIV         = 0x22,
        MUL         = 0x23,
        MOD         = 0x24,
        INC         = 0x25,
        DEC         = 0x26,
        EXP         = 0x27,


        //RESERVED to 0x2f
        AND         = 0x30,
        OR          = 0x31,
        IF          = 0x32,
    };


    //BYTES 0xa0 and up
    struct REGISTER
    {
        enum
        {
            TIMESTAMP     = 0xa0,
            OWNER         = 0xa1,
            TYPE          = 0xa2,
            STATE         = 0xa3
        };
    };


    //BYTES 0xb0 and up
    struct CALLER
    {
        enum
        {
            GENESIS      = 0xb0,
            TIMESTAMP    = 0xb1,
            TXID         = 0xb2
        };
    };


    //BYTES 0xc0 and up
    struct CHAIN
    {
        enum
        {
            HEIGHT        = 0xc0,
            TIMESTAMP     = 0xc1,
            UNIFIED       = 0xc2
        };
    };



    //BYTES 0xd0 and up
    struct CRYPTO
    {
        enum
        {
            SK256        = 0xd0,
            SK512        = 0xd1
        };
    };
};


/**  Compares two byte arrays and determines their signed equivalence byte for
 *   byte.
 **/
inline int64_t compare(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b)
{
    size_t nSize = std::min(a.size(), b.size()) - 1;
    for(int64_t i = nSize; i >= 0; --i)
    {
        //debug::log(0, "I ", i, " Byte ", std::hex, a[i], " vs Bytes ", std::hex, b[i]);
        if(a[i] != b[i])
            return a[i] - b[i];

    }
    return 0;
}



/** Get Value
 *
 *  Extracts a value from an operation ssOperation for use in the boolean validation scripts.
 *
 **/
inline bool GetValue(const TAO::Operation::Stream& ssOperation, std::vector<uint64_t> &vRet, int32_t &nLimit)
{

    /* Iterate until end of ssOperation. */
    while(!ssOperation.end())
    {

        /* Extract the operation byte. */
        uint8_t OPERATION;
        ssOperation >> OPERATION;


        /* Switch based on the operation. */
        switch(OPERATION)
        {

            /* Add two 64-bit numbers. */
            case OP::ADD:
            {
                /* Get the add from r-value. */
                std::vector<uint64_t> vAdd;
                if(!GetValue(ssOperation, vAdd, nLimit))
                    return false;

                /* Check computational bounds. */
                if(vAdd.size() > 1 || vRet.size() > 1)
                    throw std::runtime_error(debug::safe_printstr("OP::ADD computation greater than 64-bits"));

                /* Compute the return value. */
                vRet[0] += vAdd[0];

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 64;

                break;
            }


            /* Subtract one number from another. */
            case OP::SUB:
            {
                /* Get the sub from r-value. */
                std::vector<uint64_t> vSub;
                if(!GetValue(ssOperation, vSub, nLimit))
                    return false;

                /* Check computational bounds. */
                if(vSub.size() > 1 || vRet.size() > 1)
                    throw std::runtime_error(debug::safe_printstr("OP::SUB computation greater than 64-bits"));

                /* Compute the return value. */
                vRet[0] -= vSub[0];

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 64;

                break;
            }


            /* Increment a number by an order of 1. */
            case OP::INC:
            {
                /* Check computational bounds. */
                if(vRet.size() > 1)
                    throw std::runtime_error(debug::safe_printstr("OP::INC computation greater than 64-bits"));

                /* Increment the return value. */
                ++vRet[0];

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 64;

                break;
            }


            /* De-increment a number by an order of 1. */
            case OP::DEC:
            {
                /* Check computational bounds. */
                if(vRet.size() > 1)
                    throw std::runtime_error(debug::safe_printstr("OP::DEC computation greater than 64-bits"));

                /* Deincrement the return value. */
                --vRet[0];

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 64;

                break;
            }


            /* Divide a number by another. */
            case OP::DIV:
            {
                /* Get the divisor from r-value. */
                std::vector<uint64_t> vDiv;
                if(!GetValue(ssOperation, vDiv, nLimit))
                    return false;

                /* Check computational bounds. */
                if(vDiv.size() > 1 || vRet.size() > 1)
                    throw std::runtime_error(debug::safe_printstr("OP::DIV computation greater than 64-bits"));

                /* Compute the return value. */
                vRet[0] /= vDiv[0];

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 128;

                break;
            }


            /* Multiply a number by another. */
            case OP::MUL:
            {
                /* Get the multiplier from r-value. */
                std::vector<uint64_t> vMul;
                if(!GetValue(ssOperation, vMul, nLimit))
                    return false;

                /* Check computational bounds. */
                if(vMul.size() > 1 || vRet.size() > 1)
                    throw std::runtime_error(debug::safe_printstr("OP::MUL computation greater than 64-bits"));

                /* Compute the return value. */
                vRet[0] *= vMul[0];

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 128;

                break;
            }


            /* Raise a number by the power of another. */
            case OP::EXP:
            {
                /* Get the exponent from r-value. */
                std::vector<uint64_t> vExp;
                if(!GetValue(ssOperation, vExp, nLimit))
                    return false;

                /* Check computational bounds. */
                if(vExp.size() > 1 || vRet.size() > 1)
                    throw std::runtime_error(debug::safe_printstr("OP::EXP computation greater than 64-bits"));

                /* Compute the return value. */
                vRet[0] = pow(vRet[0], vExp[0]);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 256;

                break;
            }


            /* Get the remainder after a division. */
            case OP::MOD:
            {
                /* Get the modulus from r-value. */
                std::vector<uint64_t> vMod;
                if(!GetValue(ssOperation, vMod, nLimit))
                    return false;

                /* Check computational bounds. */
                if(vMod.size() > 1 || vRet.size() > 1)
                    throw std::runtime_error(debug::safe_printstr("OP::MOD computation greater than 64-bits"));

                /* Compute the return value. */
                vRet[0] %= vMod[0];

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 128;

                break;
            }


            /* Extract an uint8_t from the stream. */
            case OP::TYPES::UINT8_T:
            {
                /* Extract the byte. */
                uint8_t n;
                ssOperation >> n;

                /* Push onto the return value. */
                vRet.push_back(n);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 1;

                break;
            }


            /* Extract an uint16_t from the stream. */
            case OP::TYPES::UINT16_T:
            {
                /* Extract the short. */
                uint16_t n;
                ssOperation >> n;

                /* Push onto the return value. */
                vRet.push_back(n);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 2;

                break;
            }


            /* Extract an uint32_t from the stream. */
            case OP::TYPES::UINT32_T:
            {
                /* Extract the integer. */
                uint32_t n;
                ssOperation >> n;

                /* Push onto the return value. */
                vRet.push_back(n);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 4;

                break;
            }


            /* Extract an uint64_t from the stream. */
            case OP::TYPES::UINT64_T:
            {
                /* Extract the integer. */
                uint64_t n;
                ssOperation >> n;

                /* Push onto the return value. */
                vRet.push_back(n);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 8;

                break;
            }


            /* Extract an uint256_t from the stream. */
            case OP::TYPES::UINT256_T:
            {
                /* Extract the integer. */
                uint256_t n;
                ssOperation >> n;

                /* Push each internal 32-bit integer on return value. */
                vRet.resize(4);
                std::copy((uint8_t*)&n, (uint8_t*)&n + 32, (uint8_t*)&vRet[0]);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 32;

                break;

            }


            /* Extract an uint512_t from the stream. */
            case OP::TYPES::UINT512_T:
            {
                /* Extract the integer. */
                uint512_t n;
                ssOperation >> n;

                /* Push each internal 32-bit integer on return value. */
                vRet.resize(8);
                std::copy((uint8_t*)&n, (uint8_t*)&n + 64, (uint8_t*)&vRet[0]);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 64;

                break;
            }


            /* Extract an uint1024_t from the stream. */
            case OP::TYPES::UINT1024_T:
            {
                /* Extract the integer. */
                uint1024_t n;
                ssOperation >> n;

                /* Push each internal 32-bit integer on return value. */
                vRet.resize(16);
                std::copy((uint8_t*)&n, (uint8_t*)&n + 128, (uint8_t*)&vRet[0]);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 128;

                break;
            }


            /* Extract an uint1024_t from the stream. */
            case OP::TYPES::STRING:
            {
                /* Extract the string. */
                std::string str;
                ssOperation >> str;

                /* Push each internal 32-bit integer on return value. */
                vRet.resize(str.size() / 8);
                std::copy((uint8_t*)&str[0], (uint8_t*)&str[0] + str.size(), (uint8_t*)&vRet[0]);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= str.size();

                break;
            }


            /* Get a unified timestamp and push to return value. */
            case OP::CHAIN::UNIFIED:
            {
                /* Get the current unified timestamp. */
                uint64_t n = runtime::unifiedtimestamp();

                /* Push the timestamp onto the return value. */
                vRet.push_back(n);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 8;

                break;
            }


            /* Get a register's timestamp and push to the return value. */
            case OP::REGISTER::TIMESTAMP:
            {
                /* Read the register vAddress. */
                uint256_t hashRegister;
                ssOperation >> hashRegister;

                /* Read the register states. */
                TAO::Register::State state;
                if(!LLD::regDB->Read(hashRegister, state))
                    return false;

                /* Push the timestamp onto the return value. */
                vRet.push_back(state.nTimestamp);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 40;

                break;
            }


            /* Get a register's owner and push to the return value. */
            case OP::REGISTER::OWNER:
            {
                /* Read the register vAddress. */
                uint256_t hashRegister;
                ssOperation >> hashRegister;

                /* Read the register states. */
                TAO::Register::State state;
                if(!LLD::regDB->Read(hashRegister, state))
                    return false;

                /* Push each internal 32-bit integer on return value. */
                vRet.resize(4);
                std::copy((uint8_t*)&state.hashOwner, (uint8_t*)&state.hashOwner + 32, (uint8_t*)&vRet[0]);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 64;

                break;
            }


            /* Get a register's type and push to the return value. */
            case OP::REGISTER::TYPE:
            {
                /* Read the register vAddress. */
                uint256_t hashRegister;
                ssOperation >> hashRegister;

                /* Read the register states. */
                TAO::Register::State state;
                if(!LLD::regDB->Read(hashRegister, state))
                    return false;

                /* Push the type onto the return value. */
                vRet.push_back(state.nType);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 33;

                break;
            }


            /* Get a register's state and push to the return value. */
            case OP::REGISTER::STATE:
            {
                /* Read the register vAddress. */
                uint256_t hashRegister;
                ssOperation >> hashRegister;

                /* Read the register states. */
                TAO::Register::State state;
                if(!LLD::regDB->Read(hashRegister, state))
                    return false;

                /* Get the register's internal states. */
                std::vector<uint8_t> vState = state.GetState();

                /* Add binary vData to vRet. */
                vRet.resize(vState.size() / 8);
                std::copy((uint8_t*)&vState[0], (uint8_t*)&vState[0] + vState.size(), (uint8_t*)&vRet[0]);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 128;

                break;
            }



            /* Compute an SK256 hash of current return value. */
            case OP::CRYPTO::SK256:
            {
                /* Compute the return hash. */
                uint256_t hash = LLC::SK256((uint8_t*)&vRet[0], (uint8_t*)&vRet[0] + vRet.size() * 8);

                /* Copy new hash into return value. */
                vRet.resize(4);
                std::copy((uint8_t*)&hash, (uint8_t*)&hash + 32, (uint8_t*)&vRet[0]);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 512;

                break;
            }


            /* Compute an SK512 hash of current return value. */
            case OP::CRYPTO::SK512:
            {
                /* Compute the return hash. */
                uint512_t hash = LLC::SK512((uint8_t*)&vRet[0], (uint8_t*)&vRet[0] + vRet.size() * 8);

                /* Copy new hash into return value. */
                vRet.resize(8);
                std::copy((uint8_t*)&hash, (uint8_t*)&hash + 64, (uint8_t*)&vRet[0]);

                /* Reduce the limits to prevent operation exhuastive attacks. */
                nLimit -= 1024;

                break;
            }


            default:
            {
                /* If no applicable instruction found, rewind and return. */
                ssOperation.seek(-1);
                if(nLimit < 0)
                    debug::error(FUNCTION, "out of computational limits ", nLimit);

                return (nLimit >= 0);
            }
        }
    }

    /* Log if computational limits expired. */
    if(nLimit < 0)
        debug::error(FUNCTION, "out of computational limits ", nLimit);

    return (nLimit >= 0);
}


static uint64_t time_a = 0;
static uint64_t time_b = 0;
static uint64_t time_c = 0;
static uint64_t time_d = 0;

bool Validate(const TAO::Operation::Stream& ssOperation)
{
    int32_t nLimit = 2048;

    bool fRet = false;

    std::vector<uint64_t> lvalue;
    while(!ssOperation.end())
    {

        if(!GetValue(ssOperation, lvalue, nLimit))
            return false;

        uint8_t OPERATION;
        ssOperation >> OPERATION;

        switch(OPERATION)
        {
            case OP::EQUALS:
            {
                std::vector<uint64_t> rvalue;
                if(!GetValue(ssOperation, rvalue, nLimit))
                    return false;

                fRet = (compare(lvalue, rvalue) == 0);

                break;
            }

            case OP::LESSTHAN:
            {
                std::vector<uint64_t> rvalue;
                if(!GetValue(ssOperation, rvalue, nLimit))
                    return false;

                fRet = (compare(lvalue, rvalue) < 0);

                break;
            }

            case OP::GREATERTHAN:
            {
                std::vector<uint64_t> rvalue;
                if(!GetValue(ssOperation, rvalue, nLimit))
                    return false;

                fRet = (compare(lvalue, rvalue) > 0);

                break;
            }

            case OP::NOTEQUALS:
            {
                std::vector<uint64_t> rvalue;
                if(!GetValue(ssOperation, rvalue, nLimit))
                    return false;

                fRet = (compare(lvalue, rvalue) != 0);

                break;
            }

            case OP::AND:
            {
                return fRet && Validate(ssOperation);
            }

            case OP::OR:
            {
                if(fRet)
                    return true;

                return Validate(ssOperation);
            }

            default:
            {
                return false;
            }
        }
    }

    return fRet;
}


bool Validate2(uint64_t a, uint64_t b)
{
    uint64_t vRet;
    vRet = pow(a, b);

    return (vRet == 25);
}



int main(int argc, char **argv)
{
    TAO::Operation::Stream ssOperation;

    ssOperation << (uint8_t)OP::TYPES::UINT32_T << (uint32_t)7u << (uint8_t) OP::EXP << (uint8_t) OP::TYPES::UINT32_T << (uint32_t)2u << (uint8_t) OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << (uint32_t)49u;

    runtime::timer bench;
    bench.Reset();

    runtime::timer func;
    func.Reset();

    uint64_t time_e = 0;
    for(int t = 0; t < 1000000; ++t)
    {
        //func.Reset();
        //assert(Validate(ssOperation));
        Validate(ssOperation);
        ssOperation.reset();
        //time_e += func.ElapsedMicroseconds();
    }
    time_e = func.ElapsedMicroseconds();

    uint64_t nTime = bench.ElapsedMicroseconds();

    debug::log(0, "A ", time_a);
    debug::log(0, "B ", time_b);
    debug::log(0, "C ", time_c);
    debug::log(0, "D ", time_d);
    debug::log(0, "E ", time_e);

    debug::log(0, "Operations ", nTime);

    uint64_t nA = 5;
    uint64_t nB = 2;

    bench.Reset();
    for(uint64_t i = 0; i <= 1000000; ++i)
    {
        assert(Validate2(nA, nB));
        ssOperation.reset();
    }

    nTime = bench.ElapsedMicroseconds();
    debug::log(0, "Binary ", nTime);


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
    ssOperation << (uint8_t)OP::TYPES::STRING << strName << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::STRING << strName;
    assert(Validate(ssOperation));


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::STRING << strName << (uint8_t)OP::CRYPTO::SK256 << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT256_T << hashRegister;
    assert(Validate(ssOperation));


    uint512_t hashRegister2 = LLC::SK512(strName.begin(), strName.end());

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::STRING << strName << (uint8_t)OP::CRYPTO::SK512 << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT512_T << hashRegister2;
    assert(Validate(ssOperation));



    //////////////COMPARISONS
    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT256_T << hash << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT256_T << hash;
    assert(Validate(ssOperation));


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT256_T << hash << (uint8_t)OP::LESSTHAN << (uint8_t)OP::TYPES::UINT256_T << hash;
    assert(!Validate(ssOperation));


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT256_T << hash << (uint8_t)OP::GREATERTHAN << (uint8_t)OP::TYPES::UINT256_T << hash;
    assert(!Validate(ssOperation));


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT256_T << hash << (uint8_t)OP::NOTEQUALS << (uint8_t)OP::TYPES::UINT256_T << hash;
    assert(!Validate(ssOperation));


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT256_T << hash << (uint8_t)OP::NOTEQUALS << (uint8_t)OP::TYPES::UINT256_T << hash + 1;
    assert(Validate(ssOperation));


    runtime::sleep(3500);

    /////REGISTERS
    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::REGISTER::STATE << hash << (uint8_t)OP::EQUALS << (uint8_t) OP::TYPES::UINT256_T << hash2;
    assert(Validate(ssOperation));

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::REGISTER::OWNER << hash << (uint8_t)OP::EQUALS << (uint8_t) OP::TYPES::UINT256_T << state.hashOwner;
    assert(Validate(ssOperation));

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::REGISTER::TIMESTAMP << hash << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT32_T << 3u << (uint8_t)OP::LESSTHAN << (uint8_t) OP::CHAIN::UNIFIED;
    assert(Validate(ssOperation));

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::REGISTER::TYPE << hash << (uint8_t)OP::EQUALS << (uint8_t) OP::TYPES::UINT64_T << (uint64_t) 2;
    assert(Validate(ssOperation));




    ////////////COMPUTATION
    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 555u << (uint8_t) OP::SUB << (uint8_t) OP::TYPES::UINT32_T << 333u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 222u;
    assert(Validate(ssOperation));

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 9234837u << (uint8_t) OP::SUB << (uint8_t) OP::TYPES::UINT32_T << 384728u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 8850109u;
    assert(Validate(ssOperation));

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 905u << (uint8_t) OP::MOD << (uint8_t) OP::TYPES::UINT32_T << 30u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 5u;
    assert(Validate(ssOperation));

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT64_T << (uint64_t)1000000000000 << (uint8_t) OP::DIV << (uint8_t) OP::TYPES::UINT64_T << (uint64_t)30000 << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 33333333u;
    assert(Validate(ssOperation));

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT64_T << (uint64_t)5 << (uint8_t) OP::EXP << (uint8_t) OP::TYPES::UINT64_T << (uint64_t)15 << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT64_T << (uint64_t)30517578125;
    assert(Validate(ssOperation));

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 2u << (uint8_t) OP::EXP << (uint8_t) OP::TYPES::UINT32_T << 8u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 256u;
    assert(Validate(ssOperation));

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT64_T << (uint64_t)9837 << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT64_T << (uint64_t)7878 << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 17715u;
    assert(Validate(ssOperation));

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT64_T << (uint64_t)9837 << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT32_T << 7878u << (uint8_t)OP::LESSTHAN << (uint8_t)OP::TYPES::UINT256_T << uint256_t(17716);
    assert(Validate(ssOperation));




    //////////////// AND / OR
    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 9837u << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT32_T << 7878u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 17715u;
    ssOperation << (uint8_t)OP::AND;
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 9837u << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT32_T << 7878u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 17715u;
    ssOperation << (uint8_t)OP::AND;
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 9837u << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT32_T << 7878u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 17715u;
    assert(Validate(ssOperation));



    return 0;
}
