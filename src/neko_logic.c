#include "neko_logic.h"

#include <stdlib.h>
#include <string.h>

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(arr) (int)(sizeof(arr) / sizeof((arr)[0]))
#endif

typedef struct {
  NekoState state;
  const uint8_t *frames;
  uint8_t frame_count;
  NekoPoint step;
  bool is_moving;
  bool is_scratching;
} NekoStateConfig;

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

static const NekoStateConfig STATE_CONFIG[NEKO_STATE_COUNT] = {
  { NEKO_STATE_SITTING,          FRAMES_SITTING,        ARRAY_LENGTH(FRAMES_SITTING),        { 0,               0 }, false, false },
  { NEKO_STATE_MOVING_LEFT,      FRAMES_MOVE_LEFT,      ARRAY_LENGTH(FRAMES_MOVE_LEFT),      { -NEKO_STEP_PIXELS, 0 }, true,  false },
  { NEKO_STATE_MOVING_UP,        FRAMES_MOVE_UP,        ARRAY_LENGTH(FRAMES_MOVE_UP),        { 0, -NEKO_STEP_PIXELS }, true,  false },
  { NEKO_STATE_MOVING_RIGHT,     FRAMES_MOVE_RIGHT,     ARRAY_LENGTH(FRAMES_MOVE_RIGHT),     { NEKO_STEP_PIXELS,  0 }, true,  false },
  { NEKO_STATE_MOVING_DOWN,      FRAMES_MOVE_DOWN,      ARRAY_LENGTH(FRAMES_MOVE_DOWN),      { 0,  NEKO_STEP_PIXELS }, true,  false },
  { NEKO_STATE_MOVING_UP_RIGHT,  FRAMES_MOVE_UP_RIGHT,  ARRAY_LENGTH(FRAMES_MOVE_UP_RIGHT),  { NEKO_HALF_STEP, -NEKO_HALF_STEP }, true, false },
  { NEKO_STATE_MOVING_UP_LEFT,   FRAMES_MOVE_UP_LEFT,   ARRAY_LENGTH(FRAMES_MOVE_UP_LEFT),   { -NEKO_HALF_STEP, -NEKO_HALF_STEP }, true, false },
  { NEKO_STATE_MOVING_DOWN_LEFT, FRAMES_MOVE_DOWN_LEFT, ARRAY_LENGTH(FRAMES_MOVE_DOWN_LEFT), { -NEKO_HALF_STEP,  NEKO_HALF_STEP }, true, false },
  { NEKO_STATE_MOVING_DOWN_RIGHT,FRAMES_MOVE_DOWN_RIGHT,ARRAY_LENGTH(FRAMES_MOVE_DOWN_RIGHT),{ NEKO_HALF_STEP,  NEKO_HALF_STEP }, true, false },
  { NEKO_STATE_SCRATCHING_LEFT,  FRAMES_SCRATCH_LEFT,   ARRAY_LENGTH(FRAMES_SCRATCH_LEFT),   { 0, 0 }, false, true },
  { NEKO_STATE_SCRATCHING_RIGHT, FRAMES_SCRATCH_RIGHT,  ARRAY_LENGTH(FRAMES_SCRATCH_RIGHT),  { 0, 0 }, false, true },
  { NEKO_STATE_SCRATCHING_DOWN,  FRAMES_SCRATCH_DOWN,   ARRAY_LENGTH(FRAMES_SCRATCH_DOWN),   { 0, 0 }, false, true },
  { NEKO_STATE_SCRATCHING_UP,    FRAMES_SCRATCH_UP,     ARRAY_LENGTH(FRAMES_SCRATCH_UP),     { 0, 0 }, false, true },
  { NEKO_STATE_CLEANING,         FRAMES_CLEANING,       ARRAY_LENGTH(FRAMES_CLEANING),       { 0, 0 }, false, false },
  { NEKO_STATE_SLEEPING,         FRAMES_SLEEPING,       ARRAY_LENGTH(FRAMES_SLEEPING),       { 0, 0 }, false, false }
};

static const NekoState MOVING_STATES[] = {
  NEKO_STATE_MOVING_UP,
  NEKO_STATE_MOVING_DOWN,
  NEKO_STATE_MOVING_LEFT,
  NEKO_STATE_MOVING_RIGHT,
  NEKO_STATE_MOVING_UP_LEFT,
  NEKO_STATE_MOVING_UP_RIGHT,
  NEKO_STATE_MOVING_DOWN_LEFT,
  NEKO_STATE_MOVING_DOWN_RIGHT
};

static uint32_t prv_random(NekoContext *ctx) {
  if (ctx->rng_state == 0) {
    ctx->rng_state = 1;
  }
  ctx->rng_state = ctx->rng_state * 1664525U + 1013904223U;
  return ctx->rng_state;
}

static uint32_t prv_random_range(NekoContext *ctx, uint32_t max_value) {
  if (max_value == 0) {
    return 0;
  }
  return prv_random(ctx) % max_value;
}

static NekoState prv_random_rest_state(NekoContext *ctx) {
  return (prv_random(ctx) & 1) ? NEKO_STATE_CLEANING : NEKO_STATE_SLEEPING;
}

static NekoState prv_random_moving_state(NekoContext *ctx) {
  return MOVING_STATES[prv_random_range(ctx, ARRAY_LENGTH(MOVING_STATES))];
}

static void prv_update_current_frame(NekoContext *ctx) {
  const NekoStateConfig *config = &STATE_CONFIG[ctx->state];
  if (config->frame_count == 0) {
    ctx->current_frame_index = 0;
    ctx->animation_index = 0;
    return;
  }
  if (ctx->animation_index >= config->frame_count) {
    ctx->animation_index = 0;
  }
  ctx->current_frame_index = config->frames[ctx->animation_index];
}

static void prv_set_state(NekoContext *ctx, NekoState new_state, uint32_t now_ms, bool force_reset) {
  if (!force_reset && new_state == ctx->state) {
    return;
  }
  ctx->state = new_state;
  ctx->animation_index = 0;
  ctx->last_state_change_ms = now_ms;
  prv_update_current_frame(ctx);
}

static bool prv_point_inside_cat(const NekoContext *ctx, NekoPoint point) {
  return point.x >= ctx->cat_pos.x && point.x <= ctx->cat_pos.x + NEKO_SPRITE_SIZE &&
         point.y >= ctx->cat_pos.y && point.y <= ctx->cat_pos.y + NEKO_SPRITE_SIZE;
}

static void prv_clamp_point_to_bounds(const NekoContext *ctx, NekoPoint *point, int margin) {
  if (!ctx->bounds_initialized) {
    return;
  }
  if (point->x < margin) {
    point->x = margin;
  } else if (point->x > ctx->bounds.width - margin) {
    point->x = ctx->bounds.width - margin;
  }
  if (point->y < margin) {
    point->y = margin;
  } else if (point->y > ctx->bounds.height - margin) {
    point->y = ctx->bounds.height - margin;
  }
}

static NekoState prv_direction_from_delta(int dx, int dy) {
  const int threshold = NEKO_STEP_PIXELS / 2;
  if (dx > threshold) {
    if (dy > threshold) {
      return NEKO_STATE_MOVING_DOWN_RIGHT;
    } else if (dy < -threshold) {
      return NEKO_STATE_MOVING_UP_RIGHT;
    } else {
      return NEKO_STATE_MOVING_RIGHT;
    }
  } else if (dx < -threshold) {
    if (dy > threshold) {
      return NEKO_STATE_MOVING_DOWN_LEFT;
    } else if (dy < -threshold) {
      return NEKO_STATE_MOVING_UP_LEFT;
    } else {
      return NEKO_STATE_MOVING_LEFT;
    }
  } else {
    if (dy > threshold) {
      return NEKO_STATE_MOVING_DOWN;
    } else if (dy < -threshold) {
      return NEKO_STATE_MOVING_UP;
    }
  }
  return NEKO_STATE_SITTING;
}

static void prv_update_direction_toward_target(NekoContext *ctx, uint32_t now_ms) {
  if (!ctx->has_target || !ctx->bounds_initialized) {
    return;
  }
  NekoPoint cat_center = {
    ctx->cat_pos.x + NEKO_SPRITE_SIZE / 2,
    ctx->cat_pos.y + NEKO_SPRITE_SIZE / 2
  };
  int dx = ctx->target.x - cat_center.x;
  int dy = ctx->target.y - cat_center.y;
  NekoState candidate = prv_direction_from_delta(dx, dy);
  if (candidate == NEKO_STATE_SITTING) {
    return;
  }
  const NekoStateConfig *config = &STATE_CONFIG[ctx->state];
  if (config->is_scratching) {
    return;
  }
  if (!config->is_moving || ctx->state != candidate) {
    prv_set_state(ctx, candidate, now_ms, false);
  }
}

static void prv_handle_pointer_catch(NekoContext *ctx, uint32_t now_ms) {
  if (!ctx->has_target) {
    return;
  }
  if (prv_point_inside_cat(ctx, ctx->target)) {
    const NekoStateConfig *config = &STATE_CONFIG[ctx->state];
    if (config->is_moving) {
      prv_set_state(ctx, prv_random_rest_state(ctx), now_ms, false);
    }
  }
}

static void prv_handle_restless_pointer(NekoContext *ctx, uint32_t now_ms) {
  if (!ctx->has_target) {
    return;
  }
  const NekoStateConfig *config = &STATE_CONFIG[ctx->state];
  if (config->is_moving || config->is_scratching) {
    return;
  }
  if (!prv_point_inside_cat(ctx, ctx->target)) {
    prv_update_direction_toward_target(ctx, now_ms);
  }
}

static bool prv_advance_animation(NekoContext *ctx) {
  const NekoStateConfig *config = &STATE_CONFIG[ctx->state];
  if (config->frame_count == 0) {
    return false;
  }
  bool looped = false;
  ctx->current_frame_index = config->frames[ctx->animation_index];
  ctx->animation_index++;
  if (ctx->animation_index >= config->frame_count) {
    ctx->animation_index = 0;
    looped = true;
  }
  return looped;
}

static void prv_clamp_and_handle_edges(NekoContext *ctx, uint32_t now_ms) {
  if (!ctx->bounds_initialized) {
    return;
  }
  bool clamped = false;
  if (ctx->cat_pos.x < 0) {
    ctx->cat_pos.x = 0;
    prv_set_state(ctx, NEKO_STATE_SCRATCHING_LEFT, now_ms, true);
    clamped = true;
  } else if (ctx->cat_pos.x > ctx->bounds.width - NEKO_SPRITE_SIZE) {
    ctx->cat_pos.x = ctx->bounds.width - NEKO_SPRITE_SIZE;
    prv_set_state(ctx, NEKO_STATE_SCRATCHING_RIGHT, now_ms, true);
    clamped = true;
  }
  if (ctx->cat_pos.y < 0) {
    ctx->cat_pos.y = 0;
    prv_set_state(ctx, NEKO_STATE_SCRATCHING_UP, now_ms, true);
    clamped = true;
  } else if (ctx->cat_pos.y > ctx->bounds.height - NEKO_SPRITE_SIZE) {
    ctx->cat_pos.y = ctx->bounds.height - NEKO_SPRITE_SIZE;
    prv_set_state(ctx, NEKO_STATE_SCRATCHING_DOWN, now_ms, true);
    clamped = true;
  }
  if (!clamped) {
    prv_update_direction_toward_target(ctx, now_ms);
  }
}

static void prv_apply_state_side_effects(NekoContext *ctx, bool looped, uint32_t now_ms) {
  const NekoStateConfig config = STATE_CONFIG[ctx->state];
  if (config.is_moving) {
    ctx->cat_pos.x += config.step.x;
    ctx->cat_pos.y += config.step.y;
    prv_clamp_and_handle_edges(ctx, now_ms);
    if (prv_random_range(ctx, NEKO_STOP_CHANCE_FACTOR) == 0) {
      prv_set_state(ctx, prv_random_rest_state(ctx), now_ms, false);
    }
  } else if (config.is_scratching) {
    if (now_ms - ctx->last_state_change_ms >= NEKO_SCRATCH_MIN_MS) {
      if (prv_random_range(ctx, NEKO_START_CHANCE_FACTOR) == 0) {
        prv_set_state(ctx, prv_random_moving_state(ctx), now_ms, false);
      }
    }
  } else if (looped) {
    prv_set_state(ctx, prv_random_moving_state(ctx), now_ms, false);
  }
}

void neko_context_init(NekoContext *ctx, NekoBounds bounds, uint32_t now_ms, uint32_t seed) {
  memset(ctx, 0, sizeof(*ctx));
  ctx->rng_state = seed;
  neko_context_set_bounds(ctx, bounds);
  ctx->state = NEKO_STATE_SITTING;
  ctx->cat_pos.x = (bounds.width - NEKO_SPRITE_SIZE) / 2;
  ctx->cat_pos.y = (bounds.height - NEKO_SPRITE_SIZE) / 2;
  ctx->target.x = ctx->cat_pos.x + NEKO_SPRITE_SIZE / 2;
  ctx->target.y = ctx->cat_pos.y + NEKO_SPRITE_SIZE / 2;
  ctx->has_target = true;
  ctx->last_state_change_ms = now_ms;
  prv_update_current_frame(ctx);
}

void neko_context_set_bounds(NekoContext *ctx, NekoBounds bounds) {
  ctx->bounds = bounds;
  ctx->bounds_initialized = (bounds.width > 0 && bounds.height > 0);
  if (ctx->bounds_initialized) {
    prv_clamp_point_to_bounds(ctx, &ctx->target, NEKO_TARGET_RADIUS);
    prv_clamp_point_to_bounds(ctx, &ctx->cat_pos, NEKO_SPRITE_SIZE);
  }
}

void neko_context_set_target(NekoContext *ctx, NekoPoint target) {
  ctx->target = target;
  ctx->has_target = true;
  prv_clamp_point_to_bounds(ctx, &ctx->target, NEKO_TARGET_RADIUS);
}

void neko_context_randomize_target(NekoContext *ctx) {
  if (!ctx->bounds_initialized) {
    return;
  }
  ctx->target.x = (int16_t)prv_random_range(ctx, ctx->bounds.width);
  ctx->target.y = (int16_t)prv_random_range(ctx, ctx->bounds.height);
  ctx->has_target = true;
  prv_clamp_point_to_bounds(ctx, &ctx->target, NEKO_TARGET_RADIUS);
}

void neko_context_nudge_target(NekoContext *ctx, int dx, int dy) {
  ctx->target.x += dx;
  ctx->target.y += dy;
  ctx->has_target = true;
  prv_clamp_point_to_bounds(ctx, &ctx->target, NEKO_TARGET_RADIUS);
}

void neko_context_tick(NekoContext *ctx, uint32_t now_ms) {
  ctx->last_animation_looped = false;
  prv_handle_pointer_catch(ctx, now_ms);
  prv_handle_restless_pointer(ctx, now_ms);
  bool looped = prv_advance_animation(ctx);
  ctx->last_animation_looped = looped;
  prv_apply_state_side_effects(ctx, looped, now_ms);
}

uint8_t neko_context_current_frame(const NekoContext *ctx) {
  return ctx->current_frame_index;
}

bool neko_context_is_moving(const NekoContext *ctx) {
  return STATE_CONFIG[ctx->state].is_moving;
}

bool neko_context_is_scratching(const NekoContext *ctx) {
  return STATE_CONFIG[ctx->state].is_scratching;
}

NekoPoint neko_context_cat_position(const NekoContext *ctx) {
  return ctx->cat_pos;
}

NekoPoint neko_context_target(const NekoContext *ctx) {
  return ctx->target;
}

NekoState neko_context_state(const NekoContext *ctx) {
  return ctx->state;
}

#ifdef NEKO_LOGIC_TESTING
void neko_context_force_state(NekoContext *ctx, NekoState state, uint32_t now_ms) {
  prv_set_state(ctx, state, now_ms, true);
}
#endif
