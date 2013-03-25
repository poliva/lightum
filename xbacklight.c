#include <errno.h>
#include "xbacklight.h"

int get_screen_xbacklight_value() {
	FILE * f = popen( "xbacklight", "r" );
	if ( f == 0 ) {
		fprintf( stderr, "Could not execute\n" );
		return -1;
	}
	const int BUFSIZE = 1000;
	char buf[ BUFSIZE ];
	while( fgets( buf, BUFSIZE,  f ) ) {
		;
	}
	pclose( f );
	return atoi(buf);
}

int set_screen_xbacklight_value(int backlight) {

	char name[256];

	sprintf(name, "xbacklight -set %.3d\n", backlight);

	FILE * f = popen(name , "r" );
	if ( f == 0 ) {
		fprintf( stderr, "Could not execute\n" );
		return -1;
	}
	const int BUFSIZE = 1000;
	char buf[ BUFSIZE ];
        errno = 0;
	while( fgets( buf, BUFSIZE,  f ) ) {
		;
	}
        if (errno != 0)
            perror("xbacklight");
	pclose( f );

	return 1;
}
