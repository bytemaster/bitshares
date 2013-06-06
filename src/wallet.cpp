#include "wallet.hpp"
#include <fc/filesystem.hpp>

struct private_wallet_key
{
   fc::string            addr;
   fc::ecc::public_key   pub_key;
   fc::ecc::private_key  priv_key;
};
FC_REFLECT( public_wallet_key, (address)(pub_key)(priv_key)(used) )

struct wallet_format
{
   uint32_t                         version;
   fc::sha512                       checksum;
   std::vector<private_wallet_key>  private_keys;  // public/private keys
};

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
  bf.decrypt( file_data.data(), file_data.size() );

  auto check = fc::sha512::hash( file_data.data()+4+sizeof(fc::sha512), 
                                 file_data.size()-4-sizeof(fc::sha512 );

  if( memcmp( (char*)&check, file_data.data()+4, sizeof(check) ) != 0 )
  {
     FC_THROW_EXCEPTION( exception, "Error decrypting wallet" );
  }

  _private_keys = fc::raw::unpack<wallet_format>( _private_keys ).private_keys;

  my->_wallet_file = walletdat;
  my->_is_locked   = false;

  // TODO: index the private keys / addresses...
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
    bf.encrypt( vec.data(), vec.size() );

    fc::ofstream o( walletdat );
    o.write( vec.data(), vec.size();
}

bool wallet::is_locked()const
{
  return my->_is_locked();
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
      itr->priv_key = fc::ecc:private_key(); // clear it.
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
   save( my->_wallet_file + ".tmp", new_password );
   // backup the old wallet, until we can be sure the new one is moved 
   // into place.
   fc::rename( my->_wallet_file, my->my->_wallet_file + ".back" );
   // rename the tmp
   fc::rename( my->_wallet_file+".tmp", my->my->_wallet_file );
   // remove the backup
   fc::remove( my->my->_wallet_file + ".back" );
}

/**
 *  Pre-generates num addresses.
 */
void wallet::reserve( uint32_t num )
{
   if( is_locked() )
   {
        FC_THROW_EXCEPTION( exception, 
                            "Wallet must be unlocked to add new private keys" );
   }
   // TODO: should we force a 'save' after this?
}


std::vector<address>       wallet::get_addresses()const
{

}

fc::ecc::compact_signature wallet::sign( const fc::sha256& digest, const address& a )
{
   if( is_locked() )
   {
        FC_THROW_EXCEPTION( exception, 
                            "Wallet must be unlocked to sign" );
   }
}
