#pragma once
#include <bts/network/stcp_socket.hpp>

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
        virtual void on_connection_message( const connection_ptr& c, const message& m ){};
        virtual void on_connection_disconnected( const connection_ptr& c ){}
   };


   /**
    *  Manages a connection to a remote p2p node.
    */
   class connection : public std::enable_shared_from_this<connection>
   {
      public:
        connection( const stcp_socket_ptr& c );
        connection();
        ~connection();
   
        stcp_socket_ptr get_socket()const;

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

    
} } 
