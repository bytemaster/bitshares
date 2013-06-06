#include <fc/io/raw.hpp>
#include "wallet.hpp"
#include <fc/filesystem.hpp>
#include <fc/exception/exception.hpp>
#include <fc/crypto/blowfish.hpp>
#include <fc/io/fstream.hpp>
#include <unordered_map>

struct private_wallet_key
{
   static private_wallet_key create()
   {
        private_wallet_key k;
        k.priv_key  = fc::ecc::private_key::generate();
        k.pub_key = k.priv_key.get_public_key();
        k.addr = address(k.pub_key);
        return k;
   }
   address               addr;
   fc::ecc::public_key   pub_key;
   fc::ecc::private_key  priv_key;
};
FC_REFLECT( private_wallet_key, (addr)(pub_key)(priv_key) )

struct wallet_format
{
   uint32_t                         version;
   fc::sha512                       checksum;
   std::vector<private_wallet_key>  private_keys;  // public/private keys
};
FC_REFLECT( wallet_format, (version)(checksum)(private_keys) )

namespace detail
{
  class wallet_impl
  {
     public:
        bool                             _is_locked;
        fc::path                         _wallet_file;   // file where wallet is stored.
        std::vector<address>             _addresses;     // all used addresses
        std::vector<private_wallet_key>  _private_keys;  // public/private keys
        std::unordered_map<address,int>  _address_index; // maps to index in private_keys
  };
}


wallet::wallet()
:my( new detail::wallet_impl() )
{
  my->_is_locked   = false;
}

wallet::~wallet()
{}

fc::path wallet::wallet_file()const
{ 
  return my->_wallet_file; 
}

void wallet::load( const fc::path& walletdat, const std::string& password )
{
   if( !fc::exists( walletdat ) ) 
   {
       FC_THROW_EXCEPTION( file_not_found_exception, "Unable to wallet ${file}", ("file",walletdat) );
   }
   fc::blowfish bf;
   auto h = fc::sha256::hash( password.c_str(), password.size() );
   bf.start( (unsigned char*)&h, sizeof(h) );
   
   std::vector<char> file_data( fc::file_size(walletdat) );
   fc::ifstream in(walletdat, fc::ifstream::binary );
   in.read( file_data.data(), file_data.size() );
   bf.decrypt( (unsigned char*)file_data.data(), file_data.size() );
   
   auto check = fc::sha512::hash( file_data.data()+4+sizeof(fc::sha512), 
                                  file_data.size()-4-sizeof(fc::sha512) );
   
   if( memcmp( (char*)&check, file_data.data()+4, sizeof(check) ) != 0 )
   {
      FC_THROW_EXCEPTION( exception, "Error decrypting wallet" );
   }
   
   my->_private_keys = fc::raw::unpack<wallet_format>( file_data ).private_keys;
   
   my->_wallet_file = walletdat;
   my->_is_locked   = false;

   my->_address_index.clear();
   
   // index the keys
   for( uint32_t i = 0; i < my->_private_keys.size(); ++i )
   {
      my->_address_index[my->_private_keys[i].addr] = i;
   }

   // TODO: should we perform a sanity check on the addresses?
}


void wallet::save( const fc::path& walletdat, const std::string& password )
{
    if( is_locked() )
    {
      FC_THROW_EXCEPTION( exception, "Cannot save wallet in locked state because "
                                     "no private keys are currently loaded" );
    }

    if( !fc::exists( walletdat.parent_path() ) )
    {
      FC_THROW_EXCEPTION( file_not_found_exception, 
                          "Invalid directory ${dir}", 
                          ("dir",walletdat.parent_path() ) );
    }

    fc::sha512::encoder enc;
    fc::raw::pack( enc, my->_private_keys );

    wallet_format wf;
    wf.version      = 0;
    wf.checksum     = enc.result();
    wf.private_keys = my->_private_keys;

    auto vec = fc::raw::pack( wf );
    
    fc::blowfish bf;
    auto h = fc::sha256::hash( password.c_str(), password.size() );
    bf.start( (unsigned char*)&h, sizeof(h) );
    bf.encrypt( (unsigned char*)vec.data(), vec.size() );

    fc::ofstream o( walletdat );
    o.write( vec.data(), vec.size() );
}

bool wallet::is_locked()const
{
  return my->_is_locked;
}

/** nulls all private keys 
 **/
void wallet::lock()
{
   // TODO: check for unsaved private keys and throw an exception!
   
   for( auto itr = my->_private_keys.begin();
             itr != my->_private_keys.end();
             ++itr )
   {
       itr->priv_key = fc::ecc::private_key(); // clear it.
   }
   my->_is_locked = true;
}

/** decrypts and reloads all private keys using password */
void wallet::unlock( const std::string& password )
{
   load( my->_wallet_file, password );
}

/** encrypts the wallet with the new password, must be unlocked()
 * to call this method.
 */
void wallet::change_password( const std::string& old_password, 
                              const std::string& new_password )
{
   if( is_locked() ) unlock( old_password );

   // save it to a temporary location first, we don't want any
   // failures to cause us to lose the current wallet information.
   save( my->_wallet_file.generic_string() + fc::string(".tmp"), new_password );
   // backup the old wallet, until we can be sure the new one is moved 
   // into place.
   fc::rename( my->_wallet_file, my->_wallet_file.generic_string() + ".back" );
   // rename the tmp
   fc::rename( my->_wallet_file.generic_string()+".tmp", my->_wallet_file );
   // remove the backup
   fc::remove( my->_wallet_file.generic_string() + ".back" );
}

/**
 *  Pre-generates num addresses and returns them.
 */
std::vector<address> wallet::reserve( uint32_t num )
{
   if( is_locked() )
   {
        FC_THROW_EXCEPTION( exception, 
                            "Wallet must be unlocked to add new private keys" );
   }
   if( num > 1000 )
   {
        FC_THROW_EXCEPTION( exception, "Attempt to reserve too-many private keys at once" );
   }
   my->_private_keys.reserve( my->_private_keys.size()+num );
   std::vector<address> r(num);
   for( uint32_t i = 0; i < num; ++i )
   {
      my->_private_keys.push_back( private_wallet_key::create() );
      r[i] = my->_private_keys.back().addr;
   }
   return r;
}


std::vector<address>       wallet::get_addresses()const
{
   std::vector<address> r;
   r.reserve( my->_private_keys.size() );
   for( uint32_t i = 0; i < my->_private_keys.size(); ++i )
   {
      r.push_back( my->_private_keys[i].addr);
   }
   return r;
}

fc::ecc::compact_signature wallet::sign( const fc::sha256& digest, const address& a )
{
   if( is_locked() )
   {
        FC_THROW_EXCEPTION( exception, 
                            "Wallet must be unlocked to sign" );
   }
   auto idx = my->_address_index.find(a);
   if( idx == my->_address_index.end() )
   {
        FC_THROW_EXCEPTION( exception, "Unknown address ${addr}", ("addr",a) );
   }
   return my->_private_keys[idx->second].priv_key.sign_compact( digest );
}
