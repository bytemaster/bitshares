#pragma once
#include <bts/merkle_tree.hpp>
#include <fc/filesystem.hpp>
#include <memory>


namespace bts {

   namespace detail { class disk_merkle_tree_impl; }


   /**
    *  Maintains an on-disk merkle tree that supports updates
    *  to individual indexes in an array without having to 
    *  recalculate the entire merkle tree.
    *
    *  This class is designed with the assumption of over
    *  10+ million records which would use 2.8GB for the leaf
    *  level and therefore would not fit in RAM easily.  
    *
    *  The each level of the tree will be stored on disk as
    *  a memory mapped array.  
    */
   class disk_merkle_tree
   {
      public:
        disk_merkle_tree();
        ~disk_merkle_tree();

        /**
         *  Loads the data directory containing the files used
         *  by this merkle tree.
         */
        void     load( const fc::path& dir, bool create = false );
        void     save();

        void     resize( uint64_t s );
        uint64_t size()const;

       /** 
        *  Updates all hashes in the merkle branch to index.
        *  @throw out_of_range_exception if index > size
        */
       void       set( uint64_t index, const fc::sha224& val );

       /**
        *  @return a single level in the merkle tree
        *
        *  Level 0 is the leaf layer
        *  Level 1 is half the size of the leaf layer
        *  Level 2 is half level 1... etc
        */
       std::vector<fc::sha224> get_tree_level( uint64_t level );

       /**
        *  @throw out_of_range_exception if index > size
        */
       fc::sha224 get( uint32_t index );

       /**
        *  @return the full merkle branch for the tree.
        */
       merkle_branch get_branch( uint32_t index )const;

       fc::sha224 get_root()const;

      private:
        std::unique_ptr<detail::disk_merkle_tree_impl> my;
   };

} // bts
