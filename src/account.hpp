#pragma once

/**
 *  An account is a set of addresses and the
 *  transactions that refer to them. It also
 *  contains any outputs for which there are
 *  no transactions, as a 'starting balance'
 *  in the case that the node lacks the full
 *  transaction history of the chain.
 *
 *  An account may or may not have the private
 *  keys associated with it available and
 *  thus can be used to monitor a group of
 *  addresses and to calculate a balance.
 *
 *  Each account has its own wallet which may
 *  optionally contain the private keys used
 *  for the account.
 *
 *  An account can be managed entirely independantly
 *  from the blockchain.  When an account is first
 *  loaded, all transactions and outputs must be
 *  validated against the current block chain to
 *  make sure they are still valid / consistant. This
 *  also happens anytime a fork merges.
 */
class account
{
  public:
     account();
     ~account();

     /** returns the wallet for this account so
      * that it may be used to sign transactions that
      * spend from this account.
      */
     wallet&                           get_wallet();

     // returns all 'positive' balances by unit with the specified
     // number of confirmations.  This calculation only factors in
     // the cached outputs and transactions.
     std::map<unit,int64_t>            get_balances( uint64_t confirm = 0 );

     // TODO: returns all short positions.

     void                              set_name( const std::string& name );
     std::string                       name()const;
                                       
     void                              load( const fc::path& account_file );
     void                              save( const fc::path& account_file );
                                       
     void                              add_address( const address& a );

     /** Gets the next new address that hasn't been used by this account,
      *  yet exists in the wallet.
      **/
     address                           get_new_address();

     /**
      *  Removes all cached outputs and transactions because they
      *  may no longer be valid and the block-chain needs to be
      *  re-indexed in order to calculate a valid balance.
      */
     void                              clear_output_and_transaction_cache();

     /** used to load the initial condition, starting balance */
     void                              add_output( const trx_output& o );

     /** used to update the balance based upon the transaction history */
     void                              add_transaction( const transaction& trx );
     bool                              contains_address( const address& a );
     const std::vector<transaction>&   get_transactions()const;

     /** Gets all addresses associated with this account regardless
      *  of whether or not we have the private key
      */
     std::vector<address>              get_addresses();
  private:
     std::unique_ptr<detail::wallet> my;
};


