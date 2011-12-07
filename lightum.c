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

#define VERSION "1.2"

void usage() {
	fprintf(stderr, "lightum v%s - (c)2011 Pau Oliva Fora <pof@eslack.org>\n",VERSION);
	fprintf(stderr, "Usage:  lightum [-m value] [-p value] [-i value] [-x] [-s] [-f] [-v]\n");
	fprintf(stderr, "        -m 1..255 : maximum brightness value between 1 and 255 (default=255)\n");
	fprintf(stderr, "        -p num    : number of miliseconds between light sensor polls (default=800)\n");
	fprintf(stderr, "        -i num    : power off keyboard light on session idle seconds (default=10, 0 to disable)\n");
	fprintf(stderr, "        -x        : manual mode (will honor the brightness value set with Fn keys)\n");
	fprintf(stderr, "        -s        : power off keyboard light when screen saver is active\n");
	fprintf(stderr, "        -f        : run in foreground (do not daemonize)\n");
	fprintf(stderr, "        -v        : verbose mode, useful for debugging with -f\n");
	exit(1);
}

int main(int argc, char *argv[]) {

	int manualmode=0, c, prev=-1;
	int screensaver, queryscreensaver=0;
	int light=0, brightness, maxbrightness=255;
	int foreground=0, polltime=800, verbose=0, idleoff=10;
	int restore, restoreflag=0;
	float idletime=0;
	pid_t pid;

	while ((c = getopt(argc, argv, "xsvfm:p:i:?")) != EOF) {
		switch(c) {
			case 'x':
				manualmode=1;
				break;
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
				maxbrightness=atoi(optarg);
				if (maxbrightness < 1 || maxbrightness > 255) usage();
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

		if (!manualmode) {
			light=get_light_sensor_value();
			if (verbose) printf("light_sensor: %d ",light);
		}

		if (idleoff != 0) {
			idletime=get_session_idle_time();
			if (verbose) printf("idle_time: %f ",idletime);
		}

		if (!manualmode) brightness=calculate_keyboard_brightness_value(light, maxbrightness);
		else {
			if (idletime > idleoff) {
				if (restoreflag==0) {
					restore=get_keyboard_brightness_value();
					restoreflag=1;
				}
				brightness=0;
			} else {
				brightness=restore;
				restoreflag=0;
			}
		}

		if ((idleoff != 0) && (idletime > idleoff)) {
			brightness=0;
		}

		if (queryscreensaver) {
			screensaver=get_screensaver_active();
		 	if (screensaver) brightness=0;
		}

		if (verbose) printf("brightness: %d\n",brightness);

		if (brightness!=prev) {
			if (verbose) printf ("-> set keyboard brightness: %d\n",brightness);
			fading(prev,brightness);
			prev=brightness;
		}

		usleep(polltime*1000);
	}

	exit(0);
}
