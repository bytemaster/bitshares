#pragma once
#include <fc/reflect/reflect.hpp>
#include <fc/crypto/elliptic.hpp>
#include <string>



/**
 *  @brief encapsulates an encoded, checksumed public key in
 *  binary form.   It can be converted to base58 for display
 *  or input purposes and can also be constructed from an ecc 
 *  public key.
 *
 *  An valid address is 20 bytes with the following form:
 *
 *  First 3-bits are 0, followed by bits to 3-127 of sha256(pub_key), followed 
 *  a 32 bit checksum calculated as the first 32 bits of the sha256 of
 *  the first 128 bits of the address.
 *
 *  The result of this is an address that can be 'verified' by
 *  looking at the first character (base58) and has a built-in
 *  checksum to prevent mixing up characters.
 *
 *  It is stored as 20 bytes.
 */
struct address
{
    address(); ///< constructs empty / null address
    address( const std::string& base58str);         ///< converts to binary, validates checksum
    address( const fc::ecc::public_key& pub ); ///< converts to binary

    bool is_valid()const;

    operator std::string()const; ///< converts to base58 + checksum

    fc::array<char,20> addr;      ///< binary representation of address
};
FC_REFLECT( address, (addr) )


inline bool operator == ( const address& a, const address& b ) { return a.addr == b.addr; }
inline bool operator != ( const address& a, const address& b ) { return a.addr != b.addr; }
