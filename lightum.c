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

#define VERSION "1.5"

void usage() {
	fprintf(stderr, "lightum v%s - (c)2011 Pau Oliva Fora <pof@eslack.org>\n",VERSION);
	fprintf(stderr, "Usage:  lightum [-m value] [-p value] [-i value] [-x] [-s] [-f] [-v]\n");
	fprintf(stderr, "     -m 4..255 : maximum brightness value in auto mode (default=255)\n");
	fprintf(stderr, "     -n 0..3   : minimum brightness value in auto mode (default=0)\n");
	fprintf(stderr, "     -p num    : number of miliseconds between light sensor polls (default=800)\n");
	fprintf(stderr, "     -i num    : power off keyboard light on session idle seconds (0 to disable)\n");
	fprintf(stderr, "     -x        : manual mode (will honor the brightness value set with Fn keys)\n");
	fprintf(stderr, "     -s        : power off keyboard light when screen saver is active\n");
	fprintf(stderr, "     -f        : run in foreground (do not daemonize)\n");
	fprintf(stderr, "     -v        : verbose mode, useful for debugging with -f\n");
	exit(1);
}

int main(int argc, char *argv[]) {

	int screensaver, c, prev=-1;
	int light=0, brightness=255;
	int foreground=0, verbose=0;
	int restore, restoreflag=0;
	float idletime=0;
	pid_t pid;
	conf_data conf;
	Display *display = NULL;

	// set defaults
	conf.manualmode=0;
	conf.queryscreensaver=0;
	conf.maxbrightness=255;
	conf.minbrightness=0;
	conf.polltime=800;
	conf.idleoff=10;

	// overwrite defaults with config file
	conf = config_parse();

	// overwrite config file with command line arguments
	while ((c = getopt(argc, argv, "hxsvfm:n:p:i:?")) != EOF) {
		switch(c) {
			case 'h':
				usage();
				break;
			case 'x':
				conf.manualmode=1;
				break;
			case 's':
				conf.queryscreensaver=1;
				break;
			case 'f':
				foreground=1;
				break;
			case 'v':
				verbose=1;
				break;
			case 'm':
				conf.maxbrightness=atoi(optarg);
				break;
			case 'n':
				conf.minbrightness=atoi(optarg);
				break;
			case 'p':
				conf.polltime=atoi(optarg);
				break;
			case 'i':
				conf.idleoff=atoi(optarg);
				break;
			default:
				usage();
				break;
		}
	}

	if (verbose) printf("CONFIG:\n\tmanualmode: %d\n",conf.manualmode);
	if (verbose) printf("\tqueryscreensaver: %d\n",conf.queryscreensaver);
	if (verbose) printf("\tmaxbrightness: %d\n",conf.maxbrightness);
	if (verbose) printf("\tminbrightness: %d\n",conf.minbrightness);
	if (verbose) printf("\tpolltime: %d\n",conf.polltime);
	if (verbose) printf("\tidleoff: %d\n\n",conf.idleoff);

	// make sure all config values are correct
	if (conf.manualmode < 0 || conf.manualmode > 1) usage();
	if (conf.queryscreensaver < 0 || conf.queryscreensaver > 1) usage();
	if (conf.maxbrightness < 4 || conf.maxbrightness > 255) usage();
	if (conf.minbrightness < 0 || conf.minbrightness > 3) usage();
	if (conf.polltime < 1 || conf.polltime > 100000) usage();
	if (conf.idleoff < 0 || conf.idleoff > 86400) usage();

	if (conf.manualmode) printf("lightum v%s running in manual mode ", VERSION);
	else printf("lightum v%s running in auto mode ", VERSION);
	fflush(stdout);

	if (!foreground) {
		if ((pid = fork()) < 0) exit(1);
		else if (pid != 0) exit(0);
		/* daemon running here */
		setsid();
		chdir("/");
		umask(0);
		printf("forked into background\n");
	} else printf("\n");

	/* in manual mode, start with current brightness value */
	if (conf.manualmode) restore=get_keyboard_brightness_value();

	if (conf.idleoff != 0) {
		display = XOpenDisplay(NULL);
		if (display == NULL) {
			printf("Failed to open display\n");
			exit(1);
		}
	}

	signal_installer();

	while(1) {

		if (!conf.manualmode) {
			light=get_light_sensor_value();
			if (verbose) printf("light_sensor: %d ",light);
		}

		if (conf.idleoff != 0) {
			idletime=get_session_idle_time(display);
			if (verbose) printf("idle_time: %f ",idletime);
		}

		if (!conf.manualmode) {
			brightness=calculate_keyboard_brightness_value(light, conf.maxbrightness);
			if (verbose) printf("auto mode ");
		} else {
			if (verbose) printf("manual mode ");
			if (idletime > conf.idleoff) {
				if (restoreflag==0) {
					if (verbose) printf("restoreflag ");
					restore=get_keyboard_brightness_value();
					restoreflag=1;
				}
				brightness=conf.minbrightness;
			} else {
				printf("brightness restored ");
				brightness=restore;
				restoreflag=0;
			}
		}

		if ((conf.idleoff != 0) && (idletime > conf.idleoff)) {
			brightness=conf.minbrightness;
		}

		if (conf.queryscreensaver) {
			screensaver=get_screensaver_active();
			if (verbose) printf("screensaver: %d ",screensaver);
		 	if (screensaver) brightness=0;
		}

		if (verbose) printf("maxbrightness: %d ",conf.maxbrightness);
		if (verbose) printf("brightness: %d",brightness);

		if (brightness!=prev) {
			if (!conf.manualmode) {
				restore=get_keyboard_brightness_value();
				if (verbose) printf(" current: %d\n",restore);
				if ((restore != prev) && (restoreflag)) {
					/* make sure maxbrightness is never <4 */
					if (restore < 4) conf.maxbrightness=4;
					else conf.maxbrightness=restore;
					if (verbose) printf("-> Detected user brightness change, setting maxbrightness to %d\n",restore);
					brightness=calculate_keyboard_brightness_value(light, conf.maxbrightness);
					prev=restore;
				}
				restoreflag=1;
			}
			if (verbose) printf ("-> set keyboard brightness: %d -> %d\n",prev,brightness);
			fading(prev,brightness);
			prev=brightness;
		}
		if (verbose) printf("\n");

		usleep(conf.polltime*1000);
	}

	//if (conf.idleoff != 0) XCloseDisplay(display);
	exit(0);
}
