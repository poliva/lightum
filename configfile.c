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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "lightum.h"

#define CONFIG_PATH ".config/lightum/"
#define CONFIG_FILE "lightum.conf"

#define TRUE 1
#define FALSE 0

#define MAXLEN 80

int file_exists(char* file) {
	struct stat buf;
	if (stat(file, &buf) == 0)
		return TRUE;
	return FALSE;
}

char* default_config_file() {
	char* home = getenv("HOME");
	char* file = malloc(strlen(home) + strlen(CONFIG_PATH) + strlen(CONFIG_FILE) + 2);
	strcpy(file, "");
	strcat(file, home);
	strcat(file, "/");
	strcat(file, CONFIG_PATH);
	strcat(file, CONFIG_FILE);
	return file;
}

char* default_config_dir() {
	char* home = getenv("HOME");
	char* path = malloc(strlen(home) + strlen(CONFIG_PATH) + 2);
	strcpy(path, "");
	strcat(path, home);
	strcat(path, "/");
	strcat(path, CONFIG_PATH);
	return path;
}

int create_config_dir(char* path) {
	if (mkdir(path, 0755) != 0)
		return FALSE;
	return TRUE;
}

int create_config_file(char* file) {
	FILE* fd;
	char *path;

	path = default_config_dir();
	create_config_dir(path);

	fd = fopen(file, "w");
	if (fd == NULL)
		return FALSE;

	fprintf(fd, "# lightum configuration file\n\n");
	fprintf(fd, "# manualmode\n");
	fprintf(fd, "#   0 = automatically adjust keyboard brightness based on light sensor\n");
	fprintf(fd, "#   1 = or control keyboard brightness manually using Fn+ F5/F6 keys\n");
	fprintf(fd, "manualmode=0\n\n");
	fprintf(fd, "# maximum keyboard brightness value (between 4 and 255)\n");
	fprintf(fd, "maxbrightness=255\n\n");
	fprintf(fd, "# poll time in miliseconds (used for light sensor and session idle time)\n");
	fprintf(fd, "polltime=800\n\n");
	fprintf(fd, "# turn off keyboard brightness if computer unused for X seconds (0 to disable).\n");
	fprintf(fd, "idleoff=10\n\n");
	fprintf(fd, "# screensaver\n");
	fprintf(fd, "#   1 = turn off keyboard brightness when screensaver is active\n");
	fprintf(fd, "#   0 = do not monitor screensaver status\n");
	fprintf(fd, "queryscreensaver=0\n\n");
	fclose(fd);

	return TRUE;
}

conf_data config_parse() {

	char *file;
	char input[MAXLEN], temp[MAXLEN];
	FILE *fd;
	size_t len;
	conf_data config;

	file = default_config_file();
	if (!file_exists(file)) {
		if (!create_config_file(file)) {
			printf ("failed to create default config file: %s\n", file);
			exit(1);
		}
	}

	fd = fopen (file, "r");
	if (fd == NULL) {
		printf ("Could not open configuration file: %s\n", file);
		exit (1);
	}

	while ((fgets (input, sizeof (input), fd)) != NULL) {

		if ((strncmp ("manualmode=", input, 11)) == 0) {
			strncpy (temp, input + 11,MAXLEN-1);
			len=strlen(temp);
			temp[len+1]='\0';
			config.manualmode = atoi(temp);
		}

		if ((strncmp ("queryscreensaver=", input, 17)) == 0) {
			strncpy (temp, input + 17,MAXLEN-1);
			len=strlen(temp);
			temp[len+1]='\0';
			config.queryscreensaver = atoi(temp);
		}

		if ((strncmp ("maxbrightness=", input, 14)) == 0) {
			strncpy (temp, input + 14,MAXLEN-1);
			len=strlen(temp);
			temp[len+1]='\0';
			config.maxbrightness = atoi(temp);
		}

		if ((strncmp ("polltime=", input, 9)) == 0) {
			strncpy (temp, input + 9,MAXLEN-1);
			len=strlen(temp);
			temp[len+1]='\0';
			config.polltime = atoi(temp);
		}

		if ((strncmp ("idleoff=", input, 8)) == 0) {
			strncpy (temp, input + 8,MAXLEN-1);
			len=strlen(temp);
			temp[len+1]='\0';
			config.idleoff = atoi(temp);
		}
	}

	return config;
}
