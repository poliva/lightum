/*
 *   MacBook automatic keyboard brightness daemon
 *   Copyright 2011 Pau Oliva Fora <pof@eslack.org>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#include "lightum.h"

#define VERSION "1.1"

void usage() {
	fprintf(stderr, "lightum v%s - (c)2011 Pau Oliva Fora <pof@eslack.org>\n",VERSION);
	fprintf(stderr, "Usage:  lightum [-m value] [-p value] [-i value] [-v] [-f]\n");
	fprintf(stderr, "        -m 1..255 : maximum brightness value between 1 and 255 (default=255)\n");
	fprintf(stderr, "        -p num    : number of miliseconds between light sensor polls (default=800)\n");
	fprintf(stderr, "        -i num    : power off keyboard light on session idle seconds (default=10, 0 to disable)\n");
	fprintf(stderr, "        -s        : power off keyboard light when screen saver is active\n");
	fprintf(stderr, "        -f        : run in foreground (do not daemonize)\n");
	fprintf(stderr, "        -v        : verbose mode, useful for debugging with -f\n");
	exit(1);
}

int main(int argc, char *argv[]) {

	int screensaver,queryscreensaver=0,c,light,brightness,maxlight=255,foreground=0,prev=-1,polltime=800,verbose=0,idleoff=10;
	float idletime=0;
	pid_t pid;

	while ((c = getopt(argc, argv, "svfm:p:i:?")) != EOF) {
		switch(c) {
			case 's':
				queryscreensaver=1;
				break;
			case 'f':
				foreground=1;
				break;
			case 'v':
				verbose=1;
				break;
			case 'm':
				maxlight=atoi(optarg);
				if (maxlight < 1 || maxlight > 255) usage();
				break;
			case 'p':
				polltime=atoi(optarg);
				if (polltime < 1 || polltime > 60) usage();
				break;
			case 'i':
				idleoff=atoi(optarg);
				if (idleoff < 0 || idleoff > 86400) usage();
				break;
			default:
				usage();
				break;
		}
	}

	if (!foreground) {
		if ((pid = fork()) < 0) exit(1);
		else if (pid != 0) exit(0);
		/* daemon running here */
		setsid();
		chdir("/");
		umask(0);
	}

	while(1) {

		light=get_light_sensor_value();
		if (idleoff != 0) idletime=get_session_idle_time();

		if (verbose) printf("light_sensor: %d - idle_time: %f\n",light,idletime);

		if (queryscreensaver) {
			screensaver=get_screensaver_active();
		 	if (screensaver)
				brightness=0;
		}
		else
			brightness=calculate_keyboard_brightness_value(light, maxlight);

		if ((idleoff != 0) && (idletime > idleoff))
			brightness=0;

		if (brightness!=prev) {
			prev=brightness;
			if (verbose) printf ("-> set keyboard brightness: %d\n",brightness);
			set_keyboard_brightness_value(brightness);
		}

		usleep(polltime*1000);
	}
	exit(0);
}
