#include "proof_of_work.hpp"
#include <fc/crypto/sha512.hpp>
#include <fc/crypto/blowfish.hpp>
#include <fc/thread/thread.hpp>
#include <string.h>
#include <city.h>

#include <fc/io/raw.hpp>
#include "blockchain.hpp"
#include <utility>
#include <boost/random/mersenne_twister.hpp>

#define MB128 (128*1024*1024)

typedef std::pair<uint64, uint64> uint128;
uint128 CityHashCrc128(const char *s, size_t len);

/**
 *  This proof-of-work is computationally difficult even for a single hash,
 *  but must be so to prevent optimizations to the required memory foot print.
 *
 *  The maximum level of parallelism achievable per GB of RAM is 8, and the highest
 *  end GPUs now have 4 GB of ram which means they could in theory support 32 
 *  parallel execution of this proof-of-work.     
 *
 *  On GPU's you only tend to get 1 instruction per 4 clock cycles in a single
 *  thread context.   Modern super-scalar CPU's can get more than 1 instruction
 *  per block and CityHash is specifically optomized to take advantage of this. 
 *  In addition to getting more done per-cycle, CPU's have close to 4x the clock
 *  frequency.
 *
 *  Based upon these characteristics alone, I estimate that a CPU can execute the
 *  serial portions of this algorithm at least 16x faster than a GPU which means
 *  that an 8-core CPU should easily compete with a 128 core GPU. Fortunately,
 *  a 128 core GPU would require 16 GB of RAM.  Note also, that most GPUs have 
 *  less than 128 'real' cores that are able to handle conditionals. 
 *
 *  Further more, GPU's are not well suited for branch misprediction and code
 *  must be optimized to avoid branches as much as possible.  
 *
 *  Lastly this algorithm takes advantage of a hardware instruction that is
 *  unlikely to be included in GPUs (Intel CRC32).  The lack of this hardware
 *  instruction alone is likely to give the CPU an order of magnitude advantage
 *  over the GPUs.
 */
fc::sha1 proof_of_work( const fc::sha256& in, unsigned char* buffer_128m )
{
   // I use 8 threads to reduce latency in the calculation, it may be possible to
   // get higher-throughput by going to a single thread and performing multiple
   // proofs in parallel.   Latency is important in some cases (verification) while
   // throughput is important in mining.  Two variations of this method may eventually
   // need to be created.

   static fc::thread _threads[8]; 
   uint32_t* _seeds = (uint32_t*)&in;

   fc::future<void> ready[8];
   for( uint32_t i = 0; i < 8; ++i )
   {
     ready[i] = _threads[i].async( [=]()
     {
       boost::random::mt11213b gen( _seeds[i] ); // this generator does not optimize well on GPU, but is fast on CPU
       uint32_t* start = (uint32_t*)(buffer_128m + i * MB128/8);
       uint32_t* end = start + MB128/8/4;
       for( uint32_t* pos = start;  pos < end; ++pos )
       {
          *pos = gen();
          if( *pos % 17 > 15 ) // hardware branch misprediction, foils GPU data-parallel instructions
          {
             if( pos > (start +32/4) )
                 *pos =  CityHashCrc128( (char*)(pos-32/4), 32 ).first; // CRC is fast on Intel
          }
          else if ( *pos > gen() )  // unpredictiable branch foils GPU and CPU
          {
             if( (*pos % 2) && pos > (start +256/4) )
                 *pos =  CityHashCrc128( (char*)(pos-256/4), 32 ).first; // CRC is fast on Intel
          }
       }
     });
   }
   for( uint32_t i = 0; i < 8; ++i )
   {
    ready[i].wait();
   }

   uint64_t* buf = (uint64_t*)buffer_128m;
   const uint64_t  s = MB128/sizeof(uint64_t);
   uint64_t data = (buf+s)[-1];
   for( uint32_t x = 0; x < 128; ++x )
   {
      uint64_t d = data%s;
      uint64_t tmp = data ^ buf[d];
      std::swap( buf[tmp%s], buf[d] );
      data = tmp * (x+17);
   }
   // require full 128 MB to complete sequential step
   auto     out  = CityHashCrc128( (char*)buffer_128m, MB128 ); 
   return fc::sha1::hash( (char*)&out, sizeof(out) );
}


fc::sha1 proof_of_work( const block_header& h, unsigned char* buffer_128m )
{
    auto data = fc::raw::pack(h);
    return proof_of_work( fc::sha256::hash( data.data(), data.size() ), buffer_128m );
}

