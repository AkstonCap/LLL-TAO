/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_TYPES_INPOINT_H
#define NEXUS_LEGACY_TYPES_INPOINT_H

#include <Legacy/types/transaction.h>


namespace Legacy
{

	/** An inpoint - a combination of a transaction and an index n into its vin */
	class InPoint
	{
	public:

			/** The transaction pointer. **/
			Transaction* ptx;


			/** The index n of transaction input. **/
			uint32_t n;


			/** Default Constructor
			 *
			 *	Sets object to null state.
			 *
			 **/
			InPoint()
			{
					SetNull();
			}


			/** Constructor
			 *
			 *	@param[in] ptxIn The transaction input pointer
			 *	@param[in] nIn The index input
			 *
			 **/
			InPoint(Transaction* ptxIn, uint32_t nIn)
			{
					ptx = ptxIn;
					n = nIn;
			}


			/** Destructor **/
			~InPoint()
			{
			}


			/** Set Null
			 *
			 *	Sets the object to null state
			 *
			 **/
			void SetNull()
			{
					ptx = nullptr;
					n = -1;
			}


			/** Is Null
			 *
			 *	Checks the objects null state.
			 *
			 **/
			bool IsNull() const
			{
					return (ptx == nullptr && n == -1);
			}
	};
}

#endif
