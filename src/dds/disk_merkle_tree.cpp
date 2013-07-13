#include <bts/dds/disk_merkle_tree.hpp>
#include <bts/dds/mmap_array.hpp>
#include <fstream>

namespace bts 
{
   typedef bts::mmap_array<fc::sha224> mmap_sha224_array;
   typedef std::shared_ptr<mmap_sha224_array> mmap_sha224_array_ptr;
   namespace detail 
   {
      class disk_merkle_tree_impl
      {
        public:
          std::vector<mmap_sha224_array_ptr> layers;
      };
   } // namespace detail

   disk_merkle_tree::disk_merkle_tree()
   :my( new detail::disk_merkle_tree_impl() )
   {
   }

   disk_merkle_tree::~disk_merkle_tree()
   {}


   void      disk_merkle_tree::load( const fc::path& dir, bool create )
   {
      if( !fc::exists( dir ) )
      {
        if( create ) 
        {
           fc::create_directories(dir);
        }
        else
        {
           FC_THROW_EXCEPTION( file_not_found_exception, "directory '${dir}' does not exist", ("dir",dir) );
        }
      }

      // TODO: load the index file
      // TODO: map the memory segments
   }

   void      disk_merkle_tree::save()
   {
      // TODO: flush the mapped memory to disk
   }

   void      disk_merkle_tree::resize( uint64_t s )
   {
      // TODO: unmap all memory segments
      //       grow each layer in the tree as necessary
   }

   uint64_t  disk_merkle_tree::size()const
   {
      FC_ASSERT( my->layers.size() );
      return my->layers[0]->size();
   }

   void       disk_merkle_tree::set( uint64_t index, const fc::sha224& val )
   {
      try
      {
         FC_ASSERT( index < size() );
         my->layers[0]->at(index) = val;
         if( size() == 1 ) return;
         
         size_t layer = 1;
         while( index > 0 && layer < my->layers.size() )
         {
            uint32_t layer_index = index / 2;
            fc::sha224::encoder enc;
         
            if( index % 1 == 0 )
            {
                enc.write( (char*)&my->layers[layer-1]->at(index), sizeof(fc::sha224) );
                if( index == size() - 1 )
                {
                   fc::sha224 empty;
                   enc.write( (char*)&empty, sizeof(fc::sha224) );
                }
                else
                {
                   enc.write( (char*)&my->layers[layer-1]->at(index+1), sizeof(fc::sha224) );
                }
            }
            else 
            {
               enc.write( (char*)&my->layers[layer-1]->at(index+1), sizeof(fc::sha224) );
               enc.write( (char*)&my->layers[layer-1]->at(index), sizeof(fc::sha224) );
            }
            my->layers[layer]->at(layer_index) = enc.result();
            index = layer_index;
            ++layer;
         }
      } FC_RETHROW_EXCEPTIONS( warn, "Unable to set index ${i} to ${h}", ("i",index)("h",val) );
   }

   std::vector<fc::sha224> disk_merkle_tree::get_tree_level( uint64_t level )
   {
      FC_ASSERT( level < my->layers.size() );

      std::vector<fc::sha224> l;
      fc::sha224* b = &my->layers[level]->at(0);
      fc::sha224* e = b + my->layers[level]->size();
      l.insert( l.begin(), b, e );
      return l;
   }

   fc::sha224    disk_merkle_tree::get( uint32_t index )
   {
      FC_ASSERT( index < size() );
      return my->layers[0]->at(index);
   }

   merkle_branch disk_merkle_tree::get_branch( uint32_t index )const
   {
      FC_ASSERT( index < size() );
      // TODO: implement this
      return merkle_branch();
   }

   fc::sha224    disk_merkle_tree::get_root()const
   {
      return my->layers.back()->at(0);
   }

} // namespace bts


