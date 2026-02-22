#include <assert.h>
#include <stdio.h>

#define NEKO_LOGIC_TESTING 1
#include "../src/neko_logic.h"

static const NekoBounds TEST_BOUNDS = { .width = 144, .height = 168 };

static void test_moves_toward_right_target(void) {
  NekoContext ctx;
  neko_context_init(&ctx, TEST_BOUNDS, 0, 1234);
  NekoPoint cat = neko_context_cat_position(&ctx);
  NekoPoint target = { cat.x + 40, cat.y + NEKO_SPRITE_SIZE / 2 };
  neko_context_set_target(&ctx, target);

  neko_context_tick(&ctx, NEKO_ANIMATION_INTERVAL_MS);

  assert(neko_context_state(&ctx) == NEKO_STATE_MOVING_RIGHT);
  NekoPoint new_pos = neko_context_cat_position(&ctx);
  assert(new_pos.x > cat.x);
}

static void test_scratches_when_hitting_right_edge(void) {
  NekoContext ctx;
  neko_context_init(&ctx, TEST_BOUNDS, 0, 4321);
  NekoPoint target = { TEST_BOUNDS.width - 1, TEST_BOUNDS.height / 2 };
  neko_context_set_target(&ctx, target);
  neko_context_force_state(&ctx, NEKO_STATE_MOVING_RIGHT, 0);

  bool scratched = false;
  for (int i = 0; i < 40; ++i) {
    uint32_t now = (uint32_t)(i + 1) * NEKO_ANIMATION_INTERVAL_MS;
    neko_context_tick(&ctx, now);
    if (neko_context_state(&ctx) == NEKO_STATE_SCRATCHING_RIGHT) {
      scratched = true;
      break;
    }
    if (neko_context_state(&ctx) != NEKO_STATE_MOVING_RIGHT) {
      neko_context_force_state(&ctx, NEKO_STATE_MOVING_RIGHT, now);
    }
  }
  assert(scratched);
}

static void test_target_clamps_inside_bounds(void) {
  NekoContext ctx;
  neko_context_init(&ctx, TEST_BOUNDS, 0, 1111);
  NekoPoint outside = { -50, TEST_BOUNDS.height + 10 };
  neko_context_set_target(&ctx, outside);

  NekoPoint clamped = neko_context_target(&ctx);
  assert(clamped.x >= 0);
  assert(clamped.y <= TEST_BOUNDS.height - 1);

  neko_context_nudge_target(&ctx, 1000, 1000);
  clamped = neko_context_target(&ctx);
  assert(clamped.x <= TEST_BOUNDS.width - 1);
  assert(clamped.y <= TEST_BOUNDS.height - 1);
}

int main(void) {
  test_moves_toward_right_target();
  test_scratches_when_hitting_right_edge();
  test_target_clamps_inside_bounds();
  printf("All neko_logic tests passed.\n");
  return 0;
}
