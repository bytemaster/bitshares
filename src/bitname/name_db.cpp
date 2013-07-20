#include <bts/bitname/name_db.hpp>
#include <leveldb/db.h>
#include <fc/io/raw.hpp>
#include <fc/reflect/variant.hpp>


namespace bts { namespace bitname {

    namespace ldb = leveldb;
   
    namespace detail 
    {
       class name_db_impl 
       {
          public:
            std::unique_ptr<ldb::DB> name_trx_db;
            std::unique_ptr<ldb::DB> name_index_db;
            std::unique_ptr<ldb::DB> name_block_db;


            std::unique_ptr<ldb::DB> init_db( const fc::path& dbfile, bool create )
            {
               ldb::Options opts;
               opts.create_if_missing = create;

               ldb::DB* ndb = nullptr;
               auto ntrxstat = ldb::DB::Open( opts, dbfile.generic_string().c_str(), &ndb );
               if( !ntrxstat.ok() )
               {
                   FC_THROW_EXCEPTION( exception, "Unable to open database ${db}\n\t${msg}", 
                        ("db",dbfile)
                        ("msg",ntrxstat.ToString()) 
                        );
               }

               return std::unique_ptr<ldb::DB>(ndb);
            }
       };
    }

    name_db::name_db()
    :my( new detail::name_db_impl() ){}
    name_db::~name_db(){}

    void name_db::open( const fc::path& db_dir, bool create )
    {
       if( !fc::exists( db_dir ) )
       {
         if( !create )
         {
            FC_THROW_EXCEPTION( file_not_found_exception, "Unable to open name database ${dir}", ("dir",db_dir) );
         }
         fc::create_directories( db_dir );
       }
       auto ntrx = db_dir / "name_trx";
       auto nidx = db_dir / "name_trx_idx";
       auto nblk = db_dir / "blocks";

       my->name_trx_db   = my->init_db( db_dir / "name_trx", create ); 
       my->name_index_db = my->init_db( db_dir / "name_trx_index", create ); 
       my->name_block_db = my->init_db( db_dir / "name_block", create ); 

    }
    void name_db::close()
    {
       my->name_trx_db.reset();
       my->name_index_db.reset();
       my->name_block_db.reset();
    }

    /**
     *  TODO: should I validate the pre conditions here?
     */
    void name_db::store( const name_reg_block& b )
    {
       const name_reg_trx& n = b;
       store(n);

       auto bvec = fc::raw::pack( b );
       auto pow_id = b.id();

       ldb::Slice block_data( bvec.data(), bvec.size() );
       ldb::Slice id(pow_id.data,sizeof(pow_id));

       auto status = my->name_block_db->Put( ldb::WriteOptions(), id, block_data );
       if( !status.ok() )
       {
          FC_THROW_EXCEPTION( exception, "Unable to store name block ${b}\n\t${msg}", 
                ("b",b)("msg",status.ToString() ) );
       }
    }

    /**
     *  TODO: should I validate the pre conditions here?
     */
    void name_db::store( const name_reg_trx& b )
    {
       auto vec = fc::raw::pack(b);
       auto tid  = b.id();
       ldb::Slice id(tid.data,sizeof(tid));
       ldb::Slice key = b.name;
       ldb::Slice val( vec.data(), vec.size() );

       auto status = my->name_trx_db->Put( ldb::WriteOptions(), id, key );
       if( !status.ok() )
       {
          FC_THROW_EXCEPTION( exception, "Unable to store name block ${b}\n\t${msg}", 
                ("b",b)("msg",status.ToString() ) );
       }

       status = my->name_index_db->Put( ldb::WriteOptions(), b.name, id );
       if( !status.ok() )
       {
          FC_THROW_EXCEPTION( exception, "Unable to store name block ${b}\n\t${msg}", 
                ("b",b)("msg",status.ToString() ) );
       }
    }

    name_reg_trx   name_db::fetch_trx( const std::string& name )
    {
       try {
          std::string id;
          auto status = my->name_index_db->Get( ldb::ReadOptions(), name, &id );
          if( !status.ok() )
          {
             FC_THROW_EXCEPTION( exception, "database error: ${msg}", ("msg", status.ToString() ) );
          }
          mini_pow pid;
          memcpy( pid.data, id.c_str(), id.size() );
          return fetch_trx(pid);

       } FC_RETHROW_EXCEPTIONS( warn, "Unable to fetch name registration for '${name}'", ("name",name) );
    }

    name_reg_trx   name_db::fetch_trx( const mini_pow& trx_id )
    {
       try {
          std::string value;
          ldb::Slice id( trx_id.data, sizeof(trx_id) );
          auto status = my->name_trx_db->Get( ldb::ReadOptions(), id, &value );
          if( !status.ok() )
          {
             FC_THROW_EXCEPTION( exception, "database error: ${msg}", ("msg", status.ToString() ) );
          }

          name_reg_trx trx;
          fc::datastream<const char*> ds( value.c_str(), value.size() );
          fc::raw::unpack( ds, trx );

          return trx;
       } FC_RETHROW_EXCEPTIONS( warn, "Unable to fetch name registration for id '${id}'", ("id",trx_id) );
    }

    name_reg_block name_db::fetch_block( const mini_pow& block_id )
    {
      try {
        std::string value;
        ldb::Slice id( block_id.data, sizeof(block_id) );
        auto status = my->name_trx_db->Get( ldb::ReadOptions(), id, &value );
        if( !status.ok() )
        {
           FC_THROW_EXCEPTION( exception, "database error: ${msg}", ("msg", status.ToString() ) );
        }

        name_reg_block blk;
        fc::datastream<const char*> ds( value.c_str(), value.size() );
        fc::raw::unpack( ds, blk );
        return blk;
      } FC_RETHROW_EXCEPTIONS( warn, "error fetching block id ${id}", ("id",block_id) );
    }

    void name_db::remove_trx( const mini_pow& trx_id )
    {
        try
        {
            auto nreg = fetch_trx( trx_id );
            ldb::Slice regname( nreg.name );
            ldb::Slice id( trx_id.data, sizeof(trx_id) );

            auto status = my->name_trx_db->Delete( ldb::WriteOptions(), id );
            if( !status.ok() )
            {
               FC_THROW_EXCEPTION( exception, "database error: ${msg}", ("msg", status.ToString() ) );
            }
            status = my->name_index_db->Delete( ldb::WriteOptions(), regname );
            if( !status.ok() )
            {
               FC_THROW_EXCEPTION( exception, "database error: ${msg}", ("msg", status.ToString() ) );
            }
        }FC_RETHROW_EXCEPTIONS( warn, "error removing name trx ${trx_id}", ("trx_id",trx_id) );
    }

    void name_db::remove_block( const mini_pow& block_id )
    {
        try
        {
            remove_trx( block_id );
            
            ldb::Slice id( block_id.data, sizeof(block_id) );

            auto status = my->name_block_db->Delete( ldb::WriteOptions(), id );
            if( !status.ok() )
            {
               FC_THROW_EXCEPTION( exception, "database error: ${msg}", ("msg", status.ToString() ) );
            }
        }FC_RETHROW_EXCEPTIONS( warn, "error removing name block ${block_id}", ("block_id",block_id) );
    }


} } // bts::bitname
