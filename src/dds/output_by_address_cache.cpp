#include <bts/dds/output_by_address_cache.hpp>
#include <bts/dds/table_header.hpp>
#include <fc/interprocess/mmap_struct.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/raw.hpp>

#define OUTPUTS_PER_CHUNK  uint64_t(2*1024*1024/sizeof(bts::output_by_address_cache::entry))


namespace bts 
{

  typedef fc::mmap_struct< fc::array<output_by_address_cache::entry, OUTPUTS_PER_CHUNK > >  mmap_entry_chunk;
  typedef std::unique_ptr<mmap_entry_chunk>                                                 mmap_entry_chunk_ptr;

  namespace detail 
  {
      class output_by_address_cache_impl
      {
        public:
          output_by_address_cache_impl()
          :dirty(true){}

          std::vector<mmap_entry_chunk_ptr>  entries;
          std::vector<bool>                  dirty_chunks;
          bool                               dirty;

          // header data
          fc::sha224                         digest_hash;
          table_header                       table_head;

          void update_hashes()
          {
             if( dirty )
             {
               for( uint32_t i = 0; i < dirty_chunks.size(); ++i )
               {
                  if( dirty_chunks[i] )
                  {
                     table_head.chunk_hashes[i] = fc::sha224::hash( (char*) (*entries[i])->data, 
                                                                     sizeof( (*entries[i])->data ) );
                     dirty_chunks[i] = 0;
                  }
               }
               fc::sha224::encoder enc;
               fc::raw::pack( enc, table_head );
               digest_hash = enc.result();
             }
          }

          void load_header( const fc::path& p, bool create  )
          {
             try 
             {
                 if( !fc::exists( p ) )
                 {
                    if( !create ) 
                    {
                        FC_THROW_EXCEPTION( file_not_found_exception, 
                                  "Unable to find header '${header}' for "
                                  "output_by_address_table", ("header", p ) );
                    }
                    table_head.table_type = claim_by_address;
                    return;
                 }

                 fc::ifstream in( p, fc::ifstream::binary );
                 fc::raw::unpack( in, table_head );

             }FC_RETHROW_EXCEPTIONS( warn, "Unable to load header '${header}' for "
                                     "output_by_address_table", ("header", p ) )
          }
      };

  } // namespace detail 

  output_by_address_cache::entry::entry()
  {
     memset( (char*)this, 0, sizeof(this) );
  }


  output_by_address_cache::output_by_address_cache()
  :my( new detail::output_by_address_cache_impl() )
  {
  }

  output_by_address_cache::~output_by_address_cache()
  {
  }

  /**
   *  Saves the cache state to disk
   */
  void output_by_address_cache::save()
  {
  }

  /**
   *    
   *
   */
  void output_by_address_cache::load( const fc::path& cache_dir, bool create )
  {
     if( !fc::exists( cache_dir ) )
     {
        FC_THROW_EXCEPTION( exception, "directory '${dir}' does not exist", ("dir", cache_dir) );
     }
     my->load_header( cache_dir / "header.dat", create );
     
  }


  fc::sha224 output_by_address_cache::calculate_hash()const
  {
     my->update_hashes();
     return my->digest_hash;
  }


  const table_header&     output_by_address_cache::get_header()const
  {
      my->update_hashes();
      return my->table_head;
  }

  std::vector<char>       output_by_address_cache::get_chunk( uint32_t chunk_num )const
  {
      if( chunk_num < my->entries.size() )
      {
        //  return std::vector<char>( (char*)(*my->entries[chunk_num])->begin(), 
         //                           (const char*)(*my->entries[chunk_num])->end() );
      }
      else
      {
      }
      return std::vector<char>();
  }

  void                    output_by_address_cache::set_chunk( uint32_t chunk_num, const std::vector<char>& )
  {
  }

  /**
   *  @return the set of unspent outputs for a particular block.
   */
  std::vector<uint32_t> output_by_address_cache::get_unspent_by_block( uint32_t block_number )
  {
     return std::vector<uint32_t>(); // TODO: implement this
  }


  /**
   *  Stores entry e in the cache and returns the index it was
   *  stored at.
   *
   *  @param index - the location where e should be stored
   *
   *  @pre  index is in the free list and the contents are zeroed
   *  @pre  index <= table_header.size
   *  @post index is no longer in the free_list
   */
  void  output_by_address_cache::store( uint32_t index, const entry& e )
  {
  }

  uint32_t output_by_address_cache::get_free_slot()
  {
     if( my->table_head.free_list.size() > 0 )
     {
       return my->table_head.free_list.back();
     }
     return my->table_head.size;
  }

  /**
   *  Releases the specified index and places it in the 
   *  free-list.
   *
   *  @pre index is not in the free list
   *  @post index is in the free list and the contents of location at index are zeroed.
   */
  void     output_by_address_cache::clear( uint32_t index )
  {
      // TODO: add pre-condition check that index is not already in the free list
      my->table_head.free_list.push_back( index );
  }


  /**
   *  Finds the output and returns its index, throws if the
   *  output was not found.
   */
  uint32_t       output_by_address_cache::find( const output_reference& out )
  {
    return 0; // TODO implement
  }

  const output_by_address_cache::entry&   output_by_address_cache::get( uint32_t entry_id )
  {
     FC_THROW_EXCEPTION( exception, "not implemented" );
  }


} // namespace bts
