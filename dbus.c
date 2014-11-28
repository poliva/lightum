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

#include <gio/gio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* GNOME Screensaver */
#define GS_DBUS_SERVICE     "org.gnome.ScreenSaver"
#define GS_DBUS_PATH        "/org/gnome/ScreenSaver"
#define GS_DBUS_INTERFACE   "org.gnome.ScreenSaver"

/* Cinnamon Screensaver */
#define CS_DBUS_SERVICE     "org.cinnamon.ScreenSaver"
#define CS_DBUS_PATH        "/org/cinnamon/ScreenSaver"
#define CS_DBUS_INTERFACE   "org.cinnamon.ScreenSaver"

/* UPower */
#define UP_DBUS_SERVICE     "org.freedesktop.UPower"
#define UP_DBUS_PATH        "/org/freedesktop/UPower/KbdBacklight"
#define UP_DBUS_INTERFACE   "org.freedesktop.UPower.KbdBacklight"

/* GNOME SettingsDaemon */
#define SD_DBUS_SERVICE     "org.gnome.SettingsDaemon"
#define SD_DBUS_PATH        "/org/gnome/SettingsDaemon/Power"
#define SD_DBUS_INTERFACE   "org.gnome.SettingsDaemon.Power.Screen"

/* Cinnamon SettingsDaemon */
#define CSD_DBUS_SERVICE    "org.cinnamon.SettingsDaemon"
#define CSD_DBUS_PATH       "/org/cinnamon/SettingsDaemon/Power"
#define CSD_DBUS_INTERFACE  "org.cinnamon.SettingsDaemon.Power.Screen"

/* KDE PowerManagement */
#define KDE_DBUS_SERVICE    "org.kde.Solid.PowerManagement"
#define KDE_DBUS_PATH       "/org/kde/Solid/PowerManagement"
#define KDE_DBUS_INTERFACE  "org.kde.Solid.PowerManagement"

extern int set_screen_xbacklight_value (int backlight);

GDBusConnection *get_dbus_message_bus (int bus_type) {
 
    GDBusConnection *connection;
    GError *error = NULL;
    
    connection = g_bus_get_sync (bus_type, NULL, &error);
    if (error) {
        g_warning ("Failed to get session bus: %s", error->message);
    }
    g_clear_error (&error);
    if (connection == NULL) {
        return NULL;
    }

    return connection;
    
}

int get_screensaver_active() {

    GDBusMessage    *message, *reply;
    GDBusConnection *connection;
    GError          *error;
    GVariant        *body; 
    gint32           value;
    const char      *desktop;
    
    connection = get_dbus_message_bus (G_BUS_TYPE_SESSION);

    desktop = getenv("DESKTOP_SESSION");

    /* Different service for cinnamon */
    if (desktop != NULL && !strcasecmp(desktop, "cinnamon")) {
        message = g_dbus_message_new_method_call (
            CS_DBUS_SERVICE,
            CS_DBUS_PATH,
            CS_DBUS_INTERFACE,
            "GetActive");
    } else {
        message = g_dbus_message_new_method_call (
            GS_DBUS_SERVICE,
            GS_DBUS_PATH,
            GS_DBUS_INTERFACE,
            "GetActive");
    }
    
    if (message == NULL) {
        g_warning ("Failed to allocate the dbus message");
        return -1;
    }

    g_dbus_message_set_body (
        message, 
        g_variant_new ("(b)", TRUE)
    );  
          
    error = NULL;
    
    reply = g_dbus_connection_send_message_with_reply_sync (
        connection,
        message,
        G_DBUS_SEND_MESSAGE_FLAGS_NONE,
        -1,
        NULL,
        NULL,
        &error);
    if (error != NULL) {
        g_warning ("unable to send message: %s", error->message);
        g_clear_error (&error);
    }

    g_dbus_connection_flush_sync (connection, NULL, &error);
    if (error != NULL) {
        g_warning ("unable to flush message queue: %s", error->message);
        g_clear_error (&error);
    }
   
    body = g_dbus_message_get_body (reply);
    g_variant_get (body, "(&s)", &value);
    
    g_object_unref (connection);
    g_object_unref (message);
    g_object_unref (reply);

    return value;

}

int set_keyboard_brightness_value (int brightness) {

    GDBusMessage    *message, *reply;
    GDBusConnection *connection;
    GError          *error;
    
    connection = get_dbus_message_bus (G_BUS_TYPE_SYSTEM);

    message = g_dbus_message_new_method_call (
        UP_DBUS_SERVICE,
        UP_DBUS_PATH,
        UP_DBUS_INTERFACE,
        "SetBrightness");        
    
    if (message == NULL) {
        g_warning ("Failed to allocate the dbus message");
        return -1;
    }

    g_dbus_message_set_body (
        message, 
        g_variant_new ("(i)", brightness)
    );  
          
    error = NULL;
    
    reply = g_dbus_connection_send_message_with_reply_sync (
        connection,
        message,
        G_DBUS_SEND_MESSAGE_FLAGS_NONE,
        -1,
        NULL,
        NULL,
        &error);
    if (error != NULL) {
        g_warning ("unable to send message: %s", error->message);
        g_clear_error (&error);
    }

    g_dbus_connection_flush_sync (connection, NULL, &error);
    if (error != NULL) {
        g_warning ("unable to flush message queue: %s", error->message);
        g_clear_error (&error);
    }
 
    g_object_unref (reply);
    g_object_unref (connection);
    g_object_unref (message);

    return 0;
}

int dbus_get_screen_backlight_value() {

    GDBusMessage    *message, *reply;
    GDBusConnection *connection;
    GError          *error;
    GVariant        *body; 
    gint32           value;
    const char      *desktop;
    
    connection = get_dbus_message_bus(G_BUS_TYPE_SESSION);

    desktop = getenv("DESKTOP_SESSION");

    /* Different service for cinnamon */
    if (desktop != NULL && !strcasecmp(desktop, "cinnamon")) {
        message = g_dbus_message_new_method_call (
            CSD_DBUS_SERVICE,
            CSD_DBUS_PATH,
            CSD_DBUS_INTERFACE,
            "GetPercentage");
    } else {
        message = g_dbus_message_new_method_call (
            SD_DBUS_SERVICE,
            SD_DBUS_PATH,
            SD_DBUS_INTERFACE,
            "GetPercentage");
    }
    
    if (message == NULL) {
        g_warning ("Failed to allocate the dbus message");
        return -1;
    }

    g_warning ("before message_get_body");
    g_dbus_message_set_body (
        message, 
        g_variant_new ("(y)", NULL));  
          
    error = NULL;
    
    reply = g_dbus_connection_send_message_with_reply_sync (
        connection,
        message,
        G_DBUS_SEND_MESSAGE_FLAGS_NONE,
        -1,
        NULL,
        NULL,
        &error);
    if (error != NULL) {
        g_warning ("unable to send message: %s", error->message);
        g_clear_error (&error);
    }

    g_dbus_connection_flush_sync (connection, NULL, &error);
    if (error != NULL) {
        g_warning ("unable to flush message queue: %s", error->message);
        g_clear_error (&error);
    }
   
    body = g_dbus_message_get_body (reply);
    g_variant_get (body, "(u)", &value);
 
    g_object_unref (connection);
    g_object_unref (message);
    g_object_unref (reply);

    return value;

}

int dbus_set_screen_backlight_value_gnome (int backlight) {

    GDBusMessage    *message, *reply;
    GDBusConnection *connection;
    GError          *error;
    GVariant        *body; 
    gint32           value;
    const char      *desktop;
    
    connection = get_dbus_message_bus (G_BUS_TYPE_SESSION);
 
    desktop = getenv("DESKTOP_SESSION");

    if (desktop != NULL && !strcasecmp(desktop, "cinnamon")) {
        message = g_dbus_message_new_method_call (
            CSD_DBUS_SERVICE,
            CSD_DBUS_PATH,
            CSD_DBUS_INTERFACE,
            "SetPercentage");
    } else {
        message = g_dbus_message_new_method_call (
            SD_DBUS_SERVICE,
            SD_DBUS_PATH,
            SD_DBUS_INTERFACE,
            "SetPercentage");
    }
    
    if (message == NULL) {
        g_warning ("Failed to allocate the dbus message");
        return -1;
    }

    g_dbus_message_set_body (
        message, 
        g_variant_new ("(u)", backlight));  
          
    error = NULL;
    
    reply = g_dbus_connection_send_message_with_reply_sync (
        connection,
        message,
        G_DBUS_SEND_MESSAGE_FLAGS_NONE,
        -1,
        NULL,
        NULL,
        &error);
    if (error != NULL) {
        g_warning ("unable to send message: %s", error->message);
        g_clear_error (&error);
    }

    g_dbus_connection_flush_sync (connection, NULL, &error);
    if (error != NULL) {
        g_warning ("unable to flush message queue: %s", error->message);
        g_clear_error (&error);
    }
 
    body = g_dbus_message_get_body (reply);
    g_variant_get (body, "(u)", &value);
 
    g_object_unref (reply);
    g_object_unref (connection);
    g_object_unref (message);

	return value;
}

int dbus_set_screen_backlight_value_kde (int backlight) {

    GDBusMessage    *message, *reply;
    GDBusConnection *connection;
    GError          *error;
    GVariant        *body; 
    gint32           value;
    
    connection = get_dbus_message_bus (G_BUS_TYPE_SESSION);

    message = g_dbus_message_new_method_call (
        KDE_DBUS_SERVICE,
        KDE_DBUS_PATH,
        KDE_DBUS_INTERFACE,
        "SetBrightness");        
    
    if (message == NULL) {
        g_warning ("Failed to allocate the dbus message");
        return -1;
    }

    g_dbus_message_set_body (
        message, 
        g_variant_new ("(u)", backlight));  
          
    error = NULL;
    
    reply = g_dbus_connection_send_message_with_reply_sync (
        connection,
        message,
        G_DBUS_SEND_MESSAGE_FLAGS_NONE,
        -1,
        NULL,
        NULL,
        &error);
    if (error != NULL) {
        g_warning ("unable to send message: %s", error->message);
        g_clear_error (&error);
    }

    g_dbus_connection_flush_sync (connection, NULL, &error);
    if (error != NULL) {
        g_warning ("unable to flush message queue: %s", error->message);
        g_clear_error (&error);
    }
 
    body = g_dbus_message_get_body (reply);
    g_variant_get (body, "(u)", &value);
 
    g_object_unref (reply);
    g_object_unref (connection);
    g_object_unref (message);

	return value;

}


int dbus_set_screen_backlight_value (int backlight, int backend) {

	int ret=-1;

	if (backend == 0) ret = dbus_set_screen_backlight_value_gnome(backlight); 
	if (backend == 1) ret = dbus_set_screen_backlight_value_kde(backlight);
	if (backend == 2) ret = set_screen_xbacklight_value(backlight);

	return ret;
	
}
