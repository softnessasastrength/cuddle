#!/usr/bin/env bash
# Create the owner's public source repository and push the reviewed local main.
# Run this in the normal Linux Terminal, where gh can use its normal login flow.
set -Eeuo pipefail
PROJECT_ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO=softnessasastrength/cuddle
cd "$PROJECT_ROOT"
if [[ "${1:-}" == --help ]]; then
  echo 'Usage: bash ~/cuddle/make-github.sh'
  echo 'Installs GitHub CLI if needed, uses normal GitHub login, creates public softnessasastrength/cuddle, and pushes main.'
  exit 0
fi
if (( $# )); then echo 'Usage: bash ~/cuddle/make-github.sh' >&2; exit 2; fi
if (( EUID == 0 )); then echo 'Run as your normal Linux user, without sudo.' >&2; exit 2; fi
if ! git rev-parse --verify main >/dev/null 2>&1; then echo 'The prepared local main branch is missing.' >&2; exit 1; fi
if ! command -v gh >/dev/null; then
  sudo apt-get update
  sudo apt-get install -y gh
fi
if ! gh auth status --hostname github.com >/dev/null 2>&1; then
  gh auth login --hostname github.com --git-protocol https --web --scopes workflow
else
  # Publishing the included CI workflow requires this scope on classic OAuth tokens.
  gh auth refresh --hostname github.com --scopes workflow
fi
LOGIN=$(gh api user --jq .login)
if [[ "$LOGIN" != softnessasastrength ]]; then
  echo "Authenticated as $LOGIN; switch gh to softnessasastrength before creating this repository." >&2
  exit 1
fi
gh auth setup-git --hostname github.com
META=$(mktemp "${TMPDIR:-/tmp}/cuddle-github.XXXXXX")
trap 'rm -f "$META"' EXIT
if gh repo view "$REPO" --json nameWithOwner,isPrivate > "$META" 2>/dev/null; then
  python3 - "$META" <<'PY'
import json,sys
from pathlib import Path
path=Path(sys.argv[1]);meta=json.loads(path.read_text())
if meta['nameWithOwner'].lower()!='softnessasastrength/cuddle' or meta['isPrivate']:
 raise SystemExit('Existing repository does not match the requested public destination.')
PY
  REMOTE_HEAD=$(git ls-remote "https://github.com/$REPO.git" HEAD)
  LOCAL_HEAD=$(git rev-parse main)
  if [[ "${REMOTE_HEAD%%[[:space:]]*}" == "$LOCAL_HEAD" ]]; then
    echo "The initial source commit is already published: https://github.com/$REPO"
    gh repo view "$REPO" --json nameWithOwner,url,isPrivate,defaultBranchRef
    exit 0
  fi
  if [[ -n "$REMOTE_HEAD" ]]; then
    echo "The repository already contains a commit; preserve it and ask Codex to review the existing history before pushing." >&2
    exit 1
  fi
  if git remote get-url origin >/dev/null 2>&1; then
    URL=$(git remote get-url origin)
    [[ "$URL" == "https://github.com/$REPO.git" ]] || { echo 'Existing origin differs; left unchanged.' >&2; exit 1; }
  else
    git remote add origin "https://github.com/$REPO.git"
  fi
  git push -u origin main
else
  gh repo create "$REPO" --public --description 'Offline Litmo rooms with CLI, ncurses/Python TUI, GTK4/Qt6 GUI and a shared C library. MIT licensed.' --source "$PROJECT_ROOT" --remote origin --push
fi
echo "Published source: https://github.com/$REPO"
gh repo view "$REPO" --json nameWithOwner,url,isPrivate,defaultBranchRef
