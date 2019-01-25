/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_PACKET_LEGACY_H
#define NEXUS_LLP_PACKET_LEGACY_H

#include <queue>
#include <vector>
#include <algorithm>

#include <LLC/hash/SK.h>

#include <Util/include/debug.h>
#include <Util/include/args.h>
#include <Util/templates/serialize.h>
#include <Util/include/memory.h>


namespace LLP
{

    /* Message Packet Leading Bytes. */
    const uint8_t MESSAGE_START_TESTNET[4] = { 0xe9, 0x59, 0x0d, 0x05 };
    const uint8_t MESSAGE_START_MAINNET[4] = { 0x05, 0x0d, 0x59, 0xe9 };


    /** LegacyPacket
     *
     *  Class to handle sending and receiving of More Complese Message LLP Packets.
     *
     ***/
    class LegacyPacket
    {
    public:

        /*
        * Components of a Message LLP Packet.
        * BYTE 0 - 4    : Start
        * BYTE 5 - 17   : Message
        * BYTE 18 - 22  : Size
        * BYTE 23 - 26  : Checksum
        * BYTE 26 - X   : Data
        *
        */
        uint8_t       HEADER[4];
        char	        MESSAGE[12];
        uint32_t	    LENGTH;
        uint32_t	    CHECKSUM;

        std::vector<uint8_t> DATA;

        LegacyPacket()
        {
            SetNull();
            SetHeader();
        }

        LegacyPacket(const char* chMessage)
        {
            SetNull();
            SetHeader();
            SetMessage(chMessage);
        }

        IMPLEMENT_SERIALIZE
        (
            READWRITE(FLATDATA(HEADER));
            READWRITE(FLATDATA(MESSAGE));
            READWRITE(LENGTH);
            READWRITE(CHECKSUM);
        )


        /* Set the Packet Null Flags. */
        void SetNull()
        {
            LENGTH    = 0;
            CHECKSUM  = 0;
            SetMessage("");

            DATA.clear();
        }


        /* Get the Command of packet in a std::string type. */
        std::string GetMessage()
        {
            return std::string(MESSAGE, MESSAGE + strlen(MESSAGE));
        }


        /* Packet Null Flag. Length and Checksum both 0. */
        bool IsNull() { return (std::string(MESSAGE) == "" && LENGTH == 0 && CHECKSUM == 0); }


        /* Determine if a packet is fully read. */
        bool Complete() { return (Header() && DATA.size() == LENGTH); }


        /* Determine if header is fully read */
        bool Header()   { return IsNull() ? false : (CHECKSUM > 0 && std::string(MESSAGE) != ""); }


        /* Set the first four bytes in the packet headcer to be of the byte series selected. */
        void SetHeader()
        {
            if (config::fTestNet)
                //memcpy(HEADER, MESSAGE_START_TESTNET, sizeof(MESSAGE_START_TESTNET));
                std::copy(MESSAGE_START_TESTNET,
                    MESSAGE_START_TESTNET + sizeof(MESSAGE_START_TESTNET),
                    HEADER);
            else
                //memcpy(HEADER, MESSAGE_START_MAINNET, sizeof(MESSAGE_START_MAINNET));
                std::copy(MESSAGE_START_MAINNET,
                    MESSAGE_START_MAINNET + sizeof(MESSAGE_START_MAINNET),
                    HEADER);
        }


        /* Set the message in the packet header. */
        void SetMessage(const char* chMessage)
        {
            //std::copy((char*)chMessage, (char*)chMessage + std::min((size_t)12, sizeof(chMessage)), (char*)&MESSAGE);
            strncpy(MESSAGE, chMessage, 12);
        }


        /* Sets the size of the packet from Byte Vector. */
        void SetLength(const std::vector<uint8_t>& BYTES)
        {
            DataStream ssLength(BYTES, SER_NETWORK, MIN_PROTO_VERSION);
            ssLength >> LENGTH;
        }


        /* Set the Packet Checksum Data. */
        void SetChecksum()
        {
            uint512_t hash = LLC::SK512(DATA.begin(), DATA.end());
            //memcpy(&CHECKSUM, &hash, sizeof(CHECKSUM));
            std::copy((uint8_t *)&hash, (uint8_t *)&hash + sizeof(CHECKSUM), (uint8_t *)&CHECKSUM);
        }


        /* Set the Packet Data. */
        void SetData(const DataStream& ssData)
        {
            DATA = std::vector<uint8_t>(ssData.begin(), ssData.end());
            LENGTH = DATA.size();

            SetChecksum();
        }


        /* Check the Validity of the Packet. */
        bool IsValid()
        {
            /* Check that the packet isn't nullptr. */
            if(IsNull())
                return false;

            /* Check the Header Bytes. */
            //if(memcmp(HEADER, (config::fTestNet ? MESSAGE_START_TESTNET : MESSAGE_START_MAINNET), sizeof(HEADER)) != 0)
            if(memory::compare((uint8_t *)HEADER, (uint8_t *)(config::fTestNet ? MESSAGE_START_TESTNET : MESSAGE_START_MAINNET), sizeof(HEADER)) != 0)
                return debug::error("Message Packet (Invalid Packet Header");

            /* Make sure Packet length is within bounds. (Max 512 MB Packet Size) */
            if (LENGTH > (1024 * 1024 * 512))
                return debug::error("Message Packet (", MESSAGE, ", ", LENGTH, " bytes) : Message too Large");

            /* Double check the Message Checksum. */
            uint512_t hash = LLC::SK512(DATA.begin(), DATA.end());
            uint32_t nChecksum = 0;
            //memcpy(&nChecksum, &hash, sizeof(nChecksum));
            std::copy((uint8_t *)&hash, (uint8_t *)&hash + sizeof(nChecksum), (uint8_t *)&nChecksum);

            if (nChecksum != CHECKSUM)
                return debug::error("Message Packet (", MESSAGE, ", ", LENGTH,
                    " bytes) : CHECKSUM MISMATCH nChecksum=", nChecksum, " hdr.nChecksum=", CHECKSUM);

            return true;
        }


        /* Serializes class into a Byte Vector. Used to write Packet to Sockets. */
        std::vector<uint8_t> GetBytes() const
        {
            DataStream ssHeader(SER_NETWORK, MIN_PROTO_VERSION);
            ssHeader << *this;

            std::vector<uint8_t> BYTES(ssHeader.begin(), ssHeader.end());
            BYTES.insert(BYTES.end(), DATA.begin(), DATA.end());

            return BYTES;
        }
    };
}

#endif
