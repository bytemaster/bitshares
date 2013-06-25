#pragma once
#include "address.hpp"
#include <fc/filesystem.hpp>
#include <fc/crypto/elliptic.hpp>
#include <string>
#include <vector>

namespace detail { class wallet_impl; }

/**
 *  Manages a set of public/private keys that belong
 *  to the user.  The wallet stores a number of 
 *  pre-generated keys so that backups will remain
 *  valid for a certain number of transactions.
 *
 *  The wallet is stored in an encrypted form and only
 *  decrypted long enough to sign transactions.
 *
 *  The wallet will be stored on disk as an encrypted
 *  json file of the following form:
 *
 *  [ [address, pubkey, privkey, bool used], [address, pubkey, privkey] ]
 *
 *  The private keys are only kept in memory while the wallet is unlocked and
 *  no private key should *ever* leave the private implementation of wallet.cpp
 */
class wallet
{
  public:
     wallet();
     ~wallet();

     fc::path wallet_file()const;

     /**
      *   Imports private keys from another wallet into this wallet.
      */
     void import( const fc::path& other_wallet, const std::string& password );

     void load( const fc::path& walletdat, const std::string& password );

     /**
      *  Saves the wallet to the specified file encrypted with the given
      *  password.
      */
     void save( const fc::path& walletdat, const std::string& password );

     bool is_locked()const;

     /**
      *  Zeros all private keys until unlock is called.
      */
     void lock();

     /**
      *  @throws an exception if the wallet failed to unlock
      */
     void unlock( const std::string& password );


     /**
      *  Pre-generates num addresses and returns them.
      */
     std::vector<address> reserve( uint32_t num );

     /**
      *  @note this information will not be available until after
      *        the first time the wallet is unlocked()  An exception
      *        will be thrown if this method is called before 'unlock'.
      *
      *  Accounts should be used for managing addresses in the unlocked
      *  state.
      */
     std::vector<address>       get_addresses()const;

     /** signs the given digest with the specified address and throws an exception
      * if no private key is found for addr or if the wallet is locked.
      */
     fc::ecc::compact_signature sign( const fc::sha256& digest, const address& a );

  private:
     std::unique_ptr<detail::wallet_impl> my;
};
