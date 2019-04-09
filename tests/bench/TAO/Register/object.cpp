#include <Util/include/runtime.h>

#include <TAO/Register/include/object.h>

#include <unit/catch2/catch.hpp>


TEST_CASE( "Object Register Benchmarks", "[register]" )
{
    using namespace TAO::Register;

    Object object;
    object << std::string("byte") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT8_T) << uint8_t(55)
           << std::string("test") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::STRING) << std::string("this string")
           << std::string("bytes") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::BYTES) << std::vector<uint8_t>(10, 0xff)
           << std::string("balance") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT64_T) << uint64_t(55)
           << std::string("identifier") << uint8_t(TYPES::STRING) << std::string("NXS");

    debug::log(0, "===== Begin Object Register Benchmarks =====");

    //benchmarks
    {
        runtime::timer timer;
        timer.Start();

        for(int i = 0; i < 1000000; i++)
        {
            object.mapData.clear();
            object.Parse();
        }

        uint64_t nTime = timer.ElapsedMicroseconds();

        //5000000 -> 5 times 1000000 for microseconds -> 5 for the number of values in this object register
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Parse::", ANSI_COLOR_RESET, 5000000.0 / nTime, " million values / second");
    }



    {
        runtime::timer timer;
        timer.Start();

        uint8_t nWrite = 77;
        for(int i = 0; i < 1000000; i++)
            object.Write("byte", nWrite);

        uint64_t nTime = timer.ElapsedMicroseconds();

        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Write::", ANSI_COLOR_RESET, 1000000.0 / nTime, " million uint8_t / second");
    }


    {
        runtime::timer timer;
        timer.Start();

        uint64_t nWrite = 7777;
        for(int i = 0; i < 1000000; i++)
            object.Write("balance", nWrite);

        uint64_t nTime = timer.ElapsedMicroseconds();

        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Write::", ANSI_COLOR_RESET, 1000000.0 / nTime, " million uint64_t / second");
    }


    {
        runtime::timer timer;
        timer.Start();

        std::string strWrite = "this strnnn";
        for(int i = 0; i < 1000000; i++)
            object.Write("test", strWrite);

        uint64_t nTime = timer.ElapsedMicroseconds();

        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Write::", ANSI_COLOR_RESET, 1000000.0 / nTime, " million strings / second");
    }


    {
        runtime::timer timer;
        timer.Start();

        std::vector<uint8_t> vWrite(10, 0xff);
        for(int i = 0; i < 1000000; i++)
            object.Write("bytes", vWrite);

        uint64_t nTime = timer.ElapsedMicroseconds();

        //10m here -> 10 bytes in vector -> 1000000 iterations / microseconds
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Write::", ANSI_COLOR_RESET, 1000000.0 / nTime, " million vectors / second");
    }



    {
        runtime::timer timer;
        timer.Start();

        uint8_t nRead;
        for(int i = 0; i < 1000000; i++)
            object.Read("byte", nRead);

        uint64_t nTime = timer.ElapsedMicroseconds();

        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Read ::", ANSI_COLOR_RESET, 1000000.0 / nTime, " million uint8_t / second");
    }


    {
        runtime::timer timer;
        timer.Start();

        uint64_t nRead;
        for(int i = 0; i < 1000000; i++)
            object.Read("balance", nRead);

        uint64_t nTime = timer.ElapsedMicroseconds();

        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Read ::", ANSI_COLOR_RESET, 1000000.0 / nTime, " million uint64_t / second");
    }


    {
        runtime::timer timer;
        timer.Start();

        std::string strRead;
        for(int i = 0; i < 1000000; i++)
            object.Read("test", strRead);

        uint64_t nTime = timer.ElapsedMicroseconds();

        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Read ::", ANSI_COLOR_RESET, 1000000.0 / nTime, " million strings / second");
    }


    {
        runtime::timer timer;
        timer.Start();

        std::vector<uint8_t> vRead(10);
        for(int i = 0; i < 1000000; i++)
            object.Write("bytes", vRead);

        uint64_t nTime = timer.ElapsedMicroseconds();

        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Read ::", ANSI_COLOR_RESET, 1000000.0 / nTime, " million vectors / second");
    }


    debug::log(0, "===== End Object Register Benchmarks =====\n");
}
