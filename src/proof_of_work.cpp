#include "proof_of_work.hpp"
#include <fc/crypto/sha512.hpp>
#include <fc/crypto/blowfish.hpp>
#include <string.h>

#include <fc/io/raw.hpp>
#include "blockchain.hpp"

#define MB128 (128*1024*1024)

/**
 *  This proof-of-work is computationally difficult even for a single test,
 *  but must be so to prevent optimizations to the required memory foot print.
 *
 *  This algorithm must randomly access every byte in the memory buffer to
 *  prevent optimizations that 'cache' only the 'dirty' parts.  It must
 *  do everything in a sequential form that prevents lower-level parallelization.
 */
fc::sha1 proof_of_work( const fc::sha256& in, unsigned char* buffer_128m )
{
   return fc::sha1::hash( (char*)&in, sizeof(in) ); // for dev purposes only

   memset( buffer_128m, 0, MB128 );
   fc::blowfish bf; 
   bf.start( (unsigned char*)&in, sizeof(in) );

   uint64_t pos = in.data()[0] % (MB128-2*sizeof(fc::sha512));

   fc::sha512 init = fc::sha512::hash( (const char*)&in, sizeof(in) );
   
   bf.encrypt( (unsigned char*)&init, buffer_128m + pos, sizeof(init) );

   /** 
    *  For 1M iterations, randomly write 64 bytes overlaping with
    *  existing bytes.  
    */
   for( int i = 0; i < 1024 * 1024; ++i )
   {
      uint64_t npos = pos + sizeof(fc::sha512)/2;

      // randomly jump to a new spot in the buffer based upon
      // our current position
      uint64_t nposa = ((uint64_t*)(buffer_128m+pos))[1] % (MB128 - 2*sizeof(fc::sha512));

      if( nposa % 17 < 8 ) // prevent branch prediction
         nposa = ((uint64_t*)(buffer_128m+pos))[2] % (MB128-2*sizeof(fc::sha512));
      else if( nposa % 38 < 19 ) // prevent branch prediction
         nposa = ((uint64_t*)(buffer_128m+pos))[3] % (MB128-2*sizeof(fc::sha512));
      else if( nposa % 63 < 32 ) // prevent branch prediction
         nposa = ((uint64_t*)(buffer_128m+pos))[4] % (MB128-2*sizeof(fc::sha512));
      

      //  encrypt the data in the buffer to a new location in the buffer.
      bf.encrypt( buffer_128m + npos, buffer_128m + nposa, 2*sizeof(fc::sha512), fc::blowfish::CBC ); 

      pos = nposa;
   }

   // the entire contents of the buffer are required for the result.
   return fc::sha1::hash( (char*)buffer_128m, MB128 );
}


fc::sha1 proof_of_work( const block_header& h, unsigned char* buffer_128m )
{
    auto data = fc::raw::pack(h);
    return proof_of_work( fc::sha256::hash( data.data(), data.size() ), buffer_128m );
}

