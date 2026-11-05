#!/bin/sh
# Sign already-built Debian changes files with the configured OpenPGP key.
# Pinentry is deliberately left to debsign/GnuPG; this script never accepts
# or handles a passphrase.
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
out="$root/packaging/out"
key=0F4CA660B591C0F3

if ! command -v debsign >/dev/null 2>&1; then
  echo "debsign-out.sh: debsign is required" >&2
  exit 127
fi

if ! gpg --list-keys --with-colons "$key" 2>/dev/null | grep -q '^pub:'; then
  echo "debsign-out.sh: signing key $key is not available" >&2
  exit 2
fi

set -- "$out"/*.changes
if [ ! -f "$1" ]; then
  echo "debsign-out.sh: no .changes files in $out" >&2
  exit 2
fi

for changes do
  echo "signing $(basename -- "$changes") with pinentry"
  debsign -k"$key" "$changes"
done
