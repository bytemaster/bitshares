#pragma once
#include <fc/signal.hpp>
#include <fc/filesystem.hpp>
#include "blockchain.hpp"
#include <vector>
#include <memory>
#include <map>

namespace detail 
{
    class block_chain_impl;
}

struct exchange_state
{
    unit      sell_unit;
    unit      buy_unit; 
    int64_t   buy_price; // number of buy_unit to buy 1 sell_unit
};
FC_REFLECT( exchange_state, (sell_unit)(buy_unit)(buy_price) )

/**
 *  @class block_chain
 *  @brief Encapsulates access to the blockchain databases
 *
 */
class block_chain
{
   public:
      block_chain();
      ~block_chain();

      /**
       *  Emitted any-time the block chain changes due to a new
       *  block or transaction.  Changes include the head 'unconfirmed' block.
       */
      fc::signal<void()>      changed;

      void                    load( const fc::path& data_dir );

      /**
       *  Adds a new transaction to the chain 'free pool'
       */
      void                    add_transaction( const transaction& trx );
      transaction             get_transaction( const fc::sha256& trx_id );

      /**
       *  Returns any transactions referenced by a block in the database for
       *  which we do not have the trx ID.
       */
      std::vector<fc::sha256> get_missing_transactions( const fc::sha256& trx );

      /**
       *  Adds the block to the database, doesn't mean it goes in the head.
       */
      void                    add_block( const block& b );

      /**
       *  Returns the most recent block with all 'unconfirmed' transactions.
       *
       *  @param new_chain will start a new block-chain if there is no chain
       */
      const block&            get_unconfirmed_head();

      block                   generate_next_block( const address& a );
      void                    generate_gensis_block();


      /**
       *  Returns all balances for a particular address based upon confirmation status.
       */
      std::map<unit,int64_t>  get_balances( const address& a, int confirmations = -1 );
      share_state             get_share_info( uint64_t unit_id );
      uint64_t                current_difficulty();

      /**
       *  @return the current exchange state for 
       */
      exchange_state          get_exchange_info( uint64_t sell_unit, uint64_t buy_unit = 0);

      static int64_t          get_reward_for_height( int64_t h );

      /**
       *  Returns all outputs spendable by a particular address, this includes
       *  orders that could be canceled.
       */
      std::vector<output_cache> get_outputs( const address& a );

   private:
      std::unique_ptr<detail::block_chain_impl> my; 
};

