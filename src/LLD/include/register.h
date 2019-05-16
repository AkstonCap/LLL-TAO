/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_INCLUDE_REGISTER_H
#define NEXUS_LLD_INCLUDE_REGISTER_H

#include <LLC/types/uint1024.h>

#include <LLD/include/version.h>
#include <LLD/templates/sector.h>

#include <TAO/Register/types/state.h>
#include <TAO/Register/types/object.h>
#include <TAO/Register/include/enum.h>


namespace LLD
{

    /** RegisterDB
     *
     *  The database class for the Register Layer.
     *
     **/
    class RegisterDB : public SectorDatabase<BinaryHashMap, BinaryLRU>
    {
        std::mutex MEMORY_MUTEX;

        std::map<uint256_t, TAO::Register::State> mapStates;
        std::map<uint32_t, uint256_t> mapIdentifiers;

    public:


        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        RegisterDB(uint8_t nFlags = FLAGS::CREATE | FLAGS::APPEND)
        : SectorDatabase("registers", nFlags) {}


        /** Default Destructor **/
        virtual ~RegisterDB() {}


        /** WriteState
         *
         *  Writes a state register to the register database.
         *  If MEMPOOL flag is set, this will write state into a temporary
         *  memory to handle register state sequencing before blocks commit
         *
         *  If WRITE flag is set, this will erase the temporary memory state
         *  and commit the state to disk
         *
         *  @param[in] hashRegister The register address.
         *  @param[in] state The state register to write.
         *
         *  @return True if write was successful, false otherwise.
         *
         **/
        bool WriteState(const uint256_t& hashRegister, const TAO::Register::State& state, const uint8_t nFlags = TAO::Register::FLAGS::WRITE)
        {
            /* Memory mode for pre-database commits. */
            if(nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                LOCK(MEMORY_MUTEX);

                /* Set the state in the memory map. */
                mapStates[hashRegister] = state;

                return true;
            }
            else if(nFlags & TAO::Register::FLAGS::WRITE)
            {
                LOCK(MEMORY_MUTEX);

                /* Remove the memory state if writing the disk state. */
                if(mapStates.count(hashRegister))
                    mapStates.erase(hashRegister);
            }

            return Write(std::make_pair(std::string("state"), hashRegister), state);
        }


        /** ReadState
         *
         *  Read a state register from the register database.
         *
         *  @param[in] hashRegister The register address.
         *  @param[out] state The state register to read.
         *
         *  @return True if read was successful, false otherwise.
         *
         **/
        bool ReadState(const uint256_t& hashRegister, TAO::Register::State& state, const uint8_t nFlags = TAO::Register::FLAGS::WRITE)
        {
            /* Memory mode for pre-database commits. */
            if((nFlags & TAO::Register::FLAGS::MEMPOOL)
            || (nFlags & TAO::Register::FLAGS::PRESTATE))
            {
                LOCK(MEMORY_MUTEX);

                /* Check for state in memory map. */
                if(mapStates.count(hashRegister))
                {
                    /* Get the state from memory map. */
                    state = mapStates[hashRegister];

                    return true;
                }
            }

            return Read(std::make_pair(std::string("state"), hashRegister), state);
        }


        /** EraseState
         *
         *  Erase a state register from the register database.
         *
         *  @param[in] hashRegister The register address.
         *  @param[out] state The state register to read.
         *
         *  @return True if erase was successful, false otherwise.
         *
         **/
        bool EraseState(const uint256_t& hashRegister, const uint8_t nFlags = TAO::Register::FLAGS::WRITE)
        {
            /* Memory mode for pre-database commits. */
            if(nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                LOCK(MEMORY_MUTEX);

                /* Check for state in memory map. */
                if(mapStates.count(hashRegister))
                    mapStates.erase(hashRegister);
            }

            return Erase(std::make_pair(std::string("state"), hashRegister));
        }


        /** IndexTrust
         *
         *  Index a genesis to a register address.
         *
         *  @param[in] hashGenesis The genesis-id address.
         *  @param[in] hashRegister The state register to index to.
         *
         *  @return True if write was successful, false otherwise.
         *
         **/
        bool IndexTrust(const uint256_t& hashGenesis, const uint256_t& hashRegister)
        {
            return Index(std::make_pair(std::string("genesis"), hashGenesis), std::make_pair(std::string("state"), hashRegister));
        }


        /** HasTrust
         *
         *  Check that a genesis doesn't already exist
         *
         *  @param[in] hashGenesis The genesis-id address.
         *
         *  @return True if write was successful, false otherwise.
         *
         **/
        bool HasTrust(const uint256_t& hashGenesis)
        {
            return Exists(std::make_pair(std::string("genesis"), hashGenesis));
        }


        /** WriteTrust
         *
         *  Write a genesis to a register address.
         *
         *  @param[in] hashGenesis The genesis-id address.
         *  @param[in] state The state register to write.
         *
         *  @return True if write was successful, false otherwise.
         *
         **/
        bool WriteTrust(const uint256_t& hashGenesis, const TAO::Register::State& state)
        {
            return Write(std::make_pair(std::string("genesis"), hashGenesis), state);
        }


        /** ReadTrust
         *
         *  Index a genesis to a register address.
         *
         *  @param[in] hashGenesis The genesis-id address.
         *  @param[in] state The state register to read.
         *
         *  @return True if read was successful, false otherwise.
         *
         **/
        bool ReadTrust(const uint256_t& hashGenesis, TAO::Register::State& state)
        {
            return Read(std::make_pair(std::string("genesis"), hashGenesis), state);
        }


        /** EraseTrust
         *
         *  Erase a genesis from a register address.
         *
         *  @param[in] hashGenesis The genesis-id address.
         *  @param[in] state The state register to read.
         *
         *  @return True if read was successful, false otherwise.
         *
         **/
        bool EraseTrust(const uint256_t& hashGenesis)
        {
            return Erase(std::make_pair(std::string("genesis"), hashGenesis));
        }


        /** WriteIdentifier
         *
         *  Writes a token identifier to the register database.
         *
         *  @param[in] nIdentifier The token identifier.
         *  @param[in] hashRegister The register address of the token.
         *
         *  @return True if write was successful, false otherwise.
         *
         **/
        bool WriteIdentifier(const uint32_t nIdentifier, const uint256_t& hashRegister, const uint8_t nFlags = TAO::Register::FLAGS::WRITE)
        {
            /* Memory mode for pre-database commits. */
            if(nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                LOCK(MEMORY_MUTEX);

                mapIdentifiers[nIdentifier] = hashRegister;

                return true;
            }
            else if(nFlags & TAO::Register::FLAGS::WRITE)
            {
                LOCK(MEMORY_MUTEX);

                /* Remove the memory state if writing the disk state. */
                if(mapIdentifiers.count(nIdentifier))
                    mapIdentifiers.erase(nIdentifier);
            }

            return Write(std::make_pair(std::string("token"), nIdentifier), hashRegister);
        }


        /** EraseIdentifier
         *
         *  Erase a token identifier to the register database.
         *
         *  @param[in] nIdentifier The token identifier.
         *
         *  @return True if write was successful, false otherwise.
         *
         **/
        bool EraseIdentifier(const uint32_t nIdentifier)
        {
            return Erase(std::make_pair(std::string("token"), nIdentifier));
        }


        /** ReadIdentifier
         *
         *  Read a token identifier from the register database.
         *
         *  @param[in] nIdentifier The token identifier.
         *  @param[out] hashRegister The register address of token.
         *  @param[in] nFlags The flags for the register database.
         *
         *  @return True if read was successful, false otherwise.
         *
         **/
        bool ReadIdentifier(const uint32_t nIdentifier, uint256_t& hashRegister, const uint8_t nFlags = TAO::Register::FLAGS::WRITE)
        {
            /* Memory mode for pre-database commits. */
            if(nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                LOCK(MEMORY_MUTEX);

                /* Return the state if it is found. */
                if(mapIdentifiers.count(nIdentifier))
                {
                    hashRegister = mapIdentifiers[nIdentifier];

                    return true;
                }
            }

            return Read(std::make_pair(std::string("token"), nIdentifier), hashRegister);
        }


        /** HasIdentifier
         *
         *  Determines if an identifier exists in the database.
         *
         *  @param[in] nIdentifier The token identifier.
         *  @param[in] nFlags The flags for the register database.
         *
         *  @return True if it exists, false otherwise.
         *
         **/
        bool HasIdentifier(const uint32_t nIdentifier, const uint8_t nFlags = TAO::Register::FLAGS::WRITE)
        {
            /* Memory mode for pre-database commits. */
            if(nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                LOCK(MEMORY_MUTEX);

                /* Return the state if it is found. */
                if(mapIdentifiers.count(nIdentifier))
                    return true;
            }

            return Exists(std::make_pair(std::string("token"), nIdentifier));
        }


        /** HasState
         *
         *  Determines if a state exists in the register database.
         *
         *  @param[in] hashRegister The register address.
         *  @param[in] nFlags The flags for the register database.
         *
         *  @return True if it exists, false otherwise.
         *
         **/
        bool HasState(const uint256_t& hashRegister, const uint8_t nFlags = TAO::Register::FLAGS::WRITE)
        {
            /* Memory mode for pre-database commits. */
            if(nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                LOCK(MEMORY_MUTEX);

                /* Check for state in memory map. */
                if(mapStates.count(hashRegister))
                    return true;
            }

            return Exists(std::make_pair(std::string("state"), hashRegister));
        }


        /** GetStates
         *
         *  Get the previous states of a register.
         *
         *  @param[in] hashRegister The register address.
         *  @param[out] states The states vector to return.
         *
         *  @return True if any states were found, false otherwise.
         *
         **/
        bool GetStates(const uint256_t& hashRegister, std::vector<TAO::Register::State>& states, const uint8_t nFlags = TAO::Register::FLAGS::WRITE)
        {
            /* Memory mode for pre-database commits. */
            if(nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                LOCK(MEMORY_MUTEX);

                /* Set the state in the memory map. */
                if(mapStates.count(hashRegister))
                    states.push_back(mapStates[hashRegister]);
            }

            /* Serialize the key to search for. */
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << std::make_pair(std::string("state"), hashRegister);

            /* Get the list of sector keys. */
            std::vector<SectorKey> vKeys;
            if(!pSectorKeys->Get(ssKey.Bytes(), vKeys))
                return false;

            /* Iterate the list of keys. */
            for(const auto& key : vKeys)
            {
                std::vector<uint8_t> vData;
                if(!Get(key, vData))
                    continue;

                TAO::Register::State state;
                DataStream ssData(vData, SER_LLD, DATABASE_VERSION);
                ssData >> state;

                states.push_back(state);
            }

            return (states.size() > 0);
        }
    };

}

#endif
