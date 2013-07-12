#include <bts/db/peer_ram.hpp>
#include <unordered_map>
#include <fc/exception/exception.hpp>

namespace bts { namespace db {
   namespace detail
   {
      class peer_ram_impl
      {
         public:
           std::unordered_map<fc::ip::endpoint, peer::record> inactive_records;
           std::unordered_map<fc::ip::endpoint, peer::record> active_records;
      };
   }

   peer_ram::peer_ram()
   :my( new detail::peer_ram_impl() )
   {
   }
   peer_ram::~peer_ram()
   {}

   void          peer_ram::store( const record& r )
   {
        auto itr = my->active_records.find(r.ep);
        if( itr != my->active_records.end() )
        {
           itr->second = r;
        }
        else
        {
            my->inactive_records[r.ep] = r;
        }
   }

   peer::record        peer_ram::fetch( const fc::ip::endpoint& ep )
   {
       auto itr = my->active_records.find(ep);
       if( itr != my->active_records.end() )
       {
          return itr->second;
       }
       auto iitr = my->inactive_records.find(ep);
       if( iitr != my->inactive_records.end() )
       {
          return iitr->second;
       }
       FC_THROW_EXCEPTION( key_not_found_exception, "No peer record for ${ep}", ("ep",ep) );
   }
   void          peer_ram::remove( const fc::ip::endpoint& ep )
   {
        my->inactive_records.erase(ep);
        my->active_records.erase(ep);
   }
   uint32_t      peer_ram::inactive_count()const
   {
        return my->inactive_records.size();
   }

   void          peer_ram::set_active( const fc::ip::endpoint& ep, bool a )
   {
      if( !a )
      {
           my->inactive_records[ep] = fetch(ep);
           my->active_records.erase(ep);
      }
      else
      {
           my->active_records[ep] = fetch(ep);
           my->inactive_records.erase(ep);
      }
   }

   peer::record  peer_ram::get_random_inactive()const
   {
      auto itr = my->inactive_records.begin();
      if( itr != my->inactive_records.end() )
          return itr->second;
      FC_THROW_EXCEPTION( key_not_found_exception, "no inactive peer records" );
   }
} } // bts::db
