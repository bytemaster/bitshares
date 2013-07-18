#include <bts/bitchat.hpp>
#include <bts/inventory.hpp>
#include <bts/network/channel_pow_stats.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/thread/thread.hpp>
#include <unordered_map>
#include <map>


namespace bts 
{

   using namespace network;
   namespace detail 
   {
     class bitchat_channel;
     typedef std::shared_ptr<bitchat_channel> bitchat_channel_ptr;

     /**
      *  Tracks all messages we know about.
      */
     struct message_inventory 
     {
        fc::time_point             last_bcast;
        inventory<bitchat_message> msgs;
     };

     struct fetch_state
     {
        fetch_state( const channel_id& chan= channel_id(), const fc::time_point& it = fc::time_point::now(), uint16_t c = 1 )
        :insert_time(it),query_count(0),notice_count(c){}

        fc::time_point insert_time;  ///< first time we saw the inv id
        fc::time_point last_query;   ///< last time we requested the data, so we can rebroadcast if necessary
        channel_id     chan;
        uint8_t        query_count;  ///< how many times we have requested the data
        uint16_t       notice_count; ///< how many times we have recieved an inv (this may be a better priority metric)
     };


     typedef std::shared_ptr<message_inventory> message_inventory_ptr;

     class bitchat_impl 
     {
        public:
          bitchat_impl()
          :del(nullptr)
          {
            done = false;
          }

          bool done;
          bts::network::server_ptr netw;
          fc::future<void> fetch_loop_complete;

          /**
           * Keep track of the last time we requested sorted by
           * proof of work.
           */
          std::map<mini_pow, fetch_state> pending_fetch; 

          std::unordered_map<uint64_t, bitchat_channel_ptr>     chans;
          std::map<std::string,bitchat_contact>                 contacts;
          std::map<std::string,bitchat_identity>                idents;
          std::unordered_map<fc::ecc::public_key_data,std::string>   key_to_contact;  
          std::unordered_map<fc::ecc::public_key_data,std::string>   key_to_idents;  


          std::map<uint64_t, channel_pow_stats>             _chan_stats;
          bitchat_delegate*                                 del;

          std::unordered_map<uint64_t, message_inventory_ptr>  chan_inventory; 

          void store_inventory( const bitchat_message& m, const channel_id& c )
          {
             auto id = m.calculate_id();
             auto itr = chan_inventory.find(c.id());
             if( itr == chan_inventory.end() )
             {
                chan_inventory[c.id()] = std::make_shared<message_inventory>();
                itr = chan_inventory.find(c.id());
             }
             itr->second->msgs.store( id, m );
          }

          /**
           *  Broadcasts all new inventory items in a given channel.
           */
          void broadcast_inventory( const channel_id& c )
          {
             auto itr = chan_inventory.find(c.id());
             if( itr == chan_inventory.end() ) 
             {
                wlog( "broadcast on unintialized channel ${c} ?? ", ("c",c));
                return;
             }

             itr->second->msgs.clear_inventory( 
                    fc::time_point(), 
                    fc::time_point::now() - fc::seconds( BITCHAT_INVENTORY_WINDOW_SEC ) );

             auto items = itr->second->msgs.get_inventory( itr->second->last_bcast  );

             auto chan_cons = netw->connections_for_channel(c);
             for( auto itr = chan_cons.begin(); itr != chan_cons.end(); ++itr )
             {
                inv_message inv;
                for( auto i = items.begin(); i != items.end(); ++i )
                {
                  if( !(*itr)->knows_message(*i) )
                  {   
                    inv.items.insert(*i);
                    (*itr)->set_knows_broadcast( *i );
                  }
                }
                if( inv.items.size() )
                {
                  ilog( "broadcasting inventory ${i} to ${e} on channel ${c}", 
                        ("i", inv.items)
                        ("e",(*itr)->remote_endpoint())
                        ("c",c) );

                  (*itr)->send( bts::network::message( inv, c) );
                }
             }
          }

          bool have_message_in_inventory( const mini_pow& m, const channel_id& chan )
          {
             // TODO: validate channel protocol
             // TODO: validate max CHAT CHANNEL to prevent memory overflow attacks

             auto itr = chan_inventory.find( chan.id() );
             if( itr == chan_inventory.end() )
             {
                return false;
             }
             FC_ASSERT( !!itr->second );
             return itr->second->msgs.contains(m);
          }

          /**
           *  Places an inventory id into the queue to be fetched from other
           *  nodes at the next opportuntiy.
           */
          void schedule_fetch( const mini_pow& m, const channel_id& c )
          {
             auto pi =  pending_fetch.find( m );
             if( pi != pending_fetch.end() )
             {
                pi->second.notice_count++;
             }
             else
             {
                pending_fetch[m] = fetch_state( c, fc::time_point::now() );
                pending_fetch[m].notice_count = 1;
             }

          }

          void fetch_loop()
          {
             while( !done )
             {
                //ilog( "fetch loop... " );
                try
                {
                    for( auto itr = pending_fetch.begin(); itr != pending_fetch.end(); /*++itr happens conditionally*/ )
                    {
                        auto elap = fc::time_point::now() - itr->second.last_query;
                        if( elap > fc::seconds( 5 ) ) // TODO: define config constant for this param
                        {
                           if( itr->second.query_count > 3 ) // TODO: define config constant for this param
                           {
                              wlog( "no response for inventory query after 3 attempts" );
                              itr = pending_fetch.erase(itr);
                              continue;
                           }
                           itr->second.last_query = fc::time_point::now();
                           itr->second.query_count++;
       
                           request_data_message( itr->first, channel_id( chat_proto ) ); // TODO: don't hard code this... itr->second.chan );
       
                           // NOTE: requesting data above could yield and thus invalidate our
                           // iterators, we must a
                           itr = pending_fetch.begin();
                           continue;
                        }
                        ++itr;
                    }
                } 
                catch ( fc::exception& e )
                {
                   wlog( "unexpected exception ${e}", ("e", e.to_detail_string() ) );
                }
                // TODO.... use a signal / wait condition here to notify the fetch loop
                // when to resume, polling will work for now
                fc::usleep( fc::microseconds( 1000*50 ) );
             }
          }

          /**
           *  Select a random connection on channel c that happens to have p and
           *  request the message.
           */
          void request_data_message( const mini_pow& p, const channel_id& c )
          {
             try 
             {
               get_data_message m;
               m.items.insert(p);
             
               std::vector<connection_ptr> canidates;
             
               auto cons = netw->connections_for_channel(c);
               for( auto itr = cons.begin(); itr != cons.end(); ++itr )
               {
                  if( (*itr)->knows_message(p) )
                  {
                    canidates.push_back(*itr);
                  }
               }
             
               if( canidates.size() == 0 )
               {
                   FC_THROW_EXCEPTION( exception, "no connections know about message ${i}", ("i", p) );
               }
             
               auto index = rand() % canidates.size();
               canidates[index]->send( bts::message( m, c ) );
             
             } FC_RETHROW_EXCEPTIONS( warn, 
                     "requesting data message ${m} on channel ${c}", 
                     ("m",p)("c",c) );
          }


          void handle_inv_message( const connection_ptr& con, const inv_message& msg, const channel_id& c )
          {
             try 
             {
                ilog( "recv inventory ${i}", ("i", msg ) );

                // for each item, note the connection knows about it and schedule
                // it to be fetched if it is not already in our inventory.
                for( auto itr = msg.items.begin(); itr != msg.items.end(); ++itr )
                {
                  con->set_knows_broadcast( *itr );
                  if( !have_message_in_inventory( *itr, c ) )
                  {
                     //ilog( "scheduling fetch of ${i}", ("i", *itr) );
                     schedule_fetch( *itr, c );
                  }
                }
             } FC_RETHROW_EXCEPTIONS( warn, "Error processing inventory message: ${m}", ("m",msg) );
          }

          void handle_data_message( const connection_ptr& con, bitchat_message& msg, const channel_id& c )
          {
               // TODO validate difficulty...
               auto msg_id = msg.calculate_id();

               auto itr = pending_fetch.find(msg_id);
               if( itr == pending_fetch.end() )
               {
                  wlog( "received unrequested message... bad, bad connection" );
               }
               else
               {
                  pending_fetch.erase(itr);
               }
               store_inventory( msg, c );

               // TODO: move this to a separate loop so that it happens 
               // out of band with receiving the message
               fc::async( [=](){ broadcast_inventory(c); } );

               try_decrypt_msg( con, c, msg );
          }

          const bitchat_message& get_inv_message( const mini_pow& p, const channel_id& c )
          {
             auto cinv = chan_inventory.find( c.id() );
             if( cinv == chan_inventory.end() )
             {
                FC_THROW_EXCEPTION( key_not_found_exception, 
                                    "Unable to find inventory for channel ${c}", 
                                    ("c", c) );
             }
             return cinv->second->msgs.get(p);
          }


          /**
           *  This method is called when the node at con has requested one or more messages
           *  from our inventory.   
           */
          void handle_get_data_message( const connection_ptr& con, 
                                        const get_data_message& msg, 
                                        const channel_id& chan )
          {
               // TODO: we need to prevent abuse of this message to DOS the network... perhaps
               //       by putting a bandwidth limit on each connection to ensure it stays
               //       in compliance and is not abusing the network.
               ilog( "handle get_data_message" );

               // lookup and send the data async
               fc::async( [this,con,msg,chan]() {
                       for( auto itr = msg.items.begin(); itr != msg.items.end(); ++itr )
                       {
                          if( have_message_in_inventory( *itr, chan ) )
                          {
                              auto m = get_inv_message( *itr, chan );
                              con->send( message( m, chan ) );
                          }
                       }
                   } );
          }



          void try_decrypt_msg( const connection_ptr& con, const channel_id& cid, bitchat_message& m )
          {
            try {
              // check for messages to me
              for( auto itr = idents.begin(); itr != idents.end(); ++itr )
              {
                 if( m.decrypt( itr->second.key ) )
                 {
                    // TODO: pass contact to handle data msg?
                    handle_data_msg( con, cid, m );
                    return;
                 }
              }
              // check for messages from others
              for( auto itr = contacts.begin(); itr != contacts.end(); ++itr )
              {
              
              }
              
              // check for messages from myself...?
              for( auto itr = idents.begin(); itr != idents.end(); ++itr )
              {
              
              }
              wlog( "unable to decrypt message ${m}", ("m", m.calculate_id()) );
            } FC_RETHROW_EXCEPTIONS( warn, "attempting to decrypt message" );
          } // try_decrypt_msg


          /**
           *  Called when a message is successfully decrypted, we need to process the message, determine
           *  whehter it is a text message, contact request, invite, or some other type of message destined
           *  for this user.
           */
          void handle_data_msg( const connection_ptr& con, const channel_id& id, const bitchat_message& m )
          {
             FC_ASSERT( !m.is_encrypted() );
             fc::datastream<const char*> ds( m.get_content().body.data(), m.get_content().body.size() );
             fc::unsigned_int msg_type;
             fc::raw::unpack( ds, msg_type );

             if( msg_type.value == bitchat_text_msg )
             {
                std::string msg;
                fc::raw::unpack( ds, msg );

                auto citr = key_to_contact.find( m.get_content().from->serialize() );
                if( citr == key_to_contact.end() )
                {
                    ilog( "Received message '${msg}' from ${from}", ("msg", msg )("from", to_bitchat_address(*m.get_content().from)) );
                }
                else
                {
                    ilog( "Received message '${msg}' from ${from}", ("msg", msg )("from", citr->second) );
                }
             }
             else
             {
                wlog( "Recieved unknown message type ${t}", ("t", msg_type ) );
             }

          } // handle_data_msg



          /**
           *  Decodes message type, unpacks it, and routes it to the proper handler
           */
          void handle_message( const connection_ptr& con, const message& m )
          {
             FC_ASSERT( m.data.size() > 0 );

             // TODO: validate message is on a channel we have subscribed to


            fc::datastream<const char*>  ds(m.data.data(), m.data.size() );
            fc::unsigned_int msg_type;
            fc::raw::unpack( ds, msg_type );
//            ilog( "handle message type ${t}", ("t",msg_type.value) );

            if( msg_type.value == data_msg )
            {
               bitchat_message msg;
               fc::raw::unpack( ds, msg );
               handle_data_message( con, msg, m.chan );
            }
            else if( msg_type.value == inventory_msg )
            {
               inv_message msg;
               fc::raw::unpack( ds, msg );
               handle_inv_message( con, msg, m.chan );
            }
            else if( msg_type.value == get_data_msg )
            {
               get_data_message msg;
               fc::raw::unpack( ds, msg );
               handle_get_data_message( con, msg, m.chan );
            }
            else
            {
              wlog( "Unknown message type ${i}", ("i", msg_type) );
              FC_THROW_EXCEPTION( exception, "Unknown message type ${i}", ("i", msg_type) );
            }

          } // handle_message(...)

     };


     /** provides a 'weak pointer' for the channel that redirects calls to the private
      * impl until it is freed. 
      */
     class bitchat_channel : public bts::network::channel
     {
        public:
         bitchat_channel( bitchat_impl* s )
         :self(s)
         {
         }
      
         void handle_message( const connection_ptr& con, const message& m )
         {
            if( self ) 
            {   
              try
              {
                self->handle_message( con, m );
              } 
              catch ( const fc::exception& e )
              {
                wlog( "unexpected exception handling message\n ${e}", ("e", e.to_detail_string())("message",m));
              }
            } 
            else
            {
                FC_THROW_EXCEPTION( exception, "chat client no longer subscribed to channel" );
            }
         }
         bitchat_impl* self;
     };

   } // namesapce detail

   bitchat::bitchat( const bts::network::server_ptr& s )
   :my(new bts::detail::bitchat_impl() )
   { 
      my.reset( new bts::detail::bitchat_impl());
      my->netw = s;

      my->fetch_loop_complete = fc::async([=](){ my->fetch_loop(); });
      
      auto chan = std::make_shared<bts::detail::bitchat_channel>(my.get());
      my->netw->subscribe_to_channel( channel_id(network::chat_proto,0), chan );
   }

   bitchat::~bitchat()
   {
      my->done = true;
      my->fetch_loop_complete.cancel();

      my->netw->unsubscribe_from_channel( channel_id(network::chat_proto,0) );
      for( auto itr = my->chans.begin(); itr != my->chans.end(); ++itr )
      {
        itr->second->self = nullptr;
      }

      try {
        my->fetch_loop_complete.wait();
      } 
      catch ( const fc::exception& e )
      {
         wlog( "exception thrown durring clean up ${e}", ("e", e.to_detail_string() ) );
      }
   }

  /*
   void bitchat::set_channels( const std::vector<uint32_t>& chans )
   {
   }
  */


   void bitchat::set_delegate( bitchat_delegate* d )
   {
     my->del = d;
   }

   void bitchat::send_message( const std::string& msg, const bitchat_contact& to, const bitchat_identity& from )
   {
      bitchat_message m;
      m.timestamp = fc::time_point::now();
      m.body( fc::raw::pack( data_message( msg ) ) );
      m.sign( from.key );
      m.encrypt( to.key );
//      ilog( "send_message ${m}", ("m",m) );
      my->store_inventory( m, channel_id( chat_proto)  );
      my->broadcast_inventory( channel_id(chat_proto) );
   }

   void bitchat::request_contact( const bitchat_identity& id, const std::string& msg )
   {
   }

   void bitchat::broadcast_away( const bitchat_identity&, const std::string& msg )
   {
   }

   void bitchat::broadcast_signoff( const bitchat_identity& id )
   {
   }

   void bitchat::broadcast_signon( const bitchat_identity& id )
   {
   }
   void               bitchat::add_identity( const bitchat_identity& id )
   {
       // TODO: perform some sanity checks to prevent over-riding idents??
       my->idents[id.label] = id;
       my->key_to_idents[id.key.get_public_key().serialize()] = id.label;
   }

   bitchat_identity   bitchat::get_identity( const std::string& label )
   {
       auto itr = my->idents.find(label);
       if( itr == my->idents.end() )
       {
         FC_THROW_EXCEPTION( key_not_found_exception, 
                    "Unable to find identity with label ${label}", ("label", label) );
       }
       return itr->second;
   }

   void               bitchat::add_contact( const bitchat_contact& c )
   {
       my->contacts[c.label] = c;
       my->key_to_contact[c.key.serialize()] = c.label;
   }

   bitchat_contact    bitchat::get_contact( const std::string& label )
   {
       auto itr = my->contacts.find(label);
       if( itr == my->contacts.end() )
       {
         FC_THROW_EXCEPTION( key_not_found_exception, 
                    "Unable to find contact with label ${label}", ("label", label) );
       }
       return itr->second;
   }

   std::string to_bitchat_address( const fc::ecc::public_key& e )
   {
      auto dat = e.serialize();
      return fc::to_base58( dat.data, sizeof(dat) );
   }

   fc::ecc::public_key from_bitchat_address( const std::string& s )
   {
      auto dat = fc::from_base58( s );
      fc::ecc::public_key_data d;
      FC_ASSERT( dat.size() == sizeof(fc::ecc::public_key_data) );
      memcpy( d.data, dat.data(), dat.size() );
      return fc::ecc::public_key(d);
   }

} // namespace bts
