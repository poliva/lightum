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
	ssize_t cnt;

	/* read light sensor value */
	fd = open(kbd_backlight, O_RDONLY);
	if (fd < 0) {
		perror (kbd_backlight);
                fprintf (stderr, "Can't open %s\n",kbd_backlight);
		exit(1);
	}
	cnt=read(fd, buf, sizeof(buf)-1);
	buf[cnt]='\0';
	close(fd);

	return atoi(buf);
}

int get_light_sensor_value() {

	int fd;
	size_t i,n=0;
	char buf[10];
	char a_light[10];
	char *light_sensor="/sys/devices/platform/applesmc.768/light";
	ssize_t cnt;

	/* read light sensor value */
	fd = open(light_sensor, O_RDONLY);
	if (fd < 0) {
		perror (light_sensor);
                fprintf (stderr, "Can't open %s\n",light_sensor);
		exit(1);
	}
	cnt=read(fd, buf, sizeof(buf)-1);
	buf[cnt]='\0';
	close(fd);

	/* convert light sensor string value to integer */
	for (i=0;buf[i]!='\0';i++) {
		if (buf[i]==',') break;
		if (buf[i]!='(') {
			a_light[n]=buf[i];
			n++;
		}
	}
	a_light[n]='\0';

	return atoi(a_light);
}

int get_screen_backlight_value() {
	int ret=0, backlight=15;
	backlight = dbus_get_screen_backlight_value();
	if (backlight < 6 ) ret=0;
	else if (backlight < 13) ret=1;
	else if (backlight < 20) ret=2;
	else if (backlight < 26) ret=3;
	else if (backlight < 33) ret=4;
	else if (backlight < 40) ret=5;
	else if (backlight < 46) ret=6;
	else if (backlight < 53) ret=7;
	else if (backlight < 60) ret=8;
	else if (backlight < 66) ret=9;
	else if (backlight < 73) ret=10;
	else if (backlight < 80) ret=11;
	else if (backlight < 86) ret=12;
	else if (backlight < 93) ret=13;
	else if (backlight < 100) ret=14;
	else if (backlight == 100) ret=15;
	return ret;
}

int set_screen_backlight_value(int backlight) {
	int value=100, ret=15;
	if (backlight == 15) value=100;
	else if (backlight == 14) value=97;
	else if (backlight == 13) value=90;
	else if (backlight == 12) value=83;
	else if (backlight == 11) value=77;
	else if (backlight == 10) value=70;
	else if (backlight == 9) value=63;
	else if (backlight == 8) value=57;
	else if (backlight == 7) value=50;
	else if (backlight == 6) value=43;
	else if (backlight == 5) value=37;
	else if (backlight == 4) value=30;
	else if (backlight == 3) value=23;
	else if (backlight == 2) value=17;
	else if (backlight == 1) value=10;
	else if (backlight == 0) value=0;
	ret = dbus_set_screen_backlight_value(value);
	return ret;
}

int calculate_keyboard_brightness_value(int light, int maxlight) {

	int brightness=0;
	
	if (light == 0) brightness=maxlight;
	else brightness = (maxlight/2)/light;

	return brightness;
}

int calculate_screen_backlight_value(int light, int maxlight) {

	int backlight=0;
	
	if (light == 0 ) backlight=maxlight;
	else if (light < 4 ) backlight=maxlight*0.90;
	else if (light < 8 ) backlight=maxlight*0.80;
	else if (light < 16 ) backlight=maxlight*0.75;
	else if (light < 32) backlight=maxlight*0.60;
	else if (light < 64) backlight=maxlight*0.50;
	else if (light < 128) backlight=maxlight*0.40;
	else if (light >= 128) backlight=maxlight*0.30;

	return backlight;
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

void backlight_fading(int from, int to) {

	int step;

	if (from > to) {
		step=(from-to)/4;
		set_screen_backlight_value(from-step);
		usleep(100000);
		set_screen_backlight_value(from-step*2);
		usleep(100000);
		set_screen_backlight_value(from-step*3);
		usleep(100000);
		set_screen_backlight_value(to);
	} else {
		step=(to-from)/4;
		set_screen_backlight_value(to-step*3);
		usleep(20000);
		set_screen_backlight_value(to-step*2);
		usleep(20000);
		set_screen_backlight_value(to-step);
		usleep(20000);
		set_screen_backlight_value(to);
	}
}


float get_session_idle_time(Display *display) {

	XScreenSaverInfo info;
	float seconds;

	XScreenSaverQueryInfo(display, DefaultRootWindow(display), &info);
	seconds = (float)info.idle/1000.0f;
	return(seconds);
}

void signal_handler(int sig) {

	(void) sig;
	set_keyboard_brightness_value(0);
	printf("Killed!\n");
	exit(1);
}

void signal_installer() {

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGCHLD, signal_handler);
	signal(SIGABRT, signal_handler);
}
