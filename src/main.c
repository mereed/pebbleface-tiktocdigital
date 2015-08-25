/*
Copyright (C) 2015 Mark Reed

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "pebble.h"
#include "effect_layer.h"

EffectLayer  *effect_layer_mask;
EffectMask mask;

static AppSync sync;
static uint8_t sync_buffer[256];

enum WeatherKey {
  BLUETOOTHVIBE_KEY = 0x0,
  HOURLYVIBE_KEY = 0x1,
  FLIP_KEY = 0x2,
  COLOUR_KEY = 0x3,
  BLINK_KEY = 0x4
};

int cur_day = -1;
static GFont *date_font;

static int bluetoothvibe;
static int hourlyvibe;
static int flip;
static int colour;
static int blink;

TextLayer *layer_ampm_text;

static Window *window;
static Layer *window_layer;

TextLayer *layer_date_text;


static GBitmap *bluetooth_image;
static BitmapLayer *bluetooth_layer;

static bool appStarted = false;

BitmapLayer *layer_batt_img;
GBitmap *img_battery_100;
GBitmap *img_battery_30;
GBitmap *img_battery_00;
GBitmap *img_battery_charge;
int charge_percent = 0;

#define TOTAL_NORMAL_DIGITS 4 
static GBitmap *normal_time_digits_images[TOTAL_NORMAL_DIGITS];
static BitmapLayer *normal_time_digits_layers[TOTAL_NORMAL_DIGITS];

const int NORMAL_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_S0,
  RESOURCE_ID_IMAGE_NUM_S1,
  RESOURCE_ID_IMAGE_NUM_S2,
  RESOURCE_ID_IMAGE_NUM_S3,
  RESOURCE_ID_IMAGE_NUM_S4,
  RESOURCE_ID_IMAGE_NUM_S5,
  RESOURCE_ID_IMAGE_NUM_S6,
  RESOURCE_ID_IMAGE_NUM_S7,
  RESOURCE_ID_IMAGE_NUM_S8,
  RESOURCE_ID_IMAGE_NUM_S9,
};


void colourswap() {

if (mask.bitmap_background != NULL) {
  gbitmap_destroy(mask.bitmap_background);
  mask.bitmap_background = NULL;
}
	switch (colour) {
		case 0:
		layer_set_hidden(effect_layer_get_layer(effect_layer_mask),true);
		break;
		
		case 1:
		layer_set_hidden(effect_layer_get_layer(effect_layer_mask),false);
		mask.bitmap_background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MASK7);
		break;
		
		case 2:
		layer_set_hidden(effect_layer_get_layer(effect_layer_mask),false);
  		mask.bitmap_background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MASK);
		break;
			
		case 3:
		layer_set_hidden(effect_layer_get_layer(effect_layer_mask),false);
		mask.bitmap_background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MASK2);
		break;
		
		case 4:
		layer_set_hidden(effect_layer_get_layer(effect_layer_mask),false);
		mask.bitmap_background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MASK6);
		break; 
		
		case 5:
		layer_set_hidden(effect_layer_get_layer(effect_layer_mask),false);
  		mask.bitmap_background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MASK5);
		break;
			
		case 6:
		layer_set_hidden(effect_layer_get_layer(effect_layer_mask),false);
		mask.bitmap_background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MASK4);
		break;
		
		case 7:
		layer_set_hidden(effect_layer_get_layer(effect_layer_mask),false);
		mask.bitmap_background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MASK1);
		break;
		
		case 8:
		layer_set_hidden(effect_layer_get_layer(effect_layer_mask),false);
		mask.bitmap_background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MASK3);
		break; 
    }
    
	if (mask.bitmap_background != NULL) {
	effect_layer_add_effect(effect_layer_mask, effect_mask, &mask);
	layer_mark_dirty(effect_layer_get_layer(effect_layer_mask));
	}
}



static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint origin) {
  GBitmap *old_image = *bmp_image;

  *bmp_image = gbitmap_create_with_resource(resource_id);
#ifdef PBL_PLATFORM_BASALT
  GRect bitmap_bounds = gbitmap_get_bounds((*bmp_image));
#else
  GRect bitmap_bounds = (*bmp_image)->bounds;
#endif
  GRect frame = GRect(origin.x, origin.y, bitmap_bounds.size.w, bitmap_bounds.size.h);
  bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
  layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);

  if (old_image != NULL) {
  	gbitmap_destroy(old_image);
  }
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed);

static void sync_tuple_changed_callback(const uint32_t key,
                                        const Tuple* new_tuple,
                                        const Tuple* old_tuple,
                                        void* context) {

  // App Sync keeps new_tuple in sync_buffer, so we may use it directly
  switch (key) {
	  
    case BLUETOOTHVIBE_KEY:
      bluetoothvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(BLUETOOTHVIBE_KEY, bluetoothvibe);
      break;      
	  
    case HOURLYVIBE_KEY:
      hourlyvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(HOURLYVIBE_KEY, hourlyvibe);	  
      break;	   
	  
	case FLIP_KEY:
      flip = new_tuple->value->uint8;
	  persist_write_bool(FLIP_KEY, flip);
	break;
	  
	case COLOUR_KEY:
      colour = new_tuple->value->uint8;
	  persist_write_bool(COLOUR_KEY, colour);
	  colourswap();
	break;
	
	case BLINK_KEY:
      blink = new_tuple->value->uint8;
	  persist_write_bool(BLINK_KEY, blink);
      tick_timer_service_unsubscribe();
      if(blink) {
        tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
      }
      else {
        tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
	  }
   break;	
  }
}

void update_battery(BatteryChargeState charge_state) {

    if (charge_state.is_charging) {
        bitmap_layer_set_bitmap(layer_batt_img, img_battery_charge);

    } else {
		  if (charge_state.charge_percent <= 05) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_00);	
        } else if (charge_state.charge_percent <= 30) {    
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_30);	
		} else if (charge_state.charge_percent <= 100) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_100);	
    }
    charge_percent = charge_state.charge_percent;
    }
}

static void toggle_bluetooth_icon(bool connected) {
  if(appStarted && !connected && bluetoothvibe) {
    //vibe!
    vibes_short_pulse();
  }
  layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), connected);
}


void bluetooth_connection_callback(bool connected) {
  toggle_bluetooth_icon(connected);
}

unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) {
    return hour;
  }
  unsigned short display_hour = hour % 12;
  // Converts "0" to "12"
  return display_hour ? display_hour : 12;
}

static void update_hours(struct tm *tick_time) {

	if (appStarted && hourlyvibe) {
    //vibe!
    vibes_short_pulse();
	}
	
  unsigned short display_hour = get_display_hour(tick_time->tm_hour);

  set_container_image(&normal_time_digits_images[0], normal_time_digits_layers[0], NORMAL_IMAGE_RESOURCE_IDS[display_hour/10], GPoint(0, 40));
  set_container_image(&normal_time_digits_images[1], normal_time_digits_layers[1], NORMAL_IMAGE_RESOURCE_IDS[display_hour%10], GPoint(34, 40));

	
	if (!clock_is_24h_style()) {
	
	if (display_hour/10 == 0) {		
		 	layer_set_hidden(bitmap_layer_get_layer(normal_time_digits_layers[0]), true); 
		  } else  if (flip == 0) {
		    layer_set_hidden(bitmap_layer_get_layer(normal_time_digits_layers[0]), false);
            	}
	}		
	
				 static char ampm_text[] = "am";

		     if (!clock_is_24h_style()) {


		strftime(ampm_text, sizeof(ampm_text), "%P", tick_time);
        text_layer_set_text(layer_ampm_text, ampm_text);
			 }
}

static void update_minutes(struct tm *tick_time) {

  set_container_image(&normal_time_digits_images[2], normal_time_digits_layers[2], NORMAL_IMAGE_RESOURCE_IDS[tick_time->tm_min/10], GPoint(77, 40));
  set_container_image(&normal_time_digits_images[3], normal_time_digits_layers[3], NORMAL_IMAGE_RESOURCE_IDS[tick_time->tm_min%10], GPoint(111, 40));
	
}

static void update_seconds(struct tm *tick_time) {
  if(blink) {
    layer_set_hidden(bitmap_layer_get_layer(layer_batt_img), tick_time->tm_sec%5);
  }
  else {
    if(layer_get_hidden(bitmap_layer_get_layer(layer_batt_img))) {
      layer_set_hidden(bitmap_layer_get_layer(layer_batt_img), false);
    }
  }
}


static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
	
  if (units_changed & DAY_UNIT) {	  
	  
  static char date_text[] = "XXXX";
	  
    int new_cur_day = tick_time->tm_year*1000 + tick_time->tm_yday;
    if (new_cur_day != cur_day) {
        cur_day = new_cur_day;

	switch(tick_time->tm_mday)
  {
    case 1 :
    case 21 :
    case 31 :
      strftime(date_text, sizeof(date_text), "%est", tick_time);
      break;
    case 2 :
    case 22 :
      strftime(date_text, sizeof(date_text), "%end", tick_time);
      break;
    case 3 :
    case 23 :
      strftime(date_text, sizeof(date_text), "%erd", tick_time);
      break;
    default :
      strftime(date_text, sizeof(date_text), "%eth", tick_time);
      break;
  }
	  text_layer_set_text(layer_date_text, date_text);
		
	}
  }
	
  if (units_changed & HOUR_UNIT) {
   update_hours(tick_time);
  }
  if (units_changed & MINUTE_UNIT) {
   update_minutes(tick_time);
  }
  if (units_changed & SECOND_UNIT) {
    update_seconds(tick_time);
  }			
}

void force_update(void) {
    update_battery(battery_state_service_peek());
    toggle_bluetooth_icon(bluetooth_connection_service_peek());
}

static void init(void) {

  memset(&normal_time_digits_layers, 0, sizeof(normal_time_digits_layers));
  memset(&normal_time_digits_images, 0, sizeof(normal_time_digits_images));
		
  // Setup messaging
  const int inbound_size = 256;
  const int outbound_size = 256;
  app_message_open(inbound_size, outbound_size);	
	
  window = window_create();
  if (window == NULL) {
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "OOM: couldn't allocate window");
      return;
  }
  window_stack_push(window, true /* Animated */);
  window_layer = window_get_root_layer(window);

  window_set_background_color(window, GColorBlack);


  GRect dummy_frame = { {0, 0}, {0, 0} };

	for (int i = 0; i < TOTAL_NORMAL_DIGITS; ++i) {
    normal_time_digits_layers[i] = bitmap_layer_create(dummy_frame);
   layer_add_child(window_layer, bitmap_layer_get_layer(normal_time_digits_layers[i]));
  }	
 
	
	 // ** { begin setup mask for MASK effect
  mask.text = NULL;
  mask.bitmap_mask = NULL;

  #ifdef PBL_COLOR
    mask.mask_colors = malloc(sizeof(GColor)*4);
    mask.mask_colors[0] = GColorWhite;
    mask.mask_colors[1] = GColorDarkGray;
    mask.mask_colors[2] = GColorLightGray;
//    mask.mask_colors[3] = GColorClear;
  #else
    mask.mask_colors = malloc(sizeof(GColor)*2);
    mask.mask_colors[0] = GColorWhite;
    mask.mask_colors[1] = GColorClear;
  #endif
	  
  mask.background_color = GColorClear;
  mask.bitmap_background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MASK);


  // ** end setup mask }
	
  //creating effect layer
  effect_layer_mask = effect_layer_create(GRect(0,0,144,168));
  effect_layer_add_effect(effect_layer_mask, effect_mask, &mask);
  layer_add_child((window_layer), effect_layer_get_layer(effect_layer_mask));

	
	img_battery_100   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_080_100);
    img_battery_30   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_005_030);
    img_battery_00   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_000_005);
    img_battery_charge = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_CHARGING);
    layer_batt_img  = bitmap_layer_create(GRect(66, 102, 9,11));
	bitmap_layer_set_bitmap(layer_batt_img, img_battery_100);
	layer_add_child(window_layer, bitmap_layer_get_layer(layer_batt_img));
		
   bluetooth_image = gbitmap_create_with_resource(RESOURCE_ID_ICON_NOBLUETOOTH);
#ifdef PBL_PLATFORM_BASALT
  GRect bitmap_bounds_bt_on = gbitmap_get_bounds(bluetooth_image);
#else
  GRect bitmap_bounds_bt_on = bluetooth_image->bounds;
#endif	
  GRect frame_bt = GRect(66, 102, bitmap_bounds_bt_on.size.w, bitmap_bounds_bt_on.size.h);
  bluetooth_layer = bitmap_layer_create(frame_bt);
  bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(bluetooth_layer));
	
	date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ODIN_24));

	layer_date_text = text_layer_create(GRect(0, 10, 144, 26));
	text_layer_set_text_color(layer_date_text, GColorWhite);		
    text_layer_set_background_color(layer_date_text, GColorClear);
    text_layer_set_font(layer_date_text, date_font);
    text_layer_set_text_alignment(layer_date_text, GTextAlignmentRight);
    layer_add_child(window_layer, text_layer_get_layer(layer_date_text));

	layer_ampm_text = text_layer_create(GRect(0, 114, 140, 26));
    text_layer_set_text_color(layer_ampm_text, GColorWhite);
	text_layer_set_background_color(layer_ampm_text, GColorClear);
    text_layer_set_font(layer_ampm_text, date_font);
    text_layer_set_text_alignment(layer_ampm_text, GTextAlignmentRight);
    layer_add_child(window_layer, text_layer_get_layer(layer_ampm_text));

	
  // Avoids a blank screen on watch start.
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);  
  handle_tick(tick_time, MONTH_UNIT + DAY_UNIT + HOUR_UNIT + MINUTE_UNIT + SECOND_UNIT);

  Tuplet initial_values[] = {
    TupletInteger(BLUETOOTHVIBE_KEY, persist_read_bool(BLUETOOTHVIBE_KEY)),
    TupletInteger(HOURLYVIBE_KEY, persist_read_bool(HOURLYVIBE_KEY)),
	TupletInteger(FLIP_KEY, persist_read_bool(FLIP_KEY)),
	TupletInteger(COLOUR_KEY, persist_read_bool(COLOUR_KEY)),
	TupletInteger(BLINK_KEY, persist_read_bool(BLINK_KEY)),
  };

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values,
                ARRAY_LENGTH(initial_values), sync_tuple_changed_callback,
                NULL, NULL);

  appStarted = true;
 
	 // handlers
    battery_state_service_subscribe(&update_battery);
    bluetooth_connection_service_subscribe(&bluetooth_connection_callback);
    tick_timer_service_subscribe(SECOND_UNIT, handle_tick);

	 // draw first frame
    force_update();
}

static void deinit(void) {
  app_sync_deinit(&sync);

  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();


  layer_remove_from_parent(bitmap_layer_get_layer(bluetooth_layer));
  bitmap_layer_destroy(bluetooth_layer);
  gbitmap_destroy(bluetooth_image);

	
  layer_remove_from_parent(bitmap_layer_get_layer(layer_batt_img));
  bitmap_layer_destroy(layer_batt_img);
  gbitmap_destroy(img_battery_100);
  gbitmap_destroy(img_battery_30);
  gbitmap_destroy(img_battery_00);
  gbitmap_destroy(img_battery_charge);	


	for (int i = 0; i < TOTAL_NORMAL_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(normal_time_digits_layers[i]));
    gbitmap_destroy(normal_time_digits_images[i]);
    normal_time_digits_images[i] = NULL;
    bitmap_layer_destroy(normal_time_digits_layers[i]);
	normal_time_digits_layers[i] = NULL;
  }
	
  text_layer_destroy( layer_date_text );
  text_layer_destroy( layer_ampm_text );
  fonts_unload_custom_font(date_font);

  //clearning MASK
  gbitmap_destroy(mask.bitmap_background);
  effect_layer_destroy(effect_layer_mask);
	

  layer_remove_from_parent(window_layer);
  layer_destroy(window_layer);
	

}

int main(void) {
  init();
  app_event_loop();
  deinit();
}