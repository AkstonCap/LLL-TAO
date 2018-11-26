/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/signals.h>
#include <Util/include/convert.h>
#include <Util/include/runtime.h>
#include <Util/include/filesystem.h>

#include <TAO/API/include/cmd.h>
#include <TAO/API/include/core.h>
#include <LLP/templates/server.h>

#include <TAO/API/types/music.h>


int main(int argc, char** argv)
{
    /* Handle all the signals with signal handler method. */
    SetupSignals();


    /* Parse out the parameters */
    ParseParameters(argc, argv);


    /* Handle Commandline switch */
    for (int i = 1; i < argc; i++)
    {
        if (!IsSwitchChar(argv[i][0]) && !(strlen(argv[i]) >= 7 && strncasecmp(argv[i], "Nexus:", 7) == 0))
        {
            return TAO::API::CommandLine(argc, argv, i);
        }
    }


    /* Create directories if they don't exist yet. */
    if(filesystem::create_directory(GetDataDir(false)))
        printf(FUNCTION "Generated Path %s\n", __PRETTY_FUNCTION__, GetDataDir(false).c_str());


    /* Read the configuration file. */
    ReadConfigFile(mapArgs, mapMultiArgs);


    /* Create the database instances. */
    LLD::regDB = new LLD::RegisterDB("r+");
    LLD::legDB = new LLD::LedgerDB("r+");
    LLD::locDB = new LLD::LocalDB("r+");

    /* Initialize the API's. */
    TAO::API::Music::Initialize();

    LLP::Server<TAO::API::Core>* CORE_SERVER = new LLP::Server<TAO::API::Core>(8080, 10, 30, false, 0, 0, 60, true, false);
    while(!fShutdown)
    {
        Sleep(1000);
    }


    TAO::Ledger::SignatureChain sigChain(GetArg("-username", "user"), GetArg("-password", "default"));
    uint512_t hashGenesis = sigChain.Generate(0, GetArg("-pin", "1235"));

    printf("Genesis %s\n", hashGenesis.ToString().c_str());

    /* Extract username and password from config. */
    printf("Username: %s\n", GetArg("-username", "user").c_str());
    printf("Password: %s\n", GetArg("-password", "default").c_str());

    return 0;
}
