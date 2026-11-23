# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

NVGT (NonVisual Gaming Toolkit) is a cross-platform audio game engine. It wraps libraries (SDL3, miniaudio, Poco, Jolt, enet...) into an AngelScript engine. Game developers write `.nvgt` scripts and compile them into standalone products for Windows, macOS, Linux, Android and iOS. Scripts are largely compatible with the old Blastbay Gaming Toolkit (BGT).

This checkout is the MarcroSoft fork (`origin`) of samtupy/nvgt (`upstream`). See "Fork notes" below.

## Build

- **Dependencies:** dependencies live in a per-platform folder in the repo root (`windev`, `macosdev`, `lindev`, `droidev`). Either download the prebuilt archive from nvgt.dev, or build them from source with vcpkg: `python vcpkg/build_dependencies.py`. That requires submodules; `--archive` also zips the result. vcpkg overlay ports live in `vcpkg/ports`, and custom triplets in `vcpkg/triplets`.
- **Build:** `scons -s` from the repo root. `scons -h` lists the switches. The ones you'll use most:
  - `no_stubs=1` skips the stubs for faster iteration.
  - `debug=1`.
  - `no_<name>_plugin=1` skips a plugin; `static_<name>_plugin=1` links it statically.
  - `plugins=a,b` builds only the listed plugins.
- **Output:** `release/` holds `nvgt(.exe)` and, on Windows, `nvgtw.exe`. Stubs go to `release/stub`, shared plugins and libraries to `release/lib`, and the bundled `.nvgt` include library is in `release/include`.
- **Android:** `scons target=android` (needs `ANDROID_NDK_HOME`), then from `jni/`: `gradlew assembleRunnerRelease assembleStubRelease assembleStubncRelease`. See `doc/src/advanced/Building NVGT for Android.md`.
- **Formatting:** C++ follows `.astylerc` (attached braces, tabs, padded operators).

## Tests

The runner is `test/tests.nvgt`. It executes every `test_*` function in `test/case/*.nvgt`; a test fails by throwing, usually via `assert`. Run it from inside `test/`, and pass script arguments after `--`:

```bash
cd test && ../release/nvgt.exe -- tests.nvgt
```

- `-t<name>` runs only matching tests. `<name>` is the function name, the name without `test_`, or the file name, e.g. `-tbase64`.
- `-d<name>` disables a test.
- `-n` keeps going after failures. `-v` is verbose.
- `-l1`, `-l2` and `-l3` set the maximum level; the default is `-l2`. A test is level 2 unless it declares `const int test_foo_level = N;`. Level 3 is for slow tests such as audio simulations.

Other folders:
- `test/quick` and `test/interact` hold ad-hoc and interactive scripts, not run by the runner.
- `test/tmp` is a git-ignored scratch directory that tests may write to.

## Architecture

- **Engine core (`src/`):** each subsystem lives in its own module (`sound.cpp`, `graphics.cpp`, `internet.cpp`, `jolt.cpp`, `tts.cpp`...) and exposes a `RegisterXxx(asIScriptEngine*)` function. `nvgt_angelscript.cpp` sets up the engine and calls all of them in a fixed order; the order matters, since some modules must register before template types are instantiated. That file also handles compiling, saving and loading bytecode, and running scripts. `nvgt.cpp` is the entry point and command line handling. Platform code is in `win.cpp`, `apple.mm`, `linux.cpp`, `android.cpp` and `xplatform.cpp`.
- **Runner vs. stub:** the same sources are compiled twice.
  - The runner, `nvgt`, contains the compiler and bundler.
  - The stubs are built with `NVGT_STUB` defined and become `release/stub/nvgt_<platform>.bin`. A stub is a prebuilt executable that loads the bytecode payload appended to it.
  - Every platform also gets a compiler-less `nvgt_<platform>_nc.bin` linked against `angelscript_nc` and selected with `#pragma stub nc`.
- **Bundling:** `bundling.cpp` (runner only) copies the stub, writes the payload, and does per-platform packaging: Windows exe, macOS `.app`, Android APK via aapt2 from the zip-shaped `nvgt_android.bin`, and so on. The Android stub is not built by SConstruct's common stub section. It is built in the Android branch of SConstruct and packaged by Gradle flavors (`jni/build.gradle`) and `jni/makestub.py`.
- **`ASAddon/`:** NVGT's modified copies of the AngelScript add-ons (string, array, dictionary, builder...).
- **`dep/`:** vendored single-file dependencies.
- **Plugins:** each plugin is a folder with an `_SConscript`. The build finds them in `plugin/*` and `extra/plugin/integrated/*`; `extra` is the upstream nvgt_extra submodule. The plugin API is `src/nvgt_plugin.h`, and only `NVGT_PLUGIN_API_VERSION` is checked at load. A plugin builds both a shared library and a static library; SConstruct links the static one in when the plugin is listed in `user/static_plugins` or `static_<name>_plugin=1` is given.
  - Steam Audio is no longer in the core. It lives in the `nvgt_phonon` plugin (`plugin/phonon`).
  - `plugin/lua` is the fork's own submodule (MarcroSoft/nvgt_lua).
- **`user/` (git-ignored):** private build customization. `nvgt_config.h` overrides `src/nvgt_config.h`, `static_plugins` lists plugins to link statically, and an `_SConscript` here is run with `nvgt_env`/`plugin_env`. CI generates an encrypted `user/nvgt_config.h` and appends `lua` to `user/static_plugins`.
- **Docs:** Markdown lives in `doc/src`, with the API reference under `doc/src/references`. `doc/docgen.py` (needs mistune, run from `doc/`) generates the HTML, Markdown and CHM output.
- **CI:**
  - `.github/workflows/release.yml` builds every platform, caches the dependency folders keyed on `vcpkg/ports`, `vcpkg/triplets` and the vcpkg JSON files, then packages and publishes. A full rerun is needed after a failure, because `rerun --failed` cannot fetch the deleted encrypted-config artifact.
  - `angelscript_bump.yml` opens nightly PRs on the `angelscript-bump` branch. Pushes to that branch don't start `release.yml` by themselves: dispatch it with `gh workflow run release.yml --ref angelscript-bump` and set the `release build` commit status.

## Fork notes (MarcroSoft/nvgt)

- **AngelScript:** the `angelscript` and `angelscript-nc` vcpkg ports track **anjo76/angelscript** master, so the nightly bump works. Upstream uses samtupy/angelscript. Sam's unmerged engine commits are carried as `samtupy-scoped-reference-types.patch` in both ports; upstream's Jolt types (scoped `vector`, `quaternion`, `aabb` and `plane`, and `asEP_ENABLE_VALUE_TYPED_COMPOUND_PROPERTY_ACCESSORS`) need them.
- **Merging `upstream/main`:**
  - Keep the fork's `REPO`/`REF`/`SHA512` lines in both AngelScript portfiles.
  - Keep the `nvgt_mac*.bin` wildcard in `install/nvgt.iss`.
  - Keep the statically linked Universal CRT on Windows. Upstream links the system UCRT, but Windows 7 lacks it, so the fork leaves the CRT flags out of `deb_rel_flags` in SConstruct (see the comment there).
  - Keep `--ultra-brute` in `build/upx_sconscript.py`.
  - Regenerate the Sam patch when upstream bumps its AngelScript REF.
- **Fork-only features:**
  - Lua plugin (statically linked by CI).
  - 32-bit SAPI bridge: the `sapi5x32` engine and `nvgt_sapi32host.exe`.
  - SharpVox fallback TTS in `dep/sharpvox`.
  - Windows 7 support via YY-Thunks plus `src/win7_thunks.cpp`.
  - Input device globals (`MOUSE_IS_VIRTUAL`, `KEYBOARD_LAST_DEVICE`, `MOUSE_LAST_DEVICE`).
  - `build.shared_library_excludes` in the bundler.
- **Upstream PRs:** samtupy/nvgt's `.github/CONTRIBUTING.md` has strict rules on AI-assisted code. Read it before opening upstream PRs, and test changes for real first.
