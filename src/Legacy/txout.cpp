/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <LLP/include/version.h>

#include <Legacy/types/script.h>
#include <Legacy/types/txout.h>

#include <Util/encoding/include/string.h>
#include <memory/types/data_stream.h>


namespace Legacy
{

	/** Default Constructor. **/
	TxOut::TxOut()
	: nValue       (-1)
	, scriptPubKey ( )
	{
	}


	/** Copy Constructor. **/
	TxOut::TxOut(const TxOut& out)
	: nValue       (out.nValue)
	, scriptPubKey (out.scriptPubKey)
	{
	}


	/** Move Constructor. **/
	TxOut::TxOut(TxOut&& out) noexcept
	: nValue       (std::move(out.nValue))
	, scriptPubKey (std::move(out.scriptPubKey))
	{
	}


	/** Copy assignment. **/
	TxOut& TxOut::operator=(const TxOut& out)
	{
		nValue       = out.nValue;
		scriptPubKey = out.scriptPubKey;

		return *this;
	}


	/** Move assignment. **/
	TxOut& TxOut::operator=(TxOut&& out) noexcept
	{
		nValue       = std::move(out.nValue);
		scriptPubKey = std::move(out.scriptPubKey);

		return *this;
	}


	/** Default destructor. **/
	TxOut::~TxOut()
	{
	}


	/* Constructor */
	TxOut::TxOut(const int64_t nValueIn, const Script& scriptPubKeyIn)
	: nValue       (nValueIn)
	, scriptPubKey (scriptPubKeyIn)
	{
	}


	/* Set the object to null state. */
	void TxOut::SetNull()
	{
		nValue = -1;
		scriptPubKey.clear();
	}


	/* Determine if the object is in a null state. */
	bool TxOut::IsNull() const
	{
		return (nValue == -1);
	}


	/* Clear the object and reset value to 0. */
	void TxOut::SetEmpty()
	{
		nValue = 0;
		scriptPubKey.clear();
	}


	/* Determine if the object is in an empty state. */
	bool TxOut::IsEmpty() const
	{
		return (nValue == 0 && scriptPubKey.empty());
	}


	/* Get the hash of the object. */
	uint512_t TxOut::GetHash() const
	{
		// Most of the time is spent allocating and deallocating DataStream's
	    // buffer.  If this ever needs to be optimized further, make a CStaticStream
	    // class with its buffer on the stack.
	    DataStream ss(SER_GETHASH, LLP::PROTOCOL_VERSION);
	    ss.reserve(8192);
	    ss << *this;
	    return LLC::SK512(ss.begin(), ss.end());
	}


	/* Short Hand debug output of the object */
	std::string TxOut::ToStringShort() const
	{
		return debug::safe_printstr(" out ", FormatMoney(nValue), " ", scriptPubKey.ToString(true));
	}


	/* Full object debug output */
	std::string TxOut::ToString() const
	{
		if(IsEmpty()) return "TxOut(empty)";
		if(scriptPubKey.size() < 6)
			return "TxOut(error)";
		return debug::safe_printstr("TxOut(nValue=", FormatMoney(nValue), ", scriptPubKey=", scriptPubKey.ToString(), ")");
	}


	/* Dump the full object to the console (stdout) */
	void TxOut::print() const
	{
		debug::log(0, ToString());
	}
}
