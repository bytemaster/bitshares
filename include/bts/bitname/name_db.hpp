#pragma once
#include <bts/bitname/name_block.hpp>
#include <fc/filesystem.hpp>

namespace bts { namespace bitname {

  namespace detail { class name_db_impl; }

  /**
   *  Stores only the valid set of registered names.
   */
  class name_db
  {
      public:
        name_db();
        ~name_db();

        void open( const fc::path& dbdir, bool create = true );
        void close();

        void store( const name_reg_block& b );
        void store( const name_reg_trx& b );

        name_reg_trx   fetch_trx( const std::string& name );
        name_reg_trx   fetch_trx( const mini_pow& trx_id );
        name_reg_block fetch_block( const mini_pow& block_id );

      private:
        std::unique_ptr<detail::name_db_impl> my;
  };

} }
