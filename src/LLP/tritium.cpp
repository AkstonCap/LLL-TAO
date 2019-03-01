/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Operation/include/execute.h>

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/include/manager.h>
#include <LLP/types/tritium.h>

#include <TAO/Ledger/types/mempool.h>

#include <Util/include/runtime.h>
#include <Util/include/args.h>
#include <Util/include/debug.h>

#include <climits>

namespace LLP
{

        /* The session identifier. */
        uint64_t TritiumNode::nSessionID = LLC::GetRand();

        /** Virtual Functions to Determine Behavior of Message LLP. **/
        void TritiumNode::Event(uint8_t EVENT, uint32_t LENGTH)
        {
            switch(EVENT)
            {
                case EVENT_CONNECT:
                {
                    /* Setup the variables for this node. */
                    nLastPing    = runtime::timestamp();

                    /* Debut output. */
                    debug::log(1, NODE, " Connected at timestamp ", runtime::unifiedtimestamp());

                    /* Send version if making the connection. */
                    if(fOUTGOING)
                        PushMessage(DAT_VERSION, TritiumNode::nSessionID, GetAddress());

                    break;
                }

                case EVENT_HEADER:
                {
                    break;
                }

                case EVENT_PACKET:
                {
                    break;
                }

                case EVENT_GENERIC:
                {
                    /* Generic event - pings. */
                    if(runtime::timestamp() - nLastPing > 5)
                    {
                        /* Generate the nNonce. */
                        uint64_t nNonce = LLC::GetRand(std::numeric_limits<uint64_t>::max());

                        /* Add to latency tracker. */
                        mapLatencyTracker[nNonce] = runtime::timestamp(true);

                        /* Push a ping request. */
                        PushMessage(DAT_PING, nNonce);

                        /* Update the last ping. */
                        nLastPing = runtime::timestamp();
                    }

                    /* Generic events - unified time. */
                    if(runtime::timestamp() - nLastSamples > 30)
                    {
                        /* Generate the request identification. */
                        uint64_t nRequestID = LLC::GetRand(std::numeric_limits<uint32_t>::max());

                        /* Add sent requests. */
                        mapSentRequests[nRequestID] = runtime::timestamp();

                        /* Request time samples. */
                        PushMessage(GET_OFFSET, nRequestID, runtime::timestamp(true));

                        /* Update the samples timer. */
                        nLastSamples = runtime::timestamp();
                    }

                    break;
                }

                case EVENT_DISCONNECT:
                {
                    /* Debut output. */
                    uint32_t reason = LENGTH;
                    std::string strReason;

                    switch(reason)
                    {
                        case DISCONNECT_TIMEOUT:
                            strReason = "DISCONNECT_TIMEOUT";
                            break;
                        case DISCONNECT_ERRORS:
                            strReason = "DISCONNECT_ERRORS";
                            break;
                        case DISCONNECT_DDOS:
                            strReason = "DISCONNECT_DDOS";
                            break;
                        case DISCONNECT_FORCE:
                            strReason = "DISCONNECT_FORCE";
                            break;
                        default:
                            strReason = "UNKNOWN";
                            break;
                    }

                    /* Print disconnect and reason message */
                    debug::log(1, NODE, " Disconnected at timestamp ", runtime::unifiedtimestamp(),
                        " (", strReason, ")");

                    if(TRITIUM_SERVER && TRITIUM_SERVER->pAddressManager)
                        TRITIUM_SERVER->pAddressManager->AddAddress(GetAddress(), ConnectState::DROPPED);

                    break;
                }
            }
        }


        /** Main message handler once a packet is recieved. **/
        bool TritiumNode::ProcessPacket()
        {

            DataStream ssPacket(INCOMING.DATA, SER_NETWORK, PROTOCOL_VERSION);
            switch(INCOMING.MESSAGE)
            {

                case DAT_VERSION:
                {
                    /* Deserialize the session identifier. */
                    uint64_t nSession;
                    ssPacket >> nSession;

                    /* Get your address. */
                    BaseAddress addr;
                    ssPacket >> addr;

                    /* Check for a connect to self. */
                    if(nSession == TritiumNode::nSessionID)
                    {
                        debug::log(0, FUNCTION, "connected to self");

                        /* Cache self-address in the banned list of the Address Manager. */
                        if(TRITIUM_SERVER && TRITIUM_SERVER->pAddressManager)
                            TRITIUM_SERVER->pAddressManager->Ban(addr);

                        return false;
                    }



                    /* Send version message if connection is inbound. */
                    if(!fOUTGOING)
                        PushMessage(DAT_VERSION, TritiumNode::nSessionID, GetAddress());
                    else
                        PushMessage(GET_ADDRESSES);

                    /* Debug output for offsets. */
                    debug::log(3, NODE, "received session identifier ",nSessionID);

                    break;
                }


                case GET_OFFSET:
                {
                    /* Deserialize request id. */
                    uint32_t nRequestID;
                    ssPacket >> nRequestID;

                    /* Deserialize the timestamp. */
                    uint64_t nTimestamp;
                    ssPacket >> nTimestamp;

                    /* Find the sample offset. */
                    int32_t nOffset = static_cast<int32_t>(runtime::timestamp(true) - nTimestamp);

                    /* Debug output for offsets. */
                    debug::log(3, NODE, "received timestamp of ", nTimestamp, " sending offset ", nOffset);

                    /* Push a timestamp in response. */
                    PushMessage(DAT_OFFSET, nRequestID, nOffset);

                    break;
                }


                case DAT_OFFSET:
                {
                    /* Deserialize the request id. */
                    uint32_t nRequestID;
                    ssPacket >> nRequestID;

                    /* Check map known requests. */
                    if(!mapSentRequests.count(nRequestID))
                        return debug::error(NODE "offset not requested");

                    /* Check the time since request was sent. */
                    if(runtime::timestamp() - mapSentRequests[nRequestID] > 10)
                    {
                        debug::log(0, NODE, "offset is stale.");
                        mapSentRequests.erase(nRequestID);

                        break;
                    }

                    /* Deserialize the offset. */
                    int32_t nOffset;
                    ssPacket >> nOffset;

                    /* Debug output for offsets. */
                    debug::log(3, NODE, "received offset ", nOffset);

                    /* Remove sent requests from mpa. */
                    mapSentRequests.erase(nRequestID);

                    break;
                }


                case DAT_HAS_TX:
                {
                    /* Deserialize the data received. */
                    std::vector<uint512_t> vData;
                    ssPacket >> vData;

                    /* Request the inventory. */
                    for(const auto& hash : vData)
                        PushMessage(GET_TRANSACTION, hash);

                    break;
                }


                case GET_TRANSACTION:
                {
                    /* Deserialize the inventory. */
                    uint512_t hash;
                    ssPacket >> hash;

                    /* Check if you have it. */
                    TAO::Ledger::Transaction tx;
                    if(LLD::legDB->ReadTx(hash, tx) || TAO::Ledger::mempool.Get(hash, tx))
                        PushMessage(DAT_TRANSACTION, tx);

                    break;
                }


                case DAT_HAS_BLOCK:
                {
                    /* Deserialize the data received. */
                    std::vector<uint1024_t> vData;
                    ssPacket >> vData;

                    /* Request the inventory. */
                    for(const auto& hash : vData)
                        PushMessage(GET_BLOCK, hash);

                    break;
                }


                case GET_BLOCK:
                {
                    /* Deserialize the inventory. */
                    uint1024_t hash;
                    ssPacket >> hash;

                    /* Check if you have it. */
                    TAO::Ledger::BlockState state;
                    if(LLD::legDB->ReadBlock(hash, state))
                    {
                        TAO::Ledger::TritiumBlock block(state);
                        PushMessage(DAT_BLOCK, block);
                    }

                    break;
                }


                case DAT_TRANSACTION:
                {
                    /* Deserialize the tx. */
                    TAO::Ledger::Transaction tx;
                    ssPacket >> tx;

                    /* Check if we have it. */
                    if(!LLD::legDB->HasTx(tx.GetHash()))
                    {
                        /* Debug output for tx. */
                        debug::log(3, NODE "Received tx ", tx.GetHash().ToString().substr(0, 20));

                        /* Check if tx is valid. */
                        if(!tx.IsValid())
                        {
                            debug::error(NODE "tx ", tx.GetHash().ToString().substr(0, 20), " REJECTED");

                            break;
                        }

                        /* Add the transaction to the memory pool. */
                        TAO::Ledger::mempool.Accept(tx);
                    }

                    /* Debug output for offsets. */
                    debug::log(3, NODE "already have tx ", tx.GetHash().ToString().substr(0, 20));

                    break;
                }


                case GET_ADDRESSES:
                {
                    /* Grab the connections. */
                    if(TRITIUM_SERVER)
                    {
                        std::vector<LegacyAddress> vAddr = TRITIUM_SERVER->GetAddresses();
                        std::vector<LegacyAddress> vLegacyAddr;

                        for(auto it = vAddr.begin(); it != vAddr.end(); ++it)
                            vLegacyAddr.push_back(*it);

                        /* Push the response addresses. */
                        PushMessage(DAT_ADDRESSES, vLegacyAddr);
                    }

                    break;
                }


                case DAT_ADDRESSES:
                {
                    /* De-Serialize the Addresses. */
                    std::vector<LegacyAddress> vLegacyAddr;
                    std::vector<BaseAddress> vAddr;
                    ssPacket >> vLegacyAddr;

                    if(TRITIUM_SERVER)
                    {
                        /* try to establish the connection on the port the server is listening to */
                        for(auto it = vLegacyAddr.begin(); it != vLegacyAddr.end(); ++it)
                        {
                            if(config::mapArgs.find("-port") != config::mapArgs.end())
                            {
                                uint16_t port = static_cast<uint16_t>(atoi(config::mapArgs["-port"].c_str()));
                                it->SetPort(port);
                            }

                            else
                                it->SetPort(TRITIUM_SERVER->PORT);

                            /* Create a base address vector from legacy addresses */
                            vAddr.push_back(*it);
                        }


                        /* Add the connections to Tritium Server. */
                        if(TRITIUM_SERVER->pAddressManager)
                            TRITIUM_SERVER->pAddressManager->AddAddresses(vAddr);
                    }

                    break;
                }

                case DAT_PING:
                {
                    /* Deserialize the nOnce. */
                    uint32_t nNonce;
                    ssPacket >> nNonce;

                    /* Push a pong as a response. */
                    PushMessage(DAT_PONG, nNonce);

                    break;
                }

                case DAT_PONG:
                {
                    /* Deserialize the nOnce. */
                    uint32_t nNonce;
                    ssPacket >> nNonce;

                    /* Check for unsolicted pongs. */
                    if(!mapLatencyTracker.count(nNonce))
                        return debug::error(NODE "unsolicited pong");

                    uint32_t lat = static_cast<uint32_t>(runtime::timestamp(true) - mapLatencyTracker[nNonce]);

                    /* Set the latency used for address manager within server */
                    if(TRITIUM_SERVER && TRITIUM_SERVER->pAddressManager)
                        TRITIUM_SERVER->pAddressManager->SetLatency(lat, GetAddress());

                    /* Debug output for latency. */
                    debug::log(3, NODE "latency ", lat, " ms");

                    /* Clear the latency tracker record. */
                    mapLatencyTracker.erase(nNonce);

                    break;
                }
            }

            return true;
        }

}
