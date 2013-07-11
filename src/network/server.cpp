#include <bts/network/server.hpp>
#include <bts/network/connection.hpp>
#include <fc/network/tcp_socket.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/thread/thread.hpp>
#include <fc/thread/future.hpp>
#include <fc/io/raw.hpp>
#include <fc/log/logger.hpp>

namespace bts { namespace network {

  namespace detail
  {
     class server_impl
     {
        public:
          server_impl(const bts::db::peer_ptr& pdb )
          :peerdb(pdb),ser_del(nullptr),desired_peer_count(DESIRED_PEER_COUNT)
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
          db::peer_ptr                peerdb;
          server_delegate*            ser_del;
          std::vector<connection_ptr> connections;
          server::config              cfg;
          fc::tcp_server              tcp_serv;
          uint32_t                    desired_peer_count;

          fc::future<void>            accept_loop_complete;
          fc::future<void>            connect_complete;


          void connect_random_peer()
          {
             // grab a random peer from the database
             // attempt to connect
             // remove peer if an error occured
          }

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




  server::server(const bts::db::peer_ptr& pdb ) 
  :my( new detail::server_impl(pdb) ){}

  server::~server()
  {
     try 
     {
        if( my->connect_complete.valid() && !my->connect_complete.ready() )
        {
             my->connect_complete.cancel();
             my->connect_complete.wait();
        }
     } 
     catch ( ... )
     {
         wlog( "unhandled exception" );
     }
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

  /**
   *  Starts an async process of connecting to peers of we are connected to 
   *  less than count peers and we are not already attempting to connect to
   *  peers.  The process will stop once count peers are found and connected.
   */
  void server::connect_to_peers( uint32_t count )
  {
     my->desired_peer_count = count;

     // don't do anything if we are already trying to connect to peers.
     if( my->connect_complete.valid() && !my->connect_complete.ready() )
     {
        return; // already in the process of connecting
     }

     // do this process async because it could take a while.
     my->connect_complete = fc::async( [=](){
        // do not use iterators here because connect_next_peer could yield and
        // invalidate iterators, indexes are safe because we check them after
        // every operation.
        for( uint32_t i = my->connections.size(); i < my->desired_peer_count; ++i )
        {
           my->connect_random_peer();
        }
     });
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
