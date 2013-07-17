#include <fc/crypto/sha512.hpp>
#include <fc/crypto/bigint.hpp>
#include <bts/mini_pow.hpp>
#include <algorithm>

namespace bts {

  mini_pow mini_pow_hash( const char* data, size_t len )
  {
      auto h1 = fc::sha512::hash( data, len );
      auto h2 = fc::sha512::hash( (char*)&h1, sizeof(h1) );
      fc::bigint  h3( (char*)&h2, sizeof(h2) );
      int64_t lz = h3.log2();

      int min_shift = std::max<size_t>( 8, lz - 8 );
      h3 <<= min_shift;
      mini_pow p;
      p.data[0] = lz;
      return p;
  }

}
