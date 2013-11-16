#include "QTPlus.h"

/* Initialize listeners to show and hide Quick Tap Plus as well as update data */
void qtp_setup() {
	qtp_is_showing = false;
	accel_tap_service_subscribe(&qtp_tap_handler);
	qtp_bluetooth_image = gbitmap_create_with_resource(RESOURCE_ID_QTP_IMG_BT);
	qtp_battery_image = gbitmap_create_with_resource(RESOURCE_ID_QTP_IMG_BAT);
	
	if (qtp_is_show_weather()) {
		qtp_setup_app_message();
	}
}

/* Handle taps from the hardware */
void qtp_tap_handler(AccelAxisType axis, int32_t direction) {
	if (qtp_is_showing) {
		qtp_hide();
	} else {
		qtp_show();
	}
	qtp_is_showing = !qtp_is_showing;
}

/* Subscribe to taps and pass them to the handler */
void qtp_click_config_provider(Window *window) {
	window_single_click_subscribe(BUTTON_ID_BACK, qtp_back_click_responder);
}

/* Unusued. Subscribe to back button to exit */
void qtp_back_click_responder(ClickRecognizerRef recognizer, void *context) {
	qtp_hide();
}

/* Update the text layer for the battery status */
void qtp_update_battery_status(bool mark_dirty) {
	BatteryChargeState charge_state = battery_state_service_peek();
	static char battery_text[] = "100%";
	snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);

	text_layer_set_text(qtp_battery_text_layer, battery_text);
	if (mark_dirty) {
		layer_mark_dirty(text_layer_get_layer(qtp_battery_text_layer));
	}
}

/* Update the text layer for the bluetooth status */
void qtp_update_bluetooth_status(bool mark_dirty) {
	static char bluetooth_text[] = "Not Paired";

	if (bluetooth_connection_service_peek()) {
		snprintf(bluetooth_text, sizeof(bluetooth_text), "Paired");
	}

	text_layer_set_text(qtp_bluetooth_text_layer, bluetooth_text);
	if (mark_dirty) {
		layer_mark_dirty(text_layer_get_layer(qtp_bluetooth_text_layer));
	}
}

/* Update the text layer for the clock */
void qtp_update_time(bool mark_dirty) {
	static char time_text[10];
	clock_copy_time_string(time_text, sizeof(time_text));
	text_layer_set_text(qtp_time_layer, time_text);

	if (mark_dirty) {
		layer_mark_dirty(text_layer_get_layer(qtp_time_layer));
	}
}


/* Setup app message callbacks for weather */
void qtp_setup_app_message() {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "QTP: setting app message for weather");

	const int inbound_size = 100;
	const int outbound_size = 100;
	app_message_open(inbound_size, outbound_size);
	Tuplet initial_values[] = {
		TupletInteger(QTP_WEATHER_ICON_KEY, (uint8_t) 1),
		TupletCString(QTP_WEATHER_TEMP_F_KEY, "---\u00B0F"),
		TupletCString(QTP_WEATHER_TEMP_C_KEY, "---\u00B0F"),
		TupletCString(QTP_WEATHER_CITY_KEY, "Atlanta      "),
		TupletCString(QTP_WEATHER_DESC_KEY, "Scattered Thunderstorms")
	};
	APP_LOG(APP_LOG_LEVEL_DEBUG, "QTP: weather tuples intialized");

	app_sync_init(&qtp_sync, qtp_sync_buffer, sizeof(qtp_sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
	  qtp_sync_changed_callback, qtp_sync_error_callback, NULL);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "QTP: weather app message initialized");

}

static void qtp_sync_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
	
	switch (key) {
		case QTP_WEATHER_TEMP_F_KEY:
			APP_LOG(APP_LOG_LEVEL_DEBUG, "QTP: weather temp f received");
			if (qtp_is_showing && qtp_is_degrees_f()) {
				text_layer_set_text(qtp_temp_layer, new_tuple->value->cstring);
			}
			break;
		case QTP_WEATHER_TEMP_C_KEY:
			APP_LOG(APP_LOG_LEVEL_DEBUG, "QTP: weather temp c received");
			if (qtp_is_showing && !qtp_is_degrees_f()) {
				text_layer_set_text(qtp_temp_layer, new_tuple->value->cstring);
			}
			break;
		case QTP_WEATHER_DESC_KEY:
			APP_LOG(APP_LOG_LEVEL_DEBUG, "QTP: weather desc received: %s", new_tuple->value->cstring);
			if (qtp_is_showing) {
				text_layer_set_text(qtp_weather_desc_layer, new_tuple->value->cstring);
			}
			break;

	}

}

static void qtp_sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "QTP: weather app message error occurred: %d, %d", dict_error, app_message_error);
	if (DICT_NOT_ENOUGH_STORAGE == dict_error) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Not enough storage");
	}

	static char placeholder[] = "--\u00B0F";
	text_layer_set_text(qtp_temp_layer, placeholder);
}


/* Auto-hide the window after a certain time */
void qtp_timeout() {
	qtp_hide();
	qtp_is_showing = false;
}

/* Create the QTPlus Window and initialize the layres */
void qtp_init() {
	qtp_window = window_create();

	/* Time Layer */
	if (qtp_is_show_time()) {

		GRect time_frame = GRect( QTP_PADDING_X, QTP_PADDING_Y, QTP_SCREEN_WIDTH - QTP_PADDING_X, QTP_TIME_HEIGHT );
		qtp_time_layer = text_layer_create(time_frame);
		qtp_update_time(false);
		text_layer_set_text_alignment(qtp_time_layer, GTextAlignmentCenter);
		text_layer_set_font(qtp_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
		layer_add_child(window_get_root_layer(qtp_window), text_layer_get_layer(qtp_time_layer));
	}

	if (qtp_is_show_weather()) {

		GRect desc_frame = GRect( QTP_PADDING_X , qtp_weather_y() + QTP_WEATHER_SIZE, QTP_SCREEN_WIDTH - QTP_PADDING_X, QTP_WEATHER_SIZE);
		qtp_weather_desc_layer = text_layer_create(desc_frame);
		text_layer_set_font(qtp_weather_desc_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
		text_layer_set_text_alignment(qtp_weather_desc_layer, GTextAlignmentLeft);
		const Tuple *desc_tuple = app_sync_get(&qtp_sync, QTP_WEATHER_DESC_KEY);
		if (desc_tuple != NULL) {
			text_layer_set_text(qtp_weather_desc_layer, desc_tuple->value->cstring);
		}
		layer_add_child(window_get_root_layer(qtp_window), text_layer_get_layer(qtp_weather_desc_layer));


		GRect temp_frame = GRect( QTP_PADDING_X, qtp_weather_y(), QTP_SCREEN_WIDTH, QTP_WEATHER_SIZE);
		qtp_temp_layer = text_layer_create(temp_frame);
		text_layer_set_text_alignment(qtp_temp_layer, GTextAlignmentLeft);
		const Tuple *temp_tuple;
		if (qtp_is_degrees_f()) {
			temp_tuple = app_sync_get(&qtp_sync, QTP_WEATHER_TEMP_F_KEY);
		} else {
			temp_tuple = app_sync_get(&qtp_sync, QTP_WEATHER_TEMP_C_KEY);
		}
		if (temp_tuple != NULL) {
			text_layer_set_text(qtp_temp_layer, temp_tuple->value->cstring);
		}
		text_layer_set_font(qtp_temp_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
		layer_add_child(window_get_root_layer(qtp_window), text_layer_get_layer(qtp_temp_layer));

	}

	/* Bluetooth Logo layer */
	GRect battery_logo_frame = GRect( QTP_PADDING_X, qtp_battery_y(), QTP_BAT_ICON_SIZE, QTP_BAT_ICON_SIZE );
	qtp_battery_image_layer = bitmap_layer_create(battery_logo_frame);
	bitmap_layer_set_bitmap(qtp_battery_image_layer, qtp_battery_image);
	bitmap_layer_set_alignment(qtp_battery_image_layer, GAlignCenter);
	layer_add_child(window_get_root_layer(qtp_window), bitmap_layer_get_layer(qtp_battery_image_layer)); 

	/* Battery Status text layer */
	GRect battery_frame = GRect( 40, qtp_battery_y(), QTP_SCREEN_WIDTH - QTP_BAT_ICON_SIZE, QTP_BAT_ICON_SIZE );
	qtp_battery_text_layer =  text_layer_create(battery_frame);
	text_layer_set_font(qtp_battery_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	qtp_update_battery_status(false);
	layer_add_child(window_get_root_layer(qtp_window), text_layer_get_layer(qtp_battery_text_layer));

	/* Bluetooth Logo layer */
	GRect bluetooth_logo_frame = GRect(QTP_PADDING_X, qtp_bluetooth_y(), QTP_BT_ICON_SIZE, QTP_BT_ICON_SIZE);
	qtp_bluetooth_image_layer = bitmap_layer_create(bluetooth_logo_frame);
	bitmap_layer_set_bitmap(qtp_bluetooth_image_layer, qtp_bluetooth_image);
	bitmap_layer_set_alignment(qtp_bluetooth_image_layer, GAlignCenter);
	layer_add_child(window_get_root_layer(qtp_window), bitmap_layer_get_layer(qtp_bluetooth_image_layer)); 


	/* Bluetooth Status text layer */
	GRect bluetooth_frame = GRect(40,qtp_bluetooth_y(), QTP_SCREEN_WIDTH - QTP_BT_ICON_SIZE, QTP_BT_ICON_SIZE);
	qtp_bluetooth_text_layer =  text_layer_create(bluetooth_frame);
	text_layer_set_font(qtp_bluetooth_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	qtp_update_bluetooth_status(false);
	layer_add_child(window_get_root_layer(qtp_window), text_layer_get_layer(qtp_bluetooth_text_layer));

	/* Register for back button */
	//window_set_click_config_provider(qtp_window, (ClickConfigProvider)qtp_click_config_provider);

}


/* Deallocate QTPlus items when window is hidden */
void qtp_deinit() {
	text_layer_destroy(qtp_battery_text_layer);
	text_layer_destroy(qtp_bluetooth_text_layer);
	bitmap_layer_destroy(qtp_bluetooth_image_layer);
	window_destroy(qtp_window);
	app_timer_cancel(qtp_hide_timer);
}

/* Deallocate persistent QTPlus items when watchface exits */
void qtp_app_deinit() {
	gbitmap_destroy(qtp_battery_image);
	gbitmap_destroy(qtp_bluetooth_image);
	app_sync_deinit(&qtp_sync);

}

/* Create window, layers, text. Display QTPlus */
void qtp_show() {
	qtp_init();
	window_stack_push(qtp_window, true);
	qtp_hide_timer = app_timer_register(QTP_WINDOW_TIMEOUT, qtp_timeout, NULL);

}

/* Hide QTPlus. Free memory */
void qtp_hide() {
	window_stack_pop(true);
	qtp_deinit();
}


bool qtp_is_show_time() {
	return (qtp_conf & QTP_K_SHOW_TIME) == QTP_K_SHOW_TIME;
}
bool qtp_is_show_weather() {
	return (qtp_conf & QTP_K_SHOW_WEATHER) == QTP_K_SHOW_WEATHER;
}
bool qtp_is_autohide() {
	return (qtp_conf & QTP_K_AUTOHIDE) == QTP_K_AUTOHIDE;
}
bool qtp_is_degrees_f() {
	return (qtp_conf & QTP_K_DEGREES_F) == QTP_K_DEGREES_F;
}

int qtp_battery_y() {
	if (qtp_is_show_time()) {
		return QTP_BATTERY_BASE_Y + QTP_TIME_HEIGHT + QTP_PADDING_Y;
	} else {
		return QTP_BATTERY_BASE_Y + QTP_PADDING_Y;
	}
}

int qtp_bluetooth_y() {
	if (qtp_is_show_time()) {
		return QTP_BLUETOOTH_BASE_Y + QTP_TIME_HEIGHT + QTP_PADDING_Y;
	} else {
		return QTP_BLUETOOTH_BASE_Y + QTP_PADDING_Y;
	}
}

int qtp_weather_y() {
	if (qtp_is_show_time()) {
		return QTP_WEATHER_BASE_Y + QTP_PADDING_Y + QTP_TIME_HEIGHT;

	} else {
		return QTP_WEATHER_BASE_Y + QTP_PADDING_Y;
	}
}
