#!/usr/bin/env python3
"""Prepare, inspect, sign and explicitly upload a source-only Launchpad bundle."""
import argparse
import datetime
import email.parser
import email.utils
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_PPA = 'ppa:xtoxicwastx/cuddle'
DEFAULT_KEY = 'E7275732ECC72867AE7383960F4CA660B591C0F3'
SOURCE_DIRS = ('src', 'data', 'docs', 'examples', 'po', 'tests', 'tools', '.github')
SOURCE_FILES = ('LICENSE', 'README.md', 'meson.build', 'meson_options.txt', '.gitignore', '.gitattributes',
                'build.sh', 'rebuild-cuddle.sh', 'setup-crostini.sh', 'build-debs.sh',
                'install-desktop.sh', 'preferences-cuddle.sh', 'prepare-launchpad.sh')
HISTORY_FILES = {'INVENTORY.md', 'PORT-STATUS.md', 'SEQUENCE-STATUS.md', 'GTK-POLISH.md'}


def run(argv, **kwargs):
    print('+', ' '.join(map(str, argv)), flush=True)
    return subprocess.run(list(map(str, argv)), check=True, **kwargs)


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def control(path):
    text = path.read_text()
    if text.startswith('-----BEGIN PGP SIGNED MESSAGE-----'):
        text = text.split('\n\n', 1)[1].split('\n-----BEGIN PGP SIGNATURE-----', 1)[0]
        text = '\n'.join(line[2:] if line.startswith('- ') else line for line in text.splitlines())
    return email.parser.Parser().parsestr(text)


def references(message, directory):
    found = []
    for line in message.get('Checksums-Sha256', '').splitlines():
        if not line.strip():
            continue
        digest, size, name = line.split()
        if PurePosixPath(name).name != name or not re.fullmatch('[A-Za-z0-9.+_~:-]+', name):
            raise ValueError('Unsafe artifact name: ' + name)
        path = directory / name
        if not path.is_file() or path.is_symlink() or path.stat().st_size != int(size) or sha256(path) != digest:
            raise ValueError('Artifact checksum/size mismatch: ' + name)
        found.append(path)
    if not found:
        raise ValueError('No source checksums were found')
    return found


def manifest(bundle):
    return json.loads((bundle / 'manifest.json').read_text())


def check(bundle):
    bundle = bundle.resolve()
    meta = manifest(bundle)
    changes = bundle / meta['changes']
    if changes.name != meta['changes'] or not changes.name.endswith('_source.changes'):
        raise ValueError('Expected a source-only .changes filename')
    message = control(changes)
    if message.get('Architecture') != 'source' or message.get('Distribution') != meta['series'] or message.get('Version') != meta['version']:
        raise ValueError('Source architecture, Ubuntu series, or version does not match manifest')
    if message.get('Source') != 'cuddle':
        raise ValueError('Unexpected source package')
    files = references(message, bundle)
    if any(path.suffix in ('.deb', '.ddeb') for path in files):
        raise ValueError('Binary packages must not be uploaded to this PPA')
    dscs = [path for path in files if path.suffix == '.dsc']
    if len(dscs) != 1:
        raise ValueError('Exactly one .dsc is required')
    dsc = control(dscs[0])
    if dsc.get('Version') != meta['version'] or dsc.get('Format') != '3.0 (quilt)':
        raise ValueError('Unexpected Debian source format/version')
    sources = references(dsc, bundle)
    orig = bundle / ('cuddle_' + meta['upstream'] + '.orig.tar.xz')
    if orig not in files or orig not in sources or sha256(orig) != meta['upstream_sha256']:
        raise ValueError('Full, unchanged upstream archive must be included (-sa)')
    for archive in sources:
        if '.tar.' not in archive.name:
            continue
        with tarfile.open(archive) as tar:
            for member in tar:
                parts = PurePosixPath(member.name)
                if parts.is_absolute() or '..' in parts.parts or not (member.isdir() or member.isfile()):
                    raise ValueError('Unsafe source archive entry: ' + member.name)
    print('Verified source-only bundle:', changes)
    return meta, changes, files


def sums(bundle):
    paths = sorted(p for p in bundle.iterdir() if p.is_file() and p.name not in {'SHA256SUMS', 'build.log', 'README.txt'})
    (bundle / 'SHA256SUMS').write_text(''.join(sha256(path) + '  ' + path.name + '\n' for path in paths))


def upstream_files():
    paths = [ROOT / name for name in SOURCE_FILES]
    for folder in SOURCE_DIRS:
        paths.extend(p for p in (ROOT / folder).rglob('*') if p.is_file() or p.is_symlink())
    result = []
    for path in sorted(paths):
        relative = path.relative_to(ROOT)
        if '__pycache__' in relative.parts or path.suffix in {'.pyc', '.patch'} or path.name in HISTORY_FILES or path.name.endswith('-CHANGES.patch'):
            continue
        if path.is_symlink():
            raise ValueError('Source release does not permit symlinks: ' + str(relative))
        if not path.is_file():
            raise ValueError('Missing source file: ' + str(relative))
        result.append(path)
    return result


def frozen_orig(version, epoch, work):
    candidate = work / ('cuddle_' + version + '.orig.tar.xz')
    prefix = 'cuddle-' + version
    with tarfile.open(candidate, 'w:xz', format=tarfile.GNU_FORMAT, preset=6) as tar:
        for path in upstream_files():
            info = tar.gettarinfo(str(path), arcname=prefix + '/' + str(path.relative_to(ROOT)))
            info.uid = info.gid = 0
            info.uname = info.gname = 'root'
            info.mtime = epoch
            info.mode = 0o755 if path.read_bytes().startswith(b'#!') else 0o644
            with path.open('rb') as stream:
                tar.addfile(info, stream)
    cache = ROOT / 'releases/upstream' / candidate.name
    cache.parent.mkdir(parents=True, exist_ok=True)
    if cache.exists() and sha256(cache) != sha256(candidate):
        raise ValueError('Upstream ' + version + ' is already frozen with different contents. Bump the upstream version before creating a different release archive.')
    if not cache.exists():
        shutil.copy2(candidate, cache)
    return candidate


def prepare(args):
    run([sys.executable, ROOT / 'tools/generate-rooms.py', '--check'])
    text = (ROOT / 'meson.build').read_text()
    version = re.search(r"version:\s*'([0-9]+\.[0-9]+\.[0-9]+)'", text).group(1)
    revision = args.revision
    if not re.fullmatch('[1-9][0-9]*', revision):
        raise ValueError('PPA revision must be a positive integer')
    package_version = f'{version}-1~ppa{revision}~{args.series}1'
    run(["dpkg", "--validate-version", package_version])
    epoch = int(subprocess.check_output(['dpkg-parsechangelog', '-STimestamp'], cwd=ROOT, text=True).strip())
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S') + '-' + str(os.getpid())
    if not re.fullmatch(r'ppa:[a-z0-9][a-z0-9+.-]*/[a-z0-9][a-z0-9+.-]*', args.ppa):
        raise ValueError('Expected ppa:owner/archive')
    bundle = ROOT / 'releases/ppa' / (package_version + '-' + stamp)
    bundle.mkdir(parents=True)
    build_parent = Path(tempfile.mkdtemp(prefix='build-ppa-', dir=ROOT))
    orig = frozen_orig(version, epoch, build_parent)
    with tarfile.open(orig) as archive:
        archive.extractall(build_parent, filter='data')
    work = build_parent / ('cuddle-' + version)
    shutil.copytree(ROOT / 'debian', work / 'debian', ignore=shutil.ignore_patterns(
        '.debhelper', 'files', '*.substvars', '*.debhelper.log', 'tmp',
        'cuddle', 'cuddle-data', 'cuddle-tui', 'cuddle-gui', 'libcuddle0', 'libcuddle-dev'))
    changelog = work / 'debian/changelog'
    entry = f'''cuddle ({package_version}) {args.series}; urgency=medium

  * Ubuntu {args.series} PPA build of the Cuddle {version} offline room release.
  * Native ncurses and GTK4 frontends; plugins remain disabled.

 -- Softness as Strength <thatcomputergeek021800@gmail.com>  {email.utils.formatdate(localtime=True)}

'''
    changelog.write_text(entry + changelog.read_text())
    run(['dpkg-buildpackage', '-S', '-sa', '-us', '-uc'], cwd=work)
    source_changes = build_parent / f'cuddle_{package_version}_source.changes'
    files = references(control(source_changes), build_parent)
    for path in [source_changes, *files]:
        shutil.copy2(path, bundle / path.name)
    meta = {'source': 'cuddle', 'upstream': version, 'version': package_version, 'series': args.series,
            'ppa': args.ppa, 'changes': source_changes.name, 'upstream_sha256': sha256(orig),
            'signing_fingerprint': DEFAULT_KEY, 'status': 'unsigned; not uploaded'}
    (bundle / 'manifest.json').write_text(json.dumps(meta, indent=2) + '\n')
    check(bundle)
    extraction = build_parent / 'roundtrip'
    run(['dpkg-source', '--no-check', '-x', next(bundle.glob('*.dsc')), extraction])
    run([sys.executable, extraction / 'tools/generate-rooms.py', '--check'])
    if args.test_build:
        run(['dpkg-buildpackage', '-b', '-us', '-uc'], cwd=extraction)
        meta['local_binary_validation'] = 'passed on Debian Crostini; Ubuntu build awaits Launchpad'
        (bundle / 'manifest.json').write_text(json.dumps(meta, indent=2) + '\n')
    (bundle / 'README.txt').write_text(
        f'Cuddle source-only PPA preparation\nVersion: {package_version}\nSeries: {args.series}\nPPA: {args.ppa}\n'
        'Status: unsigned; not uploaded. All .changes-referenced files must remain together.\n'
        'Use tools/ppa-upload.sh check --bundle THIS_DIRECTORY to recheck.\n'
        'Signing needs a private GPG key on this machine; never put its passphrase into arguments.\n'
        'The signing public key must be registered on the Launchpad uploader account.\n'
        'Local verification is on Debian; Launchpad performs the actual Ubuntu build.\n')
    sums(bundle)
    link = ROOT / 'releases/ppa/latest'
    if link.is_symlink():
        link.unlink()
    if not link.exists():
        link.symlink_to(bundle.name, target_is_directory=True)
    print('Prepared unsigned source bundle:', bundle)
    print('Round-trip source tree:', extraction)


def api(url):
    if not url.startswith('https://api.launchpad.net/'):
        raise ValueError('Unexpected Launchpad API URL')
    request = urllib.request.Request(url, headers={'User-Agent': 'cuddle-release-helper/1.4'})
    with urllib.request.urlopen(request, timeout=20) as response:
        return json.load(response)


def signing_identity(key):
    if not re.fullmatch('[A-Fa-f0-9]{40}|[A-Fa-f0-9]{64}', key):
        raise ValueError('Use the full signing-key fingerprint')
    home = Path(os.environ.get('GNUPGHOME', str(Path.home() / '.gnupg')))
    if not home.is_dir():
        raise ValueError('No GPG keyring exists here. Use the machine holding your private key; never share its passphrase.')
    output = subprocess.check_output(['gpg', '--batch', '--with-colons', '--list-secret-keys', '--fingerprint', key], text=True)
    fingerprints = [line.split(':')[9].upper() for line in output.splitlines() if line.startswith('fpr:')]
    if key.upper() not in fingerprints or not any(line.startswith('sec:') for line in output.splitlines()):
        raise ValueError('The requested private signing key is unavailable')
    return key.upper()


def registered_key(ppa, fingerprint):
    match = re.fullmatch(r'ppa:([a-z0-9][a-z0-9+.-]*)/([a-z0-9][a-z0-9+.-]*)', ppa)
    if not match:
        raise ValueError('Expected ppa:owner/archive')
    owner, archive = match.groups()
    person = api('https://api.launchpad.net/1.0/~' + owner)
    keys_url = person.get('gpg_keys_collection_link', 'https://api.launchpad.net/1.0/~' + owner + '/gpg_keys')
    keys = api(keys_url)
    entries = keys.get('entries', [])
    while keys.get('next_collection_link'):
        keys = api(keys['next_collection_link'])
        entries += keys.get('entries', [])
    if not any(str(key.get('fingerprint', '')).replace(' ', '').upper() == fingerprint for key in entries):
        raise ValueError('The signing key is not registered on the Launchpad uploader account')
    archive_data = api('https://api.launchpad.net/1.0/~' + owner + '/+archive/ubuntu/' + archive)
    if archive_data.get('enabled') is False or archive_data.get('status') not in (None, 'Active'):
        raise ValueError('The target PPA is not enabled/active')
    print('Verified Launchpad account key and active PPA:', ppa)


def verify_signature(path, key):
    result = subprocess.run(['gpg', '--batch', '--status-fd=1', '--verify', str(path)], check=True, text=True, capture_output=True)
    valid = [line.split() for line in result.stdout.splitlines() if line.startswith('[GNUPG:] VALIDSIG ')]
    if not any(key in (fields[2], fields[-1]) for fields in valid):
        raise ValueError('Signature does not match the expected primary fingerprint: ' + path.name)


def sign(args):
    meta, changes, files = check(args.bundle)
    key = signing_identity(args.key)
    if not shutil.which('debsign'):
        raise ValueError('debsign is missing. Install devscripts from the normal Linux Terminal.')
    run(['debsign', '-k' + key, changes])
    meta, changes, files = check(args.bundle)
    for path in [changes, *[p for p in files if p.suffix == '.dsc' or p.suffix == '.buildinfo']]:
        verify_signature(path, key)
    meta['signing_fingerprint'] = key
    meta['status'] = 'signed; not uploaded'
    (args.bundle / 'manifest.json').write_text(json.dumps(meta, indent=2) + '\n')
    sums(args.bundle)


def dput_configuration(ppa):
    match = re.fullmatch(r'ppa:([a-z0-9][a-z0-9+.-]*)/([a-z0-9][a-z0-9+.-]*)', ppa)
    if not match:
        raise ValueError('Expected ppa:owner/archive')
    owner, archive = match.groups()
    return ('[cuddle-launchpad]\n'
            'fqdn = ppa.launchpad.net\n'
            'method = ftp\n'
            f'incoming = ~{owner}/ubuntu/{archive}/\n'
            'login = anonymous\n'
            'allow_unsigned_uploads = 0\n'
            'passive_ftp = 1\n')


def upload(args):
    meta, changes, files = check(args.bundle)
    if meta.get('status') != 'signed; not uploaded':
        raise ValueError('Sign this source bundle first')
    key = signing_identity(args.key)
    if key != meta.get('signing_fingerprint'):
        raise ValueError('Signing fingerprint does not match the prepared bundle')
    registered_key(meta['ppa'], key)
    if not shutil.which('dput'):
        raise ValueError('dput is missing. Install dput from the normal Linux Terminal.')
    for path in [changes, *[p for p in files if p.suffix in ('.dsc', '.buildinfo')]]:
        verify_signature(path, key)
    # Debian dput does not reliably support Ubuntu's ppa:owner/archive shortcut.
    # Use an explicit per-invocation profile; leave ~/.dput.cf and /etc untouched.
    with tempfile.TemporaryDirectory(prefix='cuddle-dput-') as temp:
        config = Path(temp) / 'dput.cf'
        config.write_text(dput_configuration(meta['ppa']))
        config.chmod(0o600)
        run(['dput', '-c', config, 'cuddle-launchpad', changes])
    meta['status'] = 'transferred; verify acceptance/build on Launchpad'
    (args.bundle / 'manifest.json').write_text(json.dumps(meta, indent=2) + '\n')
    sums(args.bundle)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='action', required=True)
    prepare_parser = sub.add_parser('prepare', help='build and verify unsigned source artifacts; no network or signing')
    prepare_parser.add_argument('--series', choices=['noble', 'resolute'], default='resolute')
    prepare_parser.add_argument('--revision', default='1')
    prepare_parser.add_argument('--ppa', default=DEFAULT_PPA)
    prepare_parser.add_argument('--test-build', action='store_true')
    for name in ['check', 'sign', 'dput']:
        item = sub.add_parser(name)
        item.add_argument('--bundle', type=Path, default=ROOT / 'releases/ppa/latest')
        if name != 'check':
            item.add_argument('--key', default=DEFAULT_KEY, help='full OpenPGP fingerprint; no passphrase argument')
    args = parser.parse_args()
    try:
        if args.action == 'prepare':
            prepare(args)
        elif args.action == 'check':
            check(args.bundle)
        elif args.action == 'sign':
            sign(args)
        else:
            upload(args)
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        print('ppa helper:', error, file=sys.stderr)
        return 1
    return 0

if __name__ == '__main__':
    sys.exit(main())
