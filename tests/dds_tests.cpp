#define BOOST_TEST_MODULE BitSharesTest
#include  <boost/test/unit_test.hpp>
#include <bts/dds/output_by_address_table.hpp>
#include <bts/wallet.hpp>
#include <fc/exception/exception.hpp>
#include <fc/log/logger.hpp>
#include <fc/reflect/variant.hpp>

using namespace bts;

BOOST_AUTO_TEST_CASE( output_by_address_table_load )
{
  try{
    output_by_address_table table;
    BOOST_CHECK_THROW( table.load( "out_by_addr.table" ), fc::file_not_found_exception );
    fc::temp_directory temp_dir;
    ilog( "temp dir: ${d}", ("d", temp_dir.path()  ) );
    table.load( temp_dir.path() / "out_by_addr.table", true );

    auto h = table.get_header();
    ilog( "header: ${h}", ("h", h ) );
    ilog( "digest: ${h}", ("h", table.calculate_hash() ) );

    uint32_t  fs =  table.get_free_slot();
    BOOST_CHECK( fs == 0 );

    bts::output_by_address_table::entry e;
    BOOST_CHECK_THROW( table.store( 1, e ), fc::out_of_range_exception );
    assert( e  == entry() );

    e.out_id.output_idx = 1;
    e.block_num         = 95;
    ilog( "e: ${e}", ("e",e));

    ilog( "really store store again" );
    table.store( fs, e );
    ilog( " store again" );
    try {
        BOOST_CHECK_THROW( table.store( fs, e ), fc::assert_exception ); // assert slot is not empty
    } catch ( fc::assert_exception& e ) {
        ilog( "EXPECTED ${e}", ("e", e.to_detail_string() ) );
    }
    table.clear( fs );
    ilog( "clear again?" );
    try {
        BOOST_CHECK_THROW( table.clear( fs ), fc::assert_exception );
    } catch ( fc::assert_exception& e ) {
        ilog( "EXPECTED ${e}", ("e", e.to_detail_string() ) );
    }
    ilog( "store after clear" );
    table.store( fs, e );

    table.save();

    ilog( "header: ${h}", ("h", table.get_header() ) );
    ilog( "digest: ${h}", ("h", table.calculate_hash() ) );
    
    e.block_num = 96;
    e.out_id.output_idx=3;
    uint32_t f = table.get_free_slot();
    BOOST_CHECK( f == 1 );
    table.store( f, e );

    ilog( "header: ${h}", ("h", table.get_header() ) );
    ilog( "digest: ${h}", ("h", table.calculate_hash() ) );

    table.clear(fs);

    ilog( "header: ${h}", ("h", table.get_header() ) );
    ilog( "digest: ${h}", ("h", table.calculate_hash() ) );

    e.out_id.output_idx = 1;
    table.store( fs, e );
    ilog( "header: ${h}", ("h", table.get_header() ) );
    ilog( "digest: ${h}", ("h", table.calculate_hash() ) );
    table.clear(fs);
    ilog( "header: ${h}", ("h", table.get_header() ) );
    ilog( "digest: ${h}", ("h", table.calculate_hash() ) );


  } catch ( fc::exception& e )
  {
    elog( "${e}", ("e", e.to_detail_string() ) );
    throw;
  }
}

BOOST_AUTO_TEST_CASE( wallet_test )
{
   bts::wallet w;
   w.set_seed( fc::sha256::hash( "helloworld", 10 ) );

   bts::wallet w2;
   w2.set_master_public_key( w.get_master_public_key() );

   for( uint32_t i = 0; i < 10; ++i )
   {
      BOOST_CHECK(  bts::address(w.get_public_key(i))                   == w2.get_public_key(i) );
      BOOST_CHECK(  bts::address(w.get_private_key(i).get_public_key()) == w2.get_public_key(i) );
   }
}
