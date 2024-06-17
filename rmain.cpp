#include "Server.h"

int main( int argc, char* argv[] ) {
    if( argc <= 1 ) {
        printf( "usage: %s port_number\n", basename( argv[0] ) );
        return 1;
    }

    BloomFilter<string> IpBlack(10000);
    ifstream bloom("resources/IpSet/black.txt");
    IpBlack.Insert(bloom);

    IpTree IpWhite;
    ifstream trie("resources/IpSet/white.txt");
    IpWhite.Insert(trie);

    Log mlog = 

    int port = atoi(argv[1]);
    
    {
        Server webServer(port);
        webServer.run();
    }
}