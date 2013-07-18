#include <bts/network/connection.hpp>
#include <bts/network/message.hpp>
#include <bts/config.hpp>

#include <fc/network/tcp_socket.hpp>
#include <fc/network/resolve.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/thread/thread.hpp>
#include <fc/io/raw.hpp>
#include <fc/log/logger.hpp>
#include <fc/string.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>

#include <unordered_map>

namespace bts { namespace network {

  namespace detail
  {
     class connection_impl
     {
        public:
          connection_impl(connection& s)
          :self(s),con_del(nullptr){}
          connection&          self;
          stcp_socket_ptr      sock;
          connection_delegate* con_del;

          std::unordered_map<mini_pow,fc::time_point> known_inv;

          /** used to ensure that messages are written completely */
          fc::mutex              write_lock;


          fc::future<void>       read_loop_complete;
          config_msg             remote_config;

          void exchange_config( const config_msg& m )
          {
             message msg(m, channel_id(peer_proto) );
             self.send( msg );
             remote_config = read_remote_config();
          }

          message  read_next_message()
          {
             try {
               uint64_t len;
               sock->read( (char*)&len, sizeof(len) );
               if( len > MAX_MESSAGE_SIZE )
               {
                  FC_THROW_EXCEPTION( exception, 
                    "message size ${s} exceeds maximum message size ${m} kb",
                    ("s",len / 1024)("m",MAX_MESSAGE_SIZE/1024) );
               }
               if( len % 8 != 0 )
               {
                  FC_THROW_EXCEPTION( exception, 
                    "message size ${s} is not a multiple of 8 bytes",
                    ("s",len / 1024) );
               }
               std::vector<char> tmp(len);
               sock->read( tmp.data(), len );
             //  ilog( "read ${i} ${data}" ,  ("i", tmp.size() )("data",tmp) );

               message m;
               fc::datastream<const char*> ds(tmp.data(), tmp.size());
               fc::raw::unpack(ds,  m.chan );
            //   ilog( "unpacked m.channel ${c}", ("c", m.chan) );

               m.data.resize( ds.remaining() );
               ds.read( m.data.data(),  m.data.size() );

         //      ilog( "message data size ${i}   ${data}" , 
          //            ("i", m.data.size() )("data",m.data) );
               
               return m;
             } FC_RETHROW_EXCEPTIONS( warn, "error reading message" );
          }

          config_msg read_remote_config()
          {
            try
            {
               auto msg = read_next_message();

               if( msg.chan != channel_id(peer_proto) )
               {
                  FC_THROW_EXCEPTION( exception, 
                    "first message was not on channel ${c}",
                    ("c",channel_id(peer_proto)) ); 
               }

               fc::datastream<const char*> ds( msg.data.data(), msg.data.size() );

               fc::unsigned_int msg_type;
               fc::raw::unpack( ds, msg_type );

               if( msg_type.value != message_code::config )
               {
                  message_code c = (message_code)msg_type.value;
                  FC_THROW_EXCEPTION( exception, 
                    "received ${recv_type} but expected ${expect_type} "
                    "first message was not on a config message",
                    ("recv_type", c )
                    ("expect_type",message_code::config) );
               }
               
               config_msg cfg;
               fc::raw::unpack( ds, cfg );
               return cfg;
            } FC_RETHROW_EXCEPTIONS( warn, "error reading remote configuration" );
          }

          void read_loop()
          {
            try {
               while( true )
               {
                  auto m = read_next_message();
                  assert( con_del != nullptr );
                  if( con_del )
                  {
                     con_del->on_connection_message( self, m );
                  }
               }
            } 
            catch ( const fc::canceled_exception& e )
            {
              if( con_del )
              {
                con_del->on_connection_disconnected( self );
              }
              else
              {
                wlog( "disconnected ${e}", ("e", e.to_detail_string() ) );
              }
            }
            catch ( const fc::eof_exception& e )
            {
              if( con_del )
              {
                con_del->on_connection_disconnected( self );
              }
              else
              {
                wlog( "disconnected ${e}", ("e", e.to_detail_string() ) );
              }
            }
            catch ( const fc::exception& e )
            {
              if( con_del )
              {
                con_del->on_connection_disconnected( self );
                elog( "disconnected" );
              }
              else
              {
                elog( "disconnected ${e}", ("e", e.to_detail_string() ) );
              }
              throw;
            }
          }
     };
  } // namespace detail

  connection::connection( const stcp_socket_ptr& c, const config_msg& loc_cfg  )
  :my( new detail::connection_impl(*this) )
  {
    my->sock = c;
    my->exchange_config( loc_cfg );
  }

  connection::connection()
  :my( new detail::connection_impl(*this) )
  {
  }


  connection::~connection()
  {
    try {
      close();
      if( my->read_loop_complete.valid() )
      {
        my->read_loop_complete.wait();
      }
    } 
    catch ( const fc::canceled_exception& e )
    {
      ilog( "canceled" );
    }
    catch ( const fc::exception& e )
    {
      wlog( "unhandled exception on close:\n${e}", ("e", e.to_detail_string()) );   
    }
    catch ( ... )
    {
      elog( "unhandled exception on close" );   
    }
  }
  stcp_socket_ptr connection::get_socket()const
  {
     return my->sock;
  }


  void connection::set_delegate( connection_delegate* d )
  {
     my->con_del = d;
  }

  void connection::close()
  {
     if( my->sock )
     {
       my->sock->close();
     }
  }

  void connection::connect( const std::string& host_port, const config_msg& m )
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
            my->exchange_config( m ); 
            ilog( "    connected to ${ep} with config ${conf}", ("ep", *itr)("conf",remote_config())  );
            my->read_loop_complete = fc::async( [=](){ my->read_loop(); } );
            return;
         } 
         catch ( const fc::exception& e )
         {
            wlog( "    attempt to connect to ${ep} failed.", ("ep", *itr) );
         }
      }
      FC_THROW_EXCEPTION( exception, "unable to connect to ${host_port}", ("host_port",host_port) );
  }
  void connection::start()
  {
     my->read_loop_complete = fc::async( [=](){ my->read_loop(); } );
  }

  void connection::send( const message& m )
  {
      std::vector<char>       data;
      fc::datastream<size_t> ps; 
      fc::raw::pack(ps,m.chan);

      data.resize( 8*((ps.tellp() + m.data.size() + 7)/8) );
      memset( data.data() + data.size()-8, 0, 8 );

      fc::datastream<char*> ds(data.data(), data.size());
      fc::raw::pack(ds,m.chan);
      ds.write( m.data.data(), m.data.size() );

      send( data );
  }

  void connection::send( const std::vector<char>& packed_msg )
  {
      FC_ASSERT( packed_msg.size() % 8 == 0 );
      FC_ASSERT( !!my->sock );
      //ilog( "writing ${d} bytes", ("d", packed_msg.size() ) );
      uint64_t s = packed_msg.size();

      // TODO: populate 4 bytes of size with random data
      // because otherwise they would always be 0 and thus make
      // the encryption easier to hack 
      
      { // we have to lock writes which may yield so that multiple
        // coroutines do not interleave their writes, this should be
        // a cooperative lock vs a hard OS lock
        fc::scoped_lock<fc::mutex> lock_write( my->write_lock );
        my->sock->write( (char*)&s, sizeof(s) );
        my->sock->write( packed_msg.data(), packed_msg.size() );
      }
      my->sock->flush();
  }

  config_msg connection::remote_config()const
  {
      return my->remote_config;
  }


  void connection::set_knows_broadcast( const mini_pow& p )
  {
      my->known_inv[p] = fc::time_point::now();
  }
  bool connection::knows_message( const mini_pow& p )
  {
      return my->known_inv.find(p) != my->known_inv.end();
  }
  void connection::clear_knows_message( const mini_pow& p )
  {
      my->known_inv.erase(p);
  }
  void connection::clear_old_inv( fc::time_point inv )
  {
      for( auto itr = my->known_inv.begin(); itr != my->known_inv.end();  )
      {
         if( itr->second < inv ) 
         {
            itr = my->known_inv.erase(itr);
         }
         else
         {
            ++itr;
         }
      }
  }

  fc::ip::endpoint connection::remote_endpoint()const 
  {
    return get_socket()->get_socket().remote_endpoint();
  }



} } // namespace bts::network
