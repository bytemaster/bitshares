#include "api.hpp"
#include "config.hpp"
#include "meta.hpp"
#include "proof_of_work.hpp"
#include <list>

namespace detail
{
    class block_chain_impl
    {
       public:
         std::vector<meta_block_header> _chain;
         block                          _unconfirmed_head;
         unsigned char*                 _pow_buffer;
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
   
   // if there is no chain... generate the gensis block
   generate_gensis_block();
}


const block& block_chain::get_unconfirmed_head()
{
    return my->_unconfirmed_head;
}

void  block_chain::generate_gensis_block()
{
   // generate gensis block and add it

   block b;
   b.header.version.value         = 0;
   b.header.miner_version.value   = 0;
   b.header.timestamp             = fc::time_point::now();
   b.header.height                = my->_chain.size();
   b.header.prev_block            = pow_hash(); // null
   b.header.nonce                 = 0;
   b.header.dividends             = 0;

   b.header.dividends += get_reward_for_height(b.header.height) / 2;

   // generate coin-base transaction
/*
   transaction coinbase_trx;
   coinbase_trx.outputs.push_back( trx_output( b.header.dividends, unit() ) );
   coinbase_trx.outputs.back().set_claim_function( claim_with_address( a ) );
   b.bundled_trx.emplace_back(std::move(coinbase_trx));
*/
   my->_chain.push_back( meta_block_header() );
   my->_chain.back().header = b.header;
}


block  block_chain::generate_next_block( const address& a )
{
   block b;
   b.header.version.value         = 0;
   b.header.miner_version.value   = 0;
   b.header.timestamp             = fc::time_point::now();
   b.header.height                = my->_chain.size();
   b.header.prev_block            = my->_chain.back().id;
   b.header.nonce                 = 0;
   b.header.dividends             = 0;

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

void  block_chain::add_block( const block& b )
{
   // no matter what we need to know the hash of the block.
   auto pow     = proof_of_work( b.header, my->_pow_buffer ); 

   // is this a fork of an earlier point in the chain or another chain all togeher?
   if( b.header.height <  my->_chain.size() )
   {
       wlog( "not next in the chain..." );
       return;
   }

   if( b.header.height > my->_chain.size() )
   {
       wlog( "we appear to have missed something\n" );
       return;
   }



   if( 0 == my->_chain.size() )
   {
   }
   else
   {
      if( my->_chain.back().id != b.header.prev_block )
      {
         // TODO: un-hinged... perhaps part of a forked-chain
         //       
      }
      else
      {
         
          my->_chain.back().next_blocks.push_back(pow);
          my->_chain.push_back( meta_block_header() );
          my->_chain.back().id = pow;
          my->_chain.back().header = b.header;
      }
   }
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
      ilog( "height: ${h}  ${reward}", ("h", h)("reward",reward) );
         int num  = h > REWARD_ADJUSTMENT_INTERVAL ? REWARD_ADJUSTMENT_INTERVAL : h;
         reward -= num * (reward/2/REWARD_ADJUSTMENT_INTERVAL);
      ilog( "height: ${h}  ${reward}", ("h", h)("reward",reward) );
      h -= REWARD_ADJUSTMENT_INTERVAL;
   } while( h > 0 );
  ilog( "height: ${h}  ${reward}", ("h", h)("reward",reward) );

   return reward;
}
