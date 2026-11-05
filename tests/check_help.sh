#!/bin/sh
set -eu
bin=${1:-./src/cli/cuddle}
out=$("$bin" --help)
for f in --soft --guide --hub --tui --gui --map --stop --morning --spoon --hold --too-much --flood --aftercare --parallel --touch --help --version; do
  printf '%s\n' "$out" | grep -F -q -- "$f" || {
    printf 'missing %s\n' "$f" >&2
    exit 1
  }
done
printf 'ok-help\n'
