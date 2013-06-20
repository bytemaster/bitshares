#pragma once

/**
 *  All balanes are annotated with a unit.
 */
enum unit_types 
{
    BitShares   = 0,
    OneOzGold   = 1,
    OneOzSilver = 2,
    USD         = 3,
    EUR         = 4,
    YUAN        = 5,
    BitCoin     = 6,
    LiteCoin    = 7,
    NumUnits    
};
typedef uint8_t unit;


/**
 *  The proof-of-work hash is 160 bits, just like sha1 but it is 
 *  ultimately *NOT* a sha1 hash and thus not subject to the
 *  same attacks as sha1
 */
typedef fc::sha1 pow_hash;



struct trx_input
{
    uint64_t amount;
    unit     type;  
    address  from;
};


struct transfer
{
    std::vector<trx_input>  in;  
    std::vector<trx_output> out;  
    uint32_t                sign_block;
    uint32_t                expire_block;
};

struct signed_transfer : public transfer
{
    std::vector<fc::ecc::compact_signature> sigs;
};

struct  bid
{
    uint                    sell_type;         // what we are selling
    uint                    buy_type;          // what we are buying
    uint64_t                min_buy;           // the minimum amount of buy type to transact
    uint64_t                rate;              // the number of 'buy' units per 'sell' unit
    std::vector<trx_input>  in;                // value in (max two types)
    address                 out;               // send funds to this address

    uint64_t                fee;               // amount to pay the miner.
    uint32_t                sign_block;        // bid is valid starting on this block
    uint32_t                expire_block;      // bid expires starting on this block
};

struct signed_bid : bid
{
    std::vector<fc::ecc::compact_signature> sigs;
};


/**
 *  Tracks the ratio of BitShares to Issuance for backing purposes.
 */
struct share_state
{
    uint64_t total_issue;
    uint64_t total_backing;
};
FC_REFLECT( share_state, (share_id)(total_issue)(total_backing) )


/**
 *  Block headers must be kept for 1 year after which one header can
 *  be dropped for each new block added.  The block header references
 *  the hash of an 'initial condition' that must be known in order 
 *  to build new blocks from this header.  Clients only need to
 *  store transactions for enough blocks that they can be confident
 *  in moving to the new initial condition.
 *
 *  In order to ensure that all data can be discarded after one year,
 *  all unspent outputs pay a 5% tax or minimum transaction fee if
 *  they are over 1 year old and also forfeit all dividends earned.
 *
 *  In this way the network is not forced to store outputs from
 *  private keys and 'dust' forever.
 */
struct block_header
{
   fc::unsigned_int                 version;       // compatability version, the version required by the next block which is updated anytime miner_version of the last 10,000 blocks hits 90% 
   fc::time_point                   timestamp;     // UTC seconds, must always increase from one block to the next, blocks are rejected if timestamp is more than 5 minutes in the future.
   uint32_t                         height;        // position in the block chain... 
   pow_hash                         prev_block;    // hash of previous block.
   fc::sha224                       block_state;   // used to 'capture' the status of the block after procesing trx
};

struct trx_output
{
    uint64_t       amount;
    unit           type;
    address        to;
    uint8_t        claim_func;
    array<char,69> claim_data;
};


/**
 *  Hash of all unspent outputs, dividend balances, 
 */
struct block_state
{
   std::vector<trx_output>   unspent;         // 50 bytes * 100 M = 5 GB 
   std::vector<uint64_t>     dividend_table;  // 8 units * 8 backings * 8 bytes * blocks/year =  51 MB
   std::vector<share_state> 

};


struct block_proof
{
   block_header  header;
   proof         header_proof;
};

struct pow_header
{
   uint64_t     nonce;
   pow_hash     prev_pow;
   fc::sha224   mroot;
};

struct pow_block
{
   pow_header              header;
   std::vector<fc::sha224> mtree;
};


/**
 *  A chain of hashes that ends with the hash of a
 *  block header along with the nonce that when
 *  combined with the head of the chain results in
 *  a hash that can satisify the proof of work.
 */
class proof
{
    public:
    uint64_t                  nonce;
    std::vector<fc::sha224>   merkchain; 
};

/**
 *  The proof chain is kept around forever to
 *  establish the longest most difficult history
 *  while discarding everything else that is no
 *  longer required.  Because the blockchain is
 *  a 'rolling state' and never needs to maintain
 *  more than 1 year of history we want to 
 */
class proof_chain
{
   proof       work;
   fc::sha224  prev;
};







