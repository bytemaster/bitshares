#include <fc/log/logger.hpp>
#include <fc/exception/exception.hpp>
#include <fc/filesystem.hpp>
#include <fc/io/json.hpp>
#include <fc/io/stdio.hpp>
#include <fc/io/fstream.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/thread/thread.hpp>
#include <bts/network/server.hpp>
#include <iostream>


struct bitshared_config
{
   bitshared_config()
   {
        server_config.port = 9876;
   }

   bts::network::server::config server_config;  
};

FC_REFLECT( bitshared_config, (server_config) )


int main( int argc, char** argv )
{
  try
  {
    if( argc != 2 )
    {
       fc::cerr<<"Usage "<<argv[0]<<" CONFIG\n"; 
       return -1;
    }
    fc::path cfile(argv[1]);

    if( !fc::exists( cfile ) )
    {
        auto default_str = fc::json::to_pretty_string( bitshared_config() );
        fc::ofstream out(cfile);
        out.write( default_str.c_str(), default_str.size() );
    }

    auto cfg = fc::json::from_file( cfile ).as<bitshared_config>();

    bts::network::server netw;
    netw.configure( cfg.server_config );


    fc::thread _cin("cin");
    std::string line;
    while( _cin.async([&](){ return !std::getline( std::cin, line ).eof(); } ).wait() ) 
    {
       if( line == "q" ) return 0;
    }



  } 
  catch ( fc::exception& e )
  {
    elog( "${e}", ("e",e.to_detail_string()) );
    return -1;
  }
  return 0;
}
