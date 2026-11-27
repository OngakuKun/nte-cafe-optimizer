# NTE Cafe Optimizer

Web UI and C library for optimizing NTE cafe setups: pick the best dish/character combination for a given trend, cafe count, and owned roster.

The project includes:

- **`lib/nte_cafe`** – core C optimizer/data/evaluation library.
- **`app/`** – WebUI-based server/app that serves the static UI and exposes bindings for optimizer state and calculations.
- **`tools/`** – CLI tools.
- **`tests/`** – optimizer/trend/full-dataset regression tests.
- **`docs/`** – source for the project website at <https://ongakukun.github.io/nte-cafe-optimizer/>.

> **Project website:** <https://ongakukun.github.io/nte-cafe-optimizer/> · **Live WebUI:** <https://nte.ongakukun.de/>

## Features

- **Optimizer**: calculates best result for current settings (cafes owned, trend, Hotori's Cat Decor), returning income, traffic, price buff, price multiplier, and selected dishes/characters.
- **Characters/Dishes pages**: manage ownership and levels through visual cards.
- **Per-client in-memory state**: WebUI client state is isolated by `client_id`; defaults are empty (no owned dishes/characters, 1 cafe, Hotori decor off).
- **Default setup notice**: when settings are still default, the Result card prompts users to configure characters/dishes.
- **Themes & language**: shared Catppuccin themes, theme import, and UI language selection (with header subtitle translations for Japanese/Chinese).
- **Docker deployment**: multi-stage Debian slim image running `nte-cafe-app`.
- **Desktop app**: `nte-cafe-desktop` runs the same UI and bindings in a native WebView window (falls back to the default browser if WebView is unavailable); both read/write the same `config.json`.

## Repository Layout

```text
.
├── app/                    # WebUI app/server + static UI
│   └── src/
│       ├── main.c          # WebUI bindings, file handler, per-client state, server/desktop mode
│       └── ui/             # index/characters/dishes/status pages, theme.js, shared.css
├── lib/
│   ├── nte_cafe/           # Core optimizer library
│   └── webui/              # WebUI submodule
├── patches/
│   └── webui-proxy-bridge.patch  # Proxy/bridge fixes for reverse-proxied WebUI
├── docs/                   # GitHub Pages project website
├── scripts/
│   └── sync-server.sh      # Sync remote server to origin/dev and redeploy
├── tests/                  # C tests
├── tools/                  # CLI tools
├── Dockerfile
├── docker-compose.yml
├── build.sh
└── CMakeLists.txt
```

## Requirements

### Local build

- CMake >= 3.18
- C compiler with C99 support
- Make/Ninja-compatible build tool (CMake default generator is fine)
- Git (for submodules)

### Docker

- Docker
- Docker Compose

## Getting Started

Clone with submodules:

```bash
git clone --recurse-submodules git@github.com:OngakuKun/nte-cafe-optimizer.git
cd nte-cafe-optimizer
```

If already cloned:

```bash
git submodule update --init --recursive
```

## Build & Run Locally

Using the helper script:

```bash
./build.sh Debug
./build/app/nte-cafe-app
```

Or manually with CMake:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DWEBUI_BUILD_EXAMPLES=OFF
cmake --build build --target nte-cafe-app -j
./build/app/nte-cafe-app
```

The app serves the UI through WebUI. Open it through the `nte-cafe-app` server (do not open HTML files directly, as bindings require the backend).

## Desktop App

`nte-cafe-desktop` is built from the same `main.c`, uses the same UI files and the same `config.json` as the server. It opens a native WebView window and falls back to the default browser if WebView is not installed.

```bash
./build.sh Debug
cmake --build build --target nte-cafe-desktop -j
./build/app/nte-cafe-desktop
```

Desktop mode is cross-platform: WebUI supports Windows WebView2, Linux GTK WebView and macOS WKWebView. WebView is optional on all three — without the runtime (on Windows also `WebView2Loader.dll`), the app opens in the installed default browser instead. On Linux, native WebView needs GTK3 + WebKitGTK at runtime. Windows and POSIX path separators are both accepted for asset/config lookup.

Either binary can switch modes at runtime: `--desktop` opens the window, `--server` starts the HTTP server, `--help` lists all options. UI assets and `data/gamedata.json` are copied next to each executable by the build, so a desktop binary can be run from its folder. `NTE_CAFE_CONFIG` and `NTE_CAFE_GAMEDATA` override the default paths (next to the executable). Builds are verified on Linux; Windows/macOS targets use WebUI's own CMake platform handling but were not built here.

## Downloads

Ready-to-run desktop builds are attached to each `v*` tag by `.github/workflows/release.yml`: Windows x64, Linux x64 + ARM64, and macOS Intel + Apple Silicon (all ZIP). The website's Download button points at `releases/latest`, so the links work once the first release tag is pushed. Arch Linux users can build from source with `makepkg` using `packaging/arch/PKGBUILD`.

## Configuration

Environment variables:

- `PORT` – preferred HTTP port (server mode only). Default: `80`.
  - If `PORT=80` cannot be bound, the app falls back to `8080`.
  - For other ports, if binding fails, WebUI may pick a free port.
- `NTE_CAFE_CONFIG` – path to the shared `config.json`. Default: next to the executable. The Docker image sets `/app/config/config.json` on a named volume.
- `NTE_CAFE_GAMEDATA` – path to `gamedata.json`. Default: `<executable dir>/data/gamedata.json`, with an embedded copy as fallback.

## Docker

Build and run with Compose:

```bash
docker compose up -d --build
```

Default compose mapping:

- Host `18423` -> container `80`

The Docker build applies `patches/webui-proxy-bridge.patch` to the WebUI submodule before building. This fixes bridge behavior behind a TLS/reverse proxy by using the browser origin and increasing the JS bridge buffer.

## Tests

Build and run tests from a CMake build directory:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DWEBUI_BUILD_EXAMPLES=OFF
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Available tests include:

- `optimizer`
- `trend`
- `full_dataset`
- `optimizer_current` – regression test based on a captured WebUI result, using the character roster known at capture time so future character additions don't break the baseline.
- `benchmark`

## Server Sync / Deploy

`scripts/sync-server.sh` syncs a remote server to `origin/dev`, updates submodules, and rebuilds/restarts the Docker Compose deployment.

```bash
./scripts/sync-server.sh
```

Useful overrides:

```bash
NTE_SERVER_HOST=fuu@host \
NTE_SERVER_KEY=~/.ssh/id_ed25519 \
NTE_SERVER_DIR=~/nte-cafe \
NTE_SERVER_BRANCH=dev \
./scripts/sync-server.sh
```

Use `--no-build` to sync files without rebuilding/restarting containers.

## Notes / Current Behavior

- **Per-client UI state is in memory; the shared config is on disk.** New clients are seeded from `config.json`, which is loaded at startup and can be exported/imported from Settings > Data. With no config file, defaults are the dataset minimums (empty roster, 1 cafe). The selected trend is session-only: configs always store `"trend": "None"` and every client starts at No Trend.
- **Game data comes from `data/gamedata.json`** for both server and CLI; the embedded copy is only a fallback.
- **Reverse proxy deployment**: the WebUI bridge patch is required for proper operation behind Nginx Proxy Manager/TLS proxy setups.
- **No secrets should be committed.** If adding accounts/OAuth later, use environment variables or secret files outside git.

## License

Add a license file if/when you decide on project licensing.
