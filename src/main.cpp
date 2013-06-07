#include "api.hpp"
#include "miner.hpp"
#include "account.hpp"
#include "wallet.hpp"
#include <fc/io/stdio.hpp>
#include <fc/io/json.hpp>
#include <sstream>
#include <iostream>


struct bitshare_config
{
   fc::path    data_dir;
   float       mine_effort;

};

FC_REFLECT( bitshare_config, 
            (data_dir)
            (mine_effort) 
          ) 

int main( int argc, char** argv )
{
   try
   {
      if( argc < 2 )
      {
         fc::cerr<<"Usage: "<<argv[0]<<" config\n";
         return -1;
      }

      auto cfg = fc::json::from_file( fc::path( argv[1] ) ).as<bitshare_config>();
      
      // load the block-chain
      block_chain bc;
      bc.load( cfg.data_dir / "chain" );

      // load my account
      account default_acnt;
      default_acnt.load( cfg.data_dir / "accounts" / "default", account::create );

      // start the miner
      miner bc_miner( bc, default_acnt );
      bc_miner.start( cfg.mine_effort );

      // start the network
      // node  bsnode(bc);

      // start the CLI
      fc::string line;
      fc::getline( fc::cin, line );
      while( line != "quit" )
      {
          std::string sline(line);
          std::stringstream ss(sline);
          std::string cmd;

          ss >> cmd;

          if( cmd == "help" )
          {
             fc::cout<<"Commands:\n";
             fc::cout<<"mine effort\n";
             fc::cout<<"list_block_headers [START] [END]\n";
             fc::cout.flush();
          }
          else if( cmd == "mine" )
          {
              float effort = 0;
              ss >> effort;
              bc_miner.start( effort );
          }
          else if( cmd == "list_block_headers" )
          {

          }
          fc::getline( fc::cin, line );
      }
   } 
   catch ( fc::eof_exception& e )
   {
        // expected end of cin..
        return 0;
   }
   catch ( fc::exception& e )
   {
      fc::cerr<<e.to_detail_string()<<"\n";
      return -1;
   }
   return 0;
}
