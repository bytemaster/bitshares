#pragma once
#include <bts/network/channel_id.hpp>
#include <fc/time.hpp>
#include <fc/array.hpp>
#include <fc/io/varint.hpp>
#include <fc/network/ip.hpp>
#include <fc/io/raw.hpp>

namespace bts { namespace network {

  enum message_code
  {
     generic      = 0,
     config       = 1,
     known_hosts  = 2,
     error_report = 3,
     mail         = 4, ///< bitmessage
  };

  struct message
  {
     channel_id        chan;
     std::vector<char> data;

     message(){}
     message( message&& m )
     :chan(m.chan),data( std::move(m.data) ){}
     message( const message& m )
     :chan(m.chan),data( m.data ){}

     template<typename T>
     message( const T& m, const channel_id cid = channel_id() ) 
     :chan(cid)
     {
        ilog( "message type: ${t} = ${v}", ("t", (message_code)T::type)("v", int(T::type)) );
        fc::datastream<size_t> ps;
        fc::raw::pack( ps, fc::unsigned_int( T::type ) );
        fc::raw::pack( ps, m );
        data.resize( ps.tellp() );

        fc::datastream<char*> ds( data.data(), data.size() );
        fc::raw::pack( ds, fc::unsigned_int( T::type ) );
        fc::raw::pack( ds, m );
        ilog( "message data size ${i}   ${data}" , ("i", data.size() )("data",data) );
     }
  };


  struct config_msg
  {
      enum type_enum { type = message_code::config };
      /** 
       *  A list of features supported by this client.
       */
      std::unordered_set<std::string> supported_features;
      std::set<channel_id>            subscribed_channels;
      fc::ip::endpoint                public_contact;

      uint64_t                        min_relay_fee;
  };

  struct host
  {
     fc::ip::endpoint ep;
     fc::time_point   last_com;
  };


  struct known_hosts_msg
  {
      enum type_enum { type = message_code::known_hosts };
      std::vector<host> hosts;
  };

  struct error_report_msg
  {
     enum type_enum { type = message_code::error_report };
     uint32_t     code;
     std::string  message;
  };

}} // bts::message

FC_REFLECT( bts::network::message,          (chan)(data) )

FC_REFLECT( bts::network::config_msg,       
    (supported_features)
    (subscribed_channels)
    (public_contact)
    (min_relay_fee) 
    )

FC_REFLECT_ENUM( bts::network::message_code,
  (generic)
  (config)
  (known_hosts)
  (error_report)
  (mail)
  )
FC_REFLECT( bts::network::host,             (ep)(last_com) )
FC_REFLECT( bts::network::known_hosts_msg,  (hosts) )
FC_REFLECT( bts::network::error_report_msg, (code)(message) )
