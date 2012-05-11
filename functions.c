/*
 *   MacBook automatic light sensor daemon
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
#include <sys/stat.h>
#include <string.h>

#include "lightum.h"

extern int reloadconfig;

int get_keyboard_brightness_value() {

	int fd;
	char buf[5];
	const char *kbd_backlight="/sys/devices/platform/applesmc.768/leds/smc::kbd_backlight/brightness";
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
	const char *light_sensor="/sys/devices/platform/applesmc.768/light";
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

	int fd, actual_backlight, max_backlight;
	char buf[5];
	struct stat tmp;
	const char *scr_backlight="/sys/class/backlight/acpi_video0/actual_brightness";
	const char *scr_maxbacklight="/sys/class/backlight/acpi_video0/max_brightness";
	ssize_t cnt;
        int ret=0, backlight=15;

	if (stat(scr_backlight, &tmp) == 0) {

		/* read screen backlight value */
		fd = open(scr_backlight, O_RDONLY);
		if (fd < 0) {
			perror (scr_backlight);
			fprintf (stderr, "Can't open %s\n",scr_backlight);
			exit(1);
		}
		cnt=read(fd, buf, sizeof(buf)-1);
		buf[cnt]='\0';
		close(fd);
		actual_backlight=atoi(buf);

		/* read screen max backlight value */
		fd = open(scr_maxbacklight, O_RDONLY);
		if (fd < 0) {
			perror (scr_backlight);
			fprintf (stderr, "Can't open %s\n",scr_backlight);
			exit(1);
		}
		cnt=read(fd, buf, sizeof(buf)-1);
		buf[cnt]='\0';
		close(fd);
		max_backlight=atoi(buf);

		/* make sure we always retrn a value between 0 and 15 */
		return (15*actual_backlight)/max_backlight;

	} else {
		// fallback to read the current screen backlight value using dbus
		// we prefer to avoid this because it forks gsd-backlight-helper
		// from gnome-settings-daemon on each call.

		backlight = dbus_get_screen_backlight_value();
		ret = dbus_to_acpi_backlight(backlight);
		return ret;

	}
}

int dbus_to_acpi_backlight(int backlight) {

	int value=15;
	if (backlight < 6 ) value=0;
	else if (backlight < 13) value=1;
	else if (backlight < 20) value=2;
	else if (backlight < 26) value=3;
	else if (backlight < 33) value=4;
	else if (backlight < 40) value=5;
	else if (backlight < 46) value=6;
	else if (backlight < 53) value=7;
	else if (backlight < 60) value=8;
	else if (backlight < 66) value=9;
	else if (backlight < 73) value=10;
	else if (backlight < 80) value=11;
	else if (backlight < 86) value=12;
	else if (backlight < 93) value=13;
	else if (backlight < 100) value=14;
	else if (backlight == 100) value=15;
	return value;
}

int acpi_to_dbus_backlight(int backlight) {

	int value=100;
	if (backlight == 15) value=100;
	else if (backlight == 14) value=96;
	else if (backlight == 13) value=88;
	else if (backlight == 12) value=82;
	else if (backlight == 11) value=75;
	else if (backlight == 10) value=68;
	else if (backlight == 9) value=62;
	else if (backlight == 8) value=55;
	else if (backlight == 7) value=48;
	else if (backlight == 6) value=42;
	else if (backlight == 5) value=35;
	else if (backlight == 4) value=28;
	else if (backlight == 3) value=22;
	else if (backlight == 2) value=15;
	else if (backlight == 1) value=8;
	else if (backlight == 0) value=0;
	return value;
}

int set_screen_backlight_value(int backlight, int backend) {

	int value, ret;

	value = acpi_to_dbus_backlight(backlight);
	ret = dbus_set_screen_backlight_value(value, backend);

	return ret;
}

int calculate_keyboard_brightness_value(int light, int maxlight, int minlight) {

	int brightness=0;
	
	if (light == 0) brightness=maxlight;
	else brightness = (maxlight/2)/light;
	if (brightness < minlight) brightness=minlight;

	return brightness;
}

int calculate_screen_backlight_value(int light, int maxlight, int minlight) {

	int backlight=0;

	if (light == 0) backlight=minlight;
	else if (light < 2) backlight=maxlight*0.15;
	else if (light < 4) backlight=maxlight*0.20;
	else if (light < 8) backlight=maxlight*0.30;
	else if (light < 12) backlight=maxlight*0.35;
	else if (light < 16) backlight=maxlight*0.40;
	else if (light < 24) backlight=maxlight*0.50;
	else if (light < 32) backlight=maxlight*0.55;
	else if (light < 48) backlight=maxlight*0.60;
	else if (light < 56) backlight=maxlight*0.70;
	else if (light < 60) backlight=maxlight*0.75;
	else if (light < 64) backlight=maxlight*0.80;
	else if (light < 96) backlight=maxlight*0.90;
	else if (light < 128) backlight=maxlight*0.95;
	else if (light >= 128) backlight=maxlight;
	if (backlight < minlight) backlight=minlight;

	return backlight;
}

void fading(int from, int to) {

	int step;

	if (from == -1)
		from = get_keyboard_brightness_value();

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

void backlight_fading(int from, int to, int backend) {

	int step;

	if (from == -1)
		from = get_screen_backlight_value();

	if (from > to) {
		step=(from-to)/4;
		set_screen_backlight_value(from-step, backend);
		usleep(100000);
		set_screen_backlight_value(from-step*2, backend);
		usleep(100000);
		set_screen_backlight_value(from-step*3, backend);
		usleep(100000);
		set_screen_backlight_value(to, backend);
	} else {
		step=(to-from)/4;
		set_screen_backlight_value(to-step*3, backend);
		usleep(20000);
		set_screen_backlight_value(to-step*2, backend);
		usleep(20000);
		set_screen_backlight_value(to-step, backend);
		usleep(20000);
		set_screen_backlight_value(to, backend);
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
	remove_pid_file();
	printf("Killed!\n");
	exit(1);
}

void config_reload(int sig) {

	(void) sig;
	reloadconfig=1;
}

void signal_installer() {

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGCHLD, signal_handler);
	signal(SIGABRT, signal_handler);
	signal(SIGUSR1, config_reload);
}

char* default_pid_file() {
	char* home = getenv("HOME");
	char* file = malloc(strlen(home) + strlen(CONFIG_PATH) + strlen(PID_FILE) + 2);
	strcpy(file, "");
	strcat(file, home);
	strcat(file, "/");
	strcat(file, CONFIG_PATH);
	strcat(file, PID_FILE);
	return file;
}

int create_pid_file() {

	int fd;
	char *path, *pidfile;
	char buf[100];
	ssize_t cnt;
	char* procpid = malloc( sizeof(buf) + 15 );

	path = default_config_dir();
	create_config_dir(path);

	pidfile = default_pid_file();

	if (file_exists(pidfile)) {

		// check if /proc/{pid}/cmdline exists and contains lightum
		// if it does, means lightum is already running, so we exit cowardly
		// if it does not contain lightum, then we remove the old pid file and continue

		fd = open(pidfile, O_RDONLY);
		if (fd < 0) {
			printf ("Could not open pid file: %s\n", pidfile);
			return FALSE;
		}
		cnt=read(fd, buf, sizeof(buf)-1);
		buf[cnt]='\0';
		
		close(fd);

		strcpy(procpid, "");
		strcat(procpid, "/proc/");
		strcat(procpid, buf);
		strcat(procpid, "/cmdline");

		if (file_exists(procpid)) {
			fd = open(procpid, O_RDONLY);
			if (fd < 0) {
				printf ("Could not open file: %s\n", procpid);
				return FALSE;
			}

			cnt=read(fd, buf, sizeof(buf)-1);
			buf[cnt]='\0';
			
			close(fd);

			if (strstr(buf,"lightum") != NULL) {
				printf("Refusing to start as lightum is already running\n");
				return FALSE;
			} else {
				if (!remove_pid_file()) 
					return FALSE;
			}
		}
	}

	fd = open(pidfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0 ) {
		printf("Could not write pid file: %s\n", pidfile);
		return FALSE;
	}

	sprintf( buf, "%d", getpid() );
	if (write(fd, buf, strlen(buf)) < 1) {
		perror("Something wrong happening while writing pid file");
		close(fd);
		return FALSE;
	}
	close(fd);

	free(procpid);

	return TRUE;
}

int remove_pid_file() {

	char *pidfile;

	pidfile = default_pid_file();

	if (!file_exists(pidfile)) {
		printf("pid file does not exist: %s\n", pidfile);
		return TRUE;
	}

	if (unlink(pidfile) != 0) {
		printf("Could not delete pid file: %s\n", pidfile);
		return FALSE;
	}
	return TRUE;
}
