#include <bts/network/server.hpp>
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

     class server_impl
     {
        public:
          server_impl()
          :ser_del(nullptr)
          {}

          ~server_impl()
          {
            try 
            {
                tcp_serv.close();
                if( accept_loop_complete.valid() )
                {
                    accept_loop_complete.wait();
                }
            } 
            catch ( const fc::exception& e )
            {
                wlog( "unhandled exception in destructor ${e}", ("e", e.to_detail_string() ));
            } 
            catch ( ... )
            {
                elog( "unexpected exception" );
            }
          }

          server_delegate*            ser_del;
          std::vector<connection_ptr> connections;
          server::config              cfg;
          fc::tcp_server              tcp_serv;

          fc::future<void>            accept_loop_complete;




          /**
           *  This method is called via async from accept_loop and
           *  should not throw any exceptions because they are not
           *  being caught anywhere.
           *
           *  
           */
          void accept_connection( const stcp_socket_ptr& s )
          {
             try 
             {
                // init DH handshake
                s->accept();
                ilog( "accepted connection from ${ep}", ("ep", std::string(s->get_socket().remote_endpoint()) ) );
                
                auto con = std::make_shared<connection>(s);
                
                // TODO: if the connection hangs and server_impl is
                // deleted prior to reaching this step we may have
                // a problem... 
                connections.push_back(con);
             } 
             catch ( const fc::exception& e )
             {
                wlog( "error accepting connection: ${e}", ("e", e.to_detail_string() ) );
             }
             catch( ... )
             {
                elog( "unexpected exception" );
             }
          }

          /**
           *  This method is called async 
           */
          void accept_loop() throw()
          {
             try
             {
                stcp_socket_ptr sock = std::make_shared<stcp_socket>();
                while( tcp_serv.accept( sock->get_socket() ) )
                {
                   // do the acceptance process async
                   fc::async( [=](){ accept_connection( sock ); } );

                   // limit the rate at which we accept connections to prevent
                   // DOS attacks.
                   fc::usleep( fc::microseconds( 1000*30 ) );
                }
             } 
             catch ( fc::exception& e )
             {
                elog( "tcp server socket threw exception\n ${e}", 
                                     ("e", e.to_detail_string() ) );
                // TODO: notify the server delegate of the error.
             }
             catch( ... )
             {
                elog( "unexpected exception" );
             }
          }
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
      // TODO: should I check to make sure we haven't already been configured?
      my->cfg = c;
      ilog( "listening for stcp connections on port ${p}", ("p",c.port) );
      my->tcp_serv.listen( c.port );
      my->accept_loop_complete = fc::async( [=](){ my->accept_loop(); } ); 
  }
  void server::connect_to_peers( uint32_t count )
  {
     if( my->connections.size() >= count ) return;
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
