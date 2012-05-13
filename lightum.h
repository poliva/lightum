#include <X11/Xlib.h>
#include <dbus/dbus-glib-lowlevel.h>

#define CONFIG_PATH ".config/lightum/"
#define CONFIG_FILE "lightum.conf"
#define PID_FILE "lightum.pid"

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (!FALSE)
#endif

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
int file_exists(char* file);
char* default_config_dir();
int create_config_dir(char* path);

/* functions.c */
int get_keyboard_brightness_value();
int get_light_sensor_value();
int get_screen_backlight_value();
int dbus_to_acpi_backlight(int backlight);
int acpi_to_dbus_backlight(int backlight);
int set_screen_backlight_value(int backlight, int backend);
int calculate_keyboard_brightness_value(int light, int maxlight, int minlight);
int calculate_screen_backlight_value(int light, int maxbacklight, int minbacklight);
void fading(int from, int to);
void backlight_fading(int from, int to, int backend);
float get_session_idle_time(Display *display);
void signal_installer();
int remove_pid_file();
int create_pid_file();

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
