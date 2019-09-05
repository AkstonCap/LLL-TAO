/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <LLP/types/tritium.h>
#include <LLP/include/global.h>
#include <LLP/templates/events.h>
#include <LLP/include/manager.h>

#include <TAO/Register/include/names.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/process.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/types/locator.h>
#include <TAO/Ledger/types/syncblock.h>
#include <TAO/Ledger/types/mempool.h>

#include <Legacy/wallet/wallet.h>

#include <Util/include/runtime.h>
#include <Util/include/args.h>
#include <Util/include/debug.h>
#include <Util/include/version.h>


#include <climits>
#include <memory>
#include <iomanip>
#include <bitset>

namespace LLP
{

    /* Declaration of sessions mutex. (private). */
    std::mutex TritiumNode::SESSIONS_MUTEX;


    /* Declaration of sessions sets. (private). */
    std::map<uint64_t, std::pair<uint32_t, uint32_t>> TritiumNode::mapSessions;


    /* The current sync node. */
    std::atomic<uint64_t> TritiumNode::nSyncSession(0);


    /** Default Constructor **/
    TritiumNode::TritiumNode()
    : BaseConnection<TritiumPacket>()
    , fAuthorized(false)
    , nSubscriptions(0)
    , nNotifications(0)
    , nLastPing(0)
    , nLastSamples(0)
    , mapLatencyTracker()
    , hashGenesis(0)
    , nTrust(0)
    , nProtocolVersion(0)
    , nCurrentSession(0)
    , nCurrentHeight(0)
    , hashCheckpoint(0)
    , nConsecutiveOrphans(0)
    , nConsecutiveFails(0)
    , strFullVersion()
    , hashLastBlock(0)
    , hashLastTx({0, 0})
    {
    }


    /** Constructor **/
    TritiumNode::TritiumNode(Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS)
    : BaseConnection<TritiumPacket>(SOCKET_IN, DDOS_IN, isDDOS)
    , fAuthorized(false)
    , nSubscriptions(0)
    , nNotifications(0)
    , nLastPing(0)
    , nLastSamples(0)
    , mapLatencyTracker()
    , hashGenesis(0)
    , nTrust(0)
    , nProtocolVersion(0)
    , nCurrentSession(0)
    , nCurrentHeight(0)
    , hashCheckpoint(0)
    , nConsecutiveOrphans(0)
    , nConsecutiveFails(0)
    , strFullVersion()
    , hashLastBlock(0)
    , hashLastTx({0, 0})
    {
    }


    /** Constructor **/
    TritiumNode::TritiumNode(DDOS_Filter* DDOS_IN, bool isDDOS)
    : BaseConnection<TritiumPacket>(DDOS_IN, isDDOS)
    , fAuthorized(false)
    , nSubscriptions(0)
    , nNotifications(0)
    , nLastPing(0)
    , nLastSamples(0)
    , mapLatencyTracker()
    , hashGenesis(0)
    , nTrust(0)
    , nProtocolVersion(0)
    , nCurrentSession(0)
    , nCurrentHeight(0)
    , hashCheckpoint(0)
    , nConsecutiveOrphans(0)
    , nConsecutiveFails(0)
    , strFullVersion()
    , hashLastBlock(0)
    , hashLastTx({0, 0})
    {
    }


    /** Default Destructor **/
    TritiumNode::~TritiumNode()
    {
    }


    /** Virtual Functions to Determine Behavior of Message LLP. **/
    void TritiumNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        switch(EVENT)
        {
            case EVENT_CONNECT:
            {
                debug::log(1, NODE, fOUTGOING ? "Outgoing" : "Incoming",
                       " Connected at timestamp ",   runtime::unifiedtimestamp());

                /* Set the laset ping time. */
                nLastPing    = runtime::unifiedtimestamp();

                /* Respond with version message if incoming connection. */
                if(fOUTGOING)
                {
                    /* Respond with version message. */
                    PushMessage(ACTION::VERSION, PROTOCOL_VERSION, SESSION_ID, version::CLIENT_VERSION_BUILD_STRING);

                    /* Notify node of current block height. */
                    PushMessage(ACTION::NOTIFY,
                        uint8_t(TYPES::HEIGHT),     TAO::Ledger::ChainState::nBestHeight.load(),
                        uint8_t(TYPES::CHECKPOINT), TAO::Ledger::ChainState::hashCheckpoint.load());
                }

                break;
            }

            case EVENT_HEADER:
            {
                /* Check for initialization. */
                if(nCurrentSession == 0 && nProtocolVersion == 0 && INCOMING.MESSAGE != ACTION::VERSION && DDOS)
                    DDOS->rSCORE += 25;

                break;
            }

            case EVENT_PACKET:
            {
                /* Check a packet's validity once it is finished being read. */
                if(fDDOS)
                {
                    /* Give higher score for Bad Packets. */
                    if(INCOMING.Complete() && !INCOMING.IsValid())
                    {
                        debug::log(3, NODE "dropped packet (complete: ", INCOMING.Complete() ? "Y" : "N",
                            " - Valid:)",  INCOMING.IsValid() ? "Y" : "N");

                        if(DDOS)
                            DDOS->rSCORE += 15;
                    }
                }

                if(INCOMING.Complete())
                {
                    if(config::GetArg("-verbose", 0) >= 5)
                        PrintHex(INCOMING.GetBytes());
                }

                break;
            }

            case EVENT_GENERIC:
            {
                /* Handle sending the pings to remote node.. */
                if(nLastPing + 15 < runtime::unifiedtimestamp())
                {
                    /* Create a random nonce. */
                    uint64_t nNonce = LLC::GetRand();
                    nLastPing = runtime::unifiedtimestamp();

                    /* Keep track of latency for this ping. */
                    mapLatencyTracker.insert(std::pair<uint64_t, runtime::timer>(nNonce, runtime::timer()));
                    mapLatencyTracker[nNonce].Start();

                    /* Push new message. */
                    PushMessage(ACTION::PING, nNonce);

                    /* Rebroadcast transactions. */
                    //if(!TAO::Ledger::ChainState::Synchronizing())
                    //    Legacy::Wallet::GetInstance().ResendWalletTransactions();
                }

                break;
            }

            case EVENT_DISCONNECT:
            {
                /* Debut output. */
                std::string strReason;
                switch(LENGTH)
                {
                    case DISCONNECT_TIMEOUT:
                        strReason = "Timeout";
                        break;

                    case DISCONNECT_ERRORS:
                        strReason = "Errors";
                        break;

                    case DISCONNECT_POLL_ERROR:
                        strReason = "Poll Error";
                        break;

                    case DISCONNECT_POLL_EMPTY:
                        strReason = "Unavailable";
                        break;

                    case DISCONNECT_DDOS:
                        strReason = "DDOS";
                        break;

                    case DISCONNECT_FORCE:
                        strReason = "Force";
                        break;

                    default:
                        strReason = "Unknown";
                        break;
                }

                /* Debug output for node disconnect. */
                debug::log(1, NODE, fOUTGOING ? "Outgoing" : "Incoming",
                    " Disconnected (", strReason, ") at timestamp ", runtime::unifiedtimestamp());

                if(TRITIUM_SERVER && TRITIUM_SERVER->pAddressManager)
                    TRITIUM_SERVER->pAddressManager->AddAddress(GetAddress(), ConnectState::DROPPED);

                /* Handle if sync node is disconnected. */
                if(nCurrentSession == nSyncSession.load())
                {
                    //TODO: find another node
                    nSyncSession.store(0);
                }


                {
                    LOCK(SESSIONS_MUTEX);

                    /* Check for sessions to free. */
                    if(mapSessions.count(nCurrentSession))
                    {
                        /* Make sure that we aren't freeing our session if handling duplicate connections. */
                        const std::pair<uint32_t, uint32_t>& pair = mapSessions[nCurrentSession];
                        if(pair.first == nDataThread && pair.second == nDataIndex)
                            mapSessions.erase(nCurrentSession);
                    }
                }


                break;
            }
        }
    }


    /** Main message handler once a packet is recieved. **/
    bool TritiumNode::ProcessPacket()
    {
        /* Deserialize the packeet from incoming packet payload. */
        DataStream ssPacket(INCOMING.DATA, SER_NETWORK, PROTOCOL_VERSION);
        switch(INCOMING.MESSAGE)
        {

            /* Handle for the version command. */
            case ACTION::VERSION:
            {
                /* Hard requirement for version. */
                ssPacket >> nProtocolVersion;

                /* Get the current session-id. */
                ssPacket >> nCurrentSession;

                /* Get the version string. */
                ssPacket >> strFullVersion;

                /* Check for a connect to self. */
                if(nCurrentSession == SESSION_ID)
                    return debug::drop(NODE, "connected to self");

                /* Check if session is already connected. */
                {
                    LOCK(SESSIONS_MUTEX);
                    if(mapSessions.count(nCurrentSession))
                        return debug::drop(NODE, "duplicate connection");

                    /* Set this to the current session. */
                    mapSessions[nCurrentSession] = std::make_pair(nDataThread, nDataIndex);
                }

                /* Get the current connected legacy node. */
                memory::atomic_ptr<LegacyNode>& pnode = LegacyNode::GetNode(nCurrentSession);
                try //we want to catch exceptions thrown by atomic_ptr in the case there was a free on another thread
                {
                    /* if connected, send a drop message. */
                    if(pnode != nullptr)
                        pnode->Disconnect();
                }
                catch(const std::exception& e) {}


                /* Check versions. */
                if(nProtocolVersion < MIN_PROTO_VERSION)
                    return debug::drop(NODE, "connection using obsolete protocol version");

                /* Respond with version message if incoming connection. */
                if(Incoming())
                {
                    /* Respond with version message. */
                    PushMessage(uint8_t(ACTION::VERSION), PROTOCOL_VERSION, SESSION_ID, version::CLIENT_VERSION_BUILD_STRING);

                    /* Notify node of current block height. */
                    PushMessage(ACTION::NOTIFY,
                        uint8_t(TYPES::HEIGHT),     TAO::Ledger::ChainState::nBestHeight.load(),
                        uint8_t(TYPES::CHECKPOINT), TAO::Ledger::ChainState::hashCheckpoint.load());
                }
                else if(nSyncSession == 0)
                {
                    /* Subscribe to this node. */
                    Subscribe(SUBSCRIPTION::LAST);

                    /* Set the sync session-id. */
                    nSyncSession.store(nCurrentSession);

                    /* Ask for list of blocks. This should be done regardless of whether the connection is incoming or not as
                       either node could have the longer chain. */
                    PushMessage(ACTION::LIST,
                        uint8_t(TYPES::SYNC),
                        uint8_t(TYPES::BLOCK),
                        uint8_t(TYPES::LOCATOR),
                        TAO::Ledger::Locator(TAO::Ledger::ChainState::hashBestChain.load()),
                        uint1024_t(0)
                    );
                }

                break;
            }


            /* Handle for auth command. */
            case ACTION::AUTH:
            {
                /* Disable AUTH messages when synchronizing. */
                if(TAO::Ledger::ChainState::Synchronizing())
                    return true;

                /* Hard requirement for genesis. */
                ssPacket >> hashGenesis;

                /* Debug logging. */
                debug::log(0, NODE, "new auth request from ", hashGenesis.SubString());

                /* Get the signature information. */
                if(hashGenesis == 0)
                    return debug::drop(NODE, "cannot authorize with reserved genesis");

                /* Get the crypto register. */
                TAO::Register::Object crypto;
                if(!TAO::Register::GetNameRegister(hashGenesis, "crypto", crypto))
                    return debug::drop(NODE, "authorization failed, missing crypto register");

                /* Parse the object. */
                if(!crypto.Parse())
                    return debug::drop(NODE, "failed to parse crypto register");

                /* Check the authorization hash. */
                uint256_t hashCheck = crypto.get<uint256_t>("network");
                if(hashCheck != 0) //a hash of 0 is a disabled authorization hash
                {
                    /* Get the public key. */
                    std::vector<uint8_t> vchPubKey;
                    ssPacket >> vchPubKey;

                    /* Check the public key to expected authorization key. */
                    if(LLC::SK256(vchPubKey) != hashCheck)
                        return debug::drop(NODE, "failed to authorize, invalid public key");

                    /* Get the signature. */
                    std::vector<uint8_t> vchSig;
                    ssPacket >> vchSig;

                    /* Switch based on signature type. */
                    switch(hashCheck.GetType())
                    {
                        /* Support for the FALCON signature scheeme. */
                        case TAO::Ledger::SIGNATURE::FALCON:
                        {
                            /* Create the FL Key object. */
                            LLC::FLKey key;

                            /* Set the public key and verify. */
                            key.SetPubKey(vchPubKey);
                            if(!key.Verify(hashGenesis.GetBytes(), vchSig))
                                return debug::drop(NODE, "invalid transaction signature");

                            break;
                        }

                        /* Support for the BRAINPOOL signature scheme. */
                        case TAO::Ledger::SIGNATURE::BRAINPOOL:
                        {
                            /* Create EC Key object. */
                            LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                            /* Set the public key and verify. */
                            key.SetPubKey(vchPubKey);
                            if(!key.Verify(hashGenesis.GetBytes(), vchSig))
                                return debug::drop(NODE, "invalid transaction signature");

                            break;
                        }

                        default:
                            return debug::drop(NODE, "invalid signature type");
                    }

                    /* Get the crypto register. */
                    TAO::Register::Object trust;
                    if(!TAO::Register::GetNameRegister(hashGenesis, "trust", trust))
                        return debug::drop(NODE, "authorization failed, missing trust register");

                    /* Parse the object. */
                    if(!trust.Parse())
                        return debug::drop(NODE, "failed to parse trust register");

                    /* Set the node's current trust score. */
                    nTrust = trust.get<uint64_t>("trust");

                    /* Set to authorized node if passed all cryptographic checks. */
                    fAuthorized = true;
                }

                break;
            }


            /* Handle for the subscribe command. */
            case ACTION::SUBSCRIBE:
            {
                /* Set the limits. */
                int32_t nLimits = 16;

                /* Loop through the binary stream. */
                while(!ssPacket.End() && nLimits-- > 0)
                {
                    /* Read the type. */
                    uint8_t nType = 0;
                    ssPacket >> nType;

                    /* Switch based on type. */
                    switch(nType)
                    {
                        /* Subscribe to getting blocks. */
                        case TYPES::BLOCK:
                        {
                            /* Set the block flag. */
                            nNotifications |= SUBSCRIPTION::BLOCK;

                            /* Debug output. */
                            debug::log(3, NODE, "ACTION::SUBSCRIBE: added block subscrption ", std::bitset<16>(nNotifications));

                            break;
                        }

                        /* Subscribe to getting transactions. */
                        case TYPES::TRANSACTION:
                        {
                            /* Set the transaction flag. */
                            nNotifications |= SUBSCRIPTION::TRANSACTION;

                            /* Debug output. */
                            debug::log(3, NODE, "ACTION::SUBSCRIBE: added tx subscrption ", std::bitset<16>(nNotifications));

                            break;
                        }

                        /* Subscribe to getting best height. */
                        case TYPES::HEIGHT:
                        {
                            /* Set the best height flag. */
                            nNotifications |= SUBSCRIPTION::HEIGHT;

                            /* Debug output. */
                            debug::log(3, NODE, "ACTION::SUBSCRIBE: added height subscrption ", std::bitset<16>(nNotifications));

                            break;
                        }

                        /* Subscribe to getting checkpoints. */
                        case TYPES::CHECKPOINT:
                        {
                            /* Set the checkpoints flag. */
                            nNotifications |= SUBSCRIPTION::CHECKPOINT;

                            /* Debug output. */
                            debug::log(3, NODE, "ACTION::SUBSCRIBE: added checkpoint subscrption ", std::bitset<16>(nNotifications));

                            break;
                        }

                        /* Subscribe to getting addresses. */
                        case TYPES::ADDRESS:
                        {
                            /* Set the address flag. */
                            nNotifications |= SUBSCRIPTION::ADDRESS;

                            /* Debug output. */
                            debug::log(3, NODE, "ACTION::SUBSCRIBE: added address subscrption ", std::bitset<16>(nNotifications));

                            break;
                        }

                        /* Subscribe to getting last index on list commands. */
                        case TYPES::LAST:
                        {
                            /* Set the last flag. */
                            nNotifications |= SUBSCRIPTION::LAST;

                            /* Debug output. */
                            debug::log(3, NODE, "ACTION::SUBSCRIBE: added last subscrption ", std::bitset<16>(nNotifications));

                            break;
                        }

                        /* Catch unsupported types. */
                        default:
                        {
                            /* Give score for bad types. */
                            if(DDOS)
                                DDOS->rSCORE += 50;
                        }
                    }
                }

                /* Debug output. */
                debug::log(3, NODE, "ACTION::SUBSCRIBE: node subscribed to flags ", std::bitset<16>(nNotifications));

                break;
            }


            /* Handle for list command. */
            case ACTION::LIST:
            {
                /* Set the limits. */
                int32_t nLimits = 1001;

                /* Loop through the binary stream. */
                while(!ssPacket.End() && nLimits != 0)
                {
                    /* Get the next type in stream. */
                    uint8_t nType = 0;
                    ssPacket >> nType;

                    /* Check for legacy specifier. */
                    bool fLegacy = false;
                    if(nType == TYPES::LEGACY)
                    {
                        /* Set legacy specifier. */
                        fLegacy = true;

                        /* Go to next type in stream. */
                        ssPacket >> nType;
                    }


                    /* Check for legacy specifier. */
                    bool fSyncBlock = false;
                    if(nType == TYPES::SYNC)
                    {
                        /* Check for invalid legacy type. */
                        if(fLegacy)
                            return debug::drop(NODE, "can't include two type specifiers");

                        /* Set legacy specifier. */
                        fSyncBlock = true;

                        /* Go to next type in stream. */
                        ssPacket >> nType;
                    }


                    /* Switch based on codes. */
                    switch(nType)
                    {
                        /* Standard type for a block. */
                        case TYPES::BLOCK:
                        {
                            /* Get the index of block. */
                            uint1024_t hashStart;

                            /* Get the object type. */
                            uint8_t nObject = 0;
                            ssPacket >> nObject;

                            /* Switch based on object. */
                            switch(nObject)
                            {
                                /* Check for start from uint1024 type. */
                                case TYPES::UINT1024_T:
                                {
                                    /* Deserialize start. */
                                    ssPacket >> hashStart;

                                    break;
                                }

                                /* Check for start from a locator. */
                                case TYPES::LOCATOR:
                                {
                                    /* Deserialize locator. */
                                    TAO::Ledger::Locator locator;
                                    ssPacket >> locator;

                                    /* Check locator size. */
                                    uint32_t nSize = locator.vHave.size();
                                    if(nSize > 30)
                                        return debug::drop(NODE, "locator size ", nSize, " is too large");

                                    /* Find common ancestor block. */
                                    for(const auto& have : locator.vHave)
                                    {
                                        /* Check the database for the ancestor block. */
                                        if(LLD::Ledger->HasBlock(have))
                                        {
                                            /* Set the starting hash. */
                                            hashStart = have;

                                            break;
                                        }
                                    }

                                    /* Debug output. */
                                    debug::log(3, NODE, "ACTION::LIST: Locator ", hashStart.SubString(), " found");

                                    break;
                                }

                                default:
                                    return debug::drop(NODE, "malformed starting index");
                            }

                            /* Check for search from last. */
                            if(hashStart == 0)
                                hashStart = hashLastBlock;

                            /* Get the ending hash. */
                            uint1024_t hashStop;
                            ssPacket >> hashStop;

                            /* Do a sequential read to obtain the list. */
                            std::vector<TAO::Ledger::BlockState> vStates;
                            while(--nLimits > 0 && hashStart != hashStop &&
                                LLD::Ledger->BatchRead(hashStart, "block", vStates, 1000))
                            {
                                /* Loop through all available states. */
                                for(const auto& state : vStates)
                                {
                                    /* Cache the block hash. */
                                    hashStart = state.GetHash();

                                    /* Skip if not in main chain. */
                                    if(!state.IsInMainChain())
                                        continue;

                                    /* Handle for special sync block type specifier. */
                                    if(fSyncBlock)
                                    {
                                        /* Build the sync block from state. */
                                        TAO::Ledger::SyncBlock block(state);

                                        /* Push message in response. */
                                        PushMessage(TYPES::SYNC, uint8_t(TYPES::BLOCK), block);
                                    }
                                    else
                                    {
                                        /* Check for version to send correct type */
                                        if(state.nVersion < 7)
                                        {
                                            /* Build the legacy block from state. */
                                            Legacy::LegacyBlock block(state);

                                            /* Push message in response. */
                                            PushMessage(TYPES::LEGACY, uint8_t(TYPES::BLOCK), block);
                                        }
                                        else
                                        {
                                            /* Build the legacy block from state. */
                                            TAO::Ledger::TritiumBlock block(state);

                                            /* Push message in response. */
                                            PushMessage(TYPES::BLOCK, block);
                                        }
                                    }

                                    /* Check for stop hash. */
                                    if(--nLimits <= 0 || hashStart == hashStop)
                                        break;
                                }
                            }

                            /* Set the last block. */
                            hashLastBlock = hashStart;

                            /* Check for last subscription. */
                            if(nNotifications & SUBSCRIPTION::LAST)
                                PushMessage(ACTION::NOTIFY, uint8_t(TYPES::LAST), hashLastBlock);

                            break;
                        }

                        /* Standard type for a block. */
                        case TYPES::TRANSACTION:
                        {
                            /* Get the index of block. */
                            uint512_t hashStart;
                            ssPacket >> hashStart;

                            /* Get the ending hash. */
                            uint512_t hashStop;
                            ssPacket >> hashStop;

                            /* Check for invalid specifiers. */
                            if(fSyncBlock)
                                return debug::drop(NODE, "cannot use TYPES::SYNC for transaction lists");

                            /* Check for legacy. */
                            if(fLegacy)
                            {
                                /* Check for search from last. */
                                if(hashStart == 0)
                                    hashStart = hashLastTx[0];

                                /* Do a sequential read to obtain the list. */
                                std::vector<Legacy::Transaction> vtx;
                                while(LLD::Legacy->BatchRead(hashStart, "tx", vtx, 100))
                                {
                                    /* Loop through all available states. */
                                    for(const auto& tx : vtx)
                                    {
                                        /* Cache the block hash. */
                                        hashStart = tx.GetHash();

                                        /* Push the transaction. */
                                        PushMessage(TYPES::LEGACY, uint8_t(TYPES::TRANSACTION), tx);

                                        /* Check for stop hash. */
                                        if(--nLimits == 0 || hashStart == hashStop)
                                            break;
                                    }

                                    /* Check for stop or limits. */
                                    if(nLimits == 0 || hashStart == hashStop)
                                        break;
                                }

                                /* Set the last transction. */
                                hashLastTx[0] = hashStart;
                            }
                            else
                            {
                                /* Check for search from last. */
                                if(hashStart == 0)
                                    hashStart = hashLastTx[1];

                                /* Do a sequential read to obtain the list. */
                                std::vector<TAO::Ledger::Transaction> vtx;
                                while(LLD::Ledger->BatchRead(hashStart, "tx", vtx, 100))
                                {
                                    /* Loop through all available states. */
                                    for(const auto& tx : vtx)
                                    {
                                        /* Cache the block hash. */
                                        hashStart = tx.GetHash();

                                        /* Skip if not in main chain. */
                                        if(!tx.IsConfirmed())
                                            continue;

                                        /* Push the transaction. */
                                        PushMessage(TYPES::TRANSACTION, tx);

                                        /* Check for stop hash. */
                                        if(--nLimits == 0 || hashStart == hashStop)
                                            break;
                                    }

                                    /* Check for stop or limits. */
                                    if(nLimits == 0 || hashStart == hashStop)
                                        break;
                                }

                                /* Set the last transction. */
                                hashLastTx[1] = hashStart;
                            }

                            break;
                        }


                        /* Standard type for a block. */
                        case TYPES::ADDRESS:
                        {
                            /* Get the total list amount. */
                            uint32_t nTotal;
                            ssPacket >> nTotal;

                            /* Check for size constraints. */
                            if(nTotal > 10000)
                                nTotal = 10000;

                            /* Get addresses from manager. */
                            std::vector<BaseAddress> vAddr;
                            if(TRITIUM_SERVER->pAddressManager)
                                TRITIUM_SERVER->pAddressManager->GetAddresses(vAddr);

                            /* Add the best 1000 (or less) addresses. */
                            const uint32_t nCount = std::min((uint32_t)vAddr.size(), nTotal);
                            for(uint32_t n = 0; n < nCount; ++n)
                                PushMessage(TYPES::ADDRESS, vAddr[n]);

                            break;
                        }

                        /* Catch malformed notify binary streams. */
                        default:
                            return debug::drop(NODE, "ACTION::LIST malformed binary stream");
                    }
                }

                break;
            }


            /* Handle for get command. */
            case ACTION::GET:
            {
                /* Loop through the binary stream. */
                int32_t nLimits = 1000;
                while(!ssPacket.End() && --nLimits > 0)
                {
                    /* Get the next type in stream. */
                    uint8_t nType = 0;
                    ssPacket >> nType;

                    /* Check for legacy specifier. */
                    bool fLegacy = false;
                    if(nType == TYPES::LEGACY)
                    {
                        /* Set legacy specifier. */
                        fLegacy = true;

                        /* Go to next type in stream. */
                        ssPacket >> nType;
                    }

                    /* Switch based on codes. */
                    switch(nType)
                    {
                        /* Standard type for a block. */
                        case TYPES::BLOCK:
                        {
                            /* Get the index of block. */
                            uint1024_t hashBlock;
                            ssPacket >> hashBlock;

                            /* Check for legacy. */
                            if(fLegacy)
                            {
                                /* Check the database for the block. */
                                TAO::Ledger::BlockState state;
                                if(LLD::Ledger->ReadBlock(hashBlock, state))
                                {
                                    Legacy::LegacyBlock block(state);
                                    PushMessage(TYPES::LEGACY, uint8_t(TYPES::BLOCK), block);
                                }
                            }
                            else
                            {
                                /* Check the database for the block. */
                                TAO::Ledger::BlockState state;
                                if(LLD::Ledger->ReadBlock(hashBlock, state))
                                {
                                    TAO::Ledger::TritiumBlock block(state);
                                    PushMessage(TYPES::BLOCK, block);
                                }
                            }

                            break;
                        }

                        /* Standard type for a block. */
                        case TYPES::TRANSACTION:
                        {
                            /* Get the index of transaction. */
                            uint512_t hashTx;
                            ssPacket >> hashTx;

                            /* Check for legacy. */
                            if(fLegacy)
                            {
                                /* Check legacy database. */
                                Legacy::Transaction tx;
                                if(LLD::Legacy->ReadTx(hashTx, tx, TAO::Ledger::FLAGS::MEMPOOL))
                                    PushMessage(TYPES::LEGACY, uint8_t(TYPES::TRANSACTION), tx);
                            }
                            else
                            {
                                /* Check ledger database. */
                                TAO::Ledger::Transaction tx;
                                if(LLD::Ledger->ReadTx(hashTx, tx, TAO::Ledger::FLAGS::MEMPOOL))
                                    PushMessage(TYPES::TRANSACTION, tx);
                            }

                            break;
                        }

                        /* Catch malformed notify binary streams. */
                        default:
                            return debug::drop(NODE, "ACTION::GET malformed binary stream");
                    }
                }

                break;
            }


            /* Handle for notify command. */
            case ACTION::NOTIFY:
            {
                /* Create response data stream. */
                DataStream ssResponse(SER_NETWORK, PROTOCOL_VERSION);

                /* Loop through the binary stream. */
                int32_t nLimits = 1000;
                while(!ssPacket.End() && --nLimits > 0)
                {
                    /* Get the next type in stream. */
                    uint8_t nType = 0;
                    ssPacket >> nType;

                    /* Check for legacy specifier. */
                    bool fLegacy = false;
                    if(nType == TYPES::LEGACY)
                    {
                        /* Set legacy specifier. */
                        fLegacy = true;

                        /* Go to next type in stream. */
                        ssPacket >> nType;
                    }

                    /* Switch based on codes. */
                    switch(nType)
                    {
                        /* Standard type for a block. */
                        case TYPES::BLOCK:
                        {
                            /* Check for legacy. */
                            if(fLegacy)
                                return debug::drop(NODE, "block notify can't have legacy specifier");

                            /* Get the index of block. */
                            uint1024_t hashBlock;
                            ssPacket >> hashBlock;

                            /* Check the database for the block. */
                            if(!LLD::Ledger->HasBlock(hashBlock))
                                ssResponse << uint8_t(TYPES::BLOCK) << hashBlock;

                            /* Debug output. */
                            debug::log(3, NODE, "ACTION::NOTIFY: received block ", hashBlock.SubString());

                            break;
                        }

                        /* Standard type for a block. */
                        case TYPES::TRANSACTION:
                        {
                            /* Get the index of transaction. */
                            uint512_t hashTx;
                            ssPacket >> hashTx;

                            /* Check for legacy. */
                            if(fLegacy)
                            {
                                /* Check legacy database. */
                                if(!LLD::Legacy->HasTx(hashTx, TAO::Ledger::FLAGS::MEMPOOL))
                                    ssResponse << uint8_t(TYPES::LEGACY) << uint8_t(TYPES::TRANSACTION) << hashTx;
                            }
                            else
                            {
                                /* Check ledger database. */
                                if(!LLD::Ledger->HasTx(hashTx, TAO::Ledger::FLAGS::MEMPOOL))
                                    ssResponse << uint8_t(TYPES::TRANSACTION) << hashTx;
                            }

                            /* Debug output. */
                            debug::log(3, NODE, "ACTION::NOTIFY: received tx ", hashTx.SubString());

                            break;
                        }

                        /* Standard type for height. */
                        case TYPES::HEIGHT:
                        {
                            /* Check for legacy. */
                            if(fLegacy)
                                return debug::drop(NODE, "height can't have legacy specifier");

                            /* Keep track of current height. */
                            ssPacket >> nCurrentHeight;

                            /* Debug output. */
                            debug::log(3, NODE, "ACTION::NOTIFY: received height ", nCurrentHeight);

                            break;
                        }

                        /* Standard type for a checkpoint. */
                        case TYPES::CHECKPOINT:
                        {
                            /* Check for legacy. */
                            if(fLegacy)
                                return debug::drop(NODE, "checkpoint can't have legacy specifier");

                            /* Keep track of current checkpoint. */
                            ssPacket >> hashCheckpoint;

                            /* Debug output. */
                            debug::log(3, NODE, "ACTION::NOTIFY: received checkpoint ", hashCheckpoint.SubString());

                            break;
                        }


                        /* Standard type for a checkpoint. */
                        case TYPES::LAST:
                        {
                            /* Check for subscription. */
                            if(!(nSubscriptions & SUBSCRIPTION::LAST))
                                return debug::drop(NODE, "unsolicited notification");

                            /* Keep track of current checkpoint. */
                            uint1024_t hashLast;
                            ssPacket >> hashLast;

                            /* Check if is sync node. */
                            if(nCurrentSession == nSyncSession.load())
                            {
                                /* Ask for list of blocks. */
                                PushMessage(ACTION::LIST,
                                    uint8_t(TYPES::LEGACY),
                                    uint8_t(TYPES::BLOCK),
                                    uint8_t(TYPES::UINT1024_T),
                                    uint1024_t(0),
                                    uint1024_t(0)
                                );
                            }

                            /* Debug output. */
                            debug::log(3, NODE, "ACTION::NOTIFY: received last index ", hashLast.SubString());

                            break;
                        }

                        /* Catch malformed notify binary streams. */
                        default:
                            return debug::drop(NODE, "ACTION::NOTIFY malformed binary stream");
                    }
                }

                /* Push a request for the data from notifications. */
                if(ssResponse.size() != 0)
                    WritePacket(NewMessage(ACTION::GET, ssResponse));

                break;
            }


            /* Handle for ping command. */
            case ACTION::PING:
            {
                /* Get the nonce. */
                uint64_t nNonce = 0;
                ssPacket >> nNonce;

                /* Push the pong response. */
                PushMessage(ACTION::PONG, nNonce);

                /* Bump DDOS score. */
                if(DDOS) //a ping shouldn't be sent too much
                    DDOS->rSCORE += 10;

                break;
            }


            /* Handle a pong command. */
            case ACTION::PONG:
            {
                /* Get the nonce. */
                uint64_t nNonce = 0;
                ssPacket >> nNonce;

                /* If the nonce was not received or known from pong. */
                if(!mapLatencyTracker.count(nNonce))
                {
                    /* Bump DDOS score for spammed PONG messages. */
                    if(DDOS)
                        DDOS->rSCORE += 10;

                    return true;
                }

                /* Calculate the Average Latency of the Connection. */
                nLatency = mapLatencyTracker[nNonce].ElapsedMilliseconds();
                mapLatencyTracker.erase(nNonce);

                /* Set the latency used for address manager within server */
                if(TRITIUM_SERVER->pAddressManager)
                    TRITIUM_SERVER->pAddressManager->SetLatency(nLatency, GetAddress());

                /* Debug Level 3: output Node Latencies. */
                debug::log(3, NODE, "Latency (Nonce ", std::hex, nNonce, " - ", std::dec, nLatency, " ms)");

                break;
            }


            /* Standard type for a timeseed. */
            case TYPES::TIMESEED:
            {
                /* Check for authorized node. */
                if(!Authorized())
                    return debug::drop(NODE, "cannot send timeseed if not authorized");

                /* Check trust threshold. */
                if(nTrust < 60 * 60)
                    return debug::drop(NODE, "cannot send timeseed with no trust");

                /* Get the time seed from network. */
                int64_t nTimeSeed = 0;
                ssPacket >> nTimeSeed;

                /* Keep track of the time seeds if accepted. */
                debug::log(2, NODE, "timeseed ", nTimeSeed, " ACCEPTED");

                break;
            }


            /* Standard type for a block. */
            case TYPES::ADDRESS:
            {
                /* Get the base address. */
                BaseAddress addr;
                ssPacket >> addr;

                /* Add addresses to manager.. */
                if(TRITIUM_SERVER->pAddressManager)
                    TRITIUM_SERVER->pAddressManager->AddAddress(addr);

                break;
            }


            /* Handle sync data types. */
            case TYPES::SYNC:
            {
                /* Get the type. */
                uint8_t nType = 0;
                ssPacket >> nType;

                /* Switch based on type. */
                switch(nType)
                {
                    /* Handle for legacy block. */
                    case TYPES::BLOCK:
                    {
                        /* Get the block from the stream. */
                        TAO::Ledger::SyncBlock block;
                        ssPacket >> block;

                        /* Check version switch. */
                        uint8_t nStatus = 0;
                        if(block.nVersion >= 7)
                        {
                            /* Build a tritium block from sync block. */
                            TAO::Ledger::TritiumBlock tritium(block);

                            /* Process the block. */
                            TAO::Ledger::Process(tritium, nStatus);
                        }
                        else
                        {
                            /* Build a tritium block from sync block. */
                            Legacy::LegacyBlock legacy(block);

                            /* Process the block. */
                            TAO::Ledger::Process(legacy, nStatus);
                        }

                        /* Check for specific status messages. */
                        if(nStatus & TAO::Ledger::PROCESS::ACCEPTED)
                        {
                            /* Reset the fails and orphans. */
                            nConsecutiveFails   = 0;
                            nConsecutiveOrphans = 0;
                        }

                        /* Check for failure status messages. */
                        if(nStatus & TAO::Ledger::PROCESS::REJECTED)
                            ++nConsecutiveFails;

                        /* Check for orphan status messages. */
                        if(nStatus & TAO::Ledger::PROCESS::ORPHAN)
                        {
                            ++nConsecutiveOrphans;

                            /* Check for duplicate and ask for previous block. */
                            if(!(nStatus & TAO::Ledger::PROCESS::DUPLICATE)
                            && !(nStatus & TAO::Ledger::PROCESS::IGNORED))
                            {
                                /* Ask for previous block. */
                                PushMessage(ACTION::GET, uint8_t(TYPES::LEGACY), uint8_t(TYPES::BLOCK), block.hashPrevBlock);

                                /* Ask for list of blocks. */
                                PushMessage(ACTION::LIST,
                                    uint8_t(TYPES::LEGACY), uint8_t(TYPES::BLOCK),
                                    uint8_t(TYPES::LOCATOR), TAO::Ledger::Locator(TAO::Ledger::ChainState::hashBestChain.load()),
                                    uint1024_t(0));
                            }
                        }

                        /* Check for failure limit on node. */
                        if(nConsecutiveFails >= 500)
                        {
                            /* Fast Sync node switch. */
                            if(TAO::Ledger::ChainState::Synchronizing())
                            {
                                //TODO: fine a new fast sync node
                            }

                            /* Drop pesky nodes. */
                            return debug::drop(NODE, "node reached failure limit");
                        }


                        /* Detect large orphan chains and ask for new blocks from origin again. */
                        if(nConsecutiveOrphans >= 500)
                        {
                            LOCK(TAO::Ledger::PROCESSING_MUTEX);

                            /* Clear the memory to prevent DoS attacks. */
                            TAO::Ledger::mapOrphans.clear();

                            /* Disconnect from a node with large orphan chain. */
                            return debug::drop(NODE, "node reached orphan limit");
                        }

                        break;
                    }

                    default:
                        return debug::drop(NODE, "invalid sync primitive type");
                }

                break;
            }


            /* Handle legacy data types. */
            case TYPES::LEGACY:
            {
                /* Get the type. */
                uint8_t nType = 0;
                ssPacket >> nType;

                /* Switch based on type. */
                switch(nType)
                {
                    /* Handle for legacy block. */
                    case TYPES::BLOCK:
                    {
                        /* Get the block from the stream. */
                        Legacy::LegacyBlock block;
                        ssPacket >> block;

                        /* Process the block. */
                        uint8_t nStatus = 0;
                        TAO::Ledger::Process(block, nStatus);

                        /* Check for specific status messages. */
                        if(nStatus & TAO::Ledger::PROCESS::ACCEPTED)
                        {
                            /* Reset the fails and orphans. */
                            nConsecutiveFails   = 0;
                            nConsecutiveOrphans = 0;
                        }

                        /* Check for failure status messages. */
                        if(nStatus & TAO::Ledger::PROCESS::REJECTED)
                            ++nConsecutiveFails;

                        /* Check for orphan status messages. */
                        if(nStatus & TAO::Ledger::PROCESS::ORPHAN)
                        {
                            ++nConsecutiveOrphans;

                            /* Check for duplicate and ask for previous block. */
                            if(!(nStatus & TAO::Ledger::PROCESS::DUPLICATE)
                            && !(nStatus & TAO::Ledger::PROCESS::IGNORED))
                            {
                                /* Ask for previous block. */
                                PushMessage(ACTION::GET, uint8_t(TYPES::LEGACY), uint8_t(TYPES::BLOCK), block.hashPrevBlock);

                                /* Ask for list of blocks. */
                                PushMessage(ACTION::LIST,
                                    uint8_t(TYPES::LEGACY), uint8_t(TYPES::BLOCK),
                                    uint8_t(TYPES::LOCATOR), TAO::Ledger::Locator(TAO::Ledger::ChainState::hashBestChain.load()),
                                    uint1024_t(0));
                            }
                        }

                        /* Check for failure limit on node. */
                        if(nConsecutiveFails >= 500)
                        {
                            /* Fast Sync node switch. */
                            if(TAO::Ledger::ChainState::Synchronizing())
                            {
                                //TODO: fine a new fast sync node
                            }

                            /* Drop pesky nodes. */
                            return debug::drop(NODE, "node reached failure limit");
                        }


                        /* Detect large orphan chains and ask for new blocks from origin again. */
                        if(nConsecutiveOrphans >= 500)
                        {
                            LOCK(TAO::Ledger::PROCESSING_MUTEX);

                            /* Clear the memory to prevent DoS attacks. */
                            TAO::Ledger::mapOrphans.clear();

                            /* Disconnect from a node with large orphan chain. */
                            return debug::drop(NODE, "node reached orphan limit");
                        }

                        break;
                    }

                    /* Handle for legacy transaction. */
                    case TYPES::TRANSACTION:
                    {
                        /* Get the transction from the stream. */
                        Legacy::Transaction tx;
                        ssPacket >> tx;

                        /* Accept into memory pool. */
                        if(TAO::Ledger::mempool.Accept(tx))
                        {
                            TRITIUM_SERVER->Relay(uint16_t(ACTION::NOTIFY),
                                uint8_t(TYPES::LEGACY), uint8_t(TYPES::TRANSACTION), tx.GetHash());

                            //TODO: relay to legacy nodes.
                        }

                        break;
                    }

                    default:
                        return debug::drop(NODE, "invalid legacy primitive type");
                }

                break;
            }


            /* Handle incoming block. */
            case TYPES::BLOCK:
            {
                /* Get the block from the stream. */
                TAO::Ledger::TritiumBlock block;
                ssPacket >> block;

                /* Process the block. */
                uint8_t nStatus = 0;
                TAO::Ledger::Process(block, nStatus);

                /* Check for specific status messages. */
                if(nStatus & TAO::Ledger::PROCESS::ACCEPTED)
                {
                    /* Reset the fails and orphans. */
                    nConsecutiveFails   = 0;
                    nConsecutiveOrphans = 0;
                }

                /* Check for failure status messages. */
                if(nStatus & TAO::Ledger::PROCESS::REJECTED)
                    ++nConsecutiveFails;

                /* Check for missing transactions. */
                if(nStatus & TAO::Ledger::PROCESS::INCOMPLETE)
                {
                    /* Create response data stream. */
                    DataStream ssResponse(SER_NETWORK, PROTOCOL_VERSION);

                    /* Create a list of requested transactions. */
                    for(const auto& tx : block.vMissing)
                    {
                        /* Check for legacy. */
                        if(tx.first == TAO::Ledger::TRANSACTION::LEGACY)
                            ssResponse << uint8_t(TYPES::LEGACY);

                        /* Push to stream. */
                        ssResponse << uint8_t(TYPES::TRANSACTION) << tx.second;
                    }

                    /* Ask for the block again last TODO: this can be cached for further optimization. */
                    ssResponse << uint8_t(TYPES::BLOCK) << block.GetHash();

                    /* Push the packet response. */
                    WritePacket(NewMessage(ACTION::GET, ssResponse));
                }

                /* Check for orphan status messages. */
                if(nStatus & TAO::Ledger::PROCESS::ORPHAN)
                {
                    ++nConsecutiveOrphans;

                    /* Check for duplicate and ask for previous block. */
                    if(!(nStatus & TAO::Ledger::PROCESS::DUPLICATE)
                    && !(nStatus & TAO::Ledger::PROCESS::IGNORED))
                    {
                        /* Ask for previous block. */
                        PushMessage(ACTION::GET,
                            uint8_t(TYPES::BLOCK),
                            block.hashPrevBlock
                        );

                        /* Ask for list of blocks. */
                        PushMessage(ACTION::LIST,
                            uint8_t(TYPES::BLOCK),
                            uint8_t(TYPES::LOCATOR),
                            TAO::Ledger::Locator(TAO::Ledger::ChainState::hashBestChain.load()),
                            uint1024_t(0)
                        );
                    }
                }

                /* Check for failure limit on node. */
                if(nConsecutiveFails >= 500)
                {
                    /* Fast Sync node switch. */
                    if(TAO::Ledger::ChainState::Synchronizing() && nSyncSession.load() == nCurrentSession)
                    {
                        //TODO: find a new fast sync node
                    }

                    /* Drop pesky nodes. */
                    return debug::drop(NODE, "node reached failure limit");
                }


                /* Detect large orphan chains and ask for new blocks from origin again. */
                if(nConsecutiveOrphans >= 500)
                {
                    LOCK(TAO::Ledger::PROCESSING_MUTEX);

                    /* Clear the memory to prevent DoS attacks. */
                    TAO::Ledger::mapOrphans.clear();

                    /* Disconnect from a node with large orphan chain. */
                    return debug::drop(NODE, "node reached orphan limit");
                }

                break;
            }


            /* Handle incoming transaction. */
            case TYPES::TRANSACTION:
            {
                /* Get the transction from the stream. */
                TAO::Ledger::Transaction tx;
                ssPacket >> tx;

                /* Accept into memory pool. */
                if(TAO::Ledger::mempool.Accept(tx))
                {
                    TRITIUM_SERVER->Relay(uint16_t(ACTION::NOTIFY),
                        uint8_t(TYPES::TRANSACTION), tx.GetHash());

                    //TODO: relay to legacy nodes
                }

                break;
            }

            default:
                return debug::drop(NODE, "invalid protocol message ", INCOMING.MESSAGE);
        }

        /* Check for authorization. */
        if(DDOS && !Authorized())
            DDOS->rSCORE += 5; //untrusted nodes get less requests

        /* Check for a version message. */
        if(nProtocolVersion == 0 || nCurrentSession == 0)
            return debug::drop(NODE, "first message wasn't a version message");

        return true;
    }


    /*  Non-Blocking Packet reader to build a packet from TCP Connection.
     *  This keeps thread from spending too much time for each Connection. */
    void TritiumNode::ReadPacket()
    {
        if(!INCOMING.Complete())
        {
            /** Handle Reading Packet Length Header. **/
            if(!INCOMING.Header() && Available() >= 8)
            {
                std::vector<uint8_t> BYTES(8, 0);
                if(Read(BYTES, 8) == 8)
                {
                    DataStream ssHeader(BYTES, SER_NETWORK, MIN_PROTO_VERSION);
                    ssHeader >> INCOMING;

                    Event(EVENT_HEADER);
                }
            }

            /** Handle Reading Packet Data. **/
            uint32_t nAvailable = Available();
            if(nAvailable > 0 && !INCOMING.IsNull() && INCOMING.DATA.size() < INCOMING.LENGTH)
            {
                /* Create the packet data object. */
                std::vector<uint8_t> DATA(std::min(nAvailable, (uint32_t)(INCOMING.LENGTH - INCOMING.DATA.size())), 0);



                /* Read up to 512 bytes of data. */
                if(Read(DATA, DATA.size()) == DATA.size())
                {
                    INCOMING.DATA.insert(INCOMING.DATA.end(), DATA.begin(), DATA.end());
                    Event(EVENT_PACKET, static_cast<uint32_t>(DATA.size()));
                }
            }
        }
    }


    /* Determine if a node is authorized and therfore trusted. */
    bool TritiumNode::Authorized() const
    {
        return hashGenesis != 0 && fAuthorized;
    }


    /* Subscribe to another node for notifications. */
    void TritiumNode::Subscribe(const uint16_t nFlags)
    {
        /* Build subscription message. */
        DataStream ssMessage(SER_NETWORK, MIN_PROTO_VERSION);

        /* Check for block. */
        if(nFlags & SUBSCRIPTION::BLOCK)
        {
            /* Build the message. */
            ssMessage << uint8_t(TYPES::BLOCK);
            nSubscriptions |= SUBSCRIPTION::BLOCK;
        }

        /* Check for transaction. */
        if(nFlags & SUBSCRIPTION::TRANSACTION)
        {
            /* Build the message. */
            ssMessage << uint8_t(TYPES::TRANSACTION);
            nSubscriptions |= SUBSCRIPTION::TRANSACTION;
        }

        /* Check for time seed. */
        if(nFlags & SUBSCRIPTION::TIMESEED)
        {
            /* Build the message. */
            ssMessage << uint8_t(TYPES::TIMESEED);
            nSubscriptions |= SUBSCRIPTION::TIMESEED;
        }

        /* Check for height. */
        if(nFlags & SUBSCRIPTION::HEIGHT)
        {
            /* Build the message. */
            ssMessage << uint8_t(TYPES::HEIGHT);
            nSubscriptions |= SUBSCRIPTION::HEIGHT;
        }

        /* Check for checkpoint. */
        if(nFlags & SUBSCRIPTION::CHECKPOINT)
        {
            /* Build the message. */
            ssMessage << uint8_t(TYPES::CHECKPOINT);
            nSubscriptions |= SUBSCRIPTION::CHECKPOINT;
        }

        /* Check for address. */
        if(nFlags & SUBSCRIPTION::ADDRESS)
        {
            /* Build the message. */
            ssMessage << uint8_t(TYPES::ADDRESS);
            nSubscriptions |= SUBSCRIPTION::ADDRESS;
        }

        /* Check for last. */
        if(nFlags & SUBSCRIPTION::LAST)
        {
            /* Build the message. */
            ssMessage << uint8_t(TYPES::LAST);
            nSubscriptions |= SUBSCRIPTION::LAST;
        }

        /* Write the subscription packet. */
        WritePacket(NewMessage(ACTION::SUBSCRIBE, ssMessage));

        debug::log(0, "Subscribed to ", std::bitset<16>(nSubscriptions));
    }


    /* Determine whether a session is connected. */
    bool TritiumNode::SessionActive(const uint64_t nSession)
    {
        LOCK(SESSIONS_MUTEX);

        return mapSessions.count(nSession);
    }
}
