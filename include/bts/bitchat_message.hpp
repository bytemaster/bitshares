#pragma once
#include <fc/crypto/elliptic.hpp>
#include <fc/time.hpp>
#include <fc/optional.hpp>
#include <bts/mini_pow.hpp>
#include <bts/network/channel_id.hpp>

namespace bts {
    /**
     *  Define's the structure of messages broadcast on the
     *  bitchat_message network.     
     */
    struct bitchat_message
    {
        struct content
        {
           network::channel_id  reply_channel;
           std::vector<char>    body;
        };

        /** always serializes to a multiple of 8 bytes, this
         * data is encrypted via blowfish 
         */
        struct signed_content : public content
        {
          fc::time_point                    timestamp;// certifies signature
          fc::ecc::compact_signature        from_sig; // applied to decrypted data
          fc::optional<fc::ecc::public_key> from; // not serialized
        };

        bitchat_message();

        bitchat_message&            body( const std::vector<char>& dat  );
        bitchat_message&            reply_channel( const network::channel_id& c );

        /**
         *  @brief the proof of work must be quick to verify relative to the
         *         time it took to generate because all nodes must validate 
         *         many messages.  
         */
        bitchat_message&            do_proof_work( const mini_pow& tar_per_kb );
        bitchat_message&            sign( const fc::ecc::private_key& from );

        void                        encrypt( const fc::ecc::public_key& to );
        mini_pow                    calculate_id()const;
        bool                        decrypt( const fc::ecc::private_key& k );
        bool                        is_encrypted()const;
        const signed_content&       get_content()const;     
        void                        set_content(const signed_content& c);

        uint16_t                    nonce; ///< increment timestamp after 63K tests
        fc::time_point              timestamp;
        fc::ecc::public_key         dh_key;
        uint32_t                    dh_check;
        std::vector<char>           data;

        private:
        /** transient state of data vector */
        bool                          decrypted;
        fc::optional<signed_content>  private_content;

    };

}  // namespace btc

#include <fc/reflect/reflect.hpp>
FC_REFLECT( bts::bitchat_message::content,        (reply_channel)(body) )
FC_REFLECT_DERIVED( bts::bitchat_message::signed_content, (bts::bitchat_message::content), (timestamp)(from_sig) )
FC_REFLECT( bts::bitchat_message,  (nonce)(timestamp)(dh_key)(dh_check)(data) )
