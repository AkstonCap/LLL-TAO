/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TYPES_RPCNODE_H
#define NEXUS_LLP_TYPES_RPCNODE_H

#include <LLP/types/http.h>
#include <TAO/API/types/base.h>
#include <Util/include/json.h>

namespace LLP
{


    /** RPC API Server Node
     *
     *  A node that can speak over HTTP protocols.
     *
     *  RPC API Functionality:
     *  HTTP-JSON-RPC - Nexus Ledger Level API
     *  POST / HTTP/1.1
     *  {"method":"", "params":[]}
     *
     **/
    class RPCNode : public LLP::HTTPNode
    {
    public:

        static std::string Name() { return "RPC"; }

        /* Constructors for Message LLP Class. */
        RPCNode()
        : HTTPNode() {}

        RPCNode( LLP::Socket_t SOCKET_IN, LLP::DDOS_Filter* DDOS_IN, bool isDDOS = false )
        : HTTPNode(SOCKET_IN, DDOS_IN, isDDOS) {}


        /** Virtual Functions to Determine Behavior of Message LLP.
         *
         *  @param[in] EVENT The byte header of the event type
         *  @param[in[ LENGTH The size of bytes read on packet read events
         *
         */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0) final;


        /** ProcessPacket
         *
         *  Main message handler once a packet is recieved.
         *
         *  @return True is no errors, false otherwise
         *
         **/
        bool ProcessPacket() final;

    protected:

        /** JSON Reply
         *
         *  Reply a standard response from RPC server.
         *
         *  @param[in] jsonResponse The JSON response data to send.
         *  @param[in] jsonError The JSON error response object.
         *  @param[in] jsonID The identifier of request.
         *
         *  @return The json object to respond with.
         *
         **/
        json::json JSONReply(const json::json& jsonResponse, const json::json& jsonError, const json::json& jsonID);


        /** Error Reply
         *
         *  Reply an error from the RPC server.
         *
         *  @param[in] jsonError The JSON error response object.
         *  @param[in] jsonID The identifier of request.
         *
         *
         **/
        void ErrorReply(const json::json& jsonError, const json::json& jsonID);


        /** Authorized
         *
         *  Check if an authorization base64 encoded string is correct.
         *
         *  @param[in] mapHeaders The list of headers to check.
         *
         *  @return True if this connection is authorized.
         *
         **/
        bool Authorized(std::map<std::string, std::string>& mapHeaders);

    };
}

#endif
