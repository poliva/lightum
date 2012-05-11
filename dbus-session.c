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
#include <dbus/dbus-glib-lowlevel.h>

#define CK_NAME      "org.freedesktop.ConsoleKit"
#define CK_MANAGER_PATH      "/org/freedesktop/ConsoleKit/Manager"
#define CK_MANAGER_INTERFACE "org.freedesktop.ConsoleKit.Manager"
#define CK_SESSION_INTERFACE "org.freedesktop.ConsoleKit.Session"

static gboolean get_boolean (DBusGProxy *proxy, const char *method, gboolean *value)
{
        GError  *error;
        gboolean res;

        error = NULL;
        res = dbus_g_proxy_call (proxy,
                                 method,
                                 &error,
                                 G_TYPE_INVALID,
                                 G_TYPE_BOOLEAN, value,
                                 G_TYPE_INVALID);
        if (! res) {
                g_warning ("%s failed: %s", method, error->message);
                g_error_free (error);
        }

        return res;
}

int get_session_active (DBusGProxy *proxy_session)
{
        gboolean    is_active;
        gboolean    is_local;

        get_boolean (proxy_session, "IsActive", &is_active);
        get_boolean (proxy_session, "IsLocal", &is_local);

	if (is_active && is_local) return 1;
	else return 0;

}

DBusGProxy* get_dbus_proxy_session(DBusGConnection *connection, DBusGProxy *proxy_manager)
{

        DBusGProxy *proxy;
        GError     *error;
        gboolean    res;
	char *ssid;

        error = NULL;
        res = dbus_g_proxy_call (proxy_manager,
                                 "GetCurrentSession",
                                 &error,
                                 G_TYPE_INVALID,
                                 DBUS_TYPE_G_OBJECT_PATH, &ssid,
                                 G_TYPE_INVALID);

	g_object_unref (proxy_manager);

        if (! res) {
                g_warning ("%s failed: %s", "GetCurrentSession", error->message);
                g_error_free (error);
        }

        proxy = dbus_g_proxy_new_for_name (connection,
                                           CK_NAME,
                                           ssid,
                                           CK_SESSION_INTERFACE);

        if (proxy == NULL) {
		g_warning ("Could not get dbus session proxy");
                exit (1);
	}

	return proxy;

}

DBusGProxy* get_dbus_proxy_manager(DBusGConnection *connection)
{

        DBusGProxy *proxy;

        proxy = dbus_g_proxy_new_for_name (connection,
                                           CK_NAME,
                                           CK_MANAGER_PATH,
                                           CK_MANAGER_INTERFACE);
        if (proxy == NULL) {
		g_warning ("Could not get dbus manager proxy");
                exit (1);
	}

	return proxy;

}

DBusGConnection* get_dbus_connection()
{
        DBusGConnection *connection;

        GOptionContext *context;
        gboolean        retval;
        GError         *error = NULL;

        g_type_init ();

        context = g_option_context_new (NULL);
        retval = g_option_context_parse (context, NULL, NULL, &error);

        g_option_context_free (context);

        if (! retval) {
                g_warning ("%s", error->message);
                g_error_free (error);
                exit(1);
        }

        error = NULL;
        connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
        if (connection == NULL) {
                g_message ("Failed to connect to the D-Bus daemon: %s", error->message);
                g_error_free (error);
                exit (1);
        }

	return connection;
}
