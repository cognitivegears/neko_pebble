# Neko Pebble

A Pebble-native recreation of the classic Windows Neko desktop pet. The cat uses the original 32-frame sprite layout and faithfully implements the timing, randomness, and state machine described in the supplied specification while relying on the Pebble C API documented at [developer.rebble.io](https://developer.rebble.io/docs/c/).

## Controls
- `Select` – drop the virtual cursor at a random location so Neko can chase it.
- `Up` – nudge the cursor up/left by one movement step (10 px each axis).
- `Down` – nudge the cursor down/right by one movement step.
- The target cursor (small circle) is always visible; when it overlaps the cat, Neko picks a random cleaning/sleeping routine.

## Assets
- Placeholder 1×1 PNGs live under `resources/images/*.png`. Replace each `00.png` … `31.png` with the corresponding cat frame from your 32-icon set.
- Keep the filenames unchanged so the generated `RESOURCE_ID_CAT_FRAME_xx` constants remain valid.
- The menu icon (`resources/images/menu_icon.png`) may also be swapped for a higher fidelity glyph.

## Build & Install
1. Install the Rebble-supported Pebble SDK following [the setup guide](https://developer.rebble.io/developer.pebble.com/sdk/install/linux/). Ensure the `pebble` tool is on your path.
2. From the repository root run:
   ```
   pebble build
   pebble install --phone <watch_ip>
   ```
3. For the emulator workflow, execute `./scripts/run_emulator.sh basalt` (or another platform such as `aplite`) to build, install into the QEMU-based emulator, and tail its logs in one step.

## Automated Tests
- Run the deterministic host-side state machine tests with `./scripts/run_tests.sh`.  
  This compiles `src/neko_logic.c` together with `tests/neko_logic_test.c` using the desktop `gcc` toolchain, letting you quickly validate direction decisions, edge scratching, and target clamping without the Pebble SDK.
- Integrate the script into CI or your editor’s task runner to guard against logic regressions before flashing to hardware.

## Implementation Notes
- Timing constants follow the spec: 0.3 s animation steps, 10 px moves (diagonals use half steps), ≥2 s scratching, 1/30 chance to stop while moving, 1/10 chance to leave a wall after the minimum scratch window.
- `src/neko_logic.c` is a Pebble-agnostic state machine that drives animation frames, movement vectors, scratching timers, and cursor interactions; `src/main.c` becomes a thin UI shell that feeds timer ticks and button events into that engine.
- Cursor chasing logic mirrors the Windows version: direction is chosen from the eight compass states relative to the cursor, entering scratching states on window edges, and waking from cleaning/sleeping early if the cursor leaves the cat’s body.
- Random helpers use a deterministic LCG seeded at startup so automated tests can rely on predictable sequences while still looking organic on device.
- The implementation leans on Pebble C API primitives described in the docs (e.g., `Window`, `Layer`, `AppTimer`, `Graphics`), so future tweaks can follow the same patterns.

## File Map
- `package.json` – Pebble manifest (SDK version, targets, resource map).
- `wscript` – build script that wires the `pebble` waf tool to `src/main.c`.
- `src/neko_logic.c` / `src/neko_logic.h` – platform-agnostic state machine and animation engine.
- `src/main.c` – Pebble UI shell that feeds timer ticks/events into the shared logic and renders bitmaps.
- `resources/images/` – sprite frames + menu icon.
