#include <fc/crypto/sha512.hpp>
#include <fc/crypto/bigint.hpp>
#include <bts/mini_pow.hpp>
#include <algorithm>
#include <fc/log/logger.hpp>

namespace bts {

  mini_pow mini_pow_hash( const char* data, size_t len )
  {
      auto h1 = fc::sha512::hash( data, len );
      auto h2 = fc::sha512::hash( (char*)&h1, sizeof(h1) );
      mini_pow p;
      fc::bigint  h3( (char*)&h2, sizeof(h2) );
      int64_t lz = 512 - h3.log2();
      h3 <<= lz;
      std::vector<char> bige = h3;
      bige[0] = 255-lz;
      memcpy( p.data, bige.data(), sizeof(p) );
      return p;
  }

}
