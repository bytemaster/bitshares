#include "api.hpp"
#include "config.hpp"
#include <list>

namespace detail
{
    class block_chain_impl
    {
       public:
         std::vector<block_header> _chain;
         block                     _unconfirmed_head;
    };
}


block_chain::block_chain()
:my( new detail::block_chain_impl() )
{
}

block_chain::~block_chain()
{
}

void block_chain::load( const fc::path& data_dir )
{
   if( !fc::exists( data_dir ) ) 
      fc::create_directories(data_dir);
}


const block& block_chain::get_unconfirmed_head()
{
    return my->_unconfirmed_head;
}



block  block_chain::generate_next_block( const address& a )
{
   block b;
   b.header.version.value         = 0;
   b.header.miner_version.value   = 0;
   b.header.timestamp             = fc::time_point::now();
   b.header.height                = my->_chain.size();
   b.header.nonce                 = 0;
//   b.difficulty            = current_difficulty();

   // apply all non-market transactions

   // perform market calculations
   
   // calculate dividends and updated shares.

   b.header.dividends += get_reward_for_height(b.header.height) / 2;
   // generate coin-base transaction
   transaction coinbase_trx;
   coinbase_trx.outputs.push_back( trx_output( b.header.dividends, unit() ) );
   coinbase_trx.outputs.back().set_claim_function( claim_with_address( a ) );
   b.bundled_trx.emplace_back(std::move(coinbase_trx));
   


  //  b.block_state        = TODO: ... 
  //  b.dividends          = TODO: ...
  // share_changes = ...

   return b;
}

uint64_t block_chain::current_difficulty()
{
  // calculate the average time for the past 120 blocks
  // calculate the delta percent from 10 minutes
  // adjust the difficulty by that percent.
  // difficulty of 1 = average difficulty found by single CPU in 10 minutes
  return 1;
}
int64_t  block_chain::get_reward_for_height( int64_t h )
{
   // TODO: unit-test this method
   int64_t reward = INIT_BLOCK_REWARD * SHARE;
   do
   {
      int num  = h > REWARD_ADJUSTMENT_INTERVAL ? REWARD_ADJUSTMENT_INTERVAL : h;
      reward -= num * (reward/2/REWARD_ADJUSTMENT_INTERVAL);
      h -= REWARD_ADJUSTMENT_INTERVAL;
   } while( h > 0 );

   return reward;
}
