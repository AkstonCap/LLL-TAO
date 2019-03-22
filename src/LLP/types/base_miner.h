/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TYPES_BASE_MINER_H
#define NEXUS_LLP_TYPES_BASE_MINER_H

#include <LLP/templates/connection.h>
#include <TAO/Ledger/types/block.h>
#include <Legacy/types/coinbase.h>
#include <atomic>


namespace LLP
{

    /** BaseMiner
     *
     *  Connection class that handles requests and responses from miners.
     *
     **/
    class BaseMiner : public Connection
    {
    protected:

        /* Externally set coinbase to be set on mined blocks */
        Legacy::Coinbase CoinbaseTx;

        /* Used for synchronization */
        std::mutex MUTEX;

        /** The map to hold the list of blocks that are being mined. */
        std::map<uint512_t, TAO::Ledger::Block *> mapBlocks;

        /** The current best block. **/
        std::atomic<uint32_t> nBestHeight;

        /** Subscribe to display how many blocks connection subscribed to **/
        std::atomic<uint16_t> nSubscribed;

        /** The current channel mining for. */
        std::atomic<uint8_t> nChannel;


        enum
        {
            /** DATA PACKETS **/
            BLOCK_DATA   = 0,
            SUBMIT_BLOCK = 1,
            BLOCK_HEIGHT = 2,
            SET_CHANNEL  = 3,
            BLOCK_REWARD = 4,
            SET_COINBASE = 5,
            GOOD_BLOCK   = 6,
            ORPHAN_BLOCK = 7,


            /** DATA REQUESTS **/
            CHECK_BLOCK  = 64,
            SUBSCRIBE    = 65,


            /** REQUEST PACKETS **/
            GET_BLOCK    = 129,
            GET_HEIGHT   = 130,
            GET_REWARD   = 131,


            /** SERVER COMMANDS **/
            CLEAR_MAP    = 132,
            GET_ROUND    = 133,


            /** RESPONSE PACKETS **/
            BLOCK_ACCEPTED       = 200,
            BLOCK_REJECTED       = 201,


            /** VALIDATION RESPONSES **/
            COINBASE_SET  = 202,
            COINBASE_FAIL = 203,

            /** ROUND VALIDATIONS. **/
            NEW_ROUND     = 204,
            OLD_ROUND     = 205,

            /** GENERIC **/
            PING     = 253,
            CLOSE    = 254
        };

        /* Used as an ID iterator for generating unique hashes from same block transactions. */
        uint32_t nBlockIterator;


    public:

        /** Default Constructor **/
        BaseMiner();


        /** Constructor **/
        BaseMiner(const Socket& SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false);


        /** Constructor **/
        BaseMiner(DDOS_Filter* DDOS_IN, bool isDDOS = false);


        /** Default Destructor **/
        virtual ~BaseMiner() = 0;


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


    private:

        /** respond
         *
         *  Sends a packet response.
         *
         *  @param[in] header_response The header message to send.
         *  @param[in] data The packet data to send.
         *
         **/
        void respond(uint8_t nHeader, const std::vector<uint8_t>& vData = std::vector<uint8_t>());


        /** check_best_height
         *
         *  Checks the current height index and updates best height. It will clear
         *  the block map if the height is outdated.
         *
         *  @return Returns true if best height was outdated, false otherwise.
         *
         **/
         bool check_best_height();


         /** clear_map
          *
          *  Clear the blocks map.
          *
          **/
         void clear_map();


         /** new_block
          *
          *  Adds a new block to the map.
          *
          **/
         virtual TAO::Ledger::Block *new_block() = 0;


         /** validate_block
          *
          *  validates the block for the derived miner class.
          *
          **/
          virtual bool validate_block(const uint512_t& hashMerkleRoot) = 0;


          /** sign_block
           *
           *  validates the block for the derived miner class.
           *
           **/
           virtual bool sign_block(uint64_t nNonce, const uint512_t& hashMerkleRoot) = 0;



           /** is_locked
            *
            *  Determines if the mining wallet is unlocked.
            *
            **/
           virtual bool is_locked() = 0;


           /** find_block
            *
            *  Determines if the block exists.
            *
            **/
           bool find_block(const uint512_t& hashMerkleRoot);



    };
}

#endif
