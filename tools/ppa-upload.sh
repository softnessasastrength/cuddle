#!/usr/bin/env bash
# prepare/check are local-only. sign uses pinentry; dput is the explicit upload action.
set -Eeuo pipefail
PROJECT_ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
exec python3 "$PROJECT_ROOT/tools/ppa.py" "$@"
