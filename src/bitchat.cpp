#include <bts/bitchat.hpp>
#include <bts/network/channel_pow_stats.hpp>
#include <unordered_map>
#include <map>




namespace bts 
{
   enum message_types 
   {
      data_msg      = 1, ///< a message encrypted to unknown receip
      inventory_msg = 2, ///< publishes known invintory
      get_inventory = 3, ///< publishes known invintory
      get_msg       = 4  ///< sent to request an inventory item
   };

   using namespace network;
   namespace detail 
   {
     class bitchat_channel;
     typedef std::shared_ptr<bitchat_channel> bitchat_channel_ptr;

     class bitchat_impl 
     {
        public:
          bts::network::server_ptr netw;

          std::unordered_map<uint64_t, bitchat_channel_ptr> chans;
          std::vector<bitchat_contact>                        _contacts;
          std::vector<bitchat_identity>                       _idents;
          std::map<uint64_t, channel_pow_stats>               _chan_stats;


          void handle_message( const connection_ptr& con, const message& m )
          {
             FC_ASSERT( m.data.size() > 0 );




            fc::datastream<const char*>  ds(m.data.data(), m.data.size() );
            fc::unsigned_int msg_type;
            fc::raw::unpack( ds, msg_type );

            if( msg_type.value == data_msg )
            {
               bitchat_message msg;
               fc::raw::unpack( ds, msg );

               // TODO validate difficulty...
               auto msg_id = msg.calculate_id();

               try_decrypt_msg( con, m.chan, msg );
            }
            else
            {
              FC_THROW_EXCEPTION( exception, "Unknown message type ${i}", ("i", msg_type) );
            }

          }
          void try_decrypt_msg( const connection_ptr& con, const channel_id& cid, bitchat_message& m )
          {

             // check for messages to me
             for( auto itr = _idents.begin(); itr != _idents.end(); ++itr )
             {
                if( m.decrypt( itr->key ) )
                {
                   handle_data_msg( con, cid, m );
                   return;
                }
             }
             // check for messages from others
             for( auto itr = _contacts.begin(); itr != _contacts.end(); ++itr )
             {

             }

             // check for messages from myself...?
             for( auto itr = _idents.begin(); itr != _idents.end(); ++itr )
             {

             }
          }

          void handle_data_msg( const connection_ptr& con, const channel_id& id, const bitchat_message& m )
          {
             FC_ASSERT( !m.is_encrypted() );


          }
     };

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
                self->handle_message( con, m );
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
   :my()
   { 
      my.reset( new bts::detail::bitchat_impl());
      my->netw = s;
      
      auto chan = std::make_shared<bts::detail::bitchat_channel>(my.get());
      my->netw->subscribe_to_channel( channel_id(network::chat_proto,0), chan );
   }

   bitchat::~bitchat()
   {
      for( auto itr = my->chans.begin(); itr != my->chans.end(); ++itr )
      {
        itr->second->self = nullptr;
      }
   }

  /*
   void bitchat::set_channels( const std::vector<uint32_t>& chans )
   {
   }
  */

   void bitchat::set_keys( const std::vector<fc::ecc::private_key>& mykeys )
   {
   }

   void bitchat::set_delegate( bitchat_delegate* d )
   {
   }

   void bitchat::send_message( const std::string& msg, const std::string& contact_name )
   {
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

} // namespace bts
