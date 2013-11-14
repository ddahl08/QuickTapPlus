#include <pebble.h>
/*
* User settings
* Show Clock
* Show Weather
* Autohide
*/

// Config
#define QTP_WINDOW_TIMEOUT 2000
#define QTP_K_SHOW_TIME 1
#define QTP_K_SHOW_WEATHER 2
#define QTP_K_AUTOHIDE 4

#define QTP_SCREEN_WIDTH        144
#define QTP_SCREEN_HEIGHT       168

#define QTP_PADDING_Y 5
#define QTP_PADDING_X 5
#define QTP_BT_ICON_SIZE 32
#define QTP_BAT_ICON_SIZE 32
#define QTP_TIME_HEIGHT 32
#define QTP_BATTERY_BASE_Y 0
#define QTP_BLUETOOTH_BASE_Y QTP_BAT_ICON_SIZE + 5


// Items
static Window *qtp_window;
bool qtp_is_showing;
TextLayer *qtp_battery_text_layer;
TextLayer *qtp_bluetooth_text_layer;
TextLayer *qtp_time_layer;
AppTimer *qtp_hide_timer;
GBitmap *qtp_bluetooth_image;
BitmapLayer *qtp_bluetooth_image_layer;
GBitmap *qtp_battery_image;
BitmapLayer *qtp_battery_image_layer;
int qtp_conf;

// Methods
void qtp_setup();
void qtp_app_deinit();

void qtp_show();
void qtp_hide();
void qtp_timeout();

void qtp_tap_handler(AccelAxisType axis, int32_t direction);
void qtp_click_config_provider(Window *window);
void qtp_back_click_responder(ClickRecognizerRef recognizer, void *context);

void qtp_update_battery_status(bool mark_dirty);
void qtp_update_bluetooth_status(bool mark_dirty);
void qtp_update_time(bool mark_dirty);

void qtp_init();
void qtp_deinit();

// Helpers
bool qtp_is_show_time();
bool qtp_is_show_weather();
bool qtp_is_autohide();

int qtp_battery_y();
int qtp_bluetooth_y();


