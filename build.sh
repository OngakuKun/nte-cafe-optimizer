#!/usr/bin/env sh
set -eu

# Simple build script for nte-cafe (out-of-source build).
# Usage:
#   ./build.sh                              # Debug build in ./build
#   ./build.sh Release                      # Release build in ./build
#   ./build.sh Debug build-release          # Debug build in custom dir
#   ./build.sh Debug build --target nte-cafe-app
#   ./build.sh clean                        # remove ./build
#   ./build.sh clean build-release          # remove custom build dir

usage() {
  echo "Usage:"
  echo "  $0 [Debug|Release] [build-dir] [cmake build args...]"
  echo "  $0 clean [build-dir]"
}

case "${1:-}" in
  -h|--help|help)
    usage
    exit 0
    ;;
  clean)
    BUILD_DIR="${2:-build}"
    case "$BUILD_DIR" in
      ""|"."|"..")
        echo "Refusing to clean unsafe path: $BUILD_DIR" >&2
        exit 1
        ;;
    esac
    if [ -d "$BUILD_DIR" ]; then
      rm -rf "$BUILD_DIR"
      echo "Cleaned: $BUILD_DIR"
    else
      echo "Nothing to clean: $BUILD_DIR does not exist"
    fi
    exit 0
    ;;
esac

BUILD_TYPE="${1:-Debug}"
BUILD_DIR="${2:-build}"

# Optional extra args after build dir, e.g. --target nte-cafe-app
if [ "$#" -gt 2 ]; then
  shift 2
else
  shift "$#"
fi

cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR" -j"${JOBS:-$(nproc 2>/dev/null || echo 4)}" "$@"

echo
echo "Build complete: $BUILD_DIR"
echo "App (if built): $BUILD_DIR/app/nte-cafe-app"
