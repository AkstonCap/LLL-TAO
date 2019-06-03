## USERS ##
#Create user
./nexus users/create/user/paul password=paul pin=1234

#Login
./nexus users/login/user/paul password=paul pin=1234

#List your accounts (note that account names are only returned for the logged in user)
./nexus users/list/accounts/paul

#List someone elses accounts when you know their username
./nexus users/list/accounts/jack

#list someone eles account when you only know their genesis
./neus users/list/accounts/1ff463e036cbde3595fbe2de9dff15721a89e99ef3e2e9bfa7ce48ed825e9ec2

#List your tokens
./nexus users/list/tokens/paul

#List your assets
./nexus users/list/assets/paul

#List your notifications (transactions you need to credit/claim)
./nexus users/list/notifications/paul


## FINANCE ##
#Create a new NXS account called savings (note PIN is required)
./nexus finance/create/account name=savings pin=1234

#Send (debit) 10 NXS from your default account to savings
./nexus finance/debit/account/default amount=10 name_to=savings pin=1234

#If you do NOT have the events processor running and configured to process debits, then this is how you create a credit to add the
#funds from the debit transaction to your account.
#the txid is the transaction ID of the debit transaciton (available from notifications)
./nexus finance/credit/account txid=ca2741d25a5f00f59ffe20f1d34cac71a2dedb547e83ed20a9d59c3812527c7356f7a5662bb0305dd2a4fc36b1596583d1922fdc615a25e69a8c36596370c3cd pin=1234

#Send (debit) 10 NXS from your savings to bob's savings account
./nexus finance/debit/account/savings amount=10 name_to=bob:savings pin=1234

#Send (debit) 10 NXS from your savings to someone's account when they only provided the address not the name
./nexus finance/debit/account/savings amount=10 address_to=a73b0598e1aabf97398fd02563f7074acd5292d493aaed2697f4e2037519bc20 pin=1234

#List your NXS accounts
./nexus finance/list/accounts

#List a NXS account by name
./nexus finance/get/account/default

#Get a particular field (the balance in this case) from the account info
./nexus finance/get/account/default/balance


## TOKENS ##
#Create a new token called ABC with initial supply 1000 and digits 2
./nexus tokens/create/token/abc supply=1000 digits=2 pin=1234

#Create an account called abc_main to use with the abc token (note this example assumes that YOU created the token so it is in your namespace)
./nexus tokens/create/account/abc_main token_name=abc pin=1234

#Create an account called abc_svings to use with the abc token (note this example assumes that user paul created the token but you are logged in as someone else )
./nexus tokens/create/account/abc_savings token_name=paul:abc pin=4567

#Send (debit) 100 abc tokens from the initial supply to your abc_main account
./nexus tokens/debit/token/abc name_to=abc_main amount=100 pin=1234

#Receive (credit) the above debit into your account 
./nexus tokens/credit/account txid=df5a97bc105170ec7722476586c66c08e12750242e31a8ab3953fc103dff9326894bc42628cb718311f4f0b3617018b0c1ab2d9cc34d43d2586c0e006f846ebc pin=1234

#Send (debit) 10 abc tokens from your abc_main account to jack's abc_savings account 
./nexus tokens/debit/account/abc_main name_to=jack:abc_savings amount=10 pin=1234

#Get token information
./nexus tokens/get/token/abc

#Get token account information
./nexus tokens/get/account/abc_main


## ASSETS ##
#Create a simple raw asset 
./nexus assets/create/asset name=myasset format=raw data=somepieceofmeaningfuldata pin=1234

#Create an asset in basic format (read only) with multiple fields
./nexus assets/create/asset name=myasset2 somefield=somevalue someotherfield=someothervalue lastfield=lastfieldvalue pin=1234

#Create an asset using the json format to define multiple fields with explicit field type and mutability.  
#NOTE In this example the description field is updatable (mutable)
#NOTE The json payload is designed to be submitted via "application/json" content in a GET/POST HTTP request, which is why this
#is hard to read on a CLI but it looks like this:
#[
#    {
#        "name": "serial_number",
#        "type": "uint64",
#        "value": "1234",
#        "mutable" : "false"
#    },
#    {
#        "name": "description",
#        "type": "string",
#        "value": "This is the description of my asset",
#        "mutable" : "true"
#    }
#]
./nexus assets/create/asset name=myasset3 format=JSON json='[{"name": "serial_number","type": "uint64","value": "1234","mutable" : "false"},{"name": "description","type": "string","value": "This is the description of my asset","mutable" : "true"}]' pin=1234

#Retrieve an asset by name that you own
./nexus assets/get/asset/myasset

#Retrieve an asset by name that someone else owns 
./nexus assets/get/asset/jack:myasset

#Retrieve an asset by address
./nexus assets/get/asset/a3285be4e4d29f902de73d62aed93e1b28d4d7596cc61a02b489a0e6298e68b9

#Update a mutable field in an asset (you must own it)
./nexus assets/update/asset/myasset3 description="this is the new description" pin=1234

#Transfer an asset to another signature chain (by username)
./nexus assets/transfer/asset/myasset username=jack pin=1234

#Transer an asset to another signature chain (by genesis has)
./nexus assets/transfer/asset/myasset2 destination=1ff463e036cbde3595fbe2de9dff15721a89e99ef3e2e9bfa7ce48ed825e9ec2 pin=1234

#Claim ownership of an asset that has been transferred to you 
#(or claim back ownwership if you transferred it and it hasn't yet been claimed)
./nexus assets/claim/asset txid=5fa2381c0b7ba0fbb3cc43f7ac46751b4399dcb6a0ebe2dac4a0c14d57c32695fd185564a2041b1ea04482bfb27fdb7c3ff4792767fa265fdf9849baa8c92e77

#Tokenize an asset
./nexus assets/tokenize/asset name=myasset token_name=abc pin=1234 

