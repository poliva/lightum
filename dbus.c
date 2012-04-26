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

#include <dbus/dbus.h>

int get_screensaver_active() {

	DBusConnection *connection;
	DBusError error;
	DBusMessage *message;
	DBusMessageIter iter;
	DBusBusType type;
	DBusMessage *reply;

	int retval=0;
	const char *name = NULL;
	char arg[]="org.gnome.ScreenSaver.GetActive";
	name=arg;
	const char *dest = "org.gnome.ScreenSaver";
	const char *path = "/org/gnome/ScreenSaver";

	type = DBUS_BUS_SESSION;

	int reply_timeout;
	reply_timeout = 2000;

	dbus_error_init (&error);
	connection = dbus_bus_get (type, &error);

	if (connection == NULL) {
		fprintf (stderr, "Failed to open connection to message bus: %s\n", error.message);
		dbus_error_free (&error);
		return (-1);
	}

	char *last_dot;
	last_dot = strrchr (name, '.');
	*last_dot = '\0';

	message = dbus_message_new_method_call (NULL,path,name,last_dot + 1);
	dbus_message_set_auto_start (message, TRUE);

	if (message == NULL)
	{
		fprintf (stderr, "Couldn't allocate D-Bus message\n");
		return (-1);
	}

	if (dest && !dbus_message_set_destination (message, dest))
	{
		fprintf (stderr, "Not enough memory\n");
		return (-1);
	}

	dbus_message_iter_init_append (message, &iter);

	dbus_error_init (&error);
	reply = dbus_connection_send_with_reply_and_block (connection, message, reply_timeout, &error);
	if (dbus_error_is_set (&error)) {
		fprintf (stderr, "Error %s: %s\n",error.name,error.message);
		return (-1);
	}

	if (reply)
	{
		dbus_message_iter_init (reply, &iter);
		dbus_bool_t val;
		dbus_message_iter_get_basic (&iter, &val);
		retval = val;
		dbus_message_unref (reply);
	}

	dbus_message_unref (message);
	dbus_connection_unref (connection);

	return retval;
}


int set_keyboard_brightness_value(int brightness) {

	DBusConnection *connection;
	DBusError error;
	DBusMessage *message;
	DBusMessageIter iter;
	DBusBusType type;

	const char *name = NULL;
	char arg[]="org.freedesktop.UPower.KbdBacklight.SetBrightness";
	name=arg;
	const char *dest="org.freedesktop.UPower";
	const char *path="/org/freedesktop/UPower/KbdBacklight";

	type = DBUS_BUS_SYSTEM;

	dbus_error_init (&error);
	connection = dbus_bus_get (type, &error);

	if (connection == NULL) {
		fprintf (stderr, "Failed to open connection to message bus: %s\n", error.message);
		dbus_error_free (&error);
		return (-1);
	}

	char *last_dot;
	last_dot = strrchr (name, '.');
	*last_dot = '\0';

	message = dbus_message_new_method_call (NULL,path,name,last_dot + 1);
	dbus_message_set_auto_start (message, TRUE);

	if (message == NULL)
	{
		fprintf (stderr, "Couldn't allocate D-Bus message\n");
		return (-1);
	}

	if (dest && !dbus_message_set_destination (message, dest))
	{
		fprintf (stderr, "Not enough memory\n");
		return (-1);
	}

	dbus_message_iter_init_append (message, &iter);

	dbus_int32_t int32;
	int32 = brightness;
	dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &int32);

	dbus_connection_send (connection, message, NULL);
	dbus_connection_flush (connection);
	dbus_message_unref (message);
	dbus_connection_unref (connection);
	return 0;
}


int get_screen_backlight_value() {

	DBusConnection *connection;
	DBusError error;
	DBusMessage *message;
	DBusMessageIter iter;
	DBusBusType type;
	DBusMessage *reply;

	int retval=0;
	const char *name = NULL;
	char arg[]="org.gnome.SettingsDaemon.Power.Screen.GetPercentage";
	name=arg;
	const char *dest = "org.gnome.SettingsDaemon";
	const char *path = "/org/gnome/SettingsDaemon/Power";

	type = DBUS_BUS_SESSION;

	int reply_timeout;
	reply_timeout = 2000;

	dbus_error_init (&error);
	connection = dbus_bus_get (type, &error);

	if (connection == NULL) {
		fprintf (stderr, "Failed to open connection to message bus: %s\n", error.message);
		dbus_error_free (&error);
		return (-1);
	}

	char *last_dot;
	last_dot = strrchr (name, '.');
	*last_dot = '\0';

	message = dbus_message_new_method_call (NULL,path,name,last_dot + 1);
	dbus_message_set_auto_start (message, TRUE);

	if (message == NULL)
	{
		fprintf (stderr, "Couldn't allocate D-Bus message\n");
		return (-1);
	}

	if (dest && !dbus_message_set_destination (message, dest))
	{
		fprintf (stderr, "Not enough memory\n");
		return (-1);
	}

	dbus_message_iter_init_append (message, &iter);

	dbus_error_init (&error);
	reply = dbus_connection_send_with_reply_and_block (connection, message, reply_timeout, &error);
	if (dbus_error_is_set (&error)) {
		fprintf (stderr, "Error %s: %s\n",error.name,error.message);
		return (-1);
	}

	if (reply)
	{
		dbus_message_iter_init (reply, &iter);
		dbus_bool_t val;
		dbus_message_iter_get_basic (&iter, &val);
		retval = val;
		dbus_message_unref (reply);
	}

	dbus_message_unref (message);
	dbus_connection_unref (connection);

	return retval;
}

int set_screen_backlight_value(int backlight) {

	DBusConnection *connection;
	DBusError error;
	DBusMessage *message;
	DBusMessageIter iter;
	DBusBusType type;
	DBusMessage *reply;

	int retval=0;
	const char *name = NULL;
	char arg[]="org.gnome.SettingsDaemon.Power.Screen.SetPercentage";
	name=arg;
	const char *dest = "org.gnome.SettingsDaemon";
	const char *path = "/org/gnome/SettingsDaemon/Power";

	type = DBUS_BUS_SESSION;

	int reply_timeout;
	reply_timeout = 2000;

	dbus_error_init (&error);
	connection = dbus_bus_get (type, &error);

	if (connection == NULL) {
		fprintf (stderr, "Failed to open connection to message bus: %s\n", error.message);
		dbus_error_free (&error);
		return (-1);
	}

	char *last_dot;
	last_dot = strrchr (name, '.');
	*last_dot = '\0';

	message = dbus_message_new_method_call (NULL,path,name,last_dot + 1);
	dbus_message_set_auto_start (message, TRUE);

	if (message == NULL)
	{
		fprintf (stderr, "Couldn't allocate D-Bus message\n");
		return (-1);
	}

	if (dest && !dbus_message_set_destination (message, dest))
	{
		fprintf (stderr, "Not enough memory\n");
		return (-1);
	}


	dbus_message_iter_init_append (message, &iter);

	dbus_uint32_t uint32;
	uint32 = backlight;
	dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &uint32);

	dbus_error_init (&error);
	reply = dbus_connection_send_with_reply_and_block (connection, message, reply_timeout, &error);
	if (dbus_error_is_set (&error)) {
		fprintf (stderr, "Error %s: %s\n",error.name,error.message);
		return (-1);
	}

	if (reply)
	{
		dbus_message_iter_init (reply, &iter);
		dbus_bool_t val;
		dbus_message_iter_get_basic (&iter, &val);
		retval = val;
		dbus_message_unref (reply);
	}

	dbus_message_unref (message);
	dbus_connection_unref (connection);

	return retval;
}

