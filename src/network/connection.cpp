#include <bts/network/connection.hpp>
#include <bts/network/message.hpp>
#include <fc/network/tcp_socket.hpp>
#include <fc/network/resolve.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/thread/thread.hpp>
#include <fc/io/raw.hpp>
#include <fc/log/logger.hpp>
#include <fc/string.hpp>

namespace bts { namespace network {

  namespace detail
  {
     class connection_impl
     {
        public:
          connection_impl()
          :con_del(nullptr){}
          stcp_socket_ptr      sock;
          connection_delegate* con_del;
     };
  } // namespace detail

  connection::connection( const stcp_socket_ptr& c )
  :my( new detail::connection_impl() )
  {
    my->sock = c;
  }

  connection::connection()
  :my( new detail::connection_impl() )
  {
  }

  connection::~connection()
  {
    try {
        my->sock->close();
    } 
    catch ( ... )
    {
      wlog( "unhandled exception on close" );   
    }
  }


  void connection::set_delegate( connection_delegate* d )
  {
     my->con_del = d;
  }

  void connection::close()
  {
     my->sock->close();
  }

  void connection::connect( const std::string& host_port )
  {
      int idx = host_port.find( ':' );
      auto eps = fc::resolve( host_port.substr( 0, idx ), fc::to_int64(host_port.substr( idx+1 )));
      ilog( "connect to ${host_port} and resolved ${endpoints}", ("host_port", host_port)("endpoints",eps) );
      for( auto itr = eps.begin(); itr != eps.end(); ++itr )
      {
         try 
         {
            my->sock = std::make_shared<stcp_socket>();
            my->sock->connect_to(*itr); 
            ilog( "    connected to ${ep} failed.", ("ep", *itr) );
            return;
         } 
         catch ( const fc::exception& e )
         {
            wlog( "    attempt to connect to ${ep} failed.", ("ep", *itr) );
         }
      }
      FC_THROW_EXCEPTION( exception, "unable to connect to ${host_port}", ("host_port",host_port) );
  }

  void connection::send( const message& m )
  {
      send( fc::raw::pack(m) );     
  }

  void connection::send( const std::vector<char>& packed_msg )
  {
  }

} } // namespace bts::network
