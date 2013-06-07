#pragma once
#include "blockchain.hpp"

/**
 *  Tracks information / state about a transactions.
 */
class meta_transaction
{
    public:
      transaction             trx;       ///< the actual transaction 
      std::string             memo;      ///< user-specified memo regarding this trx
      std::string             error;     ///< any error message regarding this transaction
      int32_t                 block_num; ///< if this trx is in the valid chain, this will be set.
      std::vector<fc::sha256> blocks;    ///< all blocks that include this trx
};

FC_REFLECT( meta_transaction, (trx)(memo)(error)(block_num)(blocks) )


class meta_output_cache
{
   public:
      output_cache out;
      bool         spent;
      int32_t      block_num; ///< non 0 if included in a block
};

FC_REFLECT( meta_output_cache, (out)(block_num) )
