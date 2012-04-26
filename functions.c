/*
 *   MacBook automatic keyboard brightness daemon
 *   Copyright 2011 Pau Oliva Fora <pof@eslack.org>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>
#include <signal.h>

#include "lightum.h"

int get_keyboard_brightness_value() {

	int fd;
	char buf[5];
	char *kbd_backlight="/sys/devices/platform/applesmc.768/leds/smc::kbd_backlight/brightness";

	/* read light sensor value */
	fd = open(kbd_backlight, O_RDONLY);
	if (fd < 0) {
		perror (kbd_backlight);
                fprintf (stderr, "Can't open %s\n",kbd_backlight);
		exit(1);
	}
        read(fd, buf, 3);
	buf[4]='\0';
	close(fd);

	return atoi(buf);
}

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
	
	if (light == 0) brightness=maxlight;
	else brightness = (maxlight/2)/light;

	return brightness;
}

void fading(int from, int to) {

	int step;

	if (from > to) {
		step=(from-to)/4;
		set_keyboard_brightness_value(from-step);
		usleep(100000);
		set_keyboard_brightness_value(from-step*2);
		usleep(100000);
		set_keyboard_brightness_value(from-step*3);
		usleep(100000);
		set_keyboard_brightness_value(to);
	} else {
		step=(to-from)/4;
		set_keyboard_brightness_value(to-step*3);
		usleep(20000);
		set_keyboard_brightness_value(to-step*2);
		usleep(20000);
		set_keyboard_brightness_value(to-step);
		usleep(20000);
		set_keyboard_brightness_value(to);
	}
}

float get_session_idle_time(Display *display) {

	XScreenSaverInfo info;
	float seconds;

	XScreenSaverQueryInfo(display, DefaultRootWindow(display), &info);
	seconds = (float)info.idle/1000.0f;
	return(seconds);
}

int signal_handler(int sig) {

	set_keyboard_brightness_value(0);
	printf("Killed!\n");
	exit(1);
}

void signal_installer() {

	signal(SIGINT, (void (*)(int))signal_handler);
	signal(SIGTERM, (void (*)(int))signal_handler);
	signal(SIGHUP, (void (*)(int))signal_handler);
	signal(SIGQUIT, (void (*)(int))signal_handler);
	signal(SIGCHLD, (void (*)(int))signal_handler);
	signal(SIGABRT, (void (*)(int))signal_handler);
}
