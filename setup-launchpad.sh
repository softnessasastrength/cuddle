#!/usr/bin/env bash
# Run in Crostini Terminal. Private-key/passphrase prompts belong to GPG itself.
# This helper never uploads a package. The frozen source bundle is kept intact.
set -Eeuo pipefail
PROJECT_ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
CHECK=0
NEW_KEY=0
PUBLISH=0
SIGN=0
KEY=''
usage() {
  cat <<'HELP'
Usage: bash ~/cuddle/setup-launchpad.sh [options]

  --check         Read-only tool/keyring/source-bundle readiness check
  --new-key       Create a new local signing key using GPG's normal prompts
  --key FPR       Select an existing key by its full public fingerprint
  --publish-key   Publish only the selected public key to Ubuntu's keyserver
  --sign          Sign the prepared source bundle through normal GPG pinentry
  --help          Show this help

No package upload occurs. Never enter a passphrase in this helper's arguments.
Without options, missing signing tools are installed and available key
fingerprints are listed. A missing key is reported rather than created silently.
HELP
}
while (( $# )); do
  case "$1" in
    --check) CHECK=1 ;;
    --new-key) NEW_KEY=1 ;;
    --publish-key) PUBLISH=1 ;;
    --sign) SIGN=1 ;;
    --key) [[ $# -ge 2 ]] || { echo '--key requires a full fingerprint' >&2; exit 2; }; KEY=$2; shift ;;
    --help|-h) usage; exit 0 ;;
    *) usage >&2; exit 2 ;;
  esac
  shift
done
if (( CHECK && (NEW_KEY || PUBLISH || SIGN) )); then
  echo '--check cannot create/publish keys or sign files.' >&2
  exit 2
fi
if [[ -n "$KEY" && ! "$KEY" =~ ^([[:xdigit:]]{40}|[[:xdigit:]]{64})$ ]]; then
  echo 'Use the full public fingerprint, not a passphrase or short key ID.' >&2
  exit 2
fi
bash "$PROJECT_ROOT/tools/ppa-upload.sh" check
missing=()
for command in gpg debsign dput; do
  if ! command -v "$command" >/dev/null; then missing+=("$command"); fi
done
if (( ${#missing[@]} )); then
  printf 'Missing signing tools: %s\n' "${missing[*]}"
  if (( CHECK )); then exit 2; fi
  if (( EUID == 0 )); then
    echo 'Run as your normal Linux user. sudo is requested only for apt.' >&2
    exit 2
  fi
  sudo apt-get update
  sudo apt-get install -y devscripts dput gnupg pinentry-curses
fi
if (( NEW_KEY )); then
  if [[ ! -t 0 || ! -t 1 ]]; then
    echo 'Run --new-key directly in your normal Crostini Terminal for the GPG prompts.' >&2
    exit 2
  fi
  export GPG_TTY
  GPG_TTY=$(tty)
  echo 'GPG will ask for your name, email and passphrase directly. This helper does not read them.'
  gpg --full-generate-key
fi
KEYRING=${GNUPGHOME:-"$HOME/.gnupg"}
if [[ ! -d "$KEYRING" ]]; then
  cat <<'NEXT'
No local GPG keyring is available.

To create a new key through GPG's own prompts:
  bash ~/cuddle/setup-launchpad.sh --new-key

If your old key is on another machine, use that machine to sign the portable
bundle, or import the key yourself through GPG from your secure local backup.
Do not send the private key or passphrase through chat.
NEXT
  exit 2
fi
# --with-colons lists only public metadata about locally available private keys.
mapfile -t keys < <(gpg --batch --with-colons --list-secret-keys --fingerprint 2>/dev/null |
  awk -F: '$1 == "sec" {primary=1; next} primary && $1 == "fpr" {print $10; primary=0}')
if (( ${#keys[@]} == 0 )); then
  echo 'No local private signing key is available. Use --new-key or your existing signing machine.' >&2
  exit 2
fi
printf 'Available primary signing fingerprints:\n'
printf '  %s\n' "${keys[@]}"
if [[ -z "$KEY" ]]; then
  if (( ${#keys[@]} == 1 )); then KEY=${keys[0]};
  elif (( PUBLISH || SIGN )); then
    echo 'Select one with --key FULL_FINGERPRINT.' >&2
    exit 2
  fi
fi
if (( CHECK )); then
  echo 'Local tools, source bundle and private-key metadata are available. Registration is not yet verified.'
  exit 0
fi
if [[ -n "$KEY" ]]; then
  KEY=${KEY^^}
  found=0
  for available in "${keys[@]}"; do [[ "${available^^}" != "$KEY" ]] || found=1; done
  if (( ! found )); then echo 'Selected fingerprint has no local private key.' >&2; exit 2; fi
fi
if (( PUBLISH )); then
  # --send-keys exports the public portion only; no private-key export exists here.
  gpg --keyserver hkps://keyserver.ubuntu.com --send-keys "$KEY"
fi
if (( SIGN )); then
  if [[ ! -t 0 || ! -t 1 ]]; then
    echo 'Run --sign directly in Crostini Terminal for the normal GPG pinentry prompt.' >&2
    exit 2
  fi
  export GPG_TTY
  GPG_TTY=$(tty)
  bash "$PROJECT_ROOT/tools/ppa-upload.sh" sign --key "$KEY"
fi
cat <<'NEXT'

Register the selected public fingerprint and complete Launchpad's encrypted-email
confirmation here:
  https://launchpad.net/~xtoxicwastx/+editpgpkeys

If the key has not been published yet, rerun with --publish-key.
After registration, rerun with --sign to sign the prepared bundle.
Package upload remains a separate explicit dput action; no upload occurred here.
NEXT
