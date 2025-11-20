// SPDX-License-Identifier: MIT
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define NEKO_FRAME_COUNT 32
#define NEKO_SPRITE_SIZE 32
#define NEKO_ANIMATION_INTERVAL_MS 300
#define NEKO_STEP_PIXELS 10
#define NEKO_HALF_STEP (NEKO_STEP_PIXELS / 2)
#define NEKO_SCRATCH_MIN_MS 2000
#define NEKO_STOP_CHANCE_FACTOR 30
#define NEKO_START_CHANCE_FACTOR 10
#define NEKO_TARGET_RADIUS 5

typedef enum {
  NEKO_STATE_SITTING,
  NEKO_STATE_MOVING_LEFT,
  NEKO_STATE_MOVING_UP,
  NEKO_STATE_MOVING_RIGHT,
  NEKO_STATE_MOVING_DOWN,
  NEKO_STATE_MOVING_UP_RIGHT,
  NEKO_STATE_MOVING_UP_LEFT,
  NEKO_STATE_MOVING_DOWN_LEFT,
  NEKO_STATE_MOVING_DOWN_RIGHT,
  NEKO_STATE_SCRATCHING_LEFT,
  NEKO_STATE_SCRATCHING_RIGHT,
  NEKO_STATE_SCRATCHING_DOWN,
  NEKO_STATE_SCRATCHING_UP,
  NEKO_STATE_CLEANING,
  NEKO_STATE_SLEEPING,
  NEKO_STATE_COUNT
} NekoState;

typedef struct {
  int16_t x;
  int16_t y;
} NekoPoint;

typedef struct {
  int16_t width;
  int16_t height;
} NekoBounds;

typedef struct {
  NekoState state;
  uint8_t animation_index;
  uint8_t current_frame_index;
  NekoPoint cat_pos;
  NekoPoint target;
  bool has_target;
  bool last_animation_looped;
  uint32_t last_state_change_ms;
  NekoBounds bounds;
  uint32_t rng_state;
  bool bounds_initialized;
} NekoContext;

void neko_context_init(NekoContext *ctx, NekoBounds bounds, uint32_t now_ms, uint32_t seed);
void neko_context_set_bounds(NekoContext *ctx, NekoBounds bounds);
void neko_context_set_target(NekoContext *ctx, NekoPoint target);
void neko_context_randomize_target(NekoContext *ctx);
void neko_context_nudge_target(NekoContext *ctx, int dx, int dy);
void neko_context_tick(NekoContext *ctx, uint32_t now_ms);

uint8_t neko_context_current_frame(const NekoContext *ctx);
bool neko_context_is_moving(const NekoContext *ctx);
bool neko_context_is_scratching(const NekoContext *ctx);
NekoPoint neko_context_cat_position(const NekoContext *ctx);
NekoPoint neko_context_target(const NekoContext *ctx);
NekoState neko_context_state(const NekoContext *ctx);

#ifdef NEKO_LOGIC_TESTING
void neko_context_force_state(NekoContext *ctx, NekoState state, uint32_t now_ms);
#endif
