#include <bts/network/connection.hpp>
#include <bts/network/message.hpp>
#include <fc/network/tcp_socket.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/thread/thread.hpp>
#include <fc/io/raw.hpp>
#include <fc/log/logger.hpp>

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
     // auto eps = fc::asio::resolve( host_port.substr( 0, idx ), host_port.substr( idx+1 ));
      // TODO: loop over all endpoints
  }

  void connection::send( const message& m )
  {
      send( fc::raw::pack(m) );     
  }

  void connection::send( const std::vector<char>& packed_msg )
  {
  }

} } // namespace bts::network
