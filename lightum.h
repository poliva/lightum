/* functions.c */
int get_keyboard_brightness_value();
int get_light_sensor_value();
int calculate_keyboard_brightness_value(int light, int maxlight);
void fading(int from, int to);
float get_session_idle_time();

/* dbus.c */
int get_screensaver_active();
int set_keyboard_brightness_value(int brightness);
