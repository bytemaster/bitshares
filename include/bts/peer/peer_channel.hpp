/**  @file peer_channel.hpp
 *   @brief Defines messages exchanged on the peer/discovery channel (0)
 */
#pragma once
#include <bts/network/server.hpp>
#include <bts/network/connection.hpp>

namespace bts { namespace peer {

  namespace detail { class peer_channel_impl; }

  /**
   *  Tracks a contact address and the last time it was heard about.
   */
  struct host
  {
     fc::ip::endpoint    ep;
     fc::time_point_sec  last_com;
  };

  /**
   *  Manages the state of all peers including what channels they
   *  are connected to and known IPs
   */
  class peer_channel 
  {
      public:
        peer_channel( const network::server_ptr& netw );
        ~peer_channel();

        /**
         *  broadcasts the new channel subscription to all connected nodes. If less than
         *  the minimum number of connections exist to this channel, new connections are
         *  opened.
         */
        void subscribe_to_channel( const network::channel_id& chan, const network::channel_ptr& c );
        void unsubscribe_from_channel( const network::channel_id& chan );

        std::vector<host>                    get_known_hosts()const;
        std::vector<network::connection_ptr> get_connections( const network::channel_id& chan );
      private:
        std::unique_ptr<detail::peer_channel_impl> my;
  };

  typedef std::shared_ptr<peer_channel> peer_channel_ptr;



} }

#include <fc/reflect/reflect.hpp>
FC_REFLECT( bts::peer::host,             (ep)(last_com) )

