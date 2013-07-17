#include <bts/network/server.hpp>
#include <bts/network/connection.hpp>
#include <bts/db/peer.hpp>
#include <fc/network/tcp_socket.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/thread/thread.hpp>
#include <fc/thread/future.hpp>
#include <fc/io/raw.hpp>
#include <fc/log/logger.hpp>


#include <algorithm>
#include <unordered_map>
#include <map>

namespace bts { namespace network {

  namespace detail
  {
     class server_impl : public connection_delegate
     {
        public:
          server_impl(const bts::db::peer_ptr& pdb )
          :peerdb(pdb),ser_del(nullptr),desired_peer_count(DESIRED_PEER_COUNT)
          {}

          ~server_impl()
          {
             close();
          }
          void close()
          {
              try 
              {
                  for( auto i = pending_connections.begin(); i != pending_connections.end(); ++i )
                  {
                    (*i)->close();
                  }
                  tcp_serv.close();
                  if( accept_loop_complete.valid() )
                  {
                      accept_loop_complete.wait();
                  }
              } 
              catch ( const fc::canceled_exception& e )
              {
                  ilog( "expected exception on closing tcp server\n" );  
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
          db::peer_ptr                                         peerdb;
          server_delegate*                                     ser_del;

          std::unordered_map<fc::ip::endpoint,connection_ptr>  connections;
          std::map<channel_id, std::set<connection_ptr> >      connections_by_channel;

          std::set<connection_ptr>                             pending_connections;
          server::config                                       cfg;
          fc::tcp_server                                       tcp_serv;
          uint32_t                                             desired_peer_count;

          bts::network::config_msg                             local_cfg;
                                                               
          fc::future<void>                                     accept_loop_complete;
          fc::future<void>                                     connect_complete;

          std::unordered_map<uint64_t, channel_ptr> channels;

          virtual void on_connection_message( connection& c, const message& m )
          {
             ilog( "received message from .. " ); 
          }

          virtual void on_connection_disconnected( connection& c )
          {
             ilog( "cleaning up connection after disconnect" );
             connections.erase( c.get_socket()->get_socket().remote_endpoint() );
             
             auto cptr = c.shared_from_this();
             for( auto itr = connections_by_channel.begin(); itr != connections_by_channel.end(); ++itr )
             {
                itr->second.erase( cptr );
             }
          }

          void update_connection_channel_index( const config_msg& m, connection_ptr& con )
          {
              for( auto itr = m.subscribed_channels.begin(); itr != m.subscribed_channels.end(); ++itr )
              {
                 // TODO: filter here in some way to prevent spam... perhaps a white-list of valid
                 // channels
                 connections_by_channel[*itr].insert( con );
              }
          }



          void connect_random_peer()
          {
             // TODO: make random.
             auto recs = peerdb->get_all_peers();
             ilog( "peers ${i}", ("i", recs.size()));
             if( recs.size() == 0 )
             {
               wlog( "no known peers" );
               FC_THROW_EXCEPTION( exception, "no known peers" );
             }

             auto rec = recs[rand()%recs.size()];
             uint32_t cnt = 0;
             auto itr = connections.find(rec.contact);
             while( cnt < recs.size() && (itr != connections.end()) )
             {
                 rec = recs[rand()%recs.size()];
                 ++cnt;
                 itr = connections.find(rec.contact);
             }
             if (itr != connections.end() )
             {
                wlog( "no new peers to connect to" );
                FC_THROW_EXCEPTION( exception, "unable to find any new peers to connect to" );
             }

             try 
             {
                auto con = std::make_shared<connection>();
                pending_connections.insert(con);

                ilog( "connect to ${c}", ("c", rec.contact) );
                con->connect( rec.contact, local_cfg );
                peerdb->update_last_com( rec.contact, fc::time_point::now() );
                connections[rec.contact] = con;

                auto remote_cfg = con->remote_config();
                update_connection_channel_index( remote_cfg, con );

                if( remote_cfg.public_contact != fc::ip::endpoint() )
                {
                   bts::db::peer::record rec;
                   rec.contact  = remote_cfg.public_contact;
                   rec.channels = std::move(remote_cfg.subscribed_channels);
                   rec.features = std::move(remote_cfg.supported_features);
                   rec.last_com = fc::time_point::now();
                   peerdb->store( rec );
                }
                
                pending_connections.erase(con);

                /*
                std::vector<connection_ptr>& vec = pending_connections;
                vec.erase(std::remove(vec.begin(), vec.end(), con), vec.end());
                */

                if( ser_del ) ser_del->on_connected(con);
             } 
             catch ( fc::canceled_exception& e )
             {
             }
             catch ( fc::exception& e )
             {
                wlog( "unable to connect to peer: ${e}", ("e", e.to_detail_string()) );
                throw;
             }
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
                
                auto con = std::make_shared<connection>(s,local_cfg);
                
                // TODO: if the connection hangs and server_impl is
                // deleted prior to reaching this step we may have
                // a problem... 
                connections[con->get_socket()->get_socket().remote_endpoint()] = con;
                // TODO: add delegate to handle disconnect
                con->start();
             } 
             catch ( const fc::canceled_exception& e )
             {
                ilog( "canceled accept operation" );
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
                   sock = std::make_shared<stcp_socket>();
                }
             } 
             catch ( fc::eof_exception& e )
             {
             }
             catch ( fc::canceled_exception& e )
             {
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

  void server::subscribe_to_channel( const channel_id& chan, const channel_ptr& c )
  {
     my->channels[chan.id()] = c;
     //TODO notify all of my peers that I am now subscribing to chan
  }

  void server::unsubscribe_from_channel( const channel_id&  chan )
  {
     my->channels.erase(chan.id());
     //TODO notify all of my peers that I am no longer subscribign to chan
  }

  void server::set_delegate( server_delegate* sd )
  {
     my->ser_del = sd;
  }

  void server::configure( const server::config& c )
  {
      // TODO: should I check to make sure we haven't already been configured?
      my->cfg = c;
      for( uint32_t i = 0; i < c.bootstrap_endpoints.size(); ++i )
      {
         db::peer::record r;
         r.contact = fc::ip::endpoint::from_string(c.bootstrap_endpoints[i]);
         my->peerdb->store( r );
      }

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
     for( auto itr = my->connections.begin();
          itr != my->connections.end();
          ++itr ) //uint32_t i = 0; i < my->connections.size(); ++i )
     {
        auto tmp = itr->second;
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
    return std::vector<connection_ptr>(); //my->connections;
  }


  




   void server::close()
   {
       my->close();
   }



} } // namespace bts::network
