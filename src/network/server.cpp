#include <bts/network/server.hpp>
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

     class server_impl
     {
        public:
          server_impl()
          :ser_del(nullptr)
          {}

          server_delegate*            ser_del;
          std::vector<connection_ptr> connections;
          server::config              cfg;
     };
  }


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


  server::server()
  :my( new detail::server_impl() ){}

  server::~server()
  {
  }

  void server::set_delegate( server_delegate* sd )
  {
    my->ser_del = sd;
  }

  void server::configure( const server::config& c )
  {
     my->cfg = c;
  }

  void server::broadcast( const message& m )
  {
     // allocate on heap and reference count so that multiple async
     // operations can reference the same buffer.
     auto buf = std::make_shared<std::vector<char>>(fc::raw::pack(m));
     for( uint32_t i = 0; i < my->connections.size(); ++i )
     {
        auto tmp = my->connections[i]; // capture the shared ptr,
                                       // not the index.
        fc::async( [tmp,buf](){ tmp->send( *buf ); } );
     }
  }

  /**
  void server::sendto( const connection_ptr& con, const message& m )
  {
  }
  */

  std::vector<connection_ptr> server::get_connections()const
  {
    return my->connections;
  }


  







} } // namespace bts::network
