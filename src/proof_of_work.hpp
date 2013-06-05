#pragma once
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/sha1.hpp>

/**
 *  The purpose of this method is to generate a determinstic proof-of-work
 *  that cannot be optimized via ASIC or extreme parallelism. 
 *
 *  @param in           - initial hash
 *  @param buffer_128m  - 128 MB buffer used for scratch space.
 *  @return processed hash after doing proof of work.
 */
fc::sha1 proof_of_work( const fc::sha256& in, unsigned char* buffer_128m );
