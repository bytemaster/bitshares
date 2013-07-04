#pragma once
#include <bts/dds/dividend_cache.hpp>
#include <bts/dds/transfer_output_cache.hpp>

namespace bts { namespace dds {
  
  namespace detail { class deterministic_data_store_impl; }

  /**
   *  This data structure stores the high-level index for the
   *  entire dds structure.
   */
  struct dds_index
  {
      uint32_t                block_number;
      std::vector<fc::sha224> dividend_table_hashes;
      std::vector<fc::sha224> by_address_hashes;
      //std::vector<fc::sha224> bid_hashes;
  };

  /**
   *  Encapsulates the state of the blockchain in a deterministic manner.  This data store 
   *  must provide a means to 'undo' all operations that are applied and should enable
   *  a new summary hash to be calculated as effeciently as possible by implementing a
   *  kind of merkel tree and tracking dirty segments and caching hashes of unmodified
   *  sections.
   *
   *  This datastore could potentially grow to over 10 GB and these files are all 
   *  memory mapped for effecient access. 
   *
   *  All data in this data_store must be divided into small enough chunks that they
   *  can be easily transferred within 5 minutes.
   *  
   */
  class deterministic_data_store
  {
     public:
       deterministic_data_store();
       ~deterministic_data_store();

       /**
        *  Loads data from the specified directory or creates the directory if it does
        *  not exist.
        */
       void load( const fc::path& data_store_dir );

       /**
        *  Syncs all memory-mapped files.
        */
       void sync();

       /**
        *  Calculates the combined hash of all dividend and transfer output caches
        */
       fc::sha224 calculate_hash()const

       dividend_cache&            get_dividend_cache( const unit_type& u );
       output_by_address_cache&   get_output_by_address_cache();

       //output_bid_cache&        get_output_bid_cache();
       //output_name_cache&       get_output_name_cache();
       //output_escrow_cache&     get_output_escrow_cache();

     private:
       std::unique_ptr<detail::deterministic_data_store_impl> my; 
  };

}} // namespace bts::dds
