#include <bts/bitchat.hpp>
#include <bts/network/channel_pow_stats.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/crypto/base58.hpp>
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

    enum data_message_types
    {
       bitchat_text_msg = 1
    };


    struct text_message
    {
        enum msg_type_enum { type = bitchat_text_msg };
        text_message(const std::string& s = "")
        :msg_type( type ),msg(s){}
        fc::unsigned_int msg_type;
        std::string      msg;
    };

} // namespace bts
    FC_REFLECT( bts::text_message, (msg_type)(msg) );


namespace bts 
{

   using namespace network;
   namespace detail 
   {
     class bitchat_channel;
     typedef std::shared_ptr<bitchat_channel> bitchat_channel_ptr;

     class bitchat_impl 
     {
        public:
          bitchat_impl()
          :del(nullptr){}

          bts::network::server_ptr netw;

          std::unordered_map<uint64_t, bitchat_channel_ptr> chans;
          std::map<std::string,bitchat_contact>             contacts;
          std::map<std::string,bitchat_identity>            idents;
          std::map<uint64_t, channel_pow_stats>             _chan_stats;
          bitchat_delegate*                                 del;


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
              // auto msg_id = msg.calculate_id();

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
      my->netw->unsubscribe_from_channel( channel_id(network::chat_proto,0) );
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


   void bitchat::set_delegate( bitchat_delegate* d )
   {
     my->del = d;
   }

   void bitchat::send_message( const std::string& msg, const bitchat_contact& to, const bitchat_identity& from )
   {
      bitchat_message m;
      m.body( fc::raw::pack( text_message( msg ) ) );
      m.sign( from.key );
      m.encrypt( to.key );
      ilog( "send_message ${m}", ("m",m) );
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
