#include <bts/bitname/name_block.hpp>
#include <fc/io/raw.hpp>

namespace bts { namespace bitname {
  mini_pow  name_reg_trx::id()const
  {
    auto d = fc::raw::pack(*this);
    return mini_pow_hash(d.data(),d.size());
  }


  mini_pow name_reg_block::calc_difficulty()const
  {
     fc::bigint total;
     for( auto itr = registered_names.begin(); itr != registered_names.end(); ++itr )
     {
        total += to_bigint( *itr );
     }
     total /= registered_names.size();
     return to_mini_pow( total );
  }

  mini_pow name_reg_block::calc_merkle_root()const
  {
     if( registered_names.size() == 0 ) return mini_pow();
     if( registered_names.size() == 1 ) return *registered_names.begin();

     std::vector<mini_pow> layer_one;
     for( auto itr = registered_names.begin(); itr != registered_names.end(); ++itr )
     {
       layer_one.push_back(*itr);
     }
     std::vector<mini_pow> layer_two;
     while( layer_one.size() > 1 )
     {
        if( layer_one.size() % 2 == 1 )
        {
          layer_one.push_back( mini_pow() );
        }

        static_assert( sizeof(mini_pow[2]) == 20, "validate there is no padding between array items" );
        for( uint32_t i = 0; i < layer_one.size(); i += 2 )
        {
            layer_two.push_back(  mini_pow_hash( layer_one[i].data, 2*sizeof(mini_pow) ) );
        }

        layer_one = std::move(layer_two);
     }
     return layer_one.front();
  }

} }
