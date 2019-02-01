/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <Legacy/types/transaction.h>
#include <Legacy/types/locator.h>
#include <Legacy/wallet/wallet.h>

#include <LLP/include/hosts.h>
#include <LLP/include/inv.h>
#include <LLP/include/global.h>
#include <LLP/types/legacy.h>
#include <LLP/templates/events.h>

#include <Util/include/args.h>
#include <Util/include/hex.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/include/chainstate.h>

namespace LLP
{

    /* Static initialization of last get blocks. */
    uint1024_t LegacyNode::hashLastGetblocks = 0;


    /* The time since last getblocks. */
    uint64_t LegacyNode::nLastGetBlocks = 0;


    /* Push a Message With Information about This Current Node. */
    void LegacyNode::PushVersion()
    {
        /* Random Session ID */
        RAND_bytes((uint8_t*)&nSessionID, sizeof(nSessionID));

        /* Current Unified timestamp. */
        int64_t nTime = runtime::unifiedtimestamp();

        /* Dummy Variable NOTE: Remove in Tritium ++ */
        uint64_t nLocalServices = 0;

        /* Relay Your Address. */
        LegacyAddress addrMe;
        LegacyAddress addrYou = GetAddress();

        /* Push the Message to receiving node. */
        PushMessage("version", LLP::PROTOCOL_VERSION, nLocalServices, nTime, addrYou, addrMe,
                    nSessionID, strProtocolName, TAO::Ledger::ChainState::nBestHeight);
    }


    /** Handle Event Inheritance. **/
    void LegacyNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        /** Handle any DDOS Packet Filters. **/
        if(EVENT == EVENT_HEADER)
        {
            const std::string message = INCOMING.GetMessage();
            uint32_t length = INCOMING.LENGTH;

            debug::log(3, NODE, "recieved Message (", message, ", ", length, ")");

            if(fDDOS)
            {

                /* Give higher DDOS score if the Node happens to try to send multiple version messages. */
                if (message == "version" && nCurrentVersion != 0)
                    DDOS->rSCORE += 25;


                /* Check the Packet Sizes to Unified Time Commands. */
                if((message == "getoffset" || message == "offset") && length != 16)
                    DDOS->Ban(debug::strprintf("INVALID PACKET SIZE | OFFSET/GETOFFSET | LENGTH %u", length));
            }

            return;
        }


        /** Handle for a Packet Data Read. **/
        if(EVENT == EVENT_PACKET)
        {

            /* Check a packet's validity once it is finished being read. */
            if(fDDOS)
            {

                /* Give higher score for Bad Packets. */
                if(INCOMING.Complete() && !INCOMING.IsValid())
                {

                    debug::log(3, NODE, "Dropped Packet (Complete: ", INCOMING.Complete() ? "Y" : "N",
                        " - Valid: )",  INCOMING.IsValid() ? "Y" : "N");

                    DDOS->rSCORE += 15;
                }

            }

            if(INCOMING.Complete())
            {
                debug::log(4, NODE, "Received Packet (", INCOMING.LENGTH, ", ", INCOMING.GetBytes().size(), ")");

                if(config::GetArg("-verbose", 0) >= 5)
                    PrintHex(INCOMING.GetBytes());
            }

            return;
        }


        /* Handle Node Pings on Generic Events */
        if(EVENT == EVENT_GENERIC)
        {

            if(nLastPing + 15 < runtime::unifiedtimestamp())
            {
                RAND_bytes((uint8_t*)&nSessionID, sizeof(nSessionID));

                nLastPing = runtime::unifiedtimestamp();

                mapLatencyTracker.emplace(nSessionID, runtime::timer());
                mapLatencyTracker[nSessionID].Start();
                            /* Reset the timeouts. */

                PushMessage("ping", nSessionID);

                /* Rebroadcast transactions. */
                Legacy::Wallet::GetInstance().ResendWalletTransactions();
            }

            //TODO: mapRequests data, if no response given retry the request at given times
        }


        /** On Connect Event, Assign the Proper Handle. **/
        if(EVENT == EVENT_CONNECT)
        {
            nLastPing    = runtime::unifiedtimestamp();

            debug::log(1, NODE, fOUTGOING ? "Outgoing" : "Incoming",
                       " Connected at timestamp ",   runtime::unifiedtimestamp());

            if(fOUTGOING)
                PushVersion();

            return;
        }


        /* Handle the Socket Disconnect */
        if(EVENT == EVENT_DISCONNECT)
        {
            std::string strReason = "";
            switch(LENGTH)
            {
                case DISCONNECT_TIMEOUT:
                    strReason = "Timeout";
                    break;

                case DISCONNECT_ERRORS:
                    strReason = "Errors";
                    break;

                case DISCONNECT_DDOS:
                    strReason = "DDOS";
                    break;

                case DISCONNECT_FORCE:
                    strReason = "Forced";
                    break;
            }

            if(LEGACY_SERVER && LEGACY_SERVER->pAddressManager)
                LEGACY_SERVER->pAddressManager->AddAddress(GetAddress(), ConnectState::DROPPED);

            debug::log(1, NODE, fOUTGOING ? "Outgoing" : "Incoming",
                " Disconnected (", strReason, ") at timestamp ", runtime::unifiedtimestamp());

            return;
        }

    }

    /** This function is necessary for a template LLP server. It handles your
        custom messaging system, and how to interpret it from raw packets. **/
    bool LegacyNode::ProcessPacket()
    {

        DataStream ssMessage(INCOMING.DATA, SER_NETWORK, MIN_PROTO_VERSION);

        const std::string message = INCOMING.GetMessage();

        if(message == "getoffset")
        {
            /* Don't service unified seeds unless time is unified. */
            //if(!Core::fTimeUnified)
            //    return true;

            /* De-Serialize the Request ID. */
            uint32_t nRequestID;
            ssMessage >> nRequestID;

            /* De-Serialize the timestamp Sent. */
            uint64_t nTimestamp;
            ssMessage >> nTimestamp;

            /* Log into the sent requests Map. */
            mapSentRequests[nRequestID] = runtime::unifiedtimestamp(true);

            /* Calculate the offset to current clock. */
            int   nOffset    = (int)(runtime::unifiedtimestamp(true) - nTimestamp);
            PushMessage("offset", nRequestID, runtime::unifiedtimestamp(true), nOffset);

            /* Verbose logging. */
            debug::log(3, NODE, ": Sent Offset ", nOffset,
                "Unified ", runtime::unifiedtimestamp());
        }

        /* Recieve a Time Offset from this Node. */
        else if(message == "offset")
        {

            /* De-Serialize the Request ID. */
            uint32_t nRequestID;
            ssMessage >> nRequestID;


            /* De-Serialize the timestamp Sent. */
            uint64_t nTimestamp;
            ssMessage >> nTimestamp;

            /* Handle the Request ID's. */
            //uint32_t nLatencyTime = (Core::runtime::unifiedtimestamp(true) - nTimestamp);


            /* Ignore Messages Recieved that weren't Requested. */
            if(!mapSentRequests.count(nRequestID))
            {
                DDOS->rSCORE += 5;

                debug::log(3, NODE, "Invalid Request : Message Not Requested [", nRequestID, "][", nNodeLatency, " ms]");

                return true;
            }


            /* Reject Samples that are recieved 30 seconds after last check on this node. */
            if(runtime::unifiedtimestamp(true) - mapSentRequests[nRequestID] > 30000)
            {
                mapSentRequests.erase(nRequestID);

                debug::log(3, NODE, "Invalid Request : Message Stale [", nRequestID, "][", nNodeLatency, " ms]");

                DDOS->rSCORE += 15;

                return true;
            }


            /* De-Serialize the Offset. */
            int nOffset;
            ssMessage >> nOffset;

            /* Adjust the Offset for Latency. */
            nOffset -= nNodeLatency;

            /* Add the Samples. */
            //setTimeSamples.insert(nOffset);

            /* Remove the Request from the Map. */
            mapSentRequests.erase(nRequestID);

            /* Verbose Logging. */
            debug::log(3, NODE, "Received Unified Offset ", nOffset, " [", nRequestID, "][", nNodeLatency, " ms]");
        }


        /* Push a transaction into the Node's Recieved Transaction Queue. */
        else if (message == "tx")
        {
            /* Deserialize the Transaction. */
            Legacy::Transaction tx;
            ssMessage >> tx;

            /* Check for sync. */
            if(TAO::Ledger::ChainState::Synchronizing())
                return true;

            /* Accept to memory pool. */
            TAO::Ledger::BlockState notUsed;
            if (TAO::Ledger::mempool.Accept(tx))
            {
                Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(tx, notUsed, true);

                std::vector<CInv> vInv = { CInv(tx.GetHash(), MSG_TX) };
                LEGACY_SERVER->Relay("inv", vInv);
            }
        }


        /* Push a block into the Node's Recieved Blocks Queue. */
        else if (message == "block")
        {
            /* Deserialize the block. */
            Legacy::LegacyBlock block;
            ssMessage >> block;

            /* Process the block. */
            if(!LegacyNode::Process(block, this))
                return false;

            return true;
        }


        /* Send a Ping with a nNonce to get Latency Calculations. */
        else if (message == "ping")
        {
            uint64_t nonce = 0;
            ssMessage >> nonce;

            PushMessage("pong", nonce);
        }


        else if(message == "pong")
        {
            uint64_t nonce = 0;
            ssMessage >> nonce;

            /* If the nonce was not received or known from pong. */
            if(!mapLatencyTracker.count(nonce))
            {
                DDOS->rSCORE += 5;
                return true;
            }

            /* Calculate the Average Latency of the Connection. */
            nNodeLatency = mapLatencyTracker[nonce].ElapsedMilliseconds();
            mapLatencyTracker.erase(nonce);

            /* Set the latency used for address manager within server */
            if(LEGACY_SERVER && LEGACY_SERVER->pAddressManager)
                LEGACY_SERVER->pAddressManager->SetLatency(nNodeLatency, GetAddress());

            /* Debug Level 3: output Node Latencies. */
            debug::log(3, NODE, "Latency (Nonce ", std::hex, nonce, " - ", std::dec, nNodeLatency, " ms)");
        }


        /* ______________________________________________________________
        *
        *
        * NOTE: These following methods will be deprecated post Tritium.
        *
        * ______________________________________________________________
        */


        /* Message Version is the first message received.
        * It gives you basic stats about the node to know how to
        * communicate with it.
        */
        else if (message == "version")
        {

            int64_t nTime;
            LegacyAddress addrMe;
            LegacyAddress addrFrom;
            uint64_t nServices = 0;

            /* Check the Protocol Versions */
            ssMessage >> nCurrentVersion;

            /* Deserialize the rest of the data. */
            ssMessage >> nServices >> nTime >> addrMe >> addrFrom >> nSessionID >> strNodeVersion >> nStartingHeight;
            debug::log(1, NODE, "version message: version ", nCurrentVersion, ", blocks=",  nStartingHeight);

            /* Check the server if it is set. */
            if(!LEGACY_SERVER->addrThisNode.IsValid())
            {
                addrMe.SetPort(config::GetArg("-port", config::fTestNet ? 8323 : 9323));
                debug::log(0, NODE, "recieved external address ", addrMe.ToString());

                LEGACY_SERVER->addrThisNode = addrMe;
            }

            /* Send version message if connection is inbound. */
            if(!fOUTGOING)
            {
                if(addrMe.ToStringIP() == LEGACY_SERVER->addrThisNode.ToStringIP())
                {
                    debug::log(0, NODE, "connected to self ", addr.ToString());

                    return false;
                }
            }


            /* Send the Version Response to ensure communication cTAO::Ledger::ChainState::hashBestChain == hashLastGetblockshannel is open. */
            PushMessage("verack");

            /* Push our version back since we just completed getting the version from the other node. */
            static uint32_t nAsked = 0;
            if (fOUTGOING && nAsked == 0)
            {
                nAsked++;
                PushGetBlocks(TAO::Ledger::ChainState::hashBestChain, uint1024_t(0));
            }
            else
                PushVersion();

            PushMessage("getaddr");
        }


        /* Handle a new Address Message.
        * This allows the exchanging of addresses on the network.
        */
        else if (message == "addr")
        {
            std::vector<LegacyAddress> vLegacyAddr;
            std::vector<BaseAddress> vAddr;

            ssMessage >> vLegacyAddr;

            /* Don't want addr from older versions unless seeding */
            if (vLegacyAddr.size() > 2000)
            {
                DDOS->rSCORE += 20;

                return debug::error(NODE, "message addr size() = ", vLegacyAddr.size(), "... Dropping Connection");
            }

            if(LEGACY_SERVER)
            {
                /* Try to establish the connection on the port the server is listening to. */
                for(auto it = vLegacyAddr.begin(); it != vLegacyAddr.end(); ++it)
                {
                    it->SetPort(LEGACY_SERVER->PORT);

                    /* Create a base address vector from legacy addresses */
                    vAddr.push_back(*it);
                }

                /* Add the connections to Legacy Server. */
                if(LEGACY_SERVER->pAddressManager)
                    LEGACY_SERVER->pAddressManager->AddAddresses(vAddr);
            }

        }


        /* Handle new Inventory Messages.
        * This is used to know what other nodes have in their inventory to compare to our own.
        */
        else if (message == "inv")
        {
            std::vector<CInv> vInv;
            ssMessage >> vInv;

            debug::log(3, NODE, "Inventory Message of ", vInv.size(), " elements");

            /* Make sure the inventory size is not too large. */
            if (vInv.size() > 10000)
            {
                DDOS->rSCORE += 20;

                return true;
            }

            /* Fast sync mode. */
            if(config::GetBoolArg("-fastsync"))
            {
                if (TAO::Ledger::ChainState::Synchronizing() && vInv.back().GetType() == MSG_BLOCK)
                {
                    /* Single block inventory message signals to check from best chain. (If nothing in 10 seconds) */
                    if(vInv.size() == 1 && TAO::Ledger::ChainState::hashBestChain == hashLastGetblocks && nLastGetBlocks + 10 < runtime::timestamp())
                    {
                        /* Special handle for unreliable leagacy nodes. */
                        if(TAO::Ledger::ChainState::Synchronizing())
                        {
                            /* Check *FOR NOW* to deal with unreliable *LEGACY* seed node. */
                            if(TAO::Ledger::ChainState::hashBestChain == hashLastGetblocks && hashLastGetblocks != 0)
                                ++nConsecutiveTimeouts;
                            else //reset consecutive timeouts
                                nConsecutiveTimeouts = 0;

                            /* Catch *FOR NOW* if seed node becomes unresponsive and gives bad data.
                             * This happens in 3 or 4 places during synchronization if it is a
                             * legacy node you are talking to. (height 1223722, 1226573 are some instances)
                             */
                            if(nConsecutiveTimeouts > 1)
                            {
                                /* Reset the timeouts. */
                                nConsecutiveTimeouts = 0;

                                /* Log that node is reconnecting. */
                                debug::log(0, NODE, "node has become unresponsive during sync... reconnecting...");

                                /* Disconnect and send TCP_RST. */
                                Disconnect();

                                /* Make the connection again. */
                                if (Attempt(addr))
                                {
                                    /* Log successful reconnect. */
                                    debug::log(1, NODE, "Connected to ", addr.ToString());

                                    /* Set the connected flag. */
                                    fCONNECTED = true;

                                    /* Push a new version message. */
                                    PushVersion();

                                    /* Ask for the blocks again nicely. */
                                    PushGetBlocks(TAO::Ledger::ChainState::hashBestChain, uint1024_t(0));

                                    return true;
                                }
                                else
                                    return false;
                            }
                        }

                        /* Normal case of asking for a getblocks inventory message. */
                        debug::log(0, NODE, "fast sync node timed out, trying a new node from best");
                        LegacyNode* pnode = LEGACY_SERVER->GetConnection();
                        if(pnode)
                            PushGetBlocks(TAO::Ledger::ChainState::hashBestChain, uint1024_t(0));
                    }

                    /* Otherwise ask for another batch of blocks from the end of this inventory. */
                    else
                    {
                        /* Normal case of asking for a getblocks inventory message. */
                        PushGetBlocks(vInv.back().GetHash(), uint1024_t(0));
                    }
                }
            }

            /* Push getdata after fastsync inv (if enabled).
             * This will ask for a new inv before blocks to
             * always stay at least 1k blocks ahead.
             */
            PushMessage("getdata", vInv);
        }


        //DEPRECATED
        else if (message == "headers")
        {
            return true; //DEPRECATED
        }


        /* Get the Data for a Specific Command.
        TODO: Expand this for the data types.
        */
        else if (message == "getdata")
        {
            std::vector<CInv> vInv;
            ssMessage >> vInv;

            if (vInv.size() > 10000)
            {
                DDOS->rSCORE += 20;

                return true;
            }

            /* Loop the inventory and deliver messages. */
            for(const auto& inv : vInv)
            {
                if (config::fShutdown)
                    return true;

                /* Log the inventory message receive. */
                debug::log(3, FUNCTION, "received getdata ", inv.ToString());

                /* Handle the block message. */
                if (inv.GetType() == LLP::MSG_BLOCK)
                {
                    /* Read the block from disk. */
                    TAO::Ledger::BlockState state;
                    if(!LLD::legDB->ReadBlock(inv.GetHash(), state))
                        continue;

                    /* Scan each transaction in the block and process those related to this wallet */
                    Legacy::LegacyBlock block(state);

                    /* Push the response message. */
                    PushMessage("block", block);

                    /* Trigger a new getblocks if hash continue is set. */
                    if (inv.GetHash() == hashContinue)
                    {
                        std::vector<CInv> vInv = { CInv(TAO::Ledger::ChainState::hashBestChain, LLP::MSG_BLOCK) };
                        PushMessage("inv", vInv);
                        hashContinue = 0;
                    }
                }
                else if (inv.GetType() == LLP::MSG_TX)
                {
                    Legacy::Transaction tx;
                    if(!TAO::Ledger::mempool.Get(inv.GetHash().getuint512(), tx) && !LLD::legacyDB->ReadTx(inv.GetHash().getuint512(), tx))
                        continue;

                    PushMessage("tx", tx);
                }
            }
        }


        /* Handle a Request to get a list of Blocks from a Node. */
        else if (message == "getblocks")
        {
            Legacy::Locator locator;
            uint1024_t hashStop;
            ssMessage >> locator >> hashStop;

            /* Return if nothing in locator. */
            if(locator.vHave.size() == 0)
                return false;

            /* Get the block state from. */
            TAO::Ledger::BlockState state;
            if(!LLD::legDB->ReadBlock(locator.vHave[0], state))
                return true;

            int32_t nLimit = 1000;
            debug::log(3, "getblocks ", state.nHeight, " to ", hashStop.ToString().substr(0, 20), " limit ", nLimit);

            /* Iterate forward the blocks required. */
            std::vector<CInv> vInv;
            while(!config::fShutdown)
            {
                if (state.GetHash() == hashStop)
                {
                    debug::log(3, "  getblocks stopping at ", state.nHeight, " to ", state.GetHash().ToString().substr(0, 20));

                    /* Tell about latest block if hash stop is found. */
                    if (hashStop != TAO::Ledger::ChainState::hashBestChain)
                        vInv.push_back(CInv(TAO::Ledger::ChainState::hashBestChain, MSG_BLOCK));

                    break;
                }

                vInv.push_back(CInv(state.GetHash(), MSG_BLOCK));
                if (--nLimit <= 0)
                {
                    // When this block is requested, we'll send an inv that'll make them
                    // getblocks the next batch of inventory.
                    debug::log(3, "  getblocks stopping at limit ", state.nHeight, " to ", state.GetHash().ToString().substr(0,20));

                    hashContinue = state.GetHash();
                    break;
                }

                state = state.Next();
            }

            /* Push the inventory. */
            if(vInv.size() > 0)
                PushMessage("inv", vInv);
        }


        //DEPRECATED
        else if (message == "getheaders")
        {
            return true; //DEPRECATED
        }


        /* TODO: Change this Algorithm. */
        else if (message == "getaddr")
        {
            /* Get addresses from manager. */
            std::vector<BaseAddress> vAddr;
            if(LEGACY_SERVER->pAddressManager)
                LEGACY_SERVER->pAddressManager->GetAddresses(vAddr);

            /* Add the best 1000 addresses. */
            std::vector<LegacyAddress> vSend;
            for(uint32_t n = 0; n < vAddr.size() && n < 1000; n++)
                vSend.push_back(vAddr[n]);

            /* Send the addresses off. */
            if(vSend.size() > 0)
                PushMessage("addr", vSend);
        }


        return true;
    }

    static std::map<uint1024_t, Legacy::LegacyBlock> mapLegacyOrphans;
    static std::mutex PROCESSING_MUTEX;

    /* pnode = Node we received block from, nullptr if we are originating the block (mined or staked) */
    bool LegacyNode::Process(const Legacy::LegacyBlock& block, LegacyNode* pnode)
    {
        /* Check if the block is valid. */
        uint1024_t hash = block.GetHash();
        if(!block.Check())
        {
            debug::log(3, FUNCTION, "block failed checks");

            return true;
        }

        /* Erase from orphan queue. */
        { LOCK(PROCESSING_MUTEX);
            if(mapLegacyOrphans.count(hash))
                mapLegacyOrphans.erase(hash);
        }

        /* Check for orphan. */
        if(!LLD::legDB->HasBlock(block.hashPrevBlock))
        {
            LOCK(PROCESSING_MUTEX);

            /* Skip if already in orphan queue. */
            if(!mapLegacyOrphans.count(block.hashPrevBlock))
                mapLegacyOrphans[block.hashPrevBlock] = block;

            /* Debug output. */
            debug::log(0, FUNCTION, "ORPHAN height=", block.nHeight, " hash=", block.GetHash().ToString().substr(0, 20));

            /* Normal sync mode (slower connections). */
            if(!TAO::Ledger::ChainState::Synchronizing())
                pnode->PushGetBlocks(TAO::Ledger::ChainState::hashBestChain, uint1024_t(0));
            else if(!config::GetBoolArg("-fastsync"))
            {
                if(!TAO::Ledger::ChainState::Synchronizing() || TAO::Ledger::ChainState::hashBestChain != LegacyNode::hashLastGetblocks || LegacyNode::nLastGetBlocks + 10 < runtime::timestamp())
                {
                    /* Special handle for unreliable leagacy nodes. */
                    if(TAO::Ledger::ChainState::Synchronizing())
                    {
                        /* Check *FOR NOW* to deal with unreliable *LEGACY* seed node. */
                        if(TAO::Ledger::ChainState::hashBestChain == LegacyNode::hashLastGetblocks && LegacyNode::hashLastGetblocks != 0)
                            ++pnode->nConsecutiveTimeouts;
                        else //reset consecutive timeouts
                            pnode->nConsecutiveTimeouts = 0;

                        /* Catch *FOR NOW* if seed node becomes unresponsive and gives bad data.
                         * This happens in 3 or 4 places during synchronization if it is a
                         * legacy node you are talking to. (height 1223722, 1226573 are some instances)
                         */
                        if(pnode->nConsecutiveTimeouts > 1)
                        {
                            /* Reset the timeouts. */
                            pnode->nConsecutiveTimeouts = 0;

                            /* Disconnect and send TCP_RST. */
                            pnode->Disconnect();

                            /* Make the connection again. */
                            if (pnode->Attempt(pnode->addr))
                            {
                                /* Set the connected flag. */
                                pnode->fCONNECTED = true;

                                /* Push a new version message. */
                                pnode->PushVersion();

                                /* Ask for the blocks again nicely. */
                                pnode->PushGetBlocks(TAO::Ledger::ChainState::hashBestChain, uint1024_t(0));

                                return true;
                            }
                            else
                                return false;
                        }
                    }

                    /* Normal case of asking for a getblocks inventory message. */
                    LegacyNode* pBest = LEGACY_SERVER->GetConnection();
                    if(pBest)
                        pBest->PushGetBlocks(TAO::Ledger::ChainState::hashBestChain, uint1024_t(0));
                }
            }

            return true;
        }

        { LOCK(PROCESSING_MUTEX);

            /* Create the Block State. */
            TAO::Ledger::BlockState state(block);

            /* Check if it exists first */
            if(LLD::legDB->HasBlock(block.GetHash()))
                return true;

            /* Check if valid in the chain. */
            if(!block.Accept())
            {
                debug::log(3, FUNCTION, "block failed to be added to chain");

                return true;
            }

            /* Process the block state. */
            if(!state.Accept())
            {
                debug::log(3, FUNCTION, "block state failed processing");

                return true;
            }

            /* Process orphan if found. */
            uint32_t nOrphans = 0;
            while(mapLegacyOrphans.count(hash))
            {
                Legacy::LegacyBlock& orphan = mapLegacyOrphans[hash];

                debug::log(0, FUNCTION, "processing ORPHAN prev=", orphan.GetHash().ToString().substr(0, 20), " size=",mapLegacyOrphans.size());
                TAO::Ledger::BlockState stateOrphan(orphan);
                if(!stateOrphan.Accept())
                    return true;

                mapLegacyOrphans.erase(hash);
                hash = stateOrphan.GetHash();

                ++nOrphans;
            }

            /* Handle for orphans. */
            if(nOrphans > 0)
            {
                debug::log(0, FUNCTION, "processed ", nOrphans, " ORPHANS");
            }


        }

        return true;
    }

}
