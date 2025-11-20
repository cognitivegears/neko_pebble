#include <pebble.h>
#include "neko_logic.h"

static Window *s_main_window;
static Layer *s_canvas_layer;
static GBitmap *s_frame_bitmaps[NEKO_FRAME_COUNT];
static AppTimer *s_timer;
static NekoContext s_neko_ctx;
static GRect s_bounds;

static const uint32_t FRAME_RESOURCE_IDS[NEKO_FRAME_COUNT] = {
  RESOURCE_ID_CAT_FRAME_00,
  RESOURCE_ID_CAT_FRAME_01,
  RESOURCE_ID_CAT_FRAME_02,
  RESOURCE_ID_CAT_FRAME_03,
  RESOURCE_ID_CAT_FRAME_04,
  RESOURCE_ID_CAT_FRAME_05,
  RESOURCE_ID_CAT_FRAME_06,
  RESOURCE_ID_CAT_FRAME_07,
  RESOURCE_ID_CAT_FRAME_08,
  RESOURCE_ID_CAT_FRAME_09,
  RESOURCE_ID_CAT_FRAME_10,
  RESOURCE_ID_CAT_FRAME_11,
  RESOURCE_ID_CAT_FRAME_12,
  RESOURCE_ID_CAT_FRAME_13,
  RESOURCE_ID_CAT_FRAME_14,
  RESOURCE_ID_CAT_FRAME_15,
  RESOURCE_ID_CAT_FRAME_16,
  RESOURCE_ID_CAT_FRAME_17,
  RESOURCE_ID_CAT_FRAME_18,
  RESOURCE_ID_CAT_FRAME_19,
  RESOURCE_ID_CAT_FRAME_20,
  RESOURCE_ID_CAT_FRAME_21,
  RESOURCE_ID_CAT_FRAME_22,
  RESOURCE_ID_CAT_FRAME_23,
  RESOURCE_ID_CAT_FRAME_24,
  RESOURCE_ID_CAT_FRAME_25,
  RESOURCE_ID_CAT_FRAME_26,
  RESOURCE_ID_CAT_FRAME_27,
  RESOURCE_ID_CAT_FRAME_28,
  RESOURCE_ID_CAT_FRAME_29,
  RESOURCE_ID_CAT_FRAME_30,
  RESOURCE_ID_CAT_FRAME_31
};

static uint32_t prv_now_ms(void) {
  time_t seconds;
  uint16_t ms;
  time_ms(&seconds, &ms);
  return (uint32_t)seconds * 1000 + ms;
}

static void prv_schedule_timer(void);

static void prv_load_bitmaps(void) {
  for (int i = 0; i < NEKO_FRAME_COUNT; ++i) {
    s_frame_bitmaps[i] = gbitmap_create_with_resource(FRAME_RESOURCE_IDS[i]);
  }
}

static void prv_unload_bitmaps(void) {
  for (int i = 0; i < NEKO_FRAME_COUNT; ++i) {
    if (s_frame_bitmaps[i]) {
      gbitmap_destroy(s_frame_bitmaps[i]);
      s_frame_bitmaps[i] = NULL;
    }
  }
}

static void prv_draw_canvas(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, s_bounds, 0, GCornerNone);

  NekoPoint target = neko_context_target(&s_neko_ctx);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorVividCerulean, GColorWhite));
  graphics_fill_circle(ctx, GPoint(target.x, target.y), NEKO_TARGET_RADIUS);
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorPictonBlue, GColorBlack));
  graphics_draw_circle(ctx, GPoint(target.x, target.y), NEKO_TARGET_RADIUS);

  uint8_t frame_index = neko_context_current_frame(&s_neko_ctx);
  if (frame_index < NEKO_FRAME_COUNT && s_frame_bitmaps[frame_index]) {
    NekoPoint cat_pos = neko_context_cat_position(&s_neko_ctx);
    GRect frame = GRect(cat_pos.x, cat_pos.y, NEKO_SPRITE_SIZE, NEKO_SPRITE_SIZE);
    graphics_draw_bitmap_in_rect(ctx, s_frame_bitmaps[frame_index], frame);
  }
}

static void prv_tick(void *context) {
  uint32_t now = prv_now_ms();
  neko_context_tick(&s_neko_ctx, now);
  layer_mark_dirty(s_canvas_layer);
  prv_schedule_timer();
}

static void prv_schedule_timer(void) {
  if (s_timer) {
    app_timer_cancel(s_timer);
  }
  s_timer = app_timer_register(NEKO_ANIMATION_INTERVAL_MS, prv_tick, NULL);
}

static void prv_click_select_handler(ClickRecognizerRef recognizer, void *context) {
  neko_context_randomize_target(&s_neko_ctx);
}

static void prv_click_up_handler(ClickRecognizerRef recognizer, void *context) {
  neko_context_nudge_target(&s_neko_ctx, -NEKO_STEP_PIXELS, -NEKO_STEP_PIXELS);
}

static void prv_click_down_handler(ClickRecognizerRef recognizer, void *context) {
  neko_context_nudge_target(&s_neko_ctx, NEKO_STEP_PIXELS, NEKO_STEP_PIXELS);
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_click_select_handler);
  window_single_click_subscribe(BUTTON_ID_UP, prv_click_up_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_click_down_handler);
}

static void prv_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_bounds = layer_get_bounds(root);

  s_canvas_layer = layer_create(s_bounds);
  layer_set_update_proc(s_canvas_layer, prv_draw_canvas);
  layer_add_child(root, s_canvas_layer);

  uint32_t now = prv_now_ms();
  NekoBounds bounds = {
    .width = s_bounds.size.w,
    .height = s_bounds.size.h
  };
  neko_context_init(&s_neko_ctx, bounds, now, (uint32_t)time(NULL));
  prv_schedule_timer();
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  s_canvas_layer = NULL;
}

static void init(void) {
  prv_load_bitmaps();
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_click_config_provider(s_main_window, prv_click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload
  });
  window_stack_push(s_main_window, true);
}

static void deinit(void) {
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
  prv_unload_bitmaps();
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
