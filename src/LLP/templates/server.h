/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TEMPLATES_SERVER_H
#define NEXUS_LLP_TEMPLATES_SERVER_H


#include <LLP/templates/data.h>
#include <LLP/include/legacy_address.h>

#include <map>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <string>
#include <vector>
#include <cstdint>

namespace LLP
{
    /* forward declarations */
    class AddressManager;
    class DDOS_Filter;
    class InfoAddress;


    /** Server
     *
     *  Base Class to create a Custom LLP Server.
     *
     *  Protocol Type class must inherit Connection,
     *  and provide a ProcessPacket method.
     *  Optional Events by providing GenericEvent method.
     *
     **/
    template <class ProtocolType>
    class Server
    {
    private:

        /** The DDOS variables. **/
        std::map<BaseAddress, DDOS_Filter *> DDOS_MAP;


        /** DDOS flag for off or on. **/
        std::atomic<bool> fDDOS;


        /** Listener Thread for accepting incoming connections. **/
        std::thread          LISTEN_THREAD;


        /** Meter Thread for tracking incoming and outgoing packet counts. **/
        std::thread          METER_THREAD;


        /** Port mapping thread for opening port in router. **/
        std::thread          UPNP_THREAD;


        /** Server's listenting port. **/
        uint16_t PORT;


    public:

        /** Maximum number of data threads for this server. **/
        uint16_t MAX_THREADS;


        /** The moving average timespan for DDOS throttling. **/
        uint32_t DDOS_TIMESPAN;


        /** The data type to keep track of current running threads. **/
        std::vector<DataThread<ProtocolType> *> DATA_THREADS;


        /** Connection Manager thread. **/
        std::thread MANAGER_THREAD;


        /** Address for handling outgoing connections **/
        AddressManager *pAddressManager;


        /** The sleep time of address manager. */
        uint32_t nSleepTime;


        /** The listener socket instance. **/
        std::pair<int32_t, int32_t> hListenSocket;


        /** Name
         *
         *  Returns the name of the protocol type of this server.
         *
         **/
        std::string Name();


        /** Constructor **/
        Server<ProtocolType>(uint16_t nPort,
                             uint16_t nMaxThreads,
                             uint32_t nTimeout = 30,
                             bool fDDOS_ = false,
                             uint32_t cScore = 0,
                             uint32_t rScore = 0,
                             uint32_t nTimespan = 60,
                             bool fListen = true,
                             bool fRemote = false,
                             bool fMeter = false,
                             bool fManager = false,
                             uint32_t nSleepTimeIn = 1000);


        /** Default Destructor **/
        virtual ~Server<ProtocolType>();


        /** GetPort
         *
         *  Returns the port number for this Server.
         *
         **/
        uint16_t GetPort() const;


        /** Shutdown
         *
         *  Cleanup and shutdown subsystems
         *
         **/
        void Shutdown();


        /** AddNode
         *
         *  Add a node address to the internal address manager
         *
         *  @param[in] strAddress	IPv4 Address of outgoing connection
         *  @param[in] strPort		Port of outgoing connection
         *
         **/
        void AddNode(std::string strAddress, uint16_t nPort, bool fLookup = false);


        /** AddConnection
         *
         *  Public Wraper to Add a Connection Manually.
         *
         *  @param[in] strAddress	IPv4 Address of outgoing connection
         *  @param[in] strPort		Port of outgoing connection
         *
         *  @return	Returns 1 If successful, 0 if unsuccessful, -1 on errors.
         *
         **/
        bool AddConnection(std::string strAddress, uint16_t nPort, bool fLookup = false);


        /** GetConnectionCount
         *
         *  Get the number of active connection pointers from data threads.
         *
         *  @return Returns the count of active connections
         *
         **/
        uint32_t GetConnectionCount(const uint8_t nFlags = FLAGS::ALL);


        /** Get Connection
         *
         *  Get the best connection based on latency
         *
         *  @param[in] pairExclude The connection that should be excluded from the search.
         *
         **/
        memory::atomic_ptr<ProtocolType>& GetConnection(const std::pair<uint32_t, uint32_t>& pairExclude);


        /** Get Connection
         *
         *  Get the best connection based on data thread index.
         *
         **/
        memory::atomic_ptr<ProtocolType>& GetConnection(const uint32_t nDataThread, const uint32_t nDataIndex);


        /** Relay
         *
         *  Relays data to all nodes on the network.
         *
         **/
        template<typename MessageType, typename... Args>
        void Relay(const MessageType& message, Args&&... args)
        {
            /* Relay message to each data thread, which will relay message to each connection of each data thread */
            for(uint16_t nThread = 0; nThread < MAX_THREADS; ++nThread)
                DATA_THREADS[nThread]->Relay(message, args...);
        }


        /** GetAddresses
         *
         *  Get the active connection pointers from data threads.
         *
         *  @return Returns the list of active connections in a vector.
         *
         **/
        std::vector<LegacyAddress> GetAddresses();


        /** DisconnectAll
        *
        *  Notifies all data threads to disconnect their connections
        *
        **/
        void DisconnectAll();


        /** NotifyEvent
         *
         *  Tell the server an event has occured to wake up thread if it is sleeping. This can be used to orchestrate communication
         *  among threads if a strong ordering needs to be guaranteed.
         *
         **/
        void NotifyEvent();


    private:


        /** Manager
         *
         *  Address Manager Thread.
         *
         **/
        void Manager();


        /** FindThread
         *
         *  Determine the first thread with the least amount of active connections.
         *  This keeps them load balanced across all server threads.
         *
         *  @return Returns the index of the found thread. or -1 if not found.
         *
         **/
        int32_t FindThread();


        /** ListeningThread
         *
         *  Main Listening Thread of LLP Server. Handles new Connections and
         *  DDOS associated with Connection if enabled.
         *
         *  @param[in] fIPv4
         *
         **/
        void ListeningThread(bool fIPv4);


        /** BindListenPort
         *
         *  Bind connection to a listening port.
         *
         *  @param[in] hListenSocket
         *  @param[in] fIPv4 Flag indicating the connection is IPv4
         *  @param[in] fRemote Flag indicating that the socket should listen on all interfaced (true) or local only (false)
         *
         *  @return
         *
         **/
        bool BindListenPort(int32_t & hListenSocket, bool fIPv4 = true, bool fRemote = false);


        /** Meter
         *
         *  LLP Meter Thread. Tracks the Requests / Second.
         *
         **/
        void Meter();


        /** UPnP
         *
         *  UPnP Thread. If UPnP is enabled then this thread will set up the required port forwarding.
         *
         **/
        void UPnP();

    };

}

#endif
