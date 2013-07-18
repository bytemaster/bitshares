#pragma once
#include <fc/crypto/elliptic.hpp>
#include <fc/time.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/crypto/sha224.hpp>
#include <bts/network/server.hpp>
#include <bts/bitchat_message.hpp>
#include <memory>

namespace bts 
{
    namespace detail { class bitchat_impl; }

    struct bitchat_contact
    {
        std::string                        label;
        fc::ecc::public_key                key;
        ///< contact broadcasts here,  I should listen
        fc::ecc::private_key               recv_broadcast;
        std::vector<network::channel_id>   send_channels;
    };

    struct bitchat_contact_status
    {
        bitchat_contact       ident;
        std::string           away_message;
        bool                  online;
    };

    struct bitchat_identity 
    {
        std::string            label;
        fc::ecc::private_key   key;
        fc::ecc::private_key   broadcast; 
        std::vector<uint32_t>  recv_channels;
    };

    class bitchat_delegate
    {
       public:
         virtual ~bitchat_delegate(){}
         virtual void contact_signon( const bitchat_contact& id ){};
         virtual void contact_signoff( const bitchat_contact& id ){};
         virtual void contact_away( const bitchat_contact& id, 
                                   const std::string& msg ){};

         virtual void contact_request( const bitchat_contact& id, 
                                      const std::string& msg ){};

         virtual void received_message( const std::string& msg, 
                                        const bitchat_identity& to,
                                        const bitchat_contact_status& from ){};
    };

    std::string to_bitchat_address( const fc::ecc::public_key& e );
    fc::ecc::public_key from_bitchat_address( const std::string& s );


    /**
     *  Provides a simple chat client that hides both the
     *  sender and receiver of the message.  
     *
     */
    class bitchat 
    {
        public:
          struct config
          {
             std::vector<bitchat_identity> idents; // my id
             std::vector<bitchat_contact>  contacts; ///< information about my contacts
          };

          bitchat( const bts::network::server_ptr& s );
          ~bitchat();

       //   void set_channels( const std::vector<uint32_t>& chans );
       //   void set_keys( const std::vector<fc::ecc::private_key>& mykeys );
          void               add_identity( const bitchat_identity& id );
          bitchat_identity   get_identity( const std::string& label );

          void               add_contact( const bitchat_contact& c );
          bitchat_contact    get_contact( const std::string& label );

          void set_delegate( bitchat_delegate* d );

          void send_message( const std::string& msg, 
                             const bitchat_contact& to, 
                             const bitchat_identity& from );

          void request_contact( const bitchat_identity& id, const std::string& msg );

          void broadcast_away( const bitchat_identity& id, const std::string& msg );
          void broadcast_signoff( const bitchat_identity& id );
          void broadcast_signon( const bitchat_identity& id );

        private:
          std::unique_ptr<detail::bitchat_impl> my;
    };

} // bts

#include <fc/reflect/reflect.hpp>
FC_REFLECT( bts::bitchat::config,  (idents)(contacts) )
FC_REFLECT( bts::bitchat_identity, (label)(key)(broadcast)(recv_channels) )
FC_REFLECT( bts::bitchat_contact,  (label)(key)(recv_broadcast)(send_channels) )


