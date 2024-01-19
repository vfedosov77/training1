#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include <string.h>
#include <fstream>

#include "PicturesProcessor.h"

using namespace std;

#define EOK 0
const int PORT = 4000,   // 9000 сменил на 51000, 9000 в Linux заняты
		  SINGLE_PORT = PORT,
		  FORK_PORT = PORT + 1,
		  FORK_LARGE_PORT = PORT + 2,
		  PREFORK_PORT = PORT + 3,
		  XINETD_PORT = PORT + 4, // 51004
		  THREAD_PORT = PORT + 5,
		  THREAD_POOL_PORT = PORT + 6,
		  PRETHREAD_PORT = PORT + 7,
		  QUEUE_PORT = PORT + 8;
const int MAXLINE = 1000000;
void errx( const char *msg, int err = EOK );  // ретранслятор тестовых пакетов TCP
void retrans( int sc );  // создание и подготовка прослушивающего сокета
int getsocket( in_port_t );
int debug = 0; 	 // уровень отладочного вывода сервера
			 // параметры строки запуска сервера
void setv( int argc, char *argv[] );

// диагностика ошибки ...
void errx( const char *msg, int err ) {
	perror( msg );
	if( err != EOK ) errno = err;
	exit( EXIT_FAILURE );
};

static char *data;

void onDataReceived(int size) {
	std::cout << "data received. size=" << size << std::endl;
	testMatch((unsigned char *)data, size);
}

// ретранслятор тестовых пакетов TCP

void retrans( int sc ) {
   int rc = read( sc, data, MAXLINE );
   if( rc > 0 ) {
	   onDataReceived(rc);
	  if ( rc < 0 ) perror( "write data failed" );
   } else if( rc < 0 ) {
	   perror( "read data failed" ); return;
   } else if( rc == 0 ) {
	   cout << "client closed connection" << endl; return;
   }
   return;
}

// создание и подготовка прослушивающего сокета
static struct sockaddr_in addr;
int getsocket(  in_port_t p ) {
   int ls;
   if( -1 == ( ls = socket( AF_INET, SOCK_STREAM, 0 ) ) )
	  errx( "create stream socket failed" );
   //if( setsockopt( ls, SOL_SOCKET, SO_REUSEADDR, &rc, sizeof( rc ) ) != 0 )
	//  errx( "set socket option failed" );
   memset( &addr, 0, sizeof( addr ) );
   addr.sin_family = AF_INET;
   addr.sin_port = htons( p );
   addr.sin_addr.s_addr = htonl( INADDR_ANY );
   if( bind( ls, (struct sockaddr*)&addr, sizeof( sockaddr ) ) != 0 )
	  errx( "bind socket address failed" );
   if( listen( ls, 25 ) != 0 ) errx( "put socket in listen state failed" );
		cout << "waiting on port " << p << " ..." << endl;
   return ls;
}

// уровень отладочного вывода сервера

void setv( int argc, char *argv[] ) {
   debug = ( argc > 1 && 0 == strcmp( argv[ 1 ], "-v" ) ) ? 1 : 0;
   if( debug ) cout << "verbose mode" << endl;
}

//void testProc();

int main() {
	//testProc();
   // return 0;
	data = new char[MAXLINE];
	srand(2500000);

   int sockfd = getsocket( SINGLE_PORT );
   //setv( argc, argv );

   while(1) {
		int s1 = accept(sockfd, NULL, NULL );
		if ( s1 < 0 )
			  errx( "accept failed" );

		int rc;
		int pos = 0;
		do {
			rc = recv( s1, data + pos, MAXLINE, 0 );
			pos += rc;
			if (rc < 0 )
				  errx( "recv failed" );
		} while (rc != 0);


		close(s1);

		onDataReceived(pos);
	}
   delete data;
   return EXIT_SUCCESS;
}
