#pragma once
#include <fc/time.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/array.hpp>
#include <fc/network/ip.hpp>

namespace bts { namespace network {

  struct message
  {
     message()
     :size(0),type(0){}

     uint32_t          size;
     fc::time_point    timestamp;
     fc::array<char,4> chaincode;
     uint16_t          type;
     std::vector<char> data;
  };

  enum message_code
  {
     generic      = 0,
     config       = 1,
     known_hosts  = 2,
     error_report = 3,
     mail         = 4, ///< bitmessage
  };

  struct config_msg
  {
      enum type_enum { type = message_code::config };
      /** 
       *  A list of features supported by this client.
       */
      std::vector<std::string> supported_features;
      uint64_t                 min_relay_fee;
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

FC_REFLECT( bts::network::message,          (size)(timestamp)(chaincode)(type)(data) )
FC_REFLECT( bts::network::config_msg,       (supported_features)(min_relay_fee) )
FC_REFLECT( bts::network::host,             (ep)(last_com) )
FC_REFLECT( bts::network::known_hosts_msg,  (hosts) )
FC_REFLECT( bts::network::error_report_msg, (code)(message) )
