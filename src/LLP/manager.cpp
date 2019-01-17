/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/macro.h>
#include <LLC/hash/SK.h>
#include <LLC/include/random.h>
#include <LLD/include/address.h>
#include <LLD/include/version.h>
#include <LLP/include/manager.h>
#include <Util/include/debug.h>
#include <Util/templates/serialize.h>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace LLP
{

    /* Default constructor */
    AddressManager::AddressManager(uint16_t nPort)
    : pDatabase(new LLD::AddressDB(nPort, LLD::FLAGS::CREATE | LLD::FLAGS::WRITE))
    , mapAddrInfo()
    {
        if(!pDatabase)
            debug::error(FUNCTION, "Failed to allocate memory for AddressManager");
    }


    /* Default destructor */
    AddressManager::~AddressManager()
    {
        if(pDatabase)
        {
            delete pDatabase;
            pDatabase = 0;
        }
    }

    /*  Determine if the address manager has any addresses in it or not */
    bool AddressManager::IsEmpty() const
    {
        std::unique_lock<std::mutex> lk(mut);
        bool empty = mapAddrInfo.empty();
        lk.unlock();
        return empty;
    }


    /*  Gets a list of addresses in the manager */
    std::vector<Address> AddressManager::GetAddresses(const uint8_t flags)
    {
        std::vector<AddressInfo> vAddrInfo;
        std::vector<Address> vAddr;

        std::unique_lock<std::mutex> lk(mut);

        get_info(vAddrInfo, flags);

        for(auto it = vAddrInfo.begin(); it != vAddrInfo.end(); ++it)
            vAddr.push_back(static_cast<Address>(*it));

        return vAddr;
    }


    /*  Gets a list of address info in the manager */
    void AddressManager::GetInfo(std::vector<AddressInfo> &vAddrInfo, const uint8_t flags)
    {
        std::unique_lock<std::mutex> lk(mut);
        get_info(vAddrInfo, flags);

    }

    uint32_t AddressManager::GetInfoCount(const uint8_t flags)
    {
        std::unique_lock<std::mutex> lk(mut);
        uint32_t c = get_info_count(flags);
        lk.unlock();
        return c;
    }


    /*  Adds the address to the manager and sets the connect state for that address */
    void AddressManager::AddAddress(const Address &addr, const uint8_t state)
    {
        uint64_t hash = addr.GetHash();

        std::unique_lock<std::mutex> lk(mut);

        if(mapAddrInfo.find(hash) == mapAddrInfo.end())
            mapAddrInfo.emplace(std::make_pair(hash, AddressInfo(addr)));


        AddressInfo *pInfo = &mapAddrInfo[hash];
        uint64_t ms = runtime::unifiedtimestamp(true);

        switch(state)
        {
        case ConnectState::NEW:
            break;

        case ConnectState::CONNECTED:
            if(pInfo->nState == ConnectState::CONNECTED)
                break;

            ++pInfo->nConnected;
            pInfo->nSession = 0;
            pInfo->nFails = 0;
            pInfo->nLastSeen = ms;
            pInfo->nState = state;
            break;

        case ConnectState::DROPPED:
            if(pInfo->nState == ConnectState::DROPPED)
                break;

            ++pInfo->nDropped;
            pInfo->nSession = ms - pInfo->nLastSeen;
            pInfo->nLastSeen = ms;
            pInfo->nState = state;
            break;

        case ConnectState::FAILED:
            ++pInfo->nFailed;
            ++pInfo->nFails;
            pInfo->nSession = 0;
            pInfo->nState = state;
            break;

        default:
            break;
        }

        debug::log(5, FUNCTION, addr.ToString());

        /* update the LLD Database with a new entry */
        pDatabase->WriteAddressInfo(hash, *pInfo);
    }


    /*  Adds the addresses to the manager and sets the connect state for that address */
    void AddressManager::AddAddresses(const std::vector<Address> &addrs, const uint8_t state)
    {
        for(uint32_t i = 0; i < addrs.size(); ++i)
            AddAddress(addrs[i], state);
    }


    /*  Finds the managed address info and sets the latency experienced by
     *  that address. */
    void AddressManager::SetLatency(uint32_t lat, const Address &addr)
    {
        uint64_t hash = addr.GetHash();
        std::unique_lock<std::mutex> lk(mut);

        auto it = mapAddrInfo.find(hash);
        if(it != mapAddrInfo.end())
            it->second.nLatency = lat;
    }


    /*  Select a good address to connect to that isn't already connected. */
    bool AddressManager::StochasticSelect(Address &addr)
    {
        uint64_t nTimestamp = runtime::unifiedtimestamp();
        uint64_t nRand = LLC::GetRand(nTimestamp);
        uint32_t nHash = LLC::SK32(BEGIN(nRand), END(nRand));

        /* put unconnected address info scores into a vector and sort */
        uint8_t flags = ConnectState::NEW | ConnectState::FAILED | ConnectState::DROPPED;

        /* critical section: get address info for selected flags */
        std::unique_lock<std::mutex> lk(mut);
        uint32_t s = get_info_count(flags);

        std::vector<AddressInfo> vInfo(s);
        get_info(vInfo, flags);

        lk.unlock();

        if(s == 0)
            return false;

        /* select an index with a good random weight bias toward the front of the list */
        uint32_t nSelect = ((std::numeric_limits<uint64_t>::max() /
            std::max((uint64_t)std::pow(nHash, 1.95) + 1, (uint64_t)1)) - 3) % s;

        /* sort info vector and assign the selected address */
        std::sort(vInfo.begin(), vInfo.end());
        std::reverse(vInfo.begin(), vInfo.end());

        addr = vInfo[nSelect];

        return true;
    }


    /*  Read the address database into the manager */
    void AddressManager::ReadDatabase()
    {
        LOCK(mut);

        std::vector<std::vector<uint8_t> > keys = pDatabase->GetKeys();

        uint32_t s = static_cast<uint32_t>(keys.size());

        for(uint32_t i = 0; i < s; ++i)
        {
            std::string str;
            uint64_t nKey;
            AddressInfo addr_info;

            DataStream ssKey(keys[i], SER_LLD, LLD::DATABASE_VERSION);
            ssKey >> str;
            ssKey >> nKey;

            pDatabase->ReadAddressInfo(nKey, addr_info);

            uint64_t nHash = addr_info.GetHash();

            mapAddrInfo[nHash] = addr_info;
        }

        print_stats();
    }

    void AddressManager::PrintStats()
    {
        std::unique_lock<std::mutex> lk(mut);
        print_stats();
    }

    void AddressManager::print_stats()
    {
        debug::log(3,
            " C=", get_info_count(ConnectState::CONNECTED),
            " D=", get_info_count(ConnectState::DROPPED),
            " F=", get_info_count(ConnectState::FAILED), " |",
            " TC=", get_total_count(ConnectState::CONNECTED),
            " TD=", get_total_count(ConnectState::DROPPED),
            " TF=", get_total_count(ConnectState::FAILED), " |",
            " size=", mapAddrInfo.size());
    }


    /*  Write the addresses from the manager into the address database */
    void AddressManager::WriteDatabase()
    {
        std::unique_lock<std::mutex> lk(mut);

        for(auto it = mapAddrInfo.begin(); it != mapAddrInfo.end(); ++it)
            pDatabase->WriteAddressInfo(it->first, it->second);

        print_stats();
    }


    /*  Helper function to get an array of info on the connected states specified
     *  by flags */
    void AddressManager::get_info(std::vector<AddressInfo> &info, const uint8_t flags)
    {
        info.clear();

        for(auto it = mapAddrInfo.begin(); it != mapAddrInfo.end(); ++it)
        {
            if(it->second.nState & flags)
                info.push_back(it->second);
        }
    }

    /*  Helper function to get the number of addresses of the connect type */
    uint32_t AddressManager::get_info_count(const uint8_t flags)
    {
        uint32_t c = 0;
        for(auto it = mapAddrInfo.begin(); it != mapAddrInfo.end(); ++it)
        {
            if(it->second.nState & flags)
                ++c;
        }
        return c;
    }


    uint32_t AddressManager::get_total_count(const uint8_t flags)
    {
        uint32_t total = 0;

        for(auto it = mapAddrInfo.begin(); it != mapAddrInfo.end(); ++it)
        {
            if(flags & ConnectState::CONNECTED)
                total += it->second.nConnected;
            if(flags & ConnectState::DROPPED)
                total += it->second.nDropped;
            if(flags & ConnectState::FAILED)
                total += it->second.nFailed;
        }

        return total;
    }
}
