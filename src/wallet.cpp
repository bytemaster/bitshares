#include "wallet.hpp"

struct private_wallet_key
{
   fc::string            addr;
   fc::ecc::public_key   pub_key;
   fc::ecc::private_key  priv_key;
};
FC_REFLECT( public_wallet_key, (address)(pub_key)(priv_key)(used) )

namespace detail
{
  class wallet_impl
  {
     public:
        fc::path                         _wallet_file;   // file where wallet is stored.
        std::vector<address>             _addresses;     // all used addresses
        std::vector<private_wallet_key>  _private_keys;  // public/private keys
        std::unordered_map<address,int>  _address_index; // maps to index in private_keys
  };
}


wallet::wallet()
:my( new detail::wallet_impl() )
{
}

wallet::~wallet()
{
}

void wallet::save( const fc::path& walletdat, const std::string& password )
{
}

bool wallet::is_locked()const
{
}

/** nulls all private keys */
void wallet::lock()
{
}

/** decrypts and reloads all private keys using password */
void wallet::unlock( const std::string& password )
{
}

/** encrypts the wallet with the new password, must be unlocked()
 * to call this method.
 */
void wallet::change_password( const std::string& old_password, 
                              const std::string& new_password )
{
   unlock( old_password );
}

/**
 *  Pre-generates num addresses.
 */
void wallet::reserve( uint32_t num )
{
}

/** Gets the next new address */
address                    wallet::get_new_address()
{
}

std::vector<address>       wallet::get_addresses()const
{
}

fc::ecc::compact_signature wallet::sign( const fc::sha256& digest, const address& a )
{

}
