#pragma once
#include <bts/blockchain/transaction.hpp>
#include <bts/dds/table_header.hpp>
#include <fc/filesystem.hpp>

namespace bts
{
  namespace detail { class output_by_address_table_impl; }

  /**
   *   2 MB blocks, each with a dirty bit to identify when re-hashing is necessary
   *   Maintain a free list stored in the last block.
   *
   *   Maintain's an index to allow quick lookup bout output_reference, this index
   *   does not need to be deterministic nor stored, it can be regenerated at any
   *   time.
   *
   *   Maintain's an index of outputs sorted by block number so that they may be
   *   quickly captured by the miners and pulled forward.  This index takes the form
   *   of around 100K unordered sets of entry id for each block.
   *
   *   1 million unspent outputs would require about 100 MB on disk.
   */
  class output_by_address_table
  {
     public:
        /**
         *  A row in the table.
         */
        struct entry
        {
            entry();
            /** The transaction id / output number in the trx */
            output_reference       out_id;
            /** The block that included the output referenced by out_id */
            uint32_t               block_num;
            /** The output from the specified transaction */
            trx_output_by_address  output;

            friend bool operator==( const entry& a, const entry& b )
            {
               return (a.out_id == b.out_id) && (a.block_num == b.block_num) && (a.output == b.output);
            }
        };
        output_by_address_table();
        ~output_by_address_table();

        /**
         *  Loads or creates the table in table_dir
         *
         *  @throw if create is false and files are not found
         */
        void load( const fc::path& table_dir, bool create = false );

        /**
         *  Sync's the table to disk.
         */
        void save();

        /**
         *  Updates the table header and returns hash.  This is a convienence method
         *  for fc::sha224::hash( get_header() );
         */
        fc::sha224 calculate_hash()const;

        /**
         *  Returns the current state of the output header.  Note there is no
         *  method to set the header because the header should always faithfully
         *  reflect the contents of the table.
         */
        const table_header&     get_header()const;

        /**
         *  Returns the raw contents of the given table chunk.
         *
         *  @throw if chunk_num is invalid.
         */
        std::vector<char>       get_chunk( uint32_t chunk_num )const;

        /**
         *  Used when downloading the table in parts to overwrite the contents of a particular chunk.
         */
        void                    set_chunk( uint32_t chunk_num, const std::vector<char>& );

        /**
         *  @return the set of unspent outputs for a particular block.
         */
        std::vector<uint32_t> get_unspent_by_block( uint32_t block_number );

        /**
         *  Stores entry e in the table and returns the index it was
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
        std::unique_ptr<detail::output_by_address_table_impl> my;
  };
  

} // namespace bts

FC_REFLECT( bts::output_by_address_table::entry, (out_id)(block_num)(output) )
