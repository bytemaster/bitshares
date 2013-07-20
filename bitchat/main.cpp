#include <fc/log/logger.hpp>
#include <fc/exception/exception.hpp>
#include <fc/filesystem.hpp>
#include <fc/io/json.hpp>
#include <fc/io/stdio.hpp>
#include <fc/io/fstream.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/thread/thread.hpp>
#include <bts/network/server.hpp>
#include <bts/network/upnp.hpp>
#include <bts/network/get_public_ip.hpp>
#include <bts/bitmessage.hpp>
#include <bts/wallet.hpp>
#include <bts/bitchat.hpp>
#include <bts/db/peer_ram.hpp>
#include <iostream>
#include <sstream>

using namespace bts;

struct bitchat_config
{
   bitchat_config()
   {
        server_config.port = 9876;
        use_upnp           = true;
   }
   bts::network::server::config server_config;  
   bool                         use_upnp;
};

FC_REFLECT( bitchat_config, (server_config)(use_upnp) )


bitchat_config load_config( const fc::path& p )
{
    if( !fc::exists( p ) )
    {
        auto default_str = fc::json::to_pretty_string( bitchat_config() );
        fc::ofstream out(p);
        out.write( default_str.c_str(), default_str.size() );
    }
    return fc::json::from_file( p ).as<bitchat_config>();
}


/**
 *   This is an example stub that is a place holder until a
 *   GUI can be developed.
 */
class bitchat_client : public bts::bitchat_delegate
{
  public:
    bitchat_client( const bts::network::server_ptr& netw )
    {
       _chat = std::make_shared<bts::bitchat>( netw );
       _chat->set_delegate(this);
    }

    ~bitchat_client()
    {

    }

    virtual void friend_signon( const bts::bitchat_contact& id ){};
    virtual void friend_signoff( const bts::bitchat_contact& id ){};
    virtual void friend_away( const bts::bitchat_contact& id, 
                              const std::string& msg ){};

    virtual void friend_request( const bts::bitchat_contact& id, 
                                 const std::string& msg ){};

    virtual void received_message( const std::string& msg, 
                                   const bts::bitchat_identity& to,
                                   const bts::bitchat_contact_status& from )
    {
       std::cout<<from.ident.label <<": "<<msg<<"\n"; 
    }


    std::shared_ptr<bts::bitchat> _chat;
};

typedef std::shared_ptr<bitchat_client> bitchat_client_ptr;



int main( int argc, char** argv )
{
  try
  {
    if( argc != 2 )
    {
       fc::cerr<<"Usage "<<argv[0]<<" CONFIG\n"; 
       return -1;
    }
    fc::path cdir(argv[1]);
    fc::path cfile = cdir / "config.json";
    
    auto cfg = load_config( cfile );

    std::shared_ptr<bts::network::upnp_service> upnpserv;

    if( cfg.use_upnp )
    {
      upnpserv = std::make_shared<bts::network::upnp_service>();
      upnpserv->map_port( cfg.server_config.port );
    }

    auto peerdb   = std::make_shared<bts::db::peer_ram>();
    auto netw     = std::make_shared<bts::network::server>(peerdb);

    netw->configure( cfg.server_config );

    ilog( "starting bitchat client" );
    bitchat_client_ptr c = std::make_shared<bitchat_client>( netw );


    ilog( "connecting to peers" );
    fc::future<void> connect_complete = fc::async( [=]() { netw->connect_to_peers( 8 ); } );


    bitchat_identity cur_id;

    std::vector<bts::bitchat_identity> idents;
    std::vector<bts::bitchat_contact> contacts;
    if( fc::exists( cdir / "idents.json" ) )
    {
        idents = fc::json::from_file( cdir / "idents.json" ).as<std::vector<bts::bitchat_identity> >();
    }
    if( fc::exists( cdir / "contacts.json" ) )
    {
        contacts = fc::json::from_file( cdir / "contacts.json" ).as<std::vector<bts::bitchat_contact> >();
    }
    for( auto itr = idents.begin(); itr != idents.end(); ++itr )
    {
        c->_chat->add_identity( *itr );
        cur_id = *itr;
    }

    for( auto itr = contacts.begin(); itr != contacts.end(); ++itr )
    {
        c->_chat->add_contact( *itr );
    }

    std::cout<<cur_id.label<<"] ";
    fc::thread _cin("cin");
    std::string line;
    while( _cin.async([&](){ return !std::getline( std::cin, line ).eof(); } ).wait() ) 
    {
       std::string cmd;
       std::stringstream ss(line);
       ss >> cmd;
       if( cmd == "q" ) break;
       else if( cmd == "h" )
       {
          std::cout<<"   new_ident  LABEL\n";
          std::cout<<"   new_contact LABEL  PUBKEY\n";
          std::cout<<"   q - quit\n";
          std::cout<<"   send FRIEND  MESSAGE\n";
          std::cout<<"   id  LABEL  - switch id\n";
       }
       else if( cmd == "new_ident" )
       {
          bitchat_identity iden;
          ss >> iden.label;
          iden.key         = fc::ecc::private_key::generate();
          iden.broadcast   = fc::ecc::private_key::generate();

          ilog( "adding identity: ${ident}", ("ident",iden) );
          ilog( "ident pub key: ${pub}", ("pub", to_bitchat_address(iden.key.get_public_key()) ) );

          c->_chat->add_identity( iden ); 
          idents.push_back(iden);
          fc::json::save_to_file( idents, cdir / "idents.json" );
          cur_id = iden;
       }
       else if( cmd == "new_contact" )
       {
          bitchat_contact con;
          ss >> con.label;

          std::string pub;
          ss >> pub;

          con.key = from_bitchat_address( pub );
          ilog( "adding contact: ${contact}", ("contact",con) );
          c->_chat->add_contact( con );
          contacts.push_back(con);
          fc::json::save_to_file( contacts, cdir / "contacts.json" );

       }
       else if( cmd == "id" )
       {
          std::string lbl;
          ss >> lbl;
          cur_id = c->_chat->get_identity(lbl);
       }
       else if( cmd == "send" )
       {
          std::string cont_lbl;
          ss >> cont_lbl;

          auto cont = c->_chat->get_contact( cont_lbl );
          std::string msg;
          std::getline( ss, msg );

          c->_chat->send_message( msg, cont, cur_id );
       }
       std::cout<<cur_id.label<<"] ";
    } 
    ilog( "closing network connections" );
    netw->close();
    ilog( "waiting for connect to complete" );
    connect_complete.wait();
  }
  catch ( fc::exception& e )
  {
    elog( "${e}", ("e",e.to_detail_string()) );
    return -1;
  }
  return 0;
}
