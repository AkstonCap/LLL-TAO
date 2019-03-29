/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/



#include <TAO/API/include/utils.h>
#include <TAO/API/types/exception.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/types/tritium.h>

#include <TAO/Operation/include/stream.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/operations.h>
#include <TAO/Operation/include/output.h>

#include <Legacy/include/evaluate.h>

#include <Util/include/args.h>
#include <Util/include/hex.h>
#include <Util/include/json.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Utils Layer namespace. */
        namespace Utils
        {
            /* Converts the block to formatted JSON */
            json::json blockToJSON(const TAO::Ledger::BlockState& block,  int nTransactionVerbosity)
            {
                /* Decalre the response object*/
                json::json result;

                result["hash"] = block.GetHash().GetHex();
                // the hash that was relevant for Proof of Stake or Proof of Work (depending on block version)
                result["proofhash"] =
                                        block.nVersion < 5 ? block.GetHash().GetHex() :
                                        ((block.nChannel == 0) ? block.StakeHash().GetHex() : block.ProofHash().GetHex());

                result["size"] = (int)::GetSerializeSize(block, SER_NETWORK, LLP::PROTOCOL_VERSION);
                result["height"] = (int)block.nHeight;
                result["channel"] = (int)block.nChannel;
                result["version"] = (int)block.nVersion;
                result["merkleroot"] = block.hashMerkleRoot.GetHex();
                result["time"] = convert::DateTimeStrFormat(block.GetBlockTime());
                result["nonce"] = (uint64_t)block.nNonce;
                result["bits"] = HexBits(block.nBits);
                result["difficulty"] = TAO::Ledger::GetDifficulty(block.nBits, block.nChannel);
                result["mint"] = Legacy::SatoshisToAmount(block.nMint);
                if (block.hashPrevBlock != 0)
                    result["previousblockhash"] = block.hashPrevBlock.GetHex();
                if (block.hashNextBlock != 0)
                    result["nextblockhash"] = block.hashNextBlock.GetHex();

                /* Add the transaction data if the caller has requested it*/
                if(nTransactionVerbosity > 0)
                {
                    json::json txinfo = json::json::array();

                    /* Iterate through each transaction hash in the block vtx*/
                    for (const auto& vtx : block.vtx)
                    {
                        if(vtx.first == TAO::Ledger::TYPE::TRITIUM_TX)
                        {
                            /* Get the tritium transaction from the database*/
                            TAO::Ledger::Transaction tx;
                            if(LLD::legDB->ReadTx(vtx.second, tx))
                            {
                                /* add the transaction JSON.  */ 
                                json::json txdata = transactionToJSON(tx, block, nTransactionVerbosity);

                                txinfo.push_back(txdata);
                            }
                        }
                        else if(vtx.first == TAO::Ledger::TYPE::LEGACY_TX)
                        {
                            /* Get the legacy transaction from the database. */
                            Legacy::Transaction tx;
                            if(LLD::legacyDB->ReadTx(vtx.second, tx))
                            {
                                /* add the transaction JSON.  */ 
                                json::json txdata = transactionToJSON(tx, block, nTransactionVerbosity);

                                txinfo.push_back(txdata);

                            }
                        }
                    }
                    /* Add the transaction data to the response */
                    result["tx"] = txinfo;
                }

                return result;
            }

            /* Converts the transaction to formatted JSON */
            json::json transactionToJSON( TAO::Ledger::Transaction& tx, const TAO::Ledger::BlockState& block, int nTransactionVerbosity)
            {
                /* Declare JSON object to return */
                json::json txdata;

                /* Always add the hash if level 1 and up*/
                if( nTransactionVerbosity >= 1)
                    txdata["hash"] = tx.GetHash().ToString();

                /* Basic TX info for level 2 and up */
                if (nTransactionVerbosity >= 2)
                {
                    txdata["type"] = tx.GetTxTypeString();
                    txdata["version"] = tx.nVersion;
                    txdata["sequence"] = tx.nSequence;
                    txdata["timestamp"] = tx.nTimestamp;
                    txdata["confirmations"] = block.IsNull() ? 0 : TAO::Ledger::ChainState::nBestHeight.load() - block.nHeight + 1;


                    /* Genesis and hashes are verbose 3 and up. */
                    if(nTransactionVerbosity >= 3)
                    {
                        txdata["genesis"] = tx.hashGenesis.ToString();
                        txdata["nexthash"] = tx.hashNext.ToString();
                        txdata["prevhash"] = tx.hashPrevTx.ToString();
                    }

                    /* Signatures and public keys are verbose level 4 and up. */
                    if(nTransactionVerbosity >= 4)
                    {
                        txdata["pubkey"] = HexStr(tx.vchPubKey.begin(), tx.vchPubKey.end());
                        txdata["signature"] = HexStr(tx.vchSig.begin(),    tx.vchSig.end());
                    }

                    txdata["operation"]  = operationToJSON(tx.ssOperation);                    
                }

                return txdata;
            }

            /* Converts the transaction to formatted JSON */
            json::json transactionToJSON( Legacy::Transaction& tx, const TAO::Ledger::BlockState& block, int nTransactionVerbosity)
            {
                /* Declare JSON object to return */
                json::json txdata;

                /* Always add the hash */
                txdata["hash"] = tx.GetHash().GetHex();

                /* Basic TX info for level 1 and up */
                if (nTransactionVerbosity > 0)
                {
                    txdata["type"] = tx.GetTxTypeString();
                    txdata["timestamp"] = tx.nTime;
                    txdata["amount"] = Legacy::SatoshisToAmount( tx.GetValueOut());
                    txdata["confirmations"] = block.IsNull() ? 0 : TAO::Ledger::ChainState::nBestHeight.load() - block.nHeight + 1;

                    /* Don't add inputs for coinbase or coinstake transactions */
                    if(!tx.IsCoinBase() && !tx.IsCoinStake())
                    {            
                        /* Declare the inputs JSON array */
                        json::json inputs = json::json::array();
                        /* Iterate through each input */
                        for(const Legacy::TxIn& txin : tx.vin)
                        {
                            /* Read the previous transaction in order to get its outputs */
                            Legacy::Transaction txprev;
                            if(!LLD::legacyDB->ReadTx(txin.prevout.hash, txprev))
                                throw APIException(-5, "Invalid transaction id");

                            Legacy::NexusAddress cAddress;
                            if(!Legacy::ExtractAddress(txprev.vout[txin.prevout.n].scriptPubKey, cAddress))
                                throw APIException(-5, "Unable to Extract Input Address");
                        
                            inputs.push_back(debug::safe_printstr("%s:%f", cAddress.ToString().c_str(), (double) tx.vout[txin.prevout.n].nValue / Legacy::COIN));
                        }
                        txdata["inputs"] = inputs;
                    }

                    /* Declare the output JSON array */
                    json::json outputs = json::json::array();
                    /* Iterate through each output */
                    for(const Legacy::TxOut& txout : tx.vout)
                    {
                        Legacy::NexusAddress cAddress;
                        if(!Legacy::ExtractAddress(txout.scriptPubKey, cAddress))
                            throw APIException(-5, "Unable to Extract Input Address");
                        
                        outputs.push_back(debug::safe_printstr("%s:%f", cAddress.ToString().c_str(), (double) txout.nValue / Legacy::COIN));
                    }
                    txdata["outputs"] = outputs;
                }

                return txdata;
            }


            /* Converts a serialized operation stream to formattted JSON */
            json::json operationToJSON(const TAO::Operation::Stream& ssOperation)
            {
                /* Declare the return JSON object*/
                json::json ret;

                /* Start the stream at the beginning. */
                ssOperation.seek(0, STREAM::BEGIN);

                /* Make sure no exceptions are thrown. */
                try
                {

                    /* Loop through the operations ssOperation. */
                    while(!ssOperation.end())
                    {
                        uint8_t OPERATION;
                        ssOperation >> OPERATION;

                        /* Check the current opcode. */
                        switch(OPERATION)
                        {

                            /* Record a new state to the register. */
                            case TAO::Operation::OP::WRITE:
                            {
                                /* Get the Address of the Register. */
                                uint256_t hashAddress;
                                ssOperation >> hashAddress;

                                /* Deserialize the register from ssOperation. */
                                std::vector<uint8_t> vchData;
                                ssOperation >> vchData;

                                /* Output the json information. */
                                ret["OP"] = "WRITE";
                                ret["address"] = hashAddress.ToString();
                                ret["data"] = HexStr(vchData.begin(), vchData.end()); 

                                break;
                            }


                            /* Append a new state to the register. */
                            case TAO::Operation::OP::APPEND:
                            {
                                /* Get the Address of the Register. */
                                uint256_t hashAddress;
                                ssOperation >> hashAddress;

                                /* Deserialize the register from ssOperation. */
                                std::vector<uint8_t> vchData;
                                ssOperation >> vchData;

                                /* Output the json information. */
                                ret["OP"] = "APPEND";
                                ret["address"] = hashAddress.ToString();
                                ret["data"] = HexStr(vchData.begin(), vchData.end()); 

                                break;
                            }


                            /* Create a new register. */
                            case TAO::Operation::OP::REGISTER:
                            {
                                /* Extract the address from the ssOperation. */
                                uint256_t hashAddress;
                                ssOperation >> hashAddress;

                                /* Extract the register type from ssOperation. */
                                uint8_t nType;
                                ssOperation >> nType;

                                /* Extract the register data from the ssOperation. */
                                std::vector<uint8_t> vchData;
                                ssOperation >> vchData;

                                /* Output the json information. */
                                ret["OP"] = "REGISTER";
                                ret["address"] = hashAddress.ToString();
                                ret["type"] = nType;
                                ret["data"] = HexStr(vchData.begin(), vchData.end()); 


                                break;
                            }


                            /* Transfer ownership of a register to another signature chain. */
                            case TAO::Operation::OP::TRANSFER:
                            {
                                /* Extract the address from the ssOperation. */
                                uint256_t hashAddress;
                                ssOperation >> hashAddress;

                                /* Read the register transfer recipient. */
                                uint256_t hashTransfer;
                                ssOperation >> hashTransfer;

                                /* Output the json information. */
                                ret["OP"] = "TRANSFER";
                                ret["address"] = hashAddress.ToString();
                                ret["transfer"] = hashTransfer.ToString();

                                break;
                            }


                            /* Debit tokens from an account you own. */
                            case TAO::Operation::OP::DEBIT:
                            {
                                uint256_t hashAddress; //the register address debit is being sent from. Hard reject if this register isn't account id
                                ssOperation >> hashAddress;

                                uint256_t hashTransfer;   //the register address debit is being sent to. Hard reject if this register isn't an account id
                                ssOperation >> hashTransfer;

                                uint64_t  nAmount;  //the amount to be transfered
                                ssOperation >> nAmount;

                                /* Output the json information. */
                                ret["OP"] = "DEBIT";
                                ret["address"] = hashAddress.ToString();
                                ret["transfer"] = hashTransfer.ToString();
                                ret["amount"] = nAmount;

                                break;
                            }


                            /* Credit tokens to an account you own. */
                            case TAO::Operation::OP::CREDIT:
                            {
                                /* The transaction that this credit is claiming. */
                                uint512_t hashTx;
                                ssOperation >> hashTx;

                                /* The proof this credit is using to make claims. */
                                uint256_t hashProof;
                                ssOperation >> hashProof;

                                /* The account that is being credited. */
                                uint256_t hashAccount;
                                ssOperation >> hashAccount;

                                /* The total to be credited. */
                                uint64_t  nCredit;
                                ssOperation >> nCredit;

                                /* Output the json information. */
                                ret["OP"] = "CREDIT";
                                ret["txid"] = hashTx.ToString();
                                ret["proof"] = hashProof.ToString();
                                ret["account"] = hashAccount.ToString();
                                ret["amount"] = nCredit;

                                break;
                            }


                            /* Coinbase operation. Creates an account if none exists. */
                            case TAO::Operation::OP::COINBASE:
                            {

                                /* The total to be credited. */
                                uint64_t  nCredit;
                                ssOperation >> nCredit;

                                /* The extra nNonce available in script. */
                                uint64_t  nExtraNonce;
                                ssOperation >> nExtraNonce;

                                /* Output the json information. */
                                ret["OP"] = "COINBASE";
                                ret["nonce"] = nExtraNonce;
                                ret["amount"] = nCredit;
                                
                                break;
                            }


                            /* Coinstake operation. Requires an account. */
                            case TAO::Operation::OP::TRUST:
                            {

                                /* The account that is being staked. */
                                uint256_t hashAccount;
                                ssOperation >> hashAccount;

                                /* The previous trust block. */
                                uint1024_t hashLastTrust;
                                ssOperation >> hashLastTrust;

                                /* Previous trust sequence number. */
                                uint32_t nSequence;
                                ssOperation >> nSequence;

                                /* The trust calculated. */
                                uint64_t nTrust;
                                ssOperation >> nTrust;

                                /* The total to be staked. */
                                uint64_t  nStake;
                                ssOperation >> nStake;

                                /* Output the json information. */
                                ret["OP"] = "TRUST";
                                ret["account"] = hashAccount.ToString();
                                ret["last"] = hashLastTrust.ToString();
                                ret["sequence"] = nSequence;
                                ret["trust"] = nTrust;
                                ret["amount"] = nStake;

                                break;
                            }


                            /* Authorize a transaction to happen with a temporal proof. */
                            case TAO::Operation::OP::AUTHORIZE:
                            {
                                /* The transaction that you are authorizing. */
                                uint512_t hashTx;
                                ssOperation >> hashTx;

                                /* The proof you are using that you have rights. */
                                uint256_t hashProof;
                                ssOperation >> hashProof;

                                /* Output the json information. */
                                ret["OP"] = "AUTHORIZE";
                                ret["txid"] = hashTx.ToString();
                                ret["proof"] = hashProof.ToString();

                                break;
                            }


                            case TAO::Operation::OP::VOTE:
                            {
                                /* The object register to tally the votes for. */
                                uint256_t hashRegister;

                                /* The account that holds the token balance. */
                                uint256_t hashProof;

                                /* Output the json information. */
                                ret["OP"] = "VOTE";
                                ret["register"] = hashRegister.ToString();
                                ret["proof"] = hashProof.ToString();

                                break;
                            }

                            //case TAO::Operation::OP::SIGNATURE:
                            //{
                                //a transaction proof that designates the hashOwner or genesisID has signed published data
                                //>> vchData. to prove this data was signed by being published. This can be a hash if needed.
                                /* Output the json information. */
                                //ret["OP"] = "SIGNATURE";
                                //ret["register"] = hashRegister.ToString();
                                //ret["proof"] = hashProof.ToString();
                            //    break;
                            //}
                        }
                    }
                }
                catch(const std::runtime_error& e)
                {

                }

                return ret;
            }

        }

    }

}