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
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define VERSION "1.0"

int get_light_sensor_value() {

	int fd;
	int i,n=0;
	char buf[10];
	char a_light[10];
	char *light_sensor="/sys/devices/platform/applesmc.768/light";

	/* read light sensor value */
	fd = open(light_sensor, O_RDONLY);
	if (fd < 0) {
		perror (light_sensor);
                fprintf (stderr, "Can't open %s\n",light_sensor);
		exit(1);
	}
        read(fd, buf, 9);
	buf[9]='\0';
	close(fd);

	/* convert light sensor string value to integer */
	for (i=0;i<sizeof(buf);i++) {
		if (buf[i]==',') break;
		if (buf[i]!='(') {
			a_light[n]=buf[i];
			n++;
		}
	}
	a_light[n]='\0';

	return atoi(a_light);
}

int calculate_keyboard_brightness_value(int light, int maxlight) {

	int brightness=0;
	
	if (light == 0 ) brightness=maxlight;
	else if (light < 8 ) brightness=maxlight*0.75;
	else if (light < 16 ) brightness=maxlight*0.66;
	else if (light < 32) brightness=maxlight*0.33;
	else if (light < 64) brightness=maxlight*0.16;
	else if (light < 128) brightness=maxlight*0.08;
	else if (light >= 128) brightness=0;

	return brightness;
}

int set_keyboard_brightness_value(int brightness) {

	char command[200];

	/* TODO: rewrite this using real dbus, instead of ugly system */
	sprintf(command,"dbus-send --system --dest=org.freedesktop.UPower /org/freedesktop/UPower/KbdBacklight org.freedesktop.UPower.KbdBacklight.SetBrightness int32:%d",brightness);
	return system(command);

}

void usage() {
	fprintf(stderr, "lightum v%s - (c)2011 Pau Oliva Fora <pof@eslack.org>\n",VERSION);
	fprintf(stderr, "Usage:  lightum [-m value] [-p value] [-f]\n");
	fprintf(stderr, "        -m 0..255 : maximum brightness value between 1 and 255 (default=255)\n");
	fprintf(stderr, "        -p num    : number of seconds between light sensor polls (default=8)\n");
	fprintf(stderr, "        -f        : run in foreground (do not daemonize)\n");
	fprintf(stderr, "        -v        : verbose mode, useful for debugging with -f\n");
	exit(1);
}

int main(int argc, char *argv[]) {

	int c,light,brightness,maxlight=255,foreground=0,prev=-1,polltime=8,verbose=0;
	pid_t pid;

	while ((c = getopt(argc, argv, "vfm:p:?")) != EOF) {
		switch(c) {
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
		if (verbose) printf("light_sensor: %d\n",light);
		brightness=calculate_keyboard_brightness_value(light, maxlight);

		if (brightness!=prev) {
			prev=brightness;
			if (verbose) printf ("-> set keyboard brightness: %d\n",brightness);
			set_keyboard_brightness_value(brightness);
		}

		sleep(polltime);
	}
	exit(0);
}
