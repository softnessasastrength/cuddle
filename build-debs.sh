#!/usr/bin/env bash
# Build unsigned native GTK4/ncurses Debian packages without installing them.
set -Eeuo pipefail
PROJECT_ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
cd "$PROJECT_ROOT"
python3 tools/generate-rooms.py --check
dpkg-checkbuilddeps
VERSION=$(dpkg-parsechangelog -SVersion)
STAMP=$(date +%Y%m%d-%H%M%S)-$$
RELEASE="$PROJECT_ROOT/releases/cuddle-$VERSION-$STAMP"
WORK_PARENT=$(mktemp -d "$PROJECT_ROOT/build-debian.XXXXXX")
WORK="$WORK_PARENT/cuddle-1.4.0"
mkdir -p "$RELEASE"
exec > >(tee "$RELEASE/build.log") 2>&1
trap 'rc=$?; echo "Package build stopped at line $LINENO. See $RELEASE/build.log" >&2; exit "$rc"' ERR
python3 - "$PROJECT_ROOT" "$WORK" <<'PY'
from pathlib import Path
import shutil,sys
source,target=map(Path,sys.argv[1:]);target.mkdir()
for path in source.iterdir():
 if path.name.startswith('build-') or path.name in {'backups','.deps','.git','releases','packaging','__pycache__'}:continue
 if path.suffix in {'.deb','.ddeb','.log','.changes','.buildinfo'}:continue
 if path.is_dir():shutil.copytree(path,target/path.name,ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
 else:shutil.copy2(path,target/path.name)
PY
(
  cd "$WORK"
  dpkg-buildpackage -us -uc -b
)
python3 - "$WORK_PARENT" "$RELEASE" <<'PY'
import hashlib,shutil,sys
from pathlib import Path
source,target=map(Path,sys.argv[1:])
artifacts=sorted(p for p in source.iterdir() if p.is_file() and p.suffix in {'.deb','.ddeb','.changes','.buildinfo'})
if len([p for p in artifacts if p.suffix=='.deb' and '-dbgsym_' not in p.name])!=6:raise SystemExit('Expected six binary Debian packages')
for path in artifacts:shutil.copy2(path,target/path.name)
(target/'SHA256SUMS').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n' for p in sorted(target.iterdir()) if p.suffix in {'.deb','.ddeb','.changes','.buildinfo'}))
PY
VERIFY="$WORK_PARENT/verify"
mkdir -p "$VERIFY"
for package in "$RELEASE"/*.deb; do
  if [[ "$package" == *-dbgsym_* ]]; then continue; fi
  dpkg-deb -x "$package" "$VERIFY"
done
export CUDDLE_SHARE="$VERIFY/usr/share/cuddle"
export CUDDLE_BINDIR="$VERIFY/usr/bin"
export LD_LIBRARY_PATH="$VERIFY/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
python3 tests/smoke_frontends.py --prefix "$VERIFY/usr" --tui curses --gui gtk
if command -v desktop-file-validate >/dev/null; then
  desktop-file-validate "$VERIFY/usr/share/applications/org.softnessasstrength.Cuddle.desktop"
fi
(
  cd "$RELEASE"
  sha256sum -c SHA256SUMS
)
python3 - "$PROJECT_ROOT" "$RELEASE" <<'PY'
from pathlib import Path
import sys
root,release=map(Path,sys.argv[1:]);link=root/'releases/latest'
if link.is_symlink():link.unlink()
if not link.exists():link.symlink_to(release.name,target_is_directory=True)
(release/'README.txt').write_text('Cuddle '+release.name+'\nSix unsigned local Debian packages, native ncurses and GTK4.\nThe Qt6 and Python/HTML variants are also available through rebuild-cuddle.sh.\nNo package was installed by this script.\nCheck SHA256SUMS for artifact integrity.\nInstall the six non-dbgsym .deb files from a normal Crostini Terminal if wanted. Debug-symbol packages are optional.\nUser-local desktop launchers continue to use the source build in build-current.\n')
print('Verified Debian packages:',release)
print('Latest release:',link)
PY
