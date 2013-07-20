#pragma once
#include <fc/crypto/elliptic.hpp>
#include <fc/optional.hpp>
#include <bts/mini_pow.hpp>

namespace bts { namespace bitname {

    /**
     *  Every name registration requires its own proof of work, this proof of work
     *  is designed to take about 1 hour on a home PC.  The process of mining
     *  your own name is also performing merged mining on the block that contains
     *  the names of other individuals.  
     *
     *  The difficulty of mining your own name is about 5000x less difficult than
     *  finding the full block with the caviot being that there is a minimum
     *  mining requirement.
     */
    struct name_reg_trx 
    {
       name_reg_trx()
       :nonce(0),utc_sec(0){}

       mini_pow id()const;

       uint32_t                                 nonce;   ///< increment to find proof of work
       uint32_t                                 utc_sec; ///< utc seconds
       mini_pow                                 prev;    ///< previous block
       mini_pow                                 mroot;   ///< mroot of all trx
       uint16_t                                 renewal; ///< how many times has this name been renewed.
       std::string                              name;    ///< name to reserve
       fc::ecc::public_key                      key;     ///< key to assign to name
       fc::optional<fc::ecc::compact_signature> oldsig;  ///< signature of old key
    };

    
    struct name_reg_block : public name_reg_trx
    {
        mini_pow            calc_merkle_root()const; 
        /**
         *  Assuming a hash value on a scale from 1000 to 0, the
         *  probability of finding a hash below 100 = 10% and the difficulty level would be 1000/100 = 10.
         *
         *  The probability of finding 4 below 100 would be equal to the probability of finding 1 below 25.
         *
         *  The result is that we can calculate the average difficulty of a set and divide by the number of
         *  items in that set to calculate the combined difficulty.  
         */
        mini_pow            calc_difficulty()const;
        std::set<mini_pow>  registered_names;
    };


} } // namespace bts::bitname

#include <fc/reflect/reflect.hpp>
FC_REFLECT( bts::bitname::name_reg_trx, 
    (prev)
    (mroot)
    (nonce)
    (utc_sec)
    (renewal)
    (name)
    (key)
    (oldsig)
)

FC_REFLECT( bts::bitname::name_reg_block,  
    (registered_names) 
)
