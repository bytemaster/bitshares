#pragma once
#include <bts/blockchain/transaction.hpp>
#include <fc/filesystem.hpp>

namespace bts
{
  namespace detail { class output_by_address_cache_impl; }

  /**
   *   2 MB blocks, each with a dirty bit to identify when re-hashing is necessary
   *   Maintain a free list stored in the last block.
   *
   *   Maintain's an index to allow quick lookup bout output_reference, this index
   *   does not need to be deterministic.
   *
   *   Maintain's an index of outputs sorted by block number so that they may be
   *   quickly captured by the miners and pulled forward.  This index takes the form
   *   of around 100K unordered sets of entry id for each block.
   *
   *   1 million unspent outputs would require about 100 MB on disk.
   */
  class output_by_address_cache
  {
     public:
        struct entry
        {
            entry();
            output_reference       out_id;
            uint32_t               block_num;
            trx_output_by_address  output;
        };
        output_by_address_cache();
        ~output_by_address_cache();

        void load( const fc::path& cache_dir );

        fc::sha224 calculate_hash()const;

        uint32_t                get_num_chunks()const;
        std::vector<char>       get_chunk( uint32_t chunk_num )const;
        std::vector<fc::sha224> get_chunk_hashes()const;

        /**
         *  @return the set of unspent outputs for a particular block.
         */
        std::vector<uint32_t> get_unspent_by_block( uint32_t block_number );


        /**
         *  Sync's the cache to disk.
         */
        void sync();

        /**
         *  Stores entry e in the cache and returns the index it was
         *  stored at.
         */
        void       store( uint32_t pos, const entry& e );
        uint32_t   get_free_slot();

        /**
         *  Releases the specified index and places it in the 
         *  free-list.
         */
        void           clear( uint32_t index );

        /**
         *  Finds the output and returns its index, throws if the
         *  output was not found.
         */
        uint32_t       find( const output_reference& out );
        const entry&   get( uint32_t entry_id );

     private:
        std::unique_ptr<detail::output_by_address_cache_impl> my;
  };
  

} // namespace bts

FC_REFLECT( bts::output_by_address_cache::entry, (out_id)(block_num)(output) )
