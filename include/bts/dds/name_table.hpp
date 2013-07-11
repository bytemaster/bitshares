#pragma once

namespace bts { 

   typedef fc::array<char,64> name_string; 

   struct name_record
   {
       enum sale_mode 
       {
          not_for_sale = -1,  //< the ask price is -1 / infinate 
          auction      = -2,  //< the ask price is the reserve price
          make_offer   = -3   //< make an offer, no promise of sale
       };
       name_string          name;
       fc::ecc::public_key  current_owner;
       fc::time_point       first_registered;   // first time this name was registered          
       fc::time_point       last_key_update;    // update key without changing owner
       fc::time_point       last_owner_change;  // update key while changing owner
       uint64_t             last_price;         // last price paid for name
       int64_t              ask_price;          // is the name for sale, what price
       sale_mode            sale_type;          
   };

   class name_table 
   {
      public:
        void load( const fc::path& table_dir, bool create = false );
        void save();

        fc::sha224              get_merkel_root()const;
        merkel_branch           get_branch( const name_string& n );

        /**
         *  @return all records between start & end 
         */
        std::vector<fc::sha224> get_index( uint64_t start, uint64_t end );

        /**
         *  @return the record for name s
         *  @throw if there is no record for s.
         */
        name_record             get_record( const name_string& s );
        name_record             set_record( const name_string& s );

   };

} 
