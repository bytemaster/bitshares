#include <bts/proof_of_work.hpp>
#include <string.h>
#include <fc/io/stdio.hpp>

int main( int argc, char** argv )
{
   unsigned char* tmp     = new unsigned char[128*1024*1024+512];

   fc::sha256 in;
   if( argc >= 2 )
      in = fc::sha256::hash(argv[1],strlen(argv[1]));
   for( int i = 0; i < 19; ++i )
   {
     proof_of_work( in, tmp );
   }
   auto out = proof_of_work( in, tmp );

   fc::cerr<<fc::string(out)<<"\n";

   delete[] tmp;
   return -1;
}
