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

#define VERSION "1.7"

void usage() {
	fprintf(stderr, "lightum v%s - (c)2011 Pau Oliva Fora <pof@eslack.org>\n",VERSION);
	fprintf(stderr, "Usage:  lightum [OPTION]...\n");
	fprintf(stderr, "     -m 4..255 : maximum brightness value in auto mode (default=255)\n");
	fprintf(stderr, "     -n 0..3   : minimum brightness value in auto mode (default=0)\n");
	fprintf(stderr, "     -M 4..15  : maximum backlight value in auto mode (default=100)\n");
	fprintf(stderr, "     -N 0..3   : minimum backlight value in auto mode (default=10)\n");
	fprintf(stderr, "     -p num    : number of milliseconds between light sensor polls (default=300)\n");
	fprintf(stderr, "     -i num    : power off keyboard light on session idle seconds (0 to disable)\n");
	fprintf(stderr, "     -I num    : power off screen backlight on session idle seconds (0 to disable)\n");
	fprintf(stderr, "     -x        : manual mode (will honor the brightness value set with Fn keys)\n");
	fprintf(stderr, "     -u        : do not ignore brightness changes happening outside lightum\n");
	fprintf(stderr, "     -s        : power off keyboard light when screen saver is active\n");
	fprintf(stderr, "     -f        : run in foreground (do not daemonize)\n");
	fprintf(stderr, "     -v        : verbose mode, useful for debugging with -f and -d\n");
	fprintf(stderr, "     -d num    : debug mode: 1 brightness, 2 backlight, 3 both\n");
	exit(1);
}

int main(int argc, char *argv[]) {

	int screensaver, c, brightness_prev=-1, backlight_prev=-1;
	int light=0, brightness=255, backlight=100;
	int foreground=0, verbose=0, debug=0;
	int brightness_restore, backlight_restore, brightness_restoreflag=0, backlight_restoreflag=0;
	int res;
	int tmp;
	float idletime=0;
	pid_t pid;
	conf_data conf;
	Display *display = NULL;

	// set defaults
	conf.manualmode=0;
	conf.ignoreuser=1;
	conf.queryscreensaver=0;
	conf.maxbrightness=255;
	conf.minbrightness=0;
	conf.maxbacklight=15;
	conf.minbacklight=1;
	conf.polltime=300;
	conf.idleoff=5;
	conf.screenidle=5;

	// overwrite defaults with config file
	conf = config_parse();

	// overwrite config file with command line arguments
	while ((c = getopt(argc, argv, "hxusvfm:n:M:N:p:I:i:d:?")) != EOF) {
		switch(c) {
			case 'h':
				usage();
				break;
			case 'x':
				conf.manualmode=1;
				break;
			case 'u':
				conf.ignoreuser=0;
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
			case 'd':
				debug=atoi(optarg);
				break;
			case 'm':
				conf.maxbrightness=atoi(optarg);
				break;
			case 'n':
				conf.minbrightness=atoi(optarg);
				break;
			case 'M':
				conf.maxbacklight=atoi(optarg);
				break;
			case 'N':
				conf.minbacklight=atoi(optarg);
				break;
			case 'p':
				conf.polltime=atoi(optarg);
				break;
			case 'i':
				conf.idleoff=atoi(optarg);
				break;
			case 'I':
				conf.screenidle=atoi(optarg);
				break;
			default:
				usage();
				break;
		}
	}

	if (verbose) printf("CONFIG:\n\tmanualmode: %d\n",conf.manualmode);
	if (verbose) printf("\tignoreuser: %d\n",conf.ignoreuser);
	if (verbose) printf("\tqueryscreensaver: %d\n",conf.queryscreensaver);
	if (verbose) printf("\tmaxbrightness: %d\n",conf.maxbrightness);
	if (verbose) printf("\tminbrightness: %d\n",conf.minbrightness);
	if (verbose) printf("\tmaxbacklight: %d\n",conf.maxbacklight);
	if (verbose) printf("\tminbacklight: %d\n",conf.minbacklight);
	if (verbose) printf("\tpolltime: %d\n",conf.polltime);
	if (verbose) printf("\tidleoff: %d\n",conf.idleoff);
	if (verbose) printf("\tscreenidle: %d\n\n",conf.screenidle);

	// make sure all config values are correct
	if (conf.manualmode < 0 || conf.manualmode > 1) usage();
	if (conf.ignoreuser < 0 || conf.ignoreuser > 1) usage();
	if (conf.queryscreensaver < 0 || conf.queryscreensaver > 1) usage();
	if (conf.maxbrightness < 4 || conf.maxbrightness > 255) usage();
	if (conf.minbrightness < 0 || conf.minbrightness > 3) usage();
	if (conf.maxbacklight < 4 || conf.maxbacklight > 15) usage();
	if (conf.minbacklight < 0 || conf.minbacklight > 3) usage();
	if (conf.polltime < 1 || conf.polltime > 100000) usage();
	if (conf.idleoff < 0 || conf.idleoff > 86400) usage();
	if (conf.screenidle < 0 || conf.screenidle > 86400) usage();
	if (debug < 0 || debug > 3) usage();

	if (conf.manualmode) printf("lightum v%s running in manual mode ", VERSION);
	else printf("lightum v%s running in auto mode ", VERSION);
	fflush(stdout);

	if (!foreground) {
		if ((pid = fork()) < 0) exit(1);
		else if (pid != 0) exit(0);
		/* daemon running here */
		setsid();
		res=chdir("/");
		if (res != 0) {
			perror("Could not chdir");
			exit(1);
		}
		umask(0);
		printf("forked into background\n");
	} else printf("\n");

	/* in manual mode, start with current brightness value */
	if (conf.manualmode) {
		brightness_restore=get_keyboard_brightness_value();
		backlight_restore=get_screen_backlight_value();
	}

	if (conf.idleoff != 0 || conf.screenidle != 0) {
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

		if (conf.idleoff != 0 || conf.screenidle != 0) {
			idletime=get_session_idle_time(display);
			if (verbose) printf("idle_time: %f ",idletime);
		}

		if (!conf.manualmode) {

			brightness=calculate_keyboard_brightness_value(light, conf.maxbrightness);
			backlight=calculate_screen_backlight_value(light, conf.maxbacklight);
			if (verbose) printf("auto mode ");

		} else {

			if (verbose) printf("manual mode ");
			if (idletime > conf.idleoff) {
				if (brightness_restoreflag==0) {
					if (debug == 1 || debug == 3) printf("brightness_restoreflag ");
					brightness_restore=get_keyboard_brightness_value();
					brightness_restoreflag=1;
				}
				brightness=conf.minbrightness;
			} else {
				if (debug == 1 || debug == 3) printf("brightness restored ");
				brightness=brightness_restore;
				brightness_restoreflag=0;
			}

			if (idletime > conf.screenidle) {
				if (backlight_restoreflag==0) {
					if (debug == 2 || debug == 3) printf("backlight_restoreflag ");
					backlight_restore=get_screen_backlight_value();
					backlight_restoreflag=1;
				}
				backlight=conf.minbacklight;
			} else {
				if (debug == 2 || debug == 3) printf("backlight restored ");
				backlight=backlight_restore;
				backlight_restoreflag=0;
			}

		}

		if ((conf.idleoff != 0) && (idletime > conf.idleoff)) {
			brightness=conf.minbrightness;
		}

		if ((conf.screenidle != 0) && (idletime > conf.screenidle)) {
			backlight=conf.minbacklight;
		}

		if (conf.queryscreensaver) {
			screensaver=get_screensaver_active();
			if (verbose) printf("screensaver: %d ",screensaver);
		 	if (screensaver) {
				brightness=0;
				backlight=conf.minbacklight;
			}
		}

		if (debug == 1 || debug == 3) printf("maxbrightness: %d ",conf.maxbrightness);
		if (verbose) printf("brightness: %d ",brightness);

		if (debug == 2 || debug == 3) printf("maxbacklight: %d ",conf.maxbacklight);
		if (verbose) printf("backlight: %d ",backlight);

		// keyboard brightness
		if (brightness!=brightness_prev) {
			if (!conf.manualmode) {
				brightness_restore=get_keyboard_brightness_value();
				if (debug == 1 || debug == 3) printf("\ncurrent brightness: %d\n",brightness_restore);
				if ((brightness_restore != brightness_prev) && (brightness_restoreflag)) {
					if (!conf.ignoreuser) {
						/* make sure maxbrightness is never <4 */
						if (brightness_restore < 4) conf.maxbrightness=4;
						else conf.maxbrightness=brightness_restore;
						if (verbose) printf("-> Detected user brightness change, setting maxbrightness to %d\n",brightness_restore);
						brightness_prev=brightness_restore;
					} else {
						if (verbose) printf("-> Ignoring user brightness change, wants to set maxbrightness to %d\n",brightness_restore);
					}
					brightness=calculate_keyboard_brightness_value(light, conf.maxbrightness);
				}
				brightness_restoreflag=1;
			}
			if (debug == 1 || debug == 3) printf ("-> set keyboard brightness: %d -> %d\n",brightness_prev,brightness);
			fading(brightness_prev,brightness);
			usleep(1500);
			tmp=get_keyboard_brightness_value();
			if (tmp!=brightness) {
				if (debug == 1 || debug == 3) printf ("\n*** forcing brightness from %d to %d\n", tmp, brightness);
				set_keyboard_brightness_value(brightness);
			}
			brightness_prev=brightness;
		}

		// screen backlight
		if (backlight!=backlight_prev) {
			if (!conf.manualmode) {
				backlight_restore=get_screen_backlight_value();
				if (debug == 2 || debug == 3) printf("\ncurrent backlight: %d\n",backlight_restore);
				if ((backlight_restore != backlight_prev) && (backlight_restoreflag)) {
					if (!conf.ignoreuser) {
						/* make sure maxbacklight is never <4 */
						if (backlight_restore < 4) conf.maxbacklight=4;
						else conf.maxbacklight=backlight_restore;
						if (verbose) printf("-> Detected user backlight change, setting maxbacklight to %d\n",backlight_restore);
						backlight_prev=backlight_restore;
					} else {
						if (verbose) printf("-> Ignoring user backlight change, wants to set maxbacklight to %d\n",backlight_restore);
					}
					backlight=calculate_screen_backlight_value(light, conf.maxbacklight);
				}
				backlight_restoreflag=1;
			}
			if (debug == 2 || debug == 3) printf ("-> set screen backlight: %d -> %d\n",backlight_prev,backlight);
			backlight_fading(backlight_prev,backlight);
			usleep(1500);
			backlight=get_screen_backlight_value();
			tmp=get_screen_backlight_value();
			if (tmp!=backlight) {
				if (debug == 2 || debug == 3) printf ("\n*** forcing backlight from %d to %d\n", tmp, backlight);
				set_screen_backlight_value(backlight);
			}
			backlight_prev=backlight;
		}

		if (verbose) printf("\n");

		usleep(conf.polltime*1000);
	}

	//if (conf.idleoff != 0) XCloseDisplay(display);
	exit(0);
}
