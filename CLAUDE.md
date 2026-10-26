# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Arduino firmware (ESP32-S3) for a gen-1 Pokémon tamagotchi on the Waveshare ESP32-S3-Touch-AMOLED-1.75 (466×466 round AMOLED, CO5300 over QSPI, CST9217 touch, AXP2101 PMU, PCF85063 RTC, ES8311 audio, microSD), plus Python tooling that generates headers and sprite files, and a browser installer in `web/`.

**This fork targets the owner's board, the Waveshare ESP32-S3-Touch-AMOLED-1.75C** (not the 1.75 upstream was written for). Same chips, but: display/touch resets on GPIO 1/2 (the 1.75's SD pins), MCLK on GPIO 16, panel rail on AXP2101 ALDO1 instead of BLDO1, **no microSD slot**, **no PCF85063 RTC**, and 32 MB QIO flash. `pin_config.h` selects the board: `BOARD_AMOLED_175C` is the default, `-DBOARD_AMOLED_175` builds the original. On the 1.75C, `sdmon.cpp` stores sprites in the flash FAT partition (`FFat`, ~23 MB) instead of SD: the 151 normal sprites + thumbnails fit, the shinies don't (`tools/send_sd.py --no-shiny`; a missing shiny falls back to the normal sprite). Pin source: `waveshareteam/ESP32-S3-Touch-AMOLED-1.75C`, `examples/arduino/libraries/board_config/pin_config.h`.

Code comments, test names, CI step comments and commit messages are mostly in **Spanish**. User-facing docs (README, CHANGELOG, web/README) are in English. Match the language of the file you're editing.

## Commands

Firmware (needs `arduino-cli` + ESP32 core `esp32:esp32@3.3.10`, the version pinned in CI; libraries: `GFX Library for Arduino`, `SensorLib`, `XPowersLib`, `U8g2`):

```bash
# 1.75C (default board)
FQBN="esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=32M,FlashMode=qio,PSRAM=opi,PartitionScheme=app5M_fat24M_32MB"
arduino-cli compile --warnings=all --fqbn "$FQBN" .
arduino-cli upload -p <port> --fqbn "$FQBN" .
python3 tools/send_sd.py --no-shiny   # sprites into the flash FAT partition over USB

# original 1.75
FQBN="esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB"
arduino-cli compile --warnings=all --fqbn "$FQBN" --build-property "compiler.cpp.extra_flags=-DBOARD_AMOLED_175" .
bash tools/build_web.sh   # 1.75 only: compile + merge into web/firmware/tamapoke.bin + rebuild web/sprites.pak
```

CI compiles both boards (matrix in `.github/workflows/ci.yml`). The web installer (`web/`) only targets the 1.75.

Host tests (no board needed; this is what CI runs):

```bash
./test/run_tests.sh            # C++ logic tests + tools tests
./test/run_tests.sh --asan     # also under AddressSanitizer + UBSan (CI uses this)
cd test && make FILTER=evolve  # only tests whose name contains "evolve"
python3 test/test_tools.py     # tools/ checks only
python3 tools/test_i18n_formats.py   # i18n format-specifier consistency (separate CI job)
ruff check tools/              # lint (pyflakes rules only, see ruff.toml)
```

Generated headers (CI fails if regenerating produces a diff):

```bash
python3 tools/gen_dex.py       # -> dex.h      (from tools/dex_data.py, dex_stats.py, dex_names.py)
python3 tools/sprites.py emit  # -> species.h  (needs pillow)
```

## Architecture

- `TamaPoke.ino` (~2600 lines) is the whole UI: hardware init, the main loop, rendering of every screen (main scene, stat card pages, gallery/Pokédex, clock/settings, keyboard, minigame, training bag, dialogs/ceremonies), touch gesture dispatch, and the serial debug console. Screens are modal via global flags (`cardOpen`, `clockOpen`, …); drawing goes to an `Arduino_Canvas` framebuffer in PSRAM (requires OPI PSRAM).
- `pet.h/.cpp` is the pure game logic: stats/tick, evolution, life cycle endings, egg rarity rolls, streak/bond/medals, battle stats, offline catch-up, NVS persistence via `Preferences`. It has no display/hardware deps — keep it that way, because it is what the host tests compile.
- `i18n.h/.cpp`: `STRINGS[LANG_COUNT][STR_COUNT]` indexed by `StrId`, accessed with `T(id)`; `gLang` global. Adding a string means adding it to every language. The sketch passes `%u`/`%s` args to some strings, and tests check every language has the same specifiers. CJK (JA/KO) render via U8g2 unifont subsets selected in `applyLangFont()`; Latin text uses the ASCII GFX font (no accents). Text width must go through the centralised measurement helpers (`textW`/`centerX`), not byte length (UTF-8).
- `dex.h` and `species.h` are **generated — never edit directly**. Edit `tools/dex_data.py` (single source for names, types, evolution lines/levels, rarity, starters), `tools/dex_stats.py`, `tools/dex_names.py` (localized names, from `gen_names.py`/PokéAPI), or `tools/sprites.py`, then regenerate.
- `sdmon.h/.cpp`: loads sprites from microSD into PSRAM — `PmdMon` (TPK2 multi-action PMD sprites, `/mons/p[s]NNN.bin`), `SdThumbs` (TPTH Pokédex thumbnails `thumbs.bin`), legacy `SdMon` (TPK1). Also implements the USB serial `PUT`/`LS` file-upload protocol used by `tools/send_sd.py` and the web installer.
- On the 1.75C `rtcBegin()` fails (no RTC chip), so time doesn't advance while powered off and the clock must be set again after a power cut; the game already handles a missing RTC.
- `rtcbat`, `audio` (non-blocking tone-synth task over I2S → ES8311), `pin_config.h` (official Waveshare pins) are thin hardware layers.
- Sprite pipeline: `tools/pack_pmd.py` (PMD SpriteCollab → `tools/sdcard/mons/*.bin`, versioned in git) → `tools/make_thumbs.py` → `tools/pack_bundle.py` (→ `web/sprites.pak`) or `tools/send_sd.py` (direct USB upload).

### Host test harness (`test/`)

`pet.cpp` and `i18n.cpp` are compiled with g++ against shims in `test/shim/` (fake `millis()`/`random()`, in-memory `Preferences` — simulate a reboot by constructing a new `Pet` and calling `begin()`). Own micro framework in `test/framework.h`. `TEST_KNOWN_ISSUE` marks a test describing correct behavior that currently fails: it doesn't break the suite and warns when it starts passing. `TamaPoke.ino`, `audio`, `sdmon`, `rtcbat` are not covered by host tests — only the CI firmware compile catches errors there.

## Conventions

- Version lives in `FW_VERSION` (`TamaPoke.ino`) and `web/manifest.json`, plus the README badge; record changes in `CHANGELOG.md`.
- For fast manual testing on hardware, lower `PET_TICK_MS`, `MINUTES_PER_LEVEL`, `FAREWELL_AGE_MIN` in `pet.h`; the serial console (115200) has debug commands (`STATS`, `SPEC <dex>`, `LVL <n>`, `HATCH`, `WIPE`, `HEALTH`, …, listed in README).
- Updating firmware without erasing must keep the saved pet: NVS layout changes need to stay backward-compatible.
