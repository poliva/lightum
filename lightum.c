/*
 *   MacBook automatic light sensor daemon
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

#define VERSION "2.2.1"

#define MAXLV	15

int reloadconfig=0;

void usage(const char *error) {
	fprintf(stderr, "lightum v%s - (c)2011 Pau Oliva Fora <pof@eslack.org>\n",VERSION);
	fprintf(stderr, "%s",error);
	fprintf(stderr, "Usage:  lightum [OPTION]...\n");
	fprintf(stderr, "     -m 4..255 : maximum brightness value in auto mode (default=255)\n");
	fprintf(stderr, "     -n 0..3   : minimum brightness value in auto mode (default=0)\n");
	fprintf(stderr, "     -M 4..15  : maximum backlight value in auto mode (default=15)\n");
	fprintf(stderr, "     -N 1..3   : minimum backlight value in auto mode (default=1)\n");
	fprintf(stderr, "     -p num    : number of milliseconds between light sensor polls (default=300)\n");
	fprintf(stderr, "     -i num    : power off keyboard light on session idle seconds (0 to disable)\n");
	fprintf(stderr, "     -I num    : power off screen backlight on session idle seconds (0 to disable)\n");
	fprintf(stderr, "     -w num    : 1 manage brightness, 2 manage backlight, 3 both (default:3)\n");
	fprintf(stderr, "     -x        : manual mode (will honor the brightness value set with Fn keys)\n");
	fprintf(stderr, "     -u        : do not ignore brightness changes happening outside lightum\n");
	fprintf(stderr, "     -U        : ignore session information from ConsoleKit\n");
	fprintf(stderr, "     -s        : power off keyboard light when screen saver is active\n");
	fprintf(stderr, "     -f        : run in foreground (do not daemonize)\n");
	fprintf(stderr, "     -v        : verbose mode, useful for debugging with -f and -d\n");
	fprintf(stderr, "     -d num    : debug mode: 1 brightness, 2 backlight, 3 both\n");
	exit(1);
}

void check_config_values(conf_data conf) {
	if (conf.manualmode < 0 || conf.manualmode > 1) usage("ERROR: Wrong value in config variable 'manualmode'\n");
	if (conf.ignoreuser < 0 || conf.ignoreuser > 1) usage("ERROR: Wrong value in config variable 'ignoreuser'\n");
	if (conf.ignoresession < 0 || conf.ignoresession > 1) usage("ERROR: Wrong value in config variable 'ignoresession'\n");
	if (conf.queryscreensaver < 0 || conf.queryscreensaver > 1) usage("ERROR: Wrong value in config variable 'queryscreensaver'\n");
	if (conf.maxbrightness < 4 || conf.maxbrightness > 255) usage("ERROR: Wrong value in config variable 'maxbrightness'\n");
	if (conf.minbrightness < 0 || conf.minbrightness > 3) usage("ERROR: Wrong value in config variable 'minbrightness'\n");
	if (conf.maxbacklight < 4 || conf.maxbacklight > 15) usage("ERROR: Wrong value in config variable 'maxbacklight'\n");
	if (conf.minbacklight < 1 || conf.minbacklight > 3) usage("ERROR: Wrong value in config variable 'minbacklight'\n");
	if (conf.polltime < 1 || conf.polltime > 100000) usage("ERROR: Wrong value in config variable 'polltime'\n");
	if (conf.idleoff < 0 || conf.idleoff > 86400) usage("ERROR: Wrong value in config variable 'idleoff'\n");
	if (conf.screenidle < 0 || conf.screenidle > 86400) usage("ERROR: Wrong value in config variable 'screenidle'\n");
	if (conf.workmode < 1 || conf.workmode > 3) usage("ERROR: Wrong value in config variable 'workmode'\n");
}

int main(int argc, char *argv[]) {

	int screensaver=0, c, brightness_prev=-1, backlight_prev=-1;
	int light=0, brightness=255, backlight=100;
	int foreground=0, verbose=0, debug=0;
	int brightness_restore, backlight_restore, brightness_restoreflag=0, backlight_restoreflag=0;
	int res, dbus_backend=-1, tmp=-1;
	float idletime=0;
	pid_t pid;
	conf_data conf;
	Display *display = NULL;
        DBusGConnection *connection;
        DBusGProxy *proxy_manager;
        DBusGProxy *proxy_session;
	uid_t uid, euid;
	int light_aux=-1, light_avg=-1;
	int lightvalues[MAXLV] = {0};
	int countarray[255] = {0};
	unsigned int i,index=0;

	// make sure we are run as a regular user
	uid = getuid();
	euid = geteuid();
	if (uid == 0 || euid == 0) {
		fprintf(stderr, "lightum must NOT be run as root.\n");
		exit(1);
	}

	// overwrite defaults with config file
	conf = config_parse();

	// overwrite config file with command line arguments
	while ((c = getopt(argc, argv, "hxuUsvfm:n:M:N:p:I:i:d:w:?")) != EOF) {
		switch(c) {
			case 'h':
				usage("");
				break;
			case 'x':
				conf.manualmode=1;
				break;
			case 'u':
				conf.ignoreuser=0;
				break;
			case 'U':
				conf.ignoresession=1;
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
			case 'w':
				conf.workmode=atoi(optarg);
				break;
			default:
				usage("ERROR: Unknown OPTION\n");
				break;
		}
	}

	if (verbose) printf("CONFIG:\n\tmanualmode: %d\n",conf.manualmode);
	if (verbose) printf("\tignoreuser: %d\n",conf.ignoreuser);
	if (verbose) printf("\tignoresession: %d\n",conf.ignoresession);
	if (verbose) printf("\tworkmode: %d\n",conf.workmode);
	if (verbose) printf("\tqueryscreensaver: %d\n",conf.queryscreensaver);
	if (verbose) printf("\tmaxbrightness: %d\n",conf.maxbrightness);
	if (verbose) printf("\tminbrightness: %d\n",conf.minbrightness);
	if (verbose) printf("\tmaxbacklight: %d\n",conf.maxbacklight);
	if (verbose) printf("\tminbacklight: %d\n",conf.minbacklight);
	if (verbose) printf("\tpolltime: %d\n",conf.polltime);
	if (verbose) printf("\tidleoff: %d\n",conf.idleoff);
	if (verbose) printf("\tscreenidle: %d\n\n",conf.screenidle);

	// make sure all config values are correct
	check_config_values(conf);
	if (debug < 0 || debug > 3) usage("ERROR: Wrong value in config variable 'debug'\n");

	// if debug enabled, force verbose mode too
	if (debug > 0) verbose=1;

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

	/* create pid file */
	if (!create_pid_file()) exit(1);

	/* start with current brightness values */
	if (conf.workmode == 1 || conf.workmode == 3) {
		brightness_restore=get_keyboard_brightness_value();
	}

	/* start with current backlight values */
	if (conf.workmode == 2 || conf.workmode == 3) {
		backlight_restore=get_screen_backlight_value();

		// detect dbus backend: 0: gnome, 1: kde
		tmp = dbus_set_screen_backlight_value_gnome(acpi_to_dbus_backlight(backlight_restore));
		if (tmp == -1) {
			tmp = dbus_set_screen_backlight_value_kde(acpi_to_dbus_backlight(backlight_restore));
			if (tmp == -1) {
				fprintf (stderr, "Can't manage screen backlight on this system.\nPlease disable backlight with config option 'workmode='1' or command line switch '-w 1'.\nIf you believe this is an error, open a bug report: https://github.com/poliva/lightum/issues\n");
				exit (1);
			} else {
				dbus_backend=1;	
			}

		} else {
			dbus_backend=0;
		}
	}

	if (conf.idleoff != 0 || conf.screenidle != 0) {
		display = XOpenDisplay(NULL);
		if (display == NULL) {
			fprintf(stderr,"Failed to open display\n");
			exit(1);
		}
	}

	signal_installer();

	if (!conf.ignoresession) {
		connection = get_dbus_connection();
		proxy_manager = get_dbus_proxy_manager(connection);
		proxy_session = get_dbus_proxy_session(connection, proxy_manager);
	}

	// initialize the light values array
	for (i=0;i<256;i++)
		countarray[i]=0;
	if (!conf.manualmode) {
		light=get_light_sensor_value();
		for (i=0; i<MAXLV; i++) 
			lightvalues[i]=light;
		countarray[light]=MAXLV;
	} else {
		for (i=0; i<MAXLV; i++) 
			lightvalues[i]=0;
	}

	while(1) {

		if (reloadconfig) {
			conf = config_parse();
			if (verbose) printf("lightum: SIGUSR1 received, configuration reloaded\n");
			check_config_values(conf);
			reloadconfig=0;
		}

		if (!conf.ignoresession) {
			if (! get_session_active(proxy_session) ) {
				if (verbose) printf("lightum: user session not active, sleeping %d milliseconds.\nIf you believe this is an error, try running lightum with 'ignoresession=1' or '-U' command line switch.\n", conf.polltime);
				usleep(conf.polltime*1000);
				continue;
			}
		}

		if (!conf.manualmode) {
			light=get_light_sensor_value();
			if (verbose) printf("light_sensor: %d ",light);

			// to avoid backlight flickering when the light sensor flaps too fequently
			// between two values, we collect lighting values and use the most common
			// value of the collected values

			if (index == MAXLV) index=0;
			lightvalues[index] = light;

			// get the most repetitive value of lightvalues array
			for (i = 0; i < MAXLV; i++) {
				countarray[lightvalues[i]]++;
			}

			light_avg=-1;
			light_aux=-1;
			for (i = 0; i < 256; i ++) {
				if(countarray[i] > light_aux) {
					light_aux = countarray[i];
					light_avg=i;
				}
				countarray[i]=0;
			}

			light=light_avg;

			if (verbose) printf("light_avg: %d ",light);
			index++;
		}

		if (conf.idleoff != 0 || conf.screenidle != 0) {
			idletime=get_session_idle_time(display);
			if (verbose) printf("idle_time: %f ",idletime);
		}

		if (!conf.manualmode) {

			if (conf.workmode == 1 || conf.workmode == 3) brightness=calculate_keyboard_brightness_value(light, conf.maxbrightness, conf.minbrightness);
			if (conf.workmode == 2 || conf.workmode == 3) backlight=calculate_screen_backlight_value(light, conf.maxbacklight, conf.minbacklight);
			if (verbose) printf("auto mode ");

		} else {

			if (verbose) printf("manual mode ");
			if (conf.workmode == 1 || conf.workmode == 3) {
				if (!screensaver) {
					if (idletime > conf.idleoff) {
						if (brightness_restoreflag==0) {
							brightness_restore=get_keyboard_brightness_value();
							brightness_restoreflag=1;
							if (debug == 1 || debug == 3) printf("brightness_restoreflag(%d) ", brightness_restore);
						}
						brightness=conf.minbrightness;
					} else {
						brightness=brightness_restore;
						brightness_restoreflag=0;
						if (debug == 1 || debug == 3) printf("brightness_restored(%d) ", brightness_restore);
					}
				}
			}

			if (conf.workmode == 2 || conf.workmode == 3) {
				if (!screensaver) {
					if (idletime > conf.screenidle) {
						if (backlight_restoreflag==0) {
							backlight_restore=get_screen_backlight_value();
							backlight_restoreflag=1;
							if (debug == 2 || debug == 3) printf("backlight_restoreflag(%d) ", backlight_restore);
						}
						backlight=conf.minbacklight;
					} else {
						backlight=backlight_restore;
						backlight_restoreflag=0;
						if (debug == 2 || debug == 3) printf("backlight_restored(%d) ", backlight_restore);
					}
				}
			}

		}

		if (conf.workmode == 1 || conf.workmode == 3) {
			if ((conf.idleoff != 0) && (idletime > conf.idleoff)) {
				brightness=conf.minbrightness;
			}
		}

		if (conf.workmode == 2 || conf.workmode == 3) {
			if ((conf.screenidle != 0) && (idletime > conf.screenidle)) {
				backlight=conf.minbacklight;
			}
		}

		if (conf.queryscreensaver) {
			screensaver=get_screensaver_active();
			if (verbose) printf("screensaver: %d ",screensaver);
		 	if (screensaver) {
				brightness=0;
				backlight=conf.minbacklight;
			}
		}

		if (conf.workmode == 1 || conf.workmode == 3)
			if (verbose) printf("brightness: %d/%d ",brightness, conf.maxbrightness);

		if (conf.workmode == 2 || conf.workmode == 3)
			if (verbose) printf("backlight: %d/%d ",backlight, conf.maxbacklight);

		// keyboard brightness
		if (conf.workmode == 1 || conf.workmode == 3) {
			if (brightness!=brightness_prev) {
				if (!conf.manualmode) {
					brightness_restore=get_keyboard_brightness_value();
					if (brightness_restore < conf.minbrightness) brightness_restore=conf.minbrightness;
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
						brightness=calculate_keyboard_brightness_value(light, conf.maxbrightness, conf.minbrightness);
					}
					brightness_restoreflag=1;
				}
				if (debug == 1 || debug == 3) printf ("-> set keyboard brightness: %d -> %d\n",brightness_prev,brightness);
				fading(brightness_prev,brightness);
				usleep(1500);
				brightness=get_keyboard_brightness_value();
				brightness_prev=brightness;
			}
			if (!conf.manualmode) {
				tmp=get_keyboard_brightness_value();
				if (tmp!=brightness) {
					if (verbose) printf("-> Detected user brightness change, current brightness is set to %d\n", tmp);
					if (conf.ignoreuser) {
						if (debug == 1 || debug == 3) printf ("\n*** forcing brightness from %d to %d\n", tmp, brightness);
						fading(tmp,brightness);
					}
				}
			}
		}

		// screen backlight
		if (conf.workmode == 2 || conf.workmode == 3) {
			if (backlight!=backlight_prev) {
				if (!conf.manualmode) {
					backlight_restore=get_screen_backlight_value();
					if (backlight_restore < conf.minbacklight) backlight_restore=conf.minbacklight;
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
						backlight=calculate_screen_backlight_value(light, conf.maxbacklight, conf.minbacklight);
					}
					backlight_restoreflag=1;
				}
				if (debug == 2 || debug == 3) printf ("-> set screen backlight: %d -> %d\n",backlight_prev,backlight);
				backlight_fading(backlight_prev, backlight, dbus_backend);
				usleep(1500);
				backlight=get_screen_backlight_value();
				backlight_prev=backlight;
			}
			if (!conf.manualmode) {
				tmp=get_screen_backlight_value();
				if (tmp!=backlight) {
					if (verbose) printf("-> Detected user backlight change, current backlight is set to %d\n", tmp);
					if (conf.ignoreuser) {
						if (debug == 2 || debug == 3) printf ("\n*** forcing backlight from %d to %d\n", tmp, backlight);
						backlight_fading(tmp, backlight, dbus_backend);
					}
				}
			}
		}

		if (verbose) printf("\n");

		usleep(conf.polltime*1000);
	}

	// we should never reach here.
	//if (conf.idleoff != 0) XCloseDisplay(display);
	//dbus_g_connection_unref(connection);
	exit(1);
}
