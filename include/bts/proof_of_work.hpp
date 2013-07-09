#pragma once
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/sha1.hpp>

/** typedef to the same size */
typedef fc::sha1 pow_hash;

/**
 *  The purpose of this method is to generate a determinstic proof-of-work
 *  that cannot be optimized via ASIC or extreme parallelism. 
 *
 *  @param in           - initial hash
 *  @param buffer_128m  - 128 MB buffer used for scratch space.
 *  @return processed hash after doing proof of work.
 */
pow_hash proof_of_work( const fc::sha256& in, unsigned char* buffer_128m );
pow_hash proof_of_work( const fc::sha256& in );

class block_header;

/**
 *  Calculates the sha256 of the block_header, then calls proof_of_work with
 *  the result.
 */
pow_hash proof_of_work( const block_header& h, unsigned char* buffer_128m );

