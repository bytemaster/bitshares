#pragma once
#include <bts/network/stcp_socket.hpp>
#include <bts/network/message.hpp>
#include <bts/mini_pow.hpp>
#include <fc/exception/exception.hpp>

namespace bts { namespace network {
  
   namespace detail { class connection_impl; }

   class connection;
   class message;
   typedef std::shared_ptr<connection> connection_ptr;

   /** 
    * @brief defines callback interface for connections
    */
   class connection_delegate
   {
      public:
        virtual ~connection_delegate(){}; 
        virtual void on_connection_message( connection& c, const message& m ){};
        virtual void on_connection_disconnected( connection& c ){}
   };

   /**
    * Common base class for channel data associated with a specific connection.
    *
    * Many channels need to maintain per-connection data to track broadcast
    * state and potential abuses.  The connection object maintains a mapping from
    * channel id to channel data.  Each channel that needs custom data stored with
    * the connection should derive a data class from channel_data and then
    * use dynamic_cast to recover the data.
    */
   class channel_data : public std::enable_shared_from_this<channel_data>
   {
      public:
          virtual ~channel_data(){}

          template<typename T>
          T& as()
          {
              T* p = dynamic_cast<T*>(this);
              FC_ASSERT( p != nullptr );
              return *p;
          }
   };
   typedef std::shared_ptr<channel_data> channel_data_ptr;


   /**
    *  Manages a connection to a remote p2p node.
    */
   class connection : public std::enable_shared_from_this<connection>
   {
      public:
        connection( const stcp_socket_ptr& c, const config_msg& local );
        connection();
        ~connection();
   
        stcp_socket_ptr  get_socket()const;
        fc::ip::endpoint remote_endpoint()const;
        
        /**
         *  @return nullptr if no data has been assigned to c
         */
        channel_data_ptr get_channel_data( const channel_id& c )const;

        /**
         *  Sets data associated with c and replaces any existing data.
         */
        void             set_channel_data( const channel_id& c, const channel_data_ptr& d );

        /**
         *  Each connection needs to track which broadcasts it
         *  should already know about so that we can avoid
         *  rebroadcasting messages to nodes.
         *
         *  When an inintory message is sent to or received
         *  from a particular node, we store all proofs in
         *  a set that will be cleared after sufficient time.
         */
        void set_knows_broadcast( const mini_pow& p );
        bool knows_message( const mini_pow& p );
        void clear_knows_message( const mini_pow& p );
        void clear_old_inv( fc::time_point t );

        void set_delegate( connection_delegate* d );
   
        void send( const message& m );
        /**
         *  This is used to facilitate broadcast messages that are packed once
         *  and then sent everywhere. 
         *
         *  @note packed_msg.size() must be a multiple of 8 bytes
         */
        void send( const std::vector<char>& packed_msg );
   
        void connect( const std::string& host_port, const config_msg& c );  
        void close();

        /**
         *  Sends config_msg and blocks until a response is
         *  provided.
         */
        config_msg remote_config()const;

        /**
         *  Start processing messages for this connection, call this
         *  *AFTER* setting the delegate
         */
        void start();
      private:
        std::unique_ptr<detail::connection_impl> my;
   };

    
} } 
