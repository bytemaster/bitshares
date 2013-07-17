#pragma once
#include <fc/io/varint.hpp>
#include <string.h>

namespace bts { namespace network {

  enum channel_proto 
  {
     null_proto = 0, ///< used for null / padding  
     peer_proto = 1, ///< used for discovery / configuration
     chat_proto = 2, ///< used for chat messages
     mail_proto = 3  ///< used for email messages
  };

  struct channel_id
  {
     channel_id( channel_proto p = null_proto, uint32_t c = 0 )
     :proto(p),chan(c){}
     /** 
      *   @brief defines the protocol being being used on a
      *          channel.
      */
     fc::unsigned_int proto;
     /**
      *    @brief identifies the channel number
      */
     fc::unsigned_int chan;

     friend bool operator == ( const channel_id& a, const channel_id& b )
     {
      return a.id() == b.id();
     }
     friend bool operator != ( const channel_id& a, const channel_id& b )
     {
      return a.id() != b.id();
     }
     friend bool operator < ( const channel_id& a, const channel_id& b )
     {
      return a.id() < b.id();
     }

     uint64_t id()const
     {
        return (uint64_t(proto.value) << 32) | chan.value;
     }
  };
}}  // namespace bts::network


#include <fc/reflect/reflect.hpp>
FC_REFLECT_ENUM( bts::network::channel_proto, 
    (null_proto)
    (peer_proto)
    (chat_proto)
    (mail_proto)
)

FC_REFLECT( bts::network::channel_id, 
    (proto)
    (chan) 
    )



