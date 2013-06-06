#pragma once
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/io/varint.hpp>
#include <fc/io/raw.hpp>
#include <fc/optional.hpp>
#include <vector>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/time.hpp>
#include "address.hpp"


/** @file blockchain.hpp
 *
 * Defines the structure of the block-chain and related data without
 * without respect to any caching or meta-data.  These structs effectively
 * define the serialization of the block-chain.
 */


/**
 *  Tracks the ratio of BitShares to Issuance for backing purposes.
 */
struct share_state
{
    uint64_t share_id;
    uint64_t total_issue;
    uint64_t total_backing;
};
FC_REFLECT( share_state, (share_id)(total_issue)(total_backing) )


/**
 *  This is the minimal information that must be maintained 'forever' 
 *  to serve as proof of work for the longest chain.  It requires
 *  some extra information not required by Bitcoin, such as
 *  share_changes, dividends, and 2 nonce values.  
 *
 *  Share changes need to be kept with the block header so that dividends
 *  may be properly calculated over time.   Two nonce values are required,
 *  one that is 'quick to check' and one that takes a few seconds to check.
 */
struct block_header
{
   fc::unsigned_int           version;       // compatability version, the version required by the next block which is updated anytime miner_version of the last 10,000 blocks hits 90% 
   fc::unsigned_int           miner_version; // version supported by the miner, as miners upgrade they specify their version info here
   fc::time_point             timestamp;     // UTC seconds, must always increase from one block to the next, blocks are rejected if timestamp is more than 5 minutes in the future.
   uint32_t                   height;        // position in the block chain... 
   uint64_t                   nonce;         // expensive to check, proof of work 
   fc::sha256                 prev_block;    // hash of previous block.
   fc::sha256                 block_state;   // used to 'capture' the status of the block after procesing trx
   uint64_t                   dividends;     // total dividends paid for this block (BitShares)
   std::vector<share_state>   share_changes; // required to calculate dividends paid over time.     
};

FC_REFLECT( block_header, 
            (version)
            (miner_version)
            (timestamp)
            (height)
            (nonce)
            (prev_block)
            (block_state)
            (dividends)
            (share_changes) )


/**
 *  All balanes are annotated with a unit.
 */
struct unit
{
    uint32_t id;
    /**
     * Percent minimal margin before, covering, initial margin requirement is 2x this value.
     *
     * Min value is 25%, anything less is too risky.
     */
    uint16_t min_margin;   
};

FC_REFLECT( unit, (id)(min_margin) ) 

/**
 * A claim function is identified by a unique ID
 * and each claim function captures some context
 * (such as an address or exchange rate) and then
 * evaluates TRUE or FALSE.
 *
 * In many ways claim_functions operate like bitcoin
 * SCRIPTS but are limited to a DEFINED set of
 * inputs and outputs.  New functions can be added
 * in the future and a 'claim_script' function could
 * also be added to support 'generic' user-defined
 * methods, however opting to use a defined subset
 * is more secure.  
 */
struct claim_function
{
    fc::unsigned_int id;
};
FC_REFLECT( claim_function, (id) )

/**
 *   This claim function will allow the output to be
 *   claimed if the transaction is signed with addr
 */
struct claim_with_address : public claim_function
{
    enum { type = 0 }; // normal

    struct input
    {
       fc::ecc::compact_signature sig;
    };

    claim_with_address(){}
    claim_with_address( const address& a )
    :addr(a){}

    address  addr;
};

/**
 *  This claim function is identical to claim_with_address
 *  but also flags the output as a coinbase and therefore
 *  requires that the output has 
 */
struct claim_coinbase : public claim_function
{
    enum { type = 1 }; // coinbase

    struct input
    {
       fc::ecc::compact_signature sig;
    };
    address  addr;
};

FC_REFLECT_DERIVED( claim_with_address, (claim_function), (addr) )
FC_REFLECT( claim_with_address::input, (sig) )


/**
 *  There are 3 ways to claim this output:
 *    - the owner of addr can spend it.
 *    - the output can be consumed as part of a transaction that
 *         sends at least minimum exchange_unit at exchange_rate 
 *         to addr and sends any change to an output with identical
 *         terms as this output.
 *    - the output can be consumed as part of a short-sell issuance
 */
struct claim_with_exchange : public claim_function
{
    enum { type = 2 };

    /**
     *   Defines the information that must be provided by the
     *   transaction input to claim an output with this claim 
     *   function.
     */
    struct input
    {
       enum exchange_types
       {
            cancel          = 0,
            sell            = 1,
            short_sell      = 2
       };
       uint64_t amount;        /// amount in the exchange_unit to sell, short, or cancel
       uint8_t  exchange_type; /// 0, to cancel, 1 to sell, and 2 to short.
       fc::optional<fc::ecc::compact_signature> cancel_sig;
    };

    address              addr;          // address to send to or used to cancel.
    unit                 exchange_unit; // unit to exchange
    uint64_t             exchange_rate; // price in BS per unit of exchange_unit
    uint64_t             minimum;       // the minimum units to exchange           
};

FC_REFLECT_DERIVED( claim_with_exchange, (claim_function), (addr)(exchange_unit)(exchange_rate)(minimum) )
FC_REFLECT( claim_with_exchange::input, (amount)(exchange_type)(cancel_sig) )
FC_REFLECT_ENUM( claim_with_exchange::input::exchange_types, (cancel)(sell)(short_sell) )


/**
 *  The output of a transaction that specifies the unit and amount.
 *  To claim this output, a transaction must provide the claim_function
 *  with the proper data.  Claim-data can be thought of as context 
 *  information required by the claim_function.
 */
struct trx_output
{
    trx_output(int64_t a=0, unit u = unit())
    :amount(a),amount_unit(u){}

    template<typename T>
    void set_claim_function( const T& f )
    {
        claim_function = T::type;
        claim_data     = fc::raw::pack( f );
    }

    int64_t           amount; // a negative amount equals 'margin' 
    unit              amount_unit;
    fc::unsigned_int  claim_function;
    std::vector<char> claim_data;
};

FC_REFLECT( trx_output, (amount)(amount_unit)(claim_function)(claim_data) )


/**
 *  Defines the parameters for the input to a transaction.
 */
struct trx_input
{
    /**
     *   Hash of the transaction that included the output.
     */
    fc::sha256          source_trx;

    /**
     *  The output index in the source trx.
     */
    fc::unsigned_int    source_trx_output;

    /**
     *  Data passed to the claim_function to release the
     *  value to this transaction.
     */
    std::vector<char>   claim_input;
};
FC_REFLECT( trx_input, (source_trx)(source_trx_output)(claim_input) )


/**
 *  Transactions take inputs from one or more
 *  sources and redistribute the outputs to
 *  one or more destinations.
 *
 *  Any surplus BitShares are counted as fees, otherwise the
 *  input and output values must match for other currencies.
 */
struct transaction
{
   transaction():expires(0){}
   fc::unsigned_int          type;
   uint32_t                  expires; // block number
   std::vector<trx_input>    inputs;
   std::vector<trx_output>   outputs;
};

FC_REFLECT( transaction, (type)(expires)(inputs)(outputs) )

/**
 *  Used to cache the output without requiring the entire transaction.
 */
struct output_cache
{
    fc::sha256        tranasction_id;
    fc::unsigned_int  output_index;
    trx_output        output_state;
};
FC_REFLECT( output_cache, (tranasction_id)(output_index)(output_state) )


/**
 *  Captures the state of the of a particular 'block', used 
 *  to calculate teh block_state hash included in the block
 *  header.
 */
struct block_state
{
    uint32_t                                  version;
    std::vector<transaction>                  new_transactions;
    std::vector< fc::optional<output_cache> > outputs; // status of all unspent outputs after applying transactions
    std::vector< fc::optional<share_state> >  shares;  // status of all shares after applying transactions
};

FC_REFLECT( block_state, (version)(new_transactions)(outputs)(shares) )

struct block
{
    block_header               header;
    std::vector<transaction>   bundled_trx;    // used for trx that were not broadcast prior to block generation (bids,coinbase,etc)
    std::vector<fc::sha256>    referenced_trx; // used for trx that were referenced.
};

FC_REFLECT( block, (header)(bundled_trx)(referenced_trx) )


