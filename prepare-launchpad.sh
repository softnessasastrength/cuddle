#!/usr/bin/env bash
# Source-only preparation plus a local native rebuild; does not sign or upload.
set -Eeuo pipefail
PROJECT_ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
exec bash "$PROJECT_ROOT/tools/ppa-upload.sh" prepare --series resolute --test-build "$@"
