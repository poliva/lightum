#include <X11/Xlib.h>
#include <dbus/dbus-glib-lowlevel.h>
/* config file */
typedef struct {
	int manualmode;
	int queryscreensaver;
	int maxbrightness;
	int minbrightness;
	int polltime;
	int idleoff;
	int ignoreuser;
	int minbacklight;
	int maxbacklight;
	int screenidle;
	int workmode;
	int ignoresession;
} conf_data;

/* configfile.c */
conf_data config_parse();

/* functions.c */
int get_keyboard_brightness_value();
int get_light_sensor_value();
int get_screen_backlight_value();
int set_screen_backlight_value(int backlight, int backend);
int calculate_keyboard_brightness_value(int light, int maxlight, int minlight);
int calculate_screen_backlight_value(int light, int maxlight, int minlight);
void fading(int from, int to);
void backlight_fading(int from, int to, int backend);
float get_session_idle_time(Display *display);
void signal_installer();

/* dbus.c */
int get_screensaver_active();
int set_keyboard_brightness_value(int brightness);
int dbus_get_screen_backlight_value();
int dbus_set_screen_backlight_value_gnome(int backlight);
int dbus_set_screen_backlight_value_kde(int backlight);
int dbus_set_screen_backlight_value(int backlight, int backend);

/* dbus-session.c */
DBusGConnection* get_dbus_connection();
DBusGProxy* get_dbus_proxy_manager(DBusGConnection *connection);
DBusGProxy* get_dbus_proxy_session(DBusGConnection *connection, DBusGProxy *proxy_manager);
int get_session_active (DBusGProxy *proxy_session);
