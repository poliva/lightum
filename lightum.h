#include <X11/Xlib.h>
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
} conf_data;

/* configfile.c */
conf_data config_parse();

/* functions.c */
int get_keyboard_brightness_value();
int get_light_sensor_value();
int get_screen_backlight_value();
int set_screen_backlight_value();
int calculate_keyboard_brightness_value(int light, int maxlight);
int calculate_screen_backlight_value(int light, int maxlight);
void fading(int from, int to);
void backlight_fading(int from, int to);
float get_session_idle_time(Display *display);
void signal_installer();

/* dbus.c */
int get_screensaver_active();
int set_keyboard_brightness_value(int brightness);
int dbus_get_screen_backlight_value();
int dbus_set_screen_backlight_value(int backlight);
