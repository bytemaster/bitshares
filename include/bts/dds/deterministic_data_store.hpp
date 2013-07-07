#pragma once
//#include <bts/dds/dividend_table.hpp>
#include <bts/dds/output_by_address_table.hpp>

namespace bts { namespace dds {
  
  namespace detail { class deterministic_data_store_impl; }

  /**
   *  This data structure stores the high-level index for the
   *  entire dds structure.
   */
  struct dds_index
  {
      dds_index():block_number(0){}

      uint32_t                block_number;
      std::vector<fc::sha224> dividend_table_hashes;
      table_header            by_address_header;
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
       void load( const fc::path& data_store_dir, bool create = true );

       /**
        *  Syncs all memory-mapped files.
        */
       void save();

       /**
        *  Calculates the combined hash of all dividend and transfer output tables
        */
       fc::sha224 calculate_hash()const;

       const dds_index&           get_index()const;

       //dividend_table&            get_dividend_table( const unit_type& u );
       output_by_address_table&   get_output_by_address_table();

       //output_bid_table&        get_output_bid_table();
       //output_escrow_table&     get_output_escrow_table();

     private:
       std::unique_ptr<detail::deterministic_data_store_impl> my; 
  };

}} // namespace bts::dds

FC_REFLECT( bts::dds::dds_index, (block_number)(dividend_table_hashes)(by_address_header) )
