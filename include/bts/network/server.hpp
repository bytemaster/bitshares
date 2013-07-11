#pragma once
#include <bts/network/message.hpp>
#include <bts/network/stcp_socket.hpp>
#include <bts/config.hpp>

namespace bts { namespace network {

  namespace detail
  {
    class connection_impl;
    class server_impl;
  };
  class connection;
  typedef std::shared_ptr<connection> connection_ptr;


  /** 
   * @brief defines callback interface for connections
   */
  class connection_delegate
  {
     public:
       virtual ~connection_delegate(){}; 
       virtual void on_connection_message( const connection_ptr& c, const message& m ){};
       virtual void on_connection_disconnected( const connection_ptr& c ){}
  };

  /**
   *  Manages a connection to a remote p2p node.
   */
  class connection
  {
     public:
       connection( const stcp_socket_ptr& c );
       connection();
       ~connection();

       void set_delegate( connection_delegate* d );

       void send( const message& m );
       /**
        *  This is used to facilitate broadcast messages that are packed once
        *  and then sent everywhere. 
        *
        *  @note packed_msg.size() must be a multiple of 8 bytes
        */
       void send( const std::vector<char>& packed_msg );

       void connect( const std::string& host_port );  
       void close();
     private:
       std::unique_ptr<detail::connection_impl> my;
  };

  
  class server_delegate
  {
     public:
       virtual ~server_delegate(){}
       virtual void on_message( const message& m ){};
       virtual void on_connected( const connection_ptr& c ){}
       virtual void on_disconnected( const connection_ptr& c ){}
  };


  /**
   *   Abstracts the process of sending and receiving messages 
   *   on the network.  
   */
  class server
  {
    public:
        struct config
        {
            config()
            :port(DEFAULT_SERVER_PORT){}
            uint16_t    port;  ///< the port to listen for incoming connections on.
            std::string chain; ///< the name of the chain this server is operating on (test,main,etc)

            std::vector<std::string> bootstrap_endpoints; // host:port strings for initial connection to the network.

            std::vector<std::string> blacklist;  // host's that are blocked from connecting
        };
        
        server();
        ~server();

        void set_delegate( server_delegate* del );
        
        /**
         * Attempts to open at least count connections to 
         * peers.
         */
        void connect_to_peers( uint32_t count );
        void configure( const config& c );

        void broadcast( const message& m );

        /**
         *  Sends a message to a particular connection.
        void sendto( const connection_ptr& con, const message& );
         */

        std::vector<connection_ptr> get_connections()const;

      private:
        std::unique_ptr<detail::server_impl> my;
  };

} } // bts::server

FC_REFLECT( bts::network::server::config, (port)(chain)(bootstrap_endpoints)(blacklist) )
