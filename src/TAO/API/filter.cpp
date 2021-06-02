/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/filter.h>
#include <TAO/API/include/get.h>
#include <TAO/API/types/exception.h>

#include <TAO/Register/types/object.h>

#include <Util/include/string.h>

namespace TAO::API
{
    /* Determines if an object should be included in a list based on input parameters. */
    bool FilterObject(const encoding::json& jParams, const TAO::Register::Object& objCheck)
    {
        /* Check for a where clause. */
        if(jParams.find("where") == jParams.end())
            return true; //no filters

        /* Loop through our where clauses. */
        bool fResult = true;
        for(const auto& jClause : jParams["where"])
        {
            /* Check that we are operating object. */
            if(jClause["class"].get<std::string>() != "object")
                continue;

            /* Grab a copy of our field to check. */
            const std::string strName = jClause["field"].get<std::string>();

            /* Check for the available type. */
            if(!objCheck.Check(strName))
                continue;

            /* Grab a reference of value to check. */
            const encoding::json& jCheck = jClause["value"];

            /* Now let's check our type. */
            uint8_t nType = 0;
            objCheck.Type(strName, nType);

            /* Grab our OP code now. */
            const std::string strOP = jClause["operator"].get<std::string>();

            /* Switch based on type. */
            bool fEvaluate = false;
            switch(nType)
            {
                /* Check for uint8_t type. */
                case TAO::Register::TYPES::UINT8_T:
                case TAO::Register::TYPES::UINT16_T:
                case TAO::Register::TYPES::UINT32_T:
                case TAO::Register::TYPES::UINT64_T:
                {
                    /* Grab a copy of our object register value. */
                    uint64_t nValue = 0; //we will store all in 64 bits
                    if(!jCheck.is_string())
                        continue;

                    /* We need to switch to decode the correct type out of the object register. */
                    switch(nType)
                    {
                        /* Case for 8-bit unsigned int, or unsigned char. */
                        case TAO::Register::TYPES::UINT8_T:
                            nValue = objCheck.get<uint8_t>(strName);
                            break;

                        /* Case for 16-bit unsigned int, or unsigned short. */
                        case TAO::Register::TYPES::UINT16_T:
                            nValue = objCheck.get<uint16_t>(strName);
                            break;

                        /* Case for 32-bit unsigned int, or unsigned int. */
                        case TAO::Register::TYPES::UINT32_T:
                            nValue = objCheck.get<uint32_t>(strName);
                            break;

                        /* Case for 64-bit unsigned int, or unsigned long. */
                        case TAO::Register::TYPES::UINT64_T:
                            nValue = objCheck.get<uint64_t>(strName);
                            break;
                    }

                    /* Special rule for balances that need to cast in and out of double. */
                    uint64_t nCheck = std::stoull(jCheck.get<std::string>());
                    if(strName == "balance" || strName == "stake" || strName == "supply")
                        nCheck = std::stod(jCheck.get<std::string>()) * GetFigures(objCheck);

                    /* Check our not operator. */
                    if(strOP.find("!=") != strOP.npos && nValue != nCheck)
                    {
                        fEvaluate = true;
                        break;
                    }

                    /* Check that our values match. */
                    if(strOP.find("=") != strOP.npos && nValue == nCheck)
                        fEvaluate = true;

                    /* Check our less than operator. */
                    if(strOP.find("<") != strOP.npos && nValue < nCheck)
                        fEvaluate = true;

                    /* Check our greater than operator. */
                    if(strOP.find(">") != strOP.npos && nValue > nCheck)
                        fEvaluate = true;

                    break;
                }

                /* Check for uint256_t type. */
                case TAO::Register::TYPES::UINT256_T:
                {
                    /* Grab a copy of our hash to check. */
                    const uint256_t hashValue = objCheck.get<uint256_t>(strName);
                    if(!jCheck.is_string())
                        continue;

                    /* Special case where value is an address. */
                    uint256_t hashCheck = uint256_t(jCheck.get<std::string>());
                    if(strName == "token" || strName == "address")
                        hashCheck = TAO::Register::Address(jCheck.get<std::string>());

                    /* Check our not operator. */
                    if(strOP.find("!=") != strOP.npos && hashValue != hashCheck)
                    {
                        fEvaluate = true;
                        break;
                    }

                    /* Check that our values match. */
                    if(strOP.find("=") != strOP.npos && hashValue == hashCheck)
                        fEvaluate = true;

                    /* Check our less than operator. */
                    if(strOP.find("<") != strOP.npos && hashValue < hashCheck)
                        fEvaluate = true;

                    /* Check our greater than operator. */
                    if(strOP.find(">") != strOP.npos && hashValue > hashCheck)
                        fEvaluate = true;

                    break;
                }

                /* Check for uint512_t type. */
                case TAO::Register::TYPES::UINT512_T:
                {
                    /* Set the return value from object register data. */
                    const uint512_t hashValue = objCheck.get<uint512_t>(strName);
                    if(!jCheck.is_string())
                        continue;

                    /* Our hash to check against. */
                    const uint512_t hashCheck =
                        uint512_t(jCheck.get<std::string>());

                    /* Check our not operator. */
                    if(strOP.find("!=") != strOP.npos && hashValue != hashCheck)
                    {
                        fEvaluate = true;
                        break;
                    }

                    /* Check that our values match. */
                    if(strOP.find("=") != strOP.npos && hashValue == hashCheck)
                        fEvaluate = true;

                    /* Check our less than operator. */
                    if(strOP.find("<") != strOP.npos && hashValue < hashCheck)
                        fEvaluate = true;

                    /* Check our greater than operator. */
                    if(strOP.find(">") != strOP.npos && hashValue > hashCheck)
                        fEvaluate = true;

                    break;
                }

                /* Check for uint1024_t type. */
                case TAO::Register::TYPES::UINT1024_T:
                {
                    /* Set the return value from object register data. */
                    const uint1024_t hashValue = objCheck.get<uint1024_t>(strName);
                    if(!jCheck.is_string())
                        continue;

                    /* Our hash to check against. */
                    const uint1024_t hashCheck =
                        uint1024_t(jCheck.get<std::string>());

                    /* Check our not operator. */
                    if(strOP.find("!=") != strOP.npos && hashValue != hashCheck)
                    {
                        fEvaluate = true;
                        break;
                    }

                    /* Check our regular operators. */
                    if(strOP.find("=") != strOP.npos && hashValue == hashCheck)
                        fEvaluate = true;

                    /* Check our less than operator. */
                    if(strOP.find("<") != strOP.npos && hashValue < hashCheck)
                        fEvaluate = true;

                    /* Check our greater than operator. */
                    if(strOP.find(">") != strOP.npos && hashValue > hashCheck)
                        fEvaluate = true;

                    break;
                }

                /* Check for string type. */
                case TAO::Register::TYPES::STRING:
                {
                    /* Check syntax to omit < and > operators for string comparisons. */
                    if(strOP.find_first_of("<>") != strOP.npos)
                        throw APIException(-57, "Query Syntax Error, only '=' and '!=' operator allowed for TYPES::STRING");

                    /* Remove trailing nulls from the data, which are padding to maxlength on mutable fields */
                    const std::string strValue = objCheck.get<std::string>(strName);
                    if(!jCheck.is_string())
                        continue;

                    /* Our string to check against. */
                    const std::string strCheck = jCheck.get<std::string>();
                    if(strCheck.find("*") != strCheck.npos) //handle for all characters wildcard
                    {
                        /* Check for multiple asterisk which will break our splitting logic. */
                        if(strCheck.find("**") != strCheck.npos)
                            throw APIException(-56, "Query Syntax Error, duplicate wildcard not allowed ", strCheck);

                        /* Check for single asterisk. */
                        if(strCheck == "*")
                        {
                            fEvaluate = true;
                            break;
                        }

                        /* Split by delimiter. */
                        std::vector<std::string> vWildcard;
                        ParseString(strCheck, '*', vWildcard);

                        /* Build an object to store our strings between wildcards. */
                        std::string::size_type nPos = 0;
                        for(uint32_t n = 0; n < vWildcard.size(); ++n)
                        {
                            /* Grab reference to check. */
                            const std::string& strWildcard = vWildcard[n];

                            /* If non empty, push to comparisons vector. */
                            if(!strWildcard.empty())
                            {
                                /* Check if we have pattern 'check*' */
                                const std::string::size_type nFind = strValue.find(strWildcard, nPos);
                                if(nFind != strValue.npos)
                                    fEvaluate = true;
                                else
                                {
                                    fEvaluate = false;
                                    break;
                                }

                                /* Check for wildcard postfix i.e. (check*) */
                                if(n == 0 && nFind != 0)
                                {
                                    fEvaluate = false;
                                    break;
                                }

                                /* Check for wildcard prefix i.e. (*check) */
                                if(n == vWildcard.size() - 1 && nFind != strValue.length() - strWildcard.length())
                                {
                                    fEvaluate = false;
                                    break;
                                }

                                /* Iterate our position past current string. */
                                nPos = (nFind + strWildcard.length());
                            }
                        }
                    }

                    /* Check for not operator. */
                    if(strOP == "!=" && strValue != strCheck)
                    {
                        fEvaluate = true;
                        break;
                    }

                    /* Check the rest of our combinations. */
                    if(strOP == "=" && strValue == strCheck)
                    {
                        fEvaluate = true;
                        break;
                    }
                }
            }

            /* Check or logical operators now. */
            fResult = fResult && fEvaluate;
        }

        return fResult;
    }


    /* If the caller has requested a fieldname to filter on then this filters the response JSON to only include that field */
    void FilterResponse(const encoding::json& jParams, encoding::json &jResponse)
    {
        /* Check for fieldname filters. */
        if(jParams.find("fieldname") != jParams.end())
        {
            /* Grab our field string to rebuild response. */
            const std::string strField = jParams["fieldname"].get<std::string>();

            /* Copy over our new field. */
            const encoding::json jRet = { strField, jResponse[strField].get<std::string>() };
            jResponse      = jRet;
        }
    }
}
