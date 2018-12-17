/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

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
#include <LLP/types/corenode.h>
#include <LLP/types/rpcnode.h>
#include <TAO/API/include/rpc.h>

#include <LLP/include/global.h>

#include <TAO/Ledger/types/mempool.h>

/* Declare the Global LLD Instances. */
namespace LLD
{
    RegisterDB* regDB;
    LedgerDB*   legDB;
    LocalDB*    locDB;
}

/* Declare the Global LLP Instances. */
namespace LLP
{
    Server<TritiumNode>* TRITIUM_SERVER;
    Server<LegacyNode> *LEGACY_SERVER;
}

#include <LLC/include/random.h>
#include <cmath>

int main(int argc, char** argv)
{

    /* Handle all the signals with signal handler method. */
    SetupSignals();


    /* Parse out the parameters */
    config::ParseParameters(argc, argv);


    /* Read the configuration file. */
    config::ReadConfigFile(config::mapArgs, config::mapMultiArgs);


    /* Handle Commandline switch */
    for (int i = 1; i < argc; i++)
    {
        if (!IsSwitchChar(argv[i][0]))
        {
            if(config::GetBoolArg("-api"))
                return TAO::API::CommandLineAPI(argc, argv, i);

            return TAO::API::CommandLineRPC(argc, argv, i);
        }
    }


    /* Create directories if they don't exist yet. */
    if(filesystem::create_directory(config::GetDataDir(false)))
        debug::log(0, FUNCTION "Generated Path %s", __PRETTY_FUNCTION__, config::GetDataDir(false).c_str());


    /* Create the database instances. */
    LLD::regDB = new LLD::RegisterDB("r+");
    LLD::legDB = new LLD::LedgerDB("r+");
    LLD::locDB = new LLD::LocalDB("r+");


    /* Initialize the Legacy Server. */
    LLP::TRITIUM_SERVER = new LLP::Server<LLP::TritiumNode>(config::GetArg("-port", config::fTestNet ? 8888 : 9888), 10, 30, false, 0, 0, 60, config::GetBoolArg("-listen", true), true);
    if(config::mapMultiArgs["-addnode"].size() > 0)
    {
        for(auto node : config::mapMultiArgs["-addnode"])
        {
            LLP::Address addr = LLP::Address(LLP::Service(debug::strprintf("%s:%i", node.c_str(), config::GetArg("-port", config::fTestNet ? 8888 : 9888)).c_str(), false));
            LLP::TRITIUM_SERVER->AddConnection(node, config::GetArg("-port", config::fTestNet ? 8888 : 9888));
        }
    }

    // LLP::LEGACY_SERVER = new LLP::Server<LLP::LegacyNode>(config::GetArg("-port", config::fTestNet ? 8323 : 9323), 10, 30, false, 0, 0, 60, config::GetBoolArg("-listen", true), true);
    // if(config::mapMultiArgs["-addnode"].size() > 0)
    // {
    //     for(auto node : config::mapMultiArgs["-addnode"])
    //         LLP::LEGACY_SERVER->AddConnection(node, config::GetArg("-port", config::fTestNet ? 8323 : 9323));
    // }


    /* Create the Core API Server. */
    LLP::Server<LLP::CoreNode>* CORE_SERVER = new LLP::Server<LLP::CoreNode>(config::GetArg("-apiport", 8080), 10, 30, false, 0, 0, 60, config::GetBoolArg("-listen", true), config::GetBoolArg("-meters", false));


    /* Set up RPC server */
    TAO::API::RPCCommands = new TAO::API::RPC();
    TAO::API::RPCCommands->Initialize();
    LLP::Server<LLP::RPCNode>* RPC_SERVER = new LLP::Server<LLP::RPCNode>(config::GetArg("-rpcport", config::fTestNet? 8336 : 9336), 1, 30, false, 0, 0, 60, config::GetBoolArg("-listen", true), config::GetBoolArg("-meters", false));


    /* Wait for Shutdown. */
    while(!config::fShutdown)
    {
        Sleep(1000);
    }

    //LLP::LEGACY_SERVER->addressManager.WriteDatabase();

    return 0;
}
