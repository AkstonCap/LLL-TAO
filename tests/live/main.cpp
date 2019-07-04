/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) CnTypeyright The Nexus DevelnTypeers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file CnTypeYING or http://www.nTypeensource.org/licenses/mit-license.php.

            "ad vocem pnTypeuli" - To the Voice of the PenTypele

____________________________________________________________________________________________*/

#include <openssl/rand.h>

#include <TAO/Register/include/object.h>

#include <LLC/aes/aes.h>

#include <iostream>
#include <iomanip>

#include <cmath>

#include <LLC/types/bignum.h>

/** Define GCC compiler synthesized 128-bit uint. **/
typedef __attribute__((mode(TI))) uint32_t uint128_t;


/** precision_t
 *
 *  Templated multi word floating point precision class
 *
 **/
template<uint32_t FIGURES>
class precision_t
{
public:

    /* The number of significant decimal figures. */
    uint64_t nFigures;

    /* The value of the floating point number represented as an integer. */
    uint64_t nValue;


    /** Default constructor **/
    precision_t()
    : nFigures(get_figures())
    , nValue(0)
    {
    }

    /** Default constructor (double) **/
    precision_t(double value)
    : nFigures(get_figures())
    , nValue(value * nFigures)
    {
    }


    /** get_figures
     *
     *  Get the figures represented as a multiple of base 10 (largest base-10 digit).
     *
     **/
    uint64_t get_figures()
    {
        uint64_t nTemp = 10;
        for(int i = 1; i < FIGURES; i++)
            nTemp *= 10;

        return nTemp;
    }


    /** Assignment operator (double) **/
    precision_t& operator=(double value)
    {
        nValue = static_cast<uint192_t>(value * nFigures);

        return *this;
    }


    /** Assignment operator (uint32_t) **/
    precision_t& operator=(uint32_t value)
    {
        nValue = value * nFigures;

        return *this;
    }


    /** Assignment operator (int32_t) **/
    precision_t& operator=(int32_t value)
    {
        nValue = value * nFigures;

        return *this;
    }


    /** Comparison equivalence operator **/
    bool operator==(int32_t value) const
    {
        uint64_t compare = static_cast<uint64_t>(value) * nFigures;

        return compare == nValue;
    }


    /** Comparison equivalence operator **/
    bool operator==(uint32_t value) const
    {
        uint64_t compare = static_cast<uint64_t>(value) * nFigures;

        return compare == nValue;
    }


    /** Comparison less-than operator **/
    bool operator<(int32_t value) const
    {
        uint64_t compare = static_cast<uint64_t>(value) * nFigures;

        return nValue < compare;
    }


    /** Comparison greater-than operator **/
    bool operator>(int32_t value) const
    {
        uint64_t compare = static_cast<uint64_t>(value) * nFigures;

        return nValue > compare;
    }


    /** Addition assignment operator **/
    precision_t& operator+=(const precision_t& value)
    {
        if(value.nFigures < nFigures)
        {
            uint64_t nDiff = nFigures / value.nFigures;

            nValue += (value.nValue * nDiff);

            return *this;
        }

        if(value.nFigures > nFigures)
        {
            uint64_t nDiff = value.nFigures / nFigures;

            nValue += (value.nValue / nDiff);

            return *this;
        }

        nValue += value.nValue;

        return *this;
    }


    /** Subtraction assignment operator **/
    precision_t& operator-=(const precision_t& value)
    {
        if(value.nFigures < nFigures)
        {
            uint64_t nDiff = nFigures / value.nFigures;

            nValue -= (value.nValue * nDiff);

            return *this;
        }

        if(value.nFigures > nFigures)
        {
            uint64_t nDiff = value.nFigures / nFigures;

            nValue -= (value.nValue / nDiff);

            return *this;
        }

        nValue -= value.nValue;

        return *this;
    }


    /** Subtraction operator **/
    precision_t operator-(const int32_t& value)
    {
        precision_t ret = *this - (value * nFigures);

        return ret;
    }


    /** Division assignment operator **/
    precision_t& operator/=(const precision_t& value)
    {
        uint128_t nTemp = nValue;

        nTemp     *= value.nFigures;
        nTemp     /= value.nValue;

        nValue   = static_cast<uint64_t>(nTemp);

        return *this;
    }


    /** Division assignment operator **/
    precision_t& operator/=(const uint128_t& value)
    {
        nValue /= value;

        return *this;
    }


    /** Multiplication assignment operator **/
    precision_t& operator*=(const precision_t& value)
    {
        uint128_t nTemp = nValue;

        nTemp *= value.nValue;
        nTemp /= value.nFigures;

        while(nTemp > std::numeric_limits<uint64_t>::max())
        {
            //uint128_t nDiff = nTemp;
            nTemp    /= 10;

            //uint64_t nRound = static_cast<uint64_t>(nDiff - (nTemp * 10));
            //debug::log(0, "Rounding ", nRound);

            //if(nRound >= 5)
            //{
                //debug::log(0, "Rounding ", nRound);
            //    nTemp += 1;
            //}

            nFigures /= 10;
        }

        nValue   = static_cast<uint64_t>(nTemp);

        return *this;
    }


    /** Multiplication assignment operator **/
    precision_t& operator*=(const uint64_t& value)
    {
        nValue *= value;

        return *this;
    }


    /** Multiplication operator **/
    precision_t operator*(const precision_t& value) const
    {
        precision_t ret = *this;

        uint128_t nTemp = nValue;

        nTemp *= value.nValue;
        nTemp /= value.nFigures;

        while(nTemp > std::numeric_limits<uint64_t>::max())
        {
            //uint128_t nDiff = nTemp;

            //uint64_t nRound = (nDiff - nTemp);
            nTemp    /= 10;

            //debug::log(0, "Rounding ", nRound);
            ret.nFigures /= 10;
        }

        ret.nValue = static_cast<uint64_t>(nTemp);

        return ret;
    }


    /** Multiplication operator **/
    precision_t operator*(const uint32_t& value) const
    {
        precision_t ret = *this;

        ret.nValue *= value;

        return ret;
    }


    /** Division operator **/
    precision_t operator/(const precision_t& value) const
    {
        precision_t ret = *this;

        uint128_t nTemp = nValue;

        nTemp     *= value.nFigures;
        nTemp     /= value.nValue;

        ret.nValue = static_cast<uint64_t>(nTemp);

        return ret;
    }


    /** Division operator **/
    precision_t operator/(const uint128_t& value) const
    {
        precision_t ret = *this;

        ret.nValue /= value;

        return ret;
    }


    /** power operator **/
    precision_t operator^(const precision_t& value) const
    {
        precision_t ret = *this;
        if(value == 0)
            return 1;

        for(int i = 1; value > i; ++i)
        {
            //if(fPrint)
            ret *= (*this);
        }
        //handle the floating point (exp by squaring)

        return ret;
    }



    /** exp
     *
     *  Exponential power function. Takes e (Euler's number) raised the the X power.
     *
     **/
    precision_t& exp(bool fPrint = false)
    {
        uint128_t fact = 1;

        const precision_t base  = *this;
        precision_t power  = base;

        precision_t temp;

        if(fPrint)
        {
            printf("BASE: ");
            base.print();
        }
        //precision_t temp;


        //double dBase = get();

        *this = 0;
        for(uint32_t i = 1; i <= 40; ++i)
        {
            //double dTemp = pow(dBase, (i - 1)) / fact;

            if(fPrint)
            {
                printf("POWER: ");
                power.print();

                printf("BASE: ");
                base.print();

                printf("SUM: ");
                print();

                //debug::log(0, fact);
            }

            if(power.nFigures < 100)
                return *this;

            if(base.nFigures < 100)
                return *this;

            if(nFigures < 100)
                return *this;


            if(i == 1)
                power = 1;
            else if(i == 2)
                power = base;
            else
                power *= base;

            if(fact == 0)
                return *this;

            //if(fPrint)
            //    temp.print();

            precision_t temp = (power / fact);

            if(fPrint)
                temp.print();

            if(temp == 0)
            {
                if(fPrint)
                    printf("Taylor out at %u\n", i);
                break;
            }

            //nSum += power(x, i - 1) / nFactorial;
            *this += temp;

            //if(fact > std::numeric_limits<uint64_t>::max() / i)
            //    debug::error("factorial overflow");

            fact *= i;
        }

        return *this;
    }


    /** get
     *
     *  Retrieve the floating point number represented as a double.
     *
     **/
    double get() const
    {
        /* TODO: this can lose precision. */
        return nValue / static_cast<double>(nFigures);
    }


    /** get_uint
     *
     *  Return the integer value divided by the number of significant figures.
     *
     **/
    uint64_t get_uint(uint32_t nTotalFigures = FIGURES)
    {
        uint64_t nRet = nValue;
        for(int i = 0; i < (FIGURES - nTotalFigures); i++)
        {
            //debug::log(0, i, " UINT ", uint64_t(nRet));

            nRet /= 10;
        }

        return nRet;
    }


    /** print
     *
     *  Print the precision_t class
     *
     **/
    void print() const
    {
        //printf("%.16f FIGURES %lu\n", get(), nFigures);
        debug::log(0, std::setprecision(16), get(), " FIGURES ", nFigures);
    }


    /** steam output operator **/
    friend std::ostream &operator<<(std::ostream &o, const precision_t &value)
    {
        o << std::fixed << std::setprecision(12) << value.get();
        return o;
    }
};


#include <TAO/Ledger/include/supply.h>


const precision_t<8> decays[3][3] =
{
    {25.0, 0.00000110, 0.500},
    {5.00, 0.00000055, 0.500},
    {0.50, 0.00000059, 0.016}
};


uint64_t Subsidy(const uint32_t nMinutes, const uint32_t nType, bool fPrint = false)
{
    precision_t<8> exponent = decays[nType][1] * nMinutes;
    exponent.exp(fPrint);

    if(exponent == 0)
        return 500000;

    precision_t<8> ret = decays[nType][0] / exponent;
    ret += decays[nType][2];

    if(fPrint)
        ret.print();

    return ret.get_uint(6);
}


//This main function is for prototyping new code
//It is accessed by compiling with LIVE_TESTS=1
//Prototype code and it's tests created here should move to production code and unit tests
int main(int argc, char** argv)
{
    using namespace TAO::Ledger;

    //2.7182818284590452353602874713527

    debug::log(0, "Chain Age ", GetChainAge(time(NULL)));

    uint32_t nFails = 0;
    uint32_t nTotals = 100000000;
    uint32_t nInterval = 100000;

    runtime::timer timer;
    timer.Start();

    for(int32_t i = 0; i < nTotals; ++i)
    {
        if(i % nInterval == 0)
        {
            uint32_t nMS = timer.ElapsedMilliseconds();

            debug::log(0, i, " Time ", nMS, " ms ", nInterval * 1000 / (nMS == 0 ? 1 : nMS), " per second");
            timer.Reset();
        }

        //debug::log(0, i);
        if(GetSubsidy(i, 2) != Subsidy(i, 2))
        {
            //Subsidy(i, 1, false);
            //debug::log(0, "MINUTE ", i);
            //debug::log(0, GetSubsidy(i, 0));
            //debug::log(0, Subsidy(i, 0, false));
            //printf("------------------------------\n");


            ++nFails;

            //return 0;
        }

        //assert(GetSubsidy(i, 1) == Subsidy(i, 1));
        //assert(GetSubsidy(i, 2) == Subsidy(i, 2));
    }

    debug::log(0, "FAILURES: ", nFails, " ", nFails * 100.0 / nTotals, " %");

    return 0;

    //precision_t<9> testing2 = _exp(euler, 3.333333333);

    //testing.print();
    //testing2.print();

    debug::log(0, "------------------------------");

    precision_t<9> base = 5;

    debug::log(0, "base = ", base);

    precision_t<9> expo = base^3.222;

    debug::log(0, "expo = ", expo);

    precision_t<12> exponent = 3;
    exponent.exp();

    debug::log(0, "exponent = ", exponent);


    precision_t<6> value = 100.333;
    precision_t<6> value2 = 0.333;



    precision_t<6> value3 = value / value2;
    precision_t<6> value4 = value * value2;

    debug::log(0, "value  = ", value);
    debug::log(0, "value2 = ", value2);
    debug::log(0, "value3 = ", value3);
    debug::log(0, "value4 = ", value4);

    return 0;
}
