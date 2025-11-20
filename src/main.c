#include <pebble.h>

#define FRAME_COUNT 32
#define CAT_SIZE 32
#define ANIMATION_INTERVAL_MS 300
#define STEP_PIXELS 10
#define HALF_STEP (STEP_PIXELS / 2)
#define SCRATCH_MIN_MS 2000
#define STOP_CHANCE_FACTOR 30
#define START_CHANCE_FACTOR 10
#define TARGET_INDICATOR_RADIUS 5

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(array) (int)(sizeof(array) / sizeof((array)[0]))
#endif

typedef enum {
  STATE_SITTING,
  STATE_MOVING_LEFT,
  STATE_MOVING_UP,
  STATE_MOVING_RIGHT,
  STATE_MOVING_DOWN,
  STATE_MOVING_UP_RIGHT,
  STATE_MOVING_UP_LEFT,
  STATE_MOVING_DOWN_LEFT,
  STATE_MOVING_DOWN_RIGHT,
  STATE_SCRATCHING_LEFT,
  STATE_SCRATCHING_RIGHT,
  STATE_SCRATCHING_DOWN,
  STATE_SCRATCHING_UP,
  STATE_CLEANING,
  STATE_SLEEPING,
  STATE_COUNT
} CatState;

typedef struct {
  CatState state;
  const uint8_t *frames;
  uint8_t frame_count;
  GPoint step;
  bool is_moving;
  bool is_scratching;
} CatStateConfig;

static const uint8_t FRAMES_SITTING[] = {0};
static const uint8_t FRAMES_MOVE_DOWN[] = {8, 9};
static const uint8_t FRAMES_MOVE_DOWN_RIGHT[] = {10, 11};
static const uint8_t FRAMES_MOVE_RIGHT[] = {12, 13};
static const uint8_t FRAMES_MOVE_UP_RIGHT[] = {14, 15};
static const uint8_t FRAMES_MOVE_UP[] = {16, 17};
static const uint8_t FRAMES_MOVE_UP_LEFT[] = {18, 19};
static const uint8_t FRAMES_MOVE_LEFT[] = {20, 21};
static const uint8_t FRAMES_MOVE_DOWN_LEFT[] = {22, 23};
static const uint8_t FRAMES_SCRATCH_DOWN[] = {24, 25};
static const uint8_t FRAMES_SCRATCH_RIGHT[] = {26, 27};
static const uint8_t FRAMES_SCRATCH_UP[] = {28, 29};
static const uint8_t FRAMES_SCRATCH_LEFT[] = {30, 31};
static const uint8_t FRAMES_CLEANING[] = {
  0, 0,
  1, 1, 2, 3, 2, 3,
  1, 1, 2, 3, 2, 3,
  0, 0, 0
};
static const uint8_t FRAMES_SLEEPING[] = {
  0, 0,
  4, 4, 4, 0, 0, 4, 4, 4, 0, 0,
  5, 6, 5, 6, 5, 6, 5, 6, 5, 6,
  7, 7,
  0, 0, 0
};

static const CatStateConfig STATE_CONFIG[STATE_COUNT] = {
  { STATE_SITTING,          FRAMES_SITTING,       ARRAY_LENGTH(FRAMES_SITTING),       { 0,           0 }, false, false },
  { STATE_MOVING_LEFT,      FRAMES_MOVE_LEFT,     ARRAY_LENGTH(FRAMES_MOVE_LEFT),     { -STEP_PIXELS, 0 }, true,  false },
  { STATE_MOVING_UP,        FRAMES_MOVE_UP,       ARRAY_LENGTH(FRAMES_MOVE_UP),       { 0, -STEP_PIXELS }, true,  false },
  { STATE_MOVING_RIGHT,     FRAMES_MOVE_RIGHT,    ARRAY_LENGTH(FRAMES_MOVE_RIGHT),    { STEP_PIXELS,  0 }, true,  false },
  { STATE_MOVING_DOWN,      FRAMES_MOVE_DOWN,     ARRAY_LENGTH(FRAMES_MOVE_DOWN),     { 0,  STEP_PIXELS }, true,  false },
  { STATE_MOVING_UP_RIGHT,  FRAMES_MOVE_UP_RIGHT, ARRAY_LENGTH(FRAMES_MOVE_UP_RIGHT), { HALF_STEP, -HALF_STEP }, true, false },
  { STATE_MOVING_UP_LEFT,   FRAMES_MOVE_UP_LEFT,  ARRAY_LENGTH(FRAMES_MOVE_UP_LEFT),  { -HALF_STEP, -HALF_STEP }, true, false },
  { STATE_MOVING_DOWN_LEFT, FRAMES_MOVE_DOWN_LEFT,ARRAY_LENGTH(FRAMES_MOVE_DOWN_LEFT),{ -HALF_STEP, HALF_STEP }, true, false },
  { STATE_MOVING_DOWN_RIGHT,FRAMES_MOVE_DOWN_RIGHT,ARRAY_LENGTH(FRAMES_MOVE_DOWN_RIGHT),{ HALF_STEP, HALF_STEP }, true, false },
  { STATE_SCRATCHING_LEFT,  FRAMES_SCRATCH_LEFT,  ARRAY_LENGTH(FRAMES_SCRATCH_LEFT),  { 0, 0 }, false, true },
  { STATE_SCRATCHING_RIGHT, FRAMES_SCRATCH_RIGHT, ARRAY_LENGTH(FRAMES_SCRATCH_RIGHT), { 0, 0 }, false, true },
  { STATE_SCRATCHING_DOWN,  FRAMES_SCRATCH_DOWN,  ARRAY_LENGTH(FRAMES_SCRATCH_DOWN),  { 0, 0 }, false, true },
  { STATE_SCRATCHING_UP,    FRAMES_SCRATCH_UP,    ARRAY_LENGTH(FRAMES_SCRATCH_UP),    { 0, 0 }, false, true },
  { STATE_CLEANING,         FRAMES_CLEANING,      ARRAY_LENGTH(FRAMES_CLEANING),      { 0, 0 }, false, false },
  { STATE_SLEEPING,         FRAMES_SLEEPING,      ARRAY_LENGTH(FRAMES_SLEEPING),      { 0, 0 }, false, false }
};

static const CatState MOVING_STATES[] = {
  STATE_MOVING_UP,
  STATE_MOVING_DOWN,
  STATE_MOVING_LEFT,
  STATE_MOVING_RIGHT,
  STATE_MOVING_UP_LEFT,
  STATE_MOVING_UP_RIGHT,
  STATE_MOVING_DOWN_LEFT,
  STATE_MOVING_DOWN_RIGHT
};

static const uint32_t FRAME_RESOURCE_IDS[FRAME_COUNT] = {
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

static Window *s_main_window;
static Layer *s_canvas_layer;
static GRect s_bounds;
static GBitmap *s_frame_bitmaps[FRAME_COUNT];
static GBitmap *s_current_bitmap;
static uint8_t s_current_frame_index;
static uint8_t s_animation_index;
static CatState s_state = STATE_SITTING;
static GPoint s_cat_pos;
static GPoint s_target;
static bool s_has_target = true;
static AppTimer *s_timer;
static uint32_t s_last_state_change_ms;

static uint32_t prv_now_ms(void) {
  time_t seconds;
  uint16_t ms;
  time_ms(&seconds, &ms);
  return (uint32_t)seconds * 1000 + ms;
}

static void prv_set_state(CatState new_state, uint32_t now, bool force_reset);
static void prv_schedule_timer(void);
static void prv_update_direction_toward_target(uint32_t now);
static void prv_clamp_and_handle_edges(uint32_t now);
static bool prv_point_inside_cat(GPoint point);
static CatState prv_random_rest_state(void);
static CatState prv_random_moving_state(void);
static void prv_move_target_random(void);
static void prv_nudge_target(int dx, int dy);
static void prv_draw_canvas(Layer *layer, GContext *ctx);
static void prv_tick(void *context);

static void prv_update_current_frame(void) {
  const CatStateConfig *config = &STATE_CONFIG[s_state];
  if (s_animation_index >= config->frame_count) {
    s_animation_index = 0;
  }
  s_current_frame_index = config->frames[s_animation_index];
  s_current_bitmap = s_frame_bitmaps[s_current_frame_index];
}

static void prv_set_state(CatState new_state, uint32_t now, bool force_reset) {
  if (!force_reset && new_state == s_state) {
    return;
  }
  s_state = new_state;
  s_animation_index = 0;
  s_last_state_change_ms = now;
  prv_update_current_frame();
}

static CatState prv_random_rest_state(void) {
  return (rand() % 2 == 0) ? STATE_CLEANING : STATE_SLEEPING;
}

static CatState prv_random_moving_state(void) {
  return MOVING_STATES[rand() % ARRAY_LENGTH(MOVING_STATES)];
}

static bool prv_point_inside_cat(GPoint point) {
  return point.x >= s_cat_pos.x && point.x <= s_cat_pos.x + CAT_SIZE &&
         point.y >= s_cat_pos.y && point.y <= s_cat_pos.y + CAT_SIZE;
}

static CatState prv_direction_from_delta(int dx, int dy) {
  const int threshold = STEP_PIXELS / 2;
  if (dx > threshold) {
    if (dy > threshold) {
      return STATE_MOVING_DOWN_RIGHT;
    } else if (dy < -threshold) {
      return STATE_MOVING_UP_RIGHT;
    } else {
      return STATE_MOVING_RIGHT;
    }
  } else if (dx < -threshold) {
    if (dy > threshold) {
      return STATE_MOVING_DOWN_LEFT;
    } else if (dy < -threshold) {
      return STATE_MOVING_UP_LEFT;
    } else {
      return STATE_MOVING_LEFT;
    }
  } else {
    if (dy > threshold) {
      return STATE_MOVING_DOWN;
    } else if (dy < -threshold) {
      return STATE_MOVING_UP;
    }
  }
  return STATE_SITTING;
}

static void prv_update_direction_toward_target(uint32_t now) {
  if (!s_has_target) {
    return;
  }

  GPoint cat_center = {
    .x = s_cat_pos.x + CAT_SIZE / 2,
    .y = s_cat_pos.y + CAT_SIZE / 2
  };
  int dx = s_target.x - cat_center.x;
  int dy = s_target.y - cat_center.y;
  CatState candidate = prv_direction_from_delta(dx, dy);
  if (candidate == STATE_SITTING) {
    return;
  }

  const CatStateConfig *config = &STATE_CONFIG[s_state];
  if (config->is_scratching) {
    return;
  }

  if (!config->is_moving || s_state != candidate) {
    prv_set_state(candidate, now, false);
  }
}

static void prv_clamp_point_to_bounds(GPoint *point, int margin) {
  if (s_bounds.size.w == 0 || s_bounds.size.h == 0) {
    return;
  }
  if (point->x < margin) {
    point->x = margin;
  } else if (point->x > s_bounds.size.w - margin) {
    point->x = s_bounds.size.w - margin;
  }
  if (point->y < margin) {
    point->y = margin;
  } else if (point->y > s_bounds.size.h - margin) {
    point->y = s_bounds.size.h - margin;
  }
}

static void prv_move_target_random(void) {
  if (s_bounds.size.w == 0 || s_bounds.size.h == 0) {
    return;
  }
  s_has_target = true;
  s_target.x = rand() % s_bounds.size.w;
  s_target.y = rand() % s_bounds.size.h;
  prv_clamp_point_to_bounds(&s_target, TARGET_INDICATOR_RADIUS);
}

static void prv_nudge_target(int dx, int dy) {
  if (s_bounds.size.w == 0 || s_bounds.size.h == 0) {
    return;
  }
  s_has_target = true;
  s_target.x += dx;
  s_target.y += dy;
  prv_clamp_point_to_bounds(&s_target, TARGET_INDICATOR_RADIUS);
}

static void prv_handle_pointer_catch(uint32_t now) {
  if (!s_has_target) {
    return;
  }
  if (prv_point_inside_cat(s_target)) {
    const CatStateConfig *config = &STATE_CONFIG[s_state];
    if (config->is_moving) {
      prv_set_state(prv_random_rest_state(), now, false);
    }
  }
}

static void prv_handle_restless_pointer(uint32_t now) {
  if (!s_has_target) {
    return;
  }
  const CatStateConfig *config = &STATE_CONFIG[s_state];
  if (config->is_moving || config->is_scratching) {
    return;
  }
  if (!prv_point_inside_cat(s_target)) {
    prv_update_direction_toward_target(now);
  }
}

static bool prv_advance_animation(void) {
  const CatStateConfig *config = &STATE_CONFIG[s_state];
  if (config->frame_count == 0) {
    return false;
  }
  uint8_t frame = config->frames[s_animation_index];
  bool looped = false;
  s_current_frame_index = frame;
  s_current_bitmap = s_frame_bitmaps[s_current_frame_index];
  s_animation_index++;
  if (s_animation_index >= config->frame_count) {
    s_animation_index = 0;
    looped = true;
  }
  return looped;
}

static void prv_clamp_and_handle_edges(uint32_t now) {
  bool clamped = false;
  if (s_cat_pos.x < 0) {
    s_cat_pos.x = 0;
    prv_set_state(STATE_SCRATCHING_LEFT, now, true);
    clamped = true;
  } else if (s_cat_pos.x > s_bounds.size.w - CAT_SIZE) {
    s_cat_pos.x = s_bounds.size.w - CAT_SIZE;
    prv_set_state(STATE_SCRATCHING_RIGHT, now, true);
    clamped = true;
  }
  if (s_cat_pos.y < 0) {
    s_cat_pos.y = 0;
    prv_set_state(STATE_SCRATCHING_UP, now, true);
    clamped = true;
  } else if (s_cat_pos.y > s_bounds.size.h - CAT_SIZE) {
    s_cat_pos.y = s_bounds.size.h - CAT_SIZE;
    prv_set_state(STATE_SCRATCHING_DOWN, now, true);
    clamped = true;
  }
  if (!clamped) {
    prv_update_direction_toward_target(now);
  }
}

static void prv_apply_state_side_effects(bool looped, uint32_t now) {
  CatStateConfig config = STATE_CONFIG[s_state];
  if (config.is_moving) {
    s_cat_pos.x += config.step.x;
    s_cat_pos.y += config.step.y;
    prv_clamp_and_handle_edges(now);
    if (rand() % STOP_CHANCE_FACTOR == 0) {
      prv_set_state(prv_random_rest_state(), now, false);
    }
  } else if (config.is_scratching) {
    if (now - s_last_state_change_ms >= SCRATCH_MIN_MS) {
      if (rand() % START_CHANCE_FACTOR == 0) {
        prv_set_state(prv_random_moving_state(), now, false);
      }
    }
  } else {
    if (looped) {
      prv_set_state(prv_random_moving_state(), now, false);
    }
  }
}

static void prv_tick(void *context) {
  uint32_t now = prv_now_ms();
  prv_handle_pointer_catch(now);
  prv_handle_restless_pointer(now);
  bool looped = prv_advance_animation();
  prv_apply_state_side_effects(looped, now);
  layer_mark_dirty(s_canvas_layer);
  prv_schedule_timer();
}

static void prv_schedule_timer(void) {
  s_timer = app_timer_register(ANIMATION_INTERVAL_MS, prv_tick, NULL);
}

static void prv_draw_canvas(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, s_bounds, 0, GCornerNone);

  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorVividCerulean, GColorWhite));
  graphics_fill_circle(ctx, s_target, TARGET_INDICATOR_RADIUS);
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorPictonBlue, GColorBlack));
  graphics_draw_circle(ctx, s_target, TARGET_INDICATOR_RADIUS);

  if (s_current_bitmap) {
    GRect frame = GRect(s_cat_pos.x, s_cat_pos.y, CAT_SIZE, CAT_SIZE);
    graphics_draw_bitmap_in_rect(ctx, s_current_bitmap, frame);
  }
}

static void prv_load_bitmaps(void) {
  for (int i = 0; i < FRAME_COUNT; ++i) {
    s_frame_bitmaps[i] = gbitmap_create_with_resource(FRAME_RESOURCE_IDS[i]);
  }
}

static void prv_unload_bitmaps(void) {
  for (int i = 0; i < FRAME_COUNT; ++i) {
    if (s_frame_bitmaps[i]) {
      gbitmap_destroy(s_frame_bitmaps[i]);
      s_frame_bitmaps[i] = NULL;
    }
  }
}

static void prv_click_select_handler(ClickRecognizerRef recognizer, void *context) {
  prv_move_target_random();
}

static void prv_click_up_handler(ClickRecognizerRef recognizer, void *context) {
  prv_nudge_target(-STEP_PIXELS, -STEP_PIXELS);
}

static void prv_click_down_handler(ClickRecognizerRef recognizer, void *context) {
  prv_nudge_target(STEP_PIXELS, STEP_PIXELS);
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

  GPoint center = grect_center_point(&s_bounds);
  s_cat_pos = GPoint(center.x - CAT_SIZE / 2, center.y - CAT_SIZE / 2);
  s_target = center;
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
}

static void init(void) {
  srand(time(NULL));
  prv_load_bitmaps();
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_click_config_provider(s_main_window, prv_click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload
  });
  window_stack_push(s_main_window, true);

  uint32_t now = prv_now_ms();
  prv_set_state(STATE_SITTING, now, true);
  prv_schedule_timer();
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
