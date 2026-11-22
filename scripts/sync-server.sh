#!/usr/bin/env bash
set -euo pipefail

# Sync NTE Cafe on the Fedora server with the git repo and rebuild/restart.
# Usage:
#   ./scripts/sync-server.sh              # sync + deploy
#   ./scripts/sync-server.sh --no-build   # sync files only
#
# Env overrides: NTE_SERVER_HOST, NTE_SERVER_KEY, NTE_SERVER_DIR, NTE_SERVER_BRANCH

HOST="${NTE_SERVER_HOST:-fuu@100.110.40.26}"
KEY="${NTE_SERVER_KEY:-$HOME/.ssh/id_ed25519_fedora}"
REMOTE_DIR="${NTE_SERVER_DIR:-~/nte-cafe}"
BRANCH="${NTE_SERVER_BRANCH:-dev}"
NO_BUILD=0

for arg in "$@"; do
  case "$arg" in
    --no-build) NO_BUILD=1 ;;
    -h|--help) sed -n '1,30p' "$0"; exit 0 ;;
    *) echo "Unknown arg: $arg" >&2; exit 1 ;;
  esac
done

SSH=(ssh -i "$KEY" -o BatchMode=yes -o StrictHostKeyChecking=accept-new -o ConnectTimeout=10 "$HOST")

echo "==> Syncing $HOST:$REMOTE_DIR ($BRANCH)"
"${SSH[@]}" bash -s -- "$REMOTE_DIR" "$BRANCH" "$NO_BUILD" <<'REMOTE'
set -euo pipefail
REMOTE_DIR="$1"; BRANCH="$2"; NO_BUILD="$3"
cd "$REMOTE_DIR"
git fetch origin "$BRANCH"
git checkout "$BRANCH"
git reset --hard "origin/$BRANCH"
git submodule update --init --recursive
if [ "$NO_BUILD" = "1" ]; then
  echo "==> Sync complete (no build)"; exit 0
fi
echo "==> Rebuilding and restarting container"
docker compose up -d --build --remove-orphans
docker compose ps
docker logs --tail=20 nte-cafe || true
REMOTE

echo "==> Done"
