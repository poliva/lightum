#include <X11/Xlib.h>
/* config file */
typedef struct {
	int manualmode;
	int queryscreensaver;
	int maxbrightness;
	int polltime;
	int idleoff;
} conf_data;

/* configfile.c */
conf_data config_parse();

/* functions.c */
int get_keyboard_brightness_value();
int get_light_sensor_value();
int calculate_keyboard_brightness_value(int light, int maxlight);
void fading(int from, int to);
float get_session_idle_time(Display *display);

/* dbus.c */
int get_screensaver_active();
int set_keyboard_brightness_value(int brightness);
