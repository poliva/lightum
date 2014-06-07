/*
 *   MacBook automatic light sensor daemon
 *   Copyright 2011 Pau Oliva Fora <pof@eslack.org>
 *
 *   Some portions of this file taken from consolekit code:
 *   Copyright (C) 2006 William Jon McCann <mccann@jhu.edu>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *
 */

#include <stdlib.h>
#include <gio/gio.h>
#include <systemd/sd-login.h>

#define SD_NAME              "org.freedesktop.login1"
#define SD_MANAGER_PATH      "/org/freedesktop/login1"
#define SD_MANAGER_INTERFACE "org.freedesktop.login1.Manager"
#define SD_SESSION_INTERFACE "org.freedesktop.login1.Session"

int get_session_active (GDBusProxy *proxy)
{

	GVariant *value;
	char	 *state;

	value = g_dbus_proxy_get_cached_property (proxy, "State");
	if (!value)
		return FALSE;

	g_variant_get (value, "&s", &state);
	g_variant_unref (value);

	if (g_strcmp0 (state, "active") == 0) {
		return TRUE;
	} else {
		return FALSE;
	}

}

GDBusProxy* get_dbus_proxy_session(GDBusConnection *connection, GDBusProxy *proxy_manager)
{

    GDBusProxy *proxy;
    GError     *error = NULL;
 	char	   *session;
    GVariant   *res;
	char	   *ssid;

	sd_pid_get_session (getpid (), &session);

    res = g_dbus_proxy_call_sync (proxy_manager,
                                  "GetSession",
				                  g_variant_new("(s)", session),
				                  G_DBUS_CALL_FLAGS_NONE,
                        	      -1,
                        	      NULL,
                        	      &error);
				      
    if (res == NULL) {
        g_warning ("%s failed: %s", "GetSession", error->message);
        g_error_free (error);
    }

	// Get the object path from the introspection data
	g_variant_get (res, "(&o)", &ssid);

    proxy = g_dbus_proxy_new_sync (connection,
				                   G_DBUS_PROXY_FLAGS_NONE,
                                   NULL,
                                   SD_NAME,
                                   ssid,
                                   SD_SESSION_INTERFACE,
                                   NULL,
                                   &error);

    if (proxy == NULL) {
	    g_warning ("%s: Could not get dbus session proxy: %s", __func__, error->message);
		g_error_free (error);
        exit (1);
	}

	g_free (session);
	g_free (ssid);

	return proxy;

}

GDBusProxy* get_dbus_proxy_manager(GDBusConnection *connection)
{

    GDBusProxy *proxy;
    GError     *error;

    proxy = g_dbus_proxy_new_sync (connection,
				                   G_DBUS_PROXY_FLAGS_NONE,
                                   NULL,
                                   SD_NAME,
                                   SD_MANAGER_PATH,
                                   SD_MANAGER_INTERFACE,
                                   NULL,
                                   &error);
    
    if (proxy == NULL) {
		g_warning ("%s: Could not get dbus manager proxy: %s", __func__, error->message);
		g_error_free (error);
        exit (1);
	}

	return proxy;

}

GDBusConnection* get_dbus_connection()
{
    GDBusConnection *connection;
    GOptionContext	*context;
    gboolean         retval;
    GError         	*error = NULL;

    context = g_option_context_new (NULL);
    retval = g_option_context_parse (context, NULL, NULL, &error);

    g_option_context_free (context);

    if (! retval) {
        g_warning ("%s", error->message);
        g_error_free (error);
        exit(1);
    }

    error = NULL;
    connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
    if (connection == NULL) {
        g_message ("%s: Failed to connect to the D-Bus daemon: %s", __func__, error->message);
        g_error_free (error);
        exit (1);
    }

	return connection;
}
