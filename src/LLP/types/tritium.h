/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TYPES_TRITIUM_H
#define NEXUS_LLP_TYPES_TRITIUM_H

#include <LLP/include/network.h>
#include <LLP/include/version.h>
#include <LLP/packets/tritium.h>
#include <LLP/templates/base_connection.h>
#include <LLP/templates/events.h>
#include <LLP/templates/ddos.h>

#include <TAO/Ledger/types/tritium.h>

namespace LLP
{

    /** Actions invoke behavior in remote node. **/
    namespace ACTION
    {
        enum
        {
            RESERVED     = 0,

            /* Verbs. */
            LIST         = 0x10,
            GET          = 0x11,
            NOTIFY       = 0x12,
            AUTH         = 0x13,
            VERSION      = 0x14,

            /* Protocol. */
            PING         = 0x1a,
            PONG         = 0x1b

        };
    }


    /** Types are objects that can be sent in packets. **/
    namespace TYPES
    {
        enum
        {
            /* Key Types. */
            UINT256_T   = 0x20,
            UINT512_T   = 0x21,
            UINT1024_T  = 0x22,
            STRING      = 0x23,
            BYTES       = 0x24,

            /* Object Types. */
            BLOCK       = 0x30,
            TRANSACTION = 0x31,
            TIMESEED    = 0x32,
            HEIGHT      = 0x33,
            CHECKPOINT  = 0x34,

            /* Specifier. */
            LEGACY      = 0x3a
        };
    }


    /** Status returns available states. **/
    namespace RESPONSE
    {
        enum
        {
            ACCEPTED    = 0x40,
            REJECTED    = 0x41,
            STALE       = 0x42,
        };
    }


    /** TritiumNode
     *
     *  A Node that processes packets and messages for the Tritium Server
     *
     **/
    class TritiumNode : public BaseConnection<TritiumPacket>
    {

        /** message_args
         *
         *  Overload of variadic templates
         *
         *  @param[out] s The data stream to write to
         *  @param[in] head The object being written
         *
         **/
        template<class Head>
        void message_args(DataStream& s, Head&& head)
        {
            s << std::forward<Head>(head);
        }


        /** message_args
         *
         *  Variadic template pack to handle any message size of any type.
         *
         *  @param[out] s The data stream to write to
         *  @param[in] head The object being written
         *  @param[in] tail The variadic paramters
         *
         **/
        template<class Head, class... Tail>
        void message_args(DataStream& s, Head&& head, Tail&&... tail)
        {
            s << std::forward<Head>(head);
            message_args(s, std::forward<Tail>(tail)...);
        }


        /** State of if node has currently verified signature. **/
        std::atomic<bool> fAuthorized;


        /** Mutex for connected sessions. **/
        static std::mutex SESSIONS_MUTEX;


        /** Set for connected session. **/
        static std::map<uint64_t, std::pair<uint32_t, uint32_t>> mapSessions;

    public:

      /** Name
       *
       *  Returns a string for the name of this type of Node.
       *
       **/
        static std::string Name() { return "Tritium"; }


        /** Default Constructor **/
        TritiumNode();


        /** Constructor **/
        TritiumNode(Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false);


        /** Constructor **/
        TritiumNode(DDOS_Filter* DDOS_IN, bool isDDOS = false);


        /** Default Destructor **/
        virtual ~TritiumNode();


        /** Counter to keep track of the last time a ping was made. **/
        std::atomic<uint64_t> nLastPing;


        /** Counter to keep track of last time sample request. */
        std::atomic<uint64_t> nLastSamples;


        /** timer object to keep track of ping latency. **/
        std::map<uint64_t, runtime::timer> mapLatencyTracker;


        /** The current genesis-id of connected peer. **/
        uint256_t hashGenesis;


        /** The current trust of the connected peer. **/
        uint64_t nTrust;


        /** This node's protocol version. **/
        uint64_t nProtocolVersion;


        /** This node's session-id. **/
        uint64_t nCurrentSession;


        /** This node's current height. **/
        uint32_t nCurrentHeight;


        /** This node's current checkpoint. **/
        uint1024_t hashCheckpoint;


        /** The node's full version string. **/
        std::string strFullVersion;


        /** Event
         *
         *  Virtual Functions to Determine Behavior of Message LLP.
         *
         *  @param[in] EVENT The byte header of the event type.
         *  @param[in[ LENGTH The size of bytes read on packet read events.
         *
         */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0) final;


        /** ProcessPacket
         *
         *  Main message handler once a packet is recieved.
         *
         *  @return True is no errors, false otherwise.
         *
         **/
        bool ProcessPacket() final;


        /** ReadPacket
         *
         *  Non-Blocking Packet reader to build a packet from TCP Connection.
         *  This keeps thread from spending too much time for each Connection.
         *
         **/
        void ReadPacket() final;


        /** Authorized
         *
         *  Determine if a node is authorized and therfore trusted.
         *
         **/
        bool Authorized() const;


        /** SessionActive
         *
         *  Determine whether a session is connected.
         *
         *  @param[in] nSession The session to check for
         *
         *  @return true if session is connected.
         *
         **/
        static bool SessionActive(const uint64_t nSession);


        /** NewMessage
         *
         *  Creates a new message with a commands and data.
         *
         *  @param[in] nMsg The message type.
         *  @param[in] ssData A datastream object with data to write.
         *
         *  @return Returns a filled out tritium packet.
         *
         **/
        TritiumPacket NewMessage(const uint16_t nMsg, const DataStream& ssData)
        {
            TritiumPacket RESPONSE(nMsg);
            RESPONSE.SetData(ssData);

            return RESPONSE;
        }


        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         *  @param[in] nMsg The message type.
         *
         **/
        void PushMessage(const uint16_t nMsg)
        {
            TritiumPacket RESPONSE(nMsg);
            WritePacket(RESPONSE);
        }


        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         *  @param[in] nMsg The message type.
         *
         **/
        void PushMessage(const uint16_t nMsg, const DataStream& ssData)
        {
            WritePacket(NewMessage(nMsg, ssData));
        }


        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         **/
        template<typename... Args>
        void PushMessage(const uint16_t nMsg, Args&&... args)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            message_args(ssData, std::forward<Args>(args)...);

            WritePacket(NewMessage(nMsg, ssData));
        }

    };

}

#endif
