#pragma once
#include <fc/network/ip.hpp>
#include <fc/time.hpp>
#include <memory>
#include <vector>

namespace bts { namespace db
{
   /**
    *  @brief abstracts the interface to the peer database
    *
    *  There may be multiple backends for storing IP/Port/stats
    *  for various peers in the network.  This interface 
    *  abstracts that process.
    */
   class peer
   {
      public:
         virtual ~peer(){}

         struct record 
         {
            record():bytes_recv(0),bytes_sent(0),warnings(0){}

            fc::ip::endpoint           ep;         // the ip/port of an active connection
            fc::ip::endpoint           server_ep;  // if the server can be connected to
            fc::time_point             last_com;
            uint64_t                   bytes_recv;
            uint64_t                   bytes_sent;
            int64_t                    warnings;  // incremented for bad behavior
            std::vector<std::string>   channels;
            std::vector<std::string>   features;  // protocol features supported by this node
         };

         virtual void          store( const record& r ) = 0;
         virtual record        fetch( const fc::ip::endpoint& ep ) = 0;
         virtual void          remove( const fc::ip::endpoint& ep ) = 0;
         virtual uint32_t      inactive_count()const = 0;

         virtual void          set_active( const fc::ip::endpoint& ep, bool a = true ) = 0;

         virtual record        get_random_inactive()const = 0;
      
   };
   typedef std::shared_ptr<peer> peer_ptr;


} } // namespace bts::db

#include <fc/reflect/reflect.hpp>
FC_REFLECT( bts::db::peer::record, 
   (ep)
   (server_ep)
   (last_com)
   (bytes_recv)
   (bytes_sent)
   (warnings)
   (channels)
   (features) )
