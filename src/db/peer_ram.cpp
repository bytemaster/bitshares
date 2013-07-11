#include <bts/db/peer_ram.hpp>

namespace bts { namespace db {
   namespace detail
   {
      class peer_ram_impl
      {
         public:
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
   }

   peer::record        peer_ram::fetch( const fc::ip::endpoint& ep )
   {
        return record();
   }
   void          peer_ram::remove( const fc::ip::endpoint& ep )
   {
   }
   uint32_t      peer_ram::count()const
   {
        return 0;
   }

   void          peer_ram::set_active( const fc::ip::endpoint& ep, bool a )
   {
   }

   peer::record  peer_ram::get_random_inactive()const
   {
      return record();
   }
} } // bts::db
